/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#include "PCH.h"
#include "Ids.h"
#include <unordered_set>
#include <random>
#include "Spoilage.h"
#include "Game.h"
#include "GameIds.h"
#include "Settings.h"

namespace Spoilage
{
	namespace
	{
		constexpr std::uint32_t kUniqueId = 'LSTD';
		constexpr std::uint32_t kRecordBatches = 'LSPB';
		constexpr std::uint32_t kRecordTime = 'LSPT';
		constexpr std::uint32_t kRecordWorld = 'LSWC';
		constexpr std::uint32_t kRecordStash = 'LSWS';
		constexpr std::uint32_t kVersion = 1;

		constexpr int    kPollSeconds = 3;        // real seconds between checks
		constexpr double kAdvanceStepHours = 0.1; // game hours between ageing steps
		constexpr double kReconcileHours = 1.0;   // game hours between inventory re-checks
		constexpr float  kMergeTolerance = 0.05f; // batches of the same food this close in age are merged
		constexpr double kInvalidTime = -1.0;
		constexpr RE::FormID kKeywordFood = 0x0008CDEA;     // VendorItemFood (Skyrim.esm)
		constexpr RE::FormID kKeywordFoodRaw = 0x000A0E56;  // VendorItemFoodRaw (Skyrim.esm)
		constexpr RE::FormID kDrinkSound = 0x000B6435;      // ITMPotionUse (Skyrim.esm): what ale, mead and wine play when drunk
		constexpr int    kWorldScanTicks = 4;     // polls between looks at the containers around the player
		constexpr float  kWorldScanRadius = 4096.0f;
		constexpr int    kWorldBudget = 24;       // containers handled per look
		constexpr float  kPerishedShare = 25.0f;  // % of the food that spoils which has gone all the way to "perished" (Last Seed's own value)

		// A stack of one food in one container, all aged the same.
		struct Batch
		{
			RE::FormID container = 0;
			RE::FormID food = 0;
			int        count = 0;
			float      perished = 0.0f;  // hours of rotting so far, at normal speed
		};

		struct Info
		{
			RE::TESBoundObject* spoiled = nullptr;  // what it turns into
			float               maxHours = 0.0f;    // hours until it does
		};

		struct Cat
		{
			RE::BGSListForm*    list = nullptr;
			RE::TESBoundObject* spoiled = nullptr;
			RE::TESGlobal*      rate = nullptr;
		};

		struct Data
		{
			Cat                 cats[std::size(ids::FoodCategories)];
			RE::BGSListForm*    preserved = nullptr;
			RE::BGSListForm*    spoiledFoods = nullptr;
			RE::TESBoundObject* perishedFood = nullptr;
			RE::TESBoundObject* iceTeeth = nullptr;
			RE::TESBoundObject* iceTeethOld = nullptr;
			RE::TESGlobal*      iceRate = nullptr;
			RE::TESGlobal*      enable = nullptr;
			RE::TESGlobal*      remove = nullptr;
			RE::TESGlobal*      speed = nullptr;
		RE::TESGlobal*      worldEnable = nullptr;
		RE::TESGlobal*      worldRate = nullptr;
			RE::FormID          provisions = 0;
			bool                ready = false;
		} d;

		std::mutex                                  lock;  // guards everything below (the settings page reads from another thread)
		std::vector<Batch>                          batches;
		std::unordered_map<RE::FormID, std::string> names;  // filled on the game thread, read by the settings page
		std::unordered_map<RE::FormID, float>       rates;  // hours until each food spoils, refreshed on the game thread
		double                                      lastAdvance = kInvalidTime;
		double                                      lastReconcile = kInvalidTime;
		float                                       speed = 1.0f;
		bool                                        active = false;
		bool                                        swapping = false;  // game thread only: our own item swaps must not be tracked
		std::atomic<bool>                           needReconcile{ true };

		// Food in containers out in the world (barrels, chests, sacks). When one is first seen, each food in it has a chance to have
		// spoiled; the swaps are remembered so that after the reset period the spoiled food can be made fresh again and re-rolled.
		struct Swap
		{
			RE::FormID fresh = 0;
			RE::FormID spoiled = 0;
			int        count = 0;
		};
		struct World
		{
			RE::FormID        container = 0;
			double            rolledAt = 0.0;  // game hours
			std::vector<Swap> swaps;
		};
		std::vector<World>             worlds;
		std::unordered_set<RE::FormID> stash;  // containers the player has put food into: never rolled, but their food ages while away
		std::unordered_set<RE::FormID> vendorChests;  // merchants' stock chests: not loot, never touched
		int                            worldTicks = 0;
		bool                           worldActive = false;

		template <class T>
		T* Look(RE::FormID a_id)
		{
			auto* dh = RE::TESDataHandler::GetSingleton();
			return dh ? dh->LookupForm<T>(ids::Ls(a_id), "LastSeed.esp") : nullptr;
		}

		// TESBoundObject has no form type of its own, so LookupForm<TESBoundObject> always fails; cast by hand.
		RE::TESBoundObject* ById(RE::FormID a_id)
		{
			return skyrim_cast<RE::TESBoundObject*>(RE::TESForm::LookupByID(a_id));
		}

		RE::TESBoundObject* LookBound(RE::FormID a_id)
		{
			auto* dh = RE::TESDataHandler::GetSingleton();
			return dh ? skyrim_cast<RE::TESBoundObject*>(dh->LookupForm(ids::Ls(a_id), "LastSeed.esp")) : nullptr;
		}

		double NowHours()
		{
			auto* cal = RE::Calendar::GetSingleton();
			return cal ? static_cast<double>(cal->GetCurrentGameTime()) * 24.0 : 0.0;
		}

		// Is this a food we age, and what does it become? Same rules as Last Seed's own food monitor.
		bool Classify(RE::TESForm* a_form, Info& a_out)
		{
			if (!a_form || !d.ready || !a_form->Is(RE::FormType::AlchemyItem)) {
				return false;
			}
			if (d.iceTeeth && a_form == d.iceTeeth) {
				a_out.spoiled = d.iceTeethOld;
				a_out.maxHours = d.iceRate ? d.iceRate->value : 0.0f;
				return a_out.spoiled && a_out.maxHours > 0.0f;
			}
			if (d.preserved && d.preserved->HasForm(a_form)) {
				return false;
			}
			for (const auto& c : d.cats) {
				if (c.list && c.list->HasForm(a_form)) {
					a_out.spoiled = (d.spoiledFoods && d.spoiledFoods->HasForm(a_form)) ? d.perishedFood : c.spoiled;
					a_out.maxHours = c.rate ? c.rate->value : 0.0f;
					return a_out.spoiled && a_out.maxHours > 0.0f;
				}
			}
			// Food Last Seed does not list (added by other mods): anything with the food flag and Skyrim's own VendorItemFood /
			// VendorItemFoodRaw keyword that is not a drink. Drinks are told apart by the drinking sound only (ITMPotionUse): eating-
			// animation mods give foods their own eating sounds, so requiring the vanilla eating sound would wrongly leave those out.
			// It rots into "perished" food.
			if (const auto& s = Settings::Get(); s.keywordFoods) {
				const auto* alch = a_form->As<RE::AlchemyItem>();
				if (alch && alch->IsFood() && !(alch->data.consumptionSound && alch->data.consumptionSound->GetFormID() == kDrinkSound) &&
					(!d.spoiledFoods || !d.spoiledFoods->HasForm(a_form))) {
					const bool raw = alch->HasKeywordID(kKeywordFoodRaw);
					if (raw || alch->HasKeywordID(kKeywordFood)) {
						a_out.spoiled = d.perishedFood;
						a_out.maxHours = raw ? s.rawFoodHours : s.otherFoodHours;
						return a_out.spoiled && a_out.maxHours > 0.0f;
					}
				}
			}
			return false;
		}

		bool IsStash(RE::FormID a_id)
		{
			std::scoped_lock l(lock);
			return stash.contains(a_id);
		}

		bool IsTracked(RE::FormID a_id)
		{
			if (a_id == 0x14 || a_id == d.provisions || IsStash(a_id)) {
				return true;
			}
			auto* actor = RE::TESForm::LookupByID<RE::Actor>(a_id);
			return actor && !actor->IsDead() && actor->IsPlayerTeammate();
		}

		std::vector<RE::TESObjectREFR*> TrackedContainers()
		{
			std::vector<RE::TESObjectREFR*> out;
			if (auto* player = RE::PlayerCharacter::GetSingleton()) {
				out.push_back(player);
			}
			if (auto* prov = RE::TESForm::LookupByID<RE::TESObjectREFR>(d.provisions)) {
				out.push_back(prov);
			}
			std::vector<RE::FormID> stashed;
			{
				std::scoped_lock l(lock);
				stashed.assign(stash.begin(), stash.end());
			}
			for (const auto id : stashed) {  // only while the container is really there: an unloaded one keeps its records and keeps ageing
				if (auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(id); ref && !ref->IsDeleted() && ref->Is3DLoaded()) {
					out.push_back(ref);
				}
			}
			if (auto* lists = RE::ProcessLists::GetSingleton()) {
				for (auto& handle : lists->highActorHandles) {
					if (auto actor = handle.get(); actor && !actor->IsDead() && actor->IsPlayerTeammate() && !actor->IsPlayerRef()) {
						out.push_back(actor.get());
					}
				}
			}
			return out;
		}

		// ---- batch bookkeeping (call with `lock` held) ----

		void AddBatch(RE::FormID a_container, RE::FormID a_food, int a_count, float a_perished)
		{
			if (a_count <= 0) {
				return;
			}
			for (auto& b : batches) {
				if (b.container == a_container && b.food == a_food && std::abs(b.perished - a_perished) <= kMergeTolerance) {
					b.count += a_count;
					return;
				}
			}
			batches.push_back({ a_container, a_food, a_count, a_perished });
		}

		int Tracked(RE::FormID a_container, RE::FormID a_food)
		{
			int n = 0;
			for (const auto& b : batches) {
				if (b.container == a_container && b.food == a_food) {
					n += b.count;
				}
			}
			return n;
		}

		// Takes up to a_count from a container's batches, newest first (so the oldest food stays, as in Last Seed). Returns the
		// pieces taken with their age.
		std::vector<std::pair<int, float>> Take(RE::FormID a_container, RE::FormID a_food, int a_count)
		{
			std::vector<std::pair<int, float>> pieces;
			for (std::size_t i = batches.size(); i-- > 0 && a_count > 0;) {
				auto& b = batches[i];
				if (b.container != a_container || b.food != a_food || b.count <= 0) {
					continue;
				}
				const int n = std::min(b.count, a_count);
				pieces.emplace_back(n, b.perished);
				b.count -= n;
				a_count -= n;
			}
			std::erase_if(batches, [](const Batch& b) { return b.count <= 0; });
			return pieces;
		}

		void Remember(RE::FormID a_id)
		{
			if (names.contains(a_id)) {
				return;
			}
			if (auto* form = RE::TESForm::LookupByID(a_id)) {
				std::string n;
				if (auto* ref = form->AsReference()) {
					n = ref->GetDisplayFullName();
					if (n.empty() && ref->IsPlayerRef()) {
						n = "You";
					}
				} else if (auto* named = form->As<RE::TESFullName>()) {
					n = named->GetFullName();
				}
				names[a_id] = n.empty() ? "?" : n;
			}
		}

		// ---- inventory events ----

		class Sink : public RE::BSTEventSink<RE::TESContainerChangedEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* a_event, RE::BSTEventSource<RE::TESContainerChangedEvent>*) override
			{
				if (!a_event || swapping || !active || a_event->itemCount <= 0) {
					return RE::BSEventNotifyControl::kContinue;
				}
				auto* form = RE::TESForm::LookupByID(a_event->baseObj);
				Info info;
				if (!Classify(form, info)) {
					return RE::BSEventNotifyControl::kContinue;
				}
				if (a_event->oldContainer == 0x14 && a_event->newContainer && a_event->newContainer != d.provisions &&
					!RE::TESForm::LookupByID<RE::Actor>(a_event->newContainer) && RE::TESForm::LookupByID<RE::TESObjectREFR>(a_event->newContainer)) {
					std::scoped_lock l(lock);  // the player is storing food somewhere: it is theirs, not loot, and it keeps ageing there
					if (stash.insert(a_event->newContainer).second) {
						std::erase_if(worlds, [&](const World& w) { return w.container == a_event->newContainer; });
						needReconcile = true;
					}
				}
				const bool from = a_event->oldContainer && IsTracked(a_event->oldContainer);
				const bool to = a_event->newContainer && IsTracked(a_event->newContainer);
				if (!from && !to) {
					return RE::BSEventNotifyControl::kContinue;
				}

				std::scoped_lock l(lock);
				std::vector<std::pair<int, float>> pieces;
				if (from) {
					pieces = Take(a_event->oldContainer, a_event->baseObj, a_event->itemCount);
				}
				if (to) {
					int moved = 0;
					for (const auto& [n, perished] : pieces) {  // moved between tracked containers: the food keeps its age
						AddBatch(a_event->newContainer, a_event->baseObj, n, perished);
						moved += n;
					}
					AddBatch(a_event->newContainer, a_event->baseObj, a_event->itemCount - moved, 0.0f);
					Remember(a_event->newContainer);
					Remember(a_event->baseObj);
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};
		Sink sink;

		// ---- periodic work (game thread) ----

		// Makes the records match what the containers really hold: picks up food that arrived without an event (the starting
		// inventory, console, other mods), drops records for food that is gone and for containers that are no longer tracked.
		void Reconcile()
		{
			const auto containers = TrackedContainers();
			std::scoped_lock l(lock);

			std::unordered_set<RE::FormID> tracked;
			for (auto* c : containers) {
				tracked.insert(c->GetFormID());
			}
			tracked.insert(stash.begin(), stash.end());  // unloaded stash containers stay on the books
			std::erase_if(batches, [&](const Batch& b) { return !tracked.contains(b.container); });

			for (auto* c : containers) {
				const auto id = c->GetFormID();
				Remember(id);
				std::unordered_map<RE::FormID, int> actual;
				for (const auto& [obj, count] : c->GetInventoryCounts([](RE::TESBoundObject& o) { return o.Is(RE::FormType::AlchemyItem); })) {
					Info info;
					if (count > 0 && Classify(obj, info)) {
						actual[obj->GetFormID()] = count;
						Remember(obj->GetFormID());
					}
				}
				for (const auto& [food, count] : actual) {
					const int have = Tracked(id, food);
					if (have < count) {
						AddBatch(id, food, count - have, 0.0f);
					} else if (have > count) {
						Take(id, food, have - count);
					}
				}
				std::erase_if(batches, [&](const Batch& b) { return b.container == id && !actual.contains(b.food); });
			}
		}

		void Advance(double a_now)
		{
			std::scoped_lock l(lock);
			if (d.speed) {
				const float v = d.speed->value;
				speed = v > 0.0f ? std::min(v, 3.0f) : 1.0f;
			}
			const double dt = a_now - lastAdvance;
			lastAdvance = a_now;
			if (dt <= 0.0) {
				return;
			}
			for (auto& b : batches) {
				b.perished += static_cast<float>(dt) * speed;
				if (!rates.contains(b.food) || dt >= 0.0) {  // keep the settings page's numbers current (the rates are game settings)
					Info info;
					rates[b.food] = Classify(RE::TESForm::LookupByID(b.food), info) ? info.maxHours : 0.0f;
				}
			}
		}

			// ---- world containers ----

		std::mt19937& Rng()
		{
			static std::mt19937 rng{ std::random_device{}() };
			return rng;
		}

		bool Chance(float a_percent)
		{
			return std::uniform_real_distribution<float>(0.0f, 100.0f)(Rng()) <= a_percent;
		}

		bool IsWorldContainer(RE::TESObjectREFR* a_ref)
		{
			if (!a_ref || a_ref->IsDeleted() || a_ref->IsDisabled() || !a_ref->Is3DLoaded() || a_ref->GetFormID() == d.provisions) {
				return false;
			}
			{
				std::scoped_lock l(lock);
				if (vendorChests.contains(a_ref->GetFormID())) {
					return false;
				}
			}
			auto* base = a_ref->GetBaseObject();
			if (!base || !base->Is(RE::FormType::Container)) {
				return false;
			}
			auto* player = RE::PlayerCharacter::GetSingleton();
			return !(player && a_ref->GetOwner() == player->GetActorBase());  // the player's own chests are theirs
		}

		int CountOf(RE::TESObjectREFR* a_container, RE::TESBoundObject* a_obj)
		{
			const auto counts = a_container->GetInventoryCounts([a_obj](RE::TESBoundObject& o) { return &o == a_obj; });
			const auto it = counts.find(a_obj);
			return it != counts.end() ? it->second : 0;
		}

		void SwapItems(RE::TESObjectREFR* a_container, RE::TESBoundObject* a_from, RE::TESBoundObject* a_to, int a_count)
		{
			swapping = true;
			a_container->RemoveItem(a_from, a_count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
			a_container->AddObjectToContainer(a_to, nullptr, a_count, nullptr);
			swapping = false;
		}

		// Brings a container up to date: spoiled food we made earlier (and the player left) becomes fresh again, then each food
		// is rolled afresh.
		void RollContainer(RE::TESObjectREFR* a_ref, double a_now, float a_chance)
		{
			const auto        id = a_ref->GetFormID();
			std::vector<Swap> old;
			{
				std::scoped_lock l(lock);
				if (auto it = std::ranges::find(worlds, id, &World::container); it != worlds.end()) {
					old = it->swaps;
				}
			}
			for (const auto& sw : old) {
				auto* fresh = ById(sw.fresh);
				auto* spoiled = ById(sw.spoiled);
				if (fresh && spoiled) {
					if (const int n = std::min(sw.count, CountOf(a_ref, spoiled)); n > 0) {
						SwapItems(a_ref, spoiled, fresh, n);
					}
				}
			}

			const auto totalItems = [&]() {
				int n = 0;
				for (const auto& [obj, count] : a_ref->GetInventoryCounts([](RE::TESBoundObject&) { return true; })) {
					n += count;
				}
				return n;
			};
			const int         before = totalItems();
			struct Rolled
			{
				RE::TESBoundObject* food;
				RE::TESBoundObject* spoiled;
				int                 spoiledN = 0;
				int                 perishedN = 0;
			};
			std::vector<Rolled> plan;
			int                 foodTotal = 0;
			int                 rotTotal = 0;
			const auto          inventory = a_ref->GetInventoryCounts([](RE::TESBoundObject& o) { return o.Is(RE::FormType::AlchemyItem); });
			for (const auto& [obj, count] : inventory) {
				Info info;
				if (count <= 0 || !Classify(obj, info) || (d.spoiledFoods && d.spoiledFoods->HasForm(obj))) {
					continue;  // not a food that spoils, or already spoiled
				}
				Rolled r{ obj, info.spoiled };
				for (int i = 0; i < count; ++i) {
					if (Chance(a_chance)) {
						(Chance(kPerishedShare) ? r.perishedN : r.spoiledN) += 1;
					}
				}
				foodTotal += count;
				rotTotal += r.spoiledN + r.perishedN;
				plan.push_back(r);
			}
			// Always leave a share of the container's food fresh: put random rotten pieces back until it holds.
			const int keepFresh = static_cast<int>(std::ceil(foodTotal * std::clamp(Settings::Get().worldFreshShare, 0.0f, 100.0f) / 100.0f));
			for (int excess = rotTotal - (foodTotal - keepFresh); excess > 0; --excess) {
				std::vector<Rolled*> candidates;
				for (auto& r : plan) {
					if (r.spoiledN + r.perishedN > 0) {
						candidates.push_back(&r);
					}
				}
				if (candidates.empty()) {
					break;
				}
				auto& r = *candidates[std::uniform_int_distribution<std::size_t>(0, candidates.size() - 1)(Rng())];
				(r.spoiledN > 0 ? r.spoiledN : r.perishedN) -= 1;
			}
			std::vector<Swap> rolled;
			for (const auto& r : plan) {
				if (r.spoiledN > 0) {
					SwapItems(a_ref, r.food, r.spoiled, r.spoiledN);
					rolled.push_back({ r.food->GetFormID(), r.spoiled->GetFormID(), r.spoiledN });
				}
				if (r.perishedN > 0) {
					SwapItems(a_ref, r.food, d.perishedFood, r.perishedN);
					rolled.push_back({ r.food->GetFormID(), d.perishedFood->GetFormID(), r.perishedN });
				}
			}

			const int after = totalItems();
			if (before != after) {
				SKSE::log::warn("Spoilage: world container {:08X} had {} items before its roll and {} after", id, before, after);
			}
			std::scoped_lock l(lock);
			auto             it = std::ranges::find(worlds, id, &World::container);
			if (it == worlds.end()) {
				worlds.push_back({ id, a_now, {} });
				it = worlds.end() - 1;
			}
			it->rolledAt = a_now;
			it->swaps = std::move(rolled);
			if (!it->swaps.empty() || before > 0) {
				SKSE::log::info("Spoilage: world container {} {:08X} rolled: {} items, {} kinds of food spoiled", a_ref->GetDisplayFullName(), id, before, it->swaps.size());
			}
		}

		void ScanWorld(double a_now)
		{
			auto*       player = RE::PlayerCharacter::GetSingleton();
			auto*       tes = RE::TES::GetSingleton();
			const auto  period = static_cast<double>(Settings::Get().containerResetDays) * 24.0;
			const float chance = std::clamp(Settings::Get().worldSpoilChance, 0.0f, 100.0f);
			auto*       cell = player ? player->GetParentCell() : nullptr;
			// Not while a save is loading or the world is not fully attached: TES::ForEachReferenceInRange crashes then, so walk
			// the attached cells ourselves.
			if (!player || !tes || !cell || !cell->IsAttached() || !player->Is3DLoaded()) {
				return;
			}
			const auto origin = player->GetPosition();
			std::vector<RE::ObjectRefHandle> found;
			const auto collect = [&](RE::TESObjectCELL* a_cell) {
				if (a_cell && a_cell->IsAttached()) {
					a_cell->ForEachReferenceInRange(origin, kWorldScanRadius, [&](auto&& a_refArg) {
						auto* a_ref = compat::Ptr(a_refArg);
						if (IsWorldContainer(a_ref)) {
							found.push_back(a_ref->GetHandle());
						}
						return RE::BSContainer::ForEachResult::kContinue;
					});
				}
			};
			if (cell->IsInteriorCell()) {
				collect(cell);
			} else if (auto* grid = tes->gridCells; grid && player->GetWorldspace()) {
				for (std::uint32_t x = 0; x < grid->length; ++x) {
					for (std::uint32_t y = 0; y < grid->length; ++y) {
						collect(grid->GetCell(x, y));
					}
				}
			}
			int budget = kWorldBudget;
			for (auto& handle : found) {
				auto ref = handle.get();
				if (!ref || budget <= 0) {
					continue;
				}
				{
					std::scoped_lock l(lock);
					if (stash.contains(ref->GetFormID())) {
						continue;
					}
					const auto it = std::ranges::find(worlds, ref->GetFormID(), &World::container);
					if (it != worlds.end() && a_now >= it->rolledAt && a_now - it->rolledAt < period) {
						continue;  // rolled recently: leave it alone
					}
				}
				--budget;
				RollContainer(ref.get(), a_now, chance);
			}
		}

	bool Blocked()
		{
			auto* ui = RE::UI::GetSingleton();
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!ui || !player || player->IsInCombat()) {
				return true;
			}
			return ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME) || ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME) ||
			       ui->IsMenuOpen(RE::BarterMenu::MENU_NAME) || ui->IsMenuOpen(RE::GiftMenu::MENU_NAME) ||
			       ui->IsMenuOpen(RE::CraftingMenu::MENU_NAME) || ui->IsMenuOpen(RE::FavoritesMenu::MENU_NAME) ||
			       ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME);
		}

		// Turns every batch that has rotted long enough into its spoiled version, in place.
		void SpoilDue()
		{
			struct Due
			{
				RE::FormID container;
				RE::FormID food;
				int        count;
				RE::TESBoundObject* spoiled;
			};
			std::vector<Due> due;
			{
				std::scoped_lock l(lock);
				for (const auto& b : batches) {
					Info info;
					if (b.count > 0 && Classify(RE::TESForm::LookupByID(b.food), info) && b.perished >= info.maxHours) {
						due.push_back({ b.container, b.food, b.count, info.spoiled });
					}
				}
			}
			if (due.empty()) {
				return;
			}

			const int removeMode = d.remove ? static_cast<int>(d.remove->value) : 1;
			for (const auto& item : due) {
				auto* container = RE::TESForm::LookupByID<RE::TESObjectREFR>(item.container);
				auto* food = ById(item.food);
				if (IsStash(item.container) && (!container || !container->Is3DLoaded())) {
					continue;  // not here right now: it spoils when the player comes back to it
				}
				int   n = 0;
				if (container && food) {
					const auto counts = container->GetInventoryCounts([food](RE::TESBoundObject& o) { return &o == food; });
					if (auto it = counts.find(food); it != counts.end()) {
						n = std::min(item.count, it->second);
					}
				}
				{
					std::scoped_lock l(lock);
					Take(item.container, item.food, item.count);  // the batch is finished either way
				}
				if (n <= 0) {
					continue;
				}
				swapping = true;
				container->RemoveItem(food, n, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				const bool keep = removeMode != 3 && !(removeMode == 2 && item.spoiled == d.perishedFood);
				if (keep && item.spoiled) {
					container->AddObjectToContainer(item.spoiled, nullptr, n, nullptr);
					Info next;  // spoiled food can itself rot on to "perished"
					if (Classify(item.spoiled, next)) {
						std::scoped_lock l(lock);
						AddBatch(item.container, item.spoiled->GetFormID(), n, 0.0f);
						Remember(item.spoiled->GetFormID());
					}
				}
				swapping = false;
				SKSE::log::info("Spoilage: {} x{} in {} spoiled", food->GetName(), n, container->GetDisplayFullName());
			}
		}

		void Tick()
		{
			active = Settings::Get().nativeSpoilage && d.ready && Game::IsRunning() && d.enable && static_cast<int>(d.enable->value) == 2;
			if (!active) {
				std::scoped_lock l(lock);
				lastAdvance = kInvalidTime;  // no catch-up for time spent switched off
				lastReconcile = kInvalidTime;
				return;
			}
			const double now = NowHours();
			if (lastAdvance == kInvalidTime) {
				lastAdvance = now;
			}
			if (needReconcile.exchange(false) || lastReconcile == kInvalidTime || now - lastReconcile >= kReconcileHours || now < lastReconcile) {
				Reconcile();
				lastReconcile = now;
			}
			if (now - lastAdvance >= kAdvanceStepHours || now < lastAdvance) {
				Advance(now);
			}
			if (!Blocked()) {
				SpoilDue();
				worldActive = Settings::Get().worldSpoilage && d.worldEnable && d.worldRate && static_cast<int>(d.worldEnable->value) == 2;
				if (worldActive && ++worldTicks >= kWorldScanTicks) {
					worldTicks = 0;
					ScanWorld(now);
				}
			}
		}

		// ---- save games (SKSE co-save) ----

		void OnSave(SKSE::SerializationInterface* a_intfc)
		{
			std::scoped_lock l(lock);
			if (a_intfc->OpenRecord(kRecordTime, kVersion)) {
				a_intfc->WriteRecordData(lastAdvance);
			}
			if (a_intfc->OpenRecord(kRecordWorld, kVersion)) {
				const double horizon = NowHours() - 2.0 * Settings::Get().containerResetDays * 24.0;
				std::uint32_t n = 0;
				for (const auto& w : worlds) {
					n += (!w.swaps.empty() || w.rolledAt >= horizon) ? 1 : 0;  // forget old containers that had nothing spoiled
				}
				a_intfc->WriteRecordData(n);
				for (const auto& w : worlds) {
					if (w.swaps.empty() && w.rolledAt < horizon) {
						continue;
					}
					a_intfc->WriteRecordData(w.container);
					a_intfc->WriteRecordData(w.rolledAt);
					const std::uint32_t m = static_cast<std::uint32_t>(w.swaps.size());
					a_intfc->WriteRecordData(m);
					for (const auto& sw : w.swaps) {
						a_intfc->WriteRecordData(sw.fresh);
						a_intfc->WriteRecordData(sw.spoiled);
						a_intfc->WriteRecordData(sw.count);
					}
				}
			}
			if (a_intfc->OpenRecord(kRecordStash, kVersion)) {
				const std::uint32_t n = static_cast<std::uint32_t>(stash.size());
				a_intfc->WriteRecordData(n);
				for (const auto id : stash) {
					a_intfc->WriteRecordData(id);
				}
			}
			if (a_intfc->OpenRecord(kRecordBatches, kVersion)) {
				const std::uint32_t n = static_cast<std::uint32_t>(batches.size());
				a_intfc->WriteRecordData(n);
				for (const auto& b : batches) {
					a_intfc->WriteRecordData(b.container);
					a_intfc->WriteRecordData(b.food);
					a_intfc->WriteRecordData(b.count);
					a_intfc->WriteRecordData(b.perished);
				}
			}
		}

		void OnLoad(SKSE::SerializationInterface* a_intfc)
		{
			std::scoped_lock l(lock);
			batches.clear();
			worlds.clear();
			stash.clear();
			worldTicks = -10;  // give a freshly loaded world ~30 s to settle before looking at its containers
			lastAdvance = kInvalidTime;
			lastReconcile = kInvalidTime;
			std::uint32_t type, version, length;
			while (a_intfc->GetNextRecordInfo(type, version, length)) {
				if (version != kVersion) {
					continue;
				}
				if (type == kRecordTime) {
					a_intfc->ReadRecordData(lastAdvance);
				} else if (type == kRecordWorld) {
					std::uint32_t n = 0;
					a_intfc->ReadRecordData(n);
					for (std::uint32_t i = 0; i < n; ++i) {
						RE::FormID    oldId = 0;
						World         w;
						std::uint32_t m = 0;
						a_intfc->ReadRecordData(oldId);
						a_intfc->ReadRecordData(w.rolledAt);
						a_intfc->ReadRecordData(m);
						for (std::uint32_t k = 0; k < m; ++k) {
							Swap       sw;
							RE::FormID oldFresh = 0, oldSpoiled = 0;
							a_intfc->ReadRecordData(oldFresh);
							a_intfc->ReadRecordData(oldSpoiled);
							a_intfc->ReadRecordData(sw.count);
							if (a_intfc->ResolveFormID(oldFresh, sw.fresh) && a_intfc->ResolveFormID(oldSpoiled, sw.spoiled) && sw.count > 0) {
								w.swaps.push_back(sw);
							}
						}
						if (a_intfc->ResolveFormID(oldId, w.container)) {
							worlds.push_back(std::move(w));
						}
					}
				} else if (type == kRecordStash) {
					std::uint32_t n = 0;
					a_intfc->ReadRecordData(n);
					for (std::uint32_t i = 0; i < n; ++i) {
						RE::FormID oldId = 0, id = 0;
						a_intfc->ReadRecordData(oldId);
						if (a_intfc->ResolveFormID(oldId, id)) {
							stash.insert(id);
						}
					}
				} else if (type == kRecordBatches) {
					std::uint32_t n = 0;
					a_intfc->ReadRecordData(n);
					for (std::uint32_t i = 0; i < n; ++i) {
						Batch b;
						RE::FormID oldContainer = 0, oldFood = 0;
						a_intfc->ReadRecordData(oldContainer);
						a_intfc->ReadRecordData(oldFood);
						a_intfc->ReadRecordData(b.count);
						a_intfc->ReadRecordData(b.perished);
						if (a_intfc->ResolveFormID(oldContainer, b.container) && a_intfc->ResolveFormID(oldFood, b.food) && b.count > 0) {
							batches.push_back(b);
						}
					}
				}
			}
			names.clear();
			needReconcile = true;
			SKSE::log::info("Spoilage: loaded {} batches, {} world containers, {} stash containers from the save", batches.size(), worlds.size(), stash.size());
		}

		void OnRevert(SKSE::SerializationInterface*)
		{
			std::scoped_lock l(lock);
			batches.clear();
			worlds.clear();
			stash.clear();
			names.clear();
			lastAdvance = kInvalidTime;
			lastReconcile = kInvalidTime;
			needReconcile = true;
		}
	}

	void Init()
	{
		auto* dh = RE::TESDataHandler::GetSingleton();
		if (!dh) {
			return;
		}
		for (std::size_t i = 0; i < std::size(ids::FoodCategories); ++i) {
			const auto& c = ids::FoodCategories[i];
			d.cats[i].list = Look<RE::BGSListForm>(c.list);
			d.cats[i].spoiled = LookBound(c.spoiled);
			d.cats[i].rate = Look<RE::TESGlobal>(c.rate);
			if (!d.cats[i].list || !d.cats[i].spoiled || !d.cats[i].rate) {
				SKSE::log::error("Spoilage: category '{}' is missing a form (list {}, spoiled {}, rate {})", c.name, !!d.cats[i].list, !!d.cats[i].spoiled, !!d.cats[i].rate);
			}
		}
		d.preserved = Look<RE::BGSListForm>(ids::PreservedList);
		d.spoiledFoods = Look<RE::BGSListForm>(ids::SpoiledFoodsList);
		d.perishedFood = LookBound(ids::PerishedFood);
		d.iceTeeth = LookBound(ids::IceWraithTeeth);
		d.iceTeethOld = LookBound(ids::IceWraithTeethOld);
		d.iceRate = Look<RE::TESGlobal>(ids::IceWraithTeethRate);
		d.enable = Look<RE::TESGlobal>(ids::SpoilageEnable);
		d.remove = Look<RE::TESGlobal>(ids::SpoilageRemove);
		d.speed = Look<RE::TESGlobal>(ids::SpoilTemperatureMulti);
		d.worldEnable = Look<RE::TESGlobal>(ids::ContainerSpoilageEnable);
		d.worldRate = Look<RE::TESGlobal>(ids::ContainerSpoilageRate);
		if (auto* prov = Look<RE::TESObjectREFR>(ids::ProvisionsContainer)) {
			d.provisions = prov->GetFormID();
		}
		{
			std::scoped_lock l(lock);
			vendorChests.clear();
			for (auto* faction : dh->GetFormArray<RE::TESFaction>()) {
				if (faction && faction->vendorData.merchantContainer) {
					vendorChests.insert(faction->vendorData.merchantContainer->GetFormID());
				}
			}
			SKSE::log::info("Spoilage: {} merchant chests are left alone", vendorChests.size());
		}
		d.ready = d.preserved && d.spoiledFoods && d.perishedFood && d.enable && d.remove && d.speed && d.provisions;
		SKSE::log::info("Spoilage: Last Seed's food data {}", d.ready ? "found" : "INCOMPLETE - native spoilage is off");
		if (d.ready) {
			RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(&sink);
		}
	}

	void Register()
	{
		auto* ser = SKSE::GetSerializationInterface();
		ser->SetUniqueID(kUniqueId);
		ser->SetSaveCallback(OnSave);
		ser->SetLoadCallback(OnLoad);
		ser->SetRevertCallback(OnRevert);
	}

	void Begin()
	{
		static std::atomic<bool> started{ false };
		if (started.exchange(true)) {
			return;
		}
		std::thread([]() {
			for (;;) {
				std::this_thread::sleep_for(std::chrono::seconds(kPollSeconds));
				SKSE::GetTaskInterface()->AddTask(Tick);
			}
		}).detach();
		SKSE::log::info("Spoilage: update thread running");
	}

	bool Active() { return Settings::Get().nativeSpoilage; }

	std::string DebugState(RE::FormID a_container, RE::FormID a_food)
	{
		std::scoped_lock l(lock);
		int n = 0;
		for (const auto& b : batches) {
			n += (b.container == a_container && b.food == a_food) ? b.count : 0;
		}
		const auto rate = rates.find(a_food);
		return std::format("active {}, nativeSpoilage {}, enableGlobal {}, ready {}, batches {} ({} here), rate {}, items of this food tracked {}", active,
			Settings::Get().nativeSpoilage, d.enable ? d.enable->value : -1.0f, d.ready, batches.size(), n, rate != rates.end() ? rate->second : -1.0f, n);
	}

	Freshness FreshnessOf(RE::FormID a_container, RE::FormID a_food)
	{
		Freshness out;
		std::scoped_lock l(lock);
		const auto rate = rates.find(a_food);
		if (!active || rate == rates.end() || rate->second <= 0.0f) {
			return out;
		}
		double sum = 0.0;
		int    n = 0;
		for (const auto& b : batches) {
			if (b.container == a_container && b.food == a_food) {
				sum += static_cast<double>(b.perished) * b.count;
				n += b.count;
			}
		}
		if (n <= 0) {
			return out;
		}
		const float avg = static_cast<float>(sum / n);
		out.valid = true;
		out.rotted = std::clamp(avg / rate->second, 0.0f, 1.0f);
		out.hoursLeft = std::max(0.0f, (rate->second - avg) / std::max(speed, 0.01f));
		return out;
	}

bool WorldActive() { return Settings::Get().nativeSpoilage && Settings::Get().worldSpoilage; }

	Stats Snapshot(std::size_t a_maxRows)
	{
		Stats s;
		std::scoped_lock l(lock);
		s.active = active;
		s.speed = speed;
		s.batches = static_cast<int>(batches.size());
		for (const auto& w : worlds) {
			s.worldContainers += w.swaps.empty() ? 0 : 1;
		}
		struct Entry
		{
			const Batch* b;
			float        hoursLeft;
			float        max;
		};
		std::vector<Entry> list;
		for (const auto& b : batches) {
			s.items += b.count;
			const auto  rate = rates.find(b.food);
			const float max = rate != rates.end() ? rate->second : 0.0f;
			list.push_back({ &b, max > 0.0f ? std::max(0.0f, (max - b.perished) / std::max(speed, 0.01f)) : 0.0f, max });
		}
		std::ranges::sort(list, [](const Entry& a, const Entry& b) { return a.hoursLeft < b.hoursLeft; });
		for (const auto& e : list) {
			if (s.rows.size() >= a_maxRows) {
				break;
			}
			Row r;
			auto f = names.find(e.b->food);
			auto c = names.find(e.b->container);
			r.food = f != names.end() ? f->second : "?";
			r.container = c != names.end() ? c->second : "?";
			r.count = e.b->count;
			r.rottedFraction = e.max > 0.0f ? std::clamp(e.b->perished / e.max, 0.0f, 1.0f) : 0.0f;
			r.hoursLeft = e.hoursLeft;
			s.rows.push_back(std::move(r));
		}
		return s;
	}
}
