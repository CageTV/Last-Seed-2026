#include "PCH.h"
#include <unordered_set>
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
		constexpr std::uint32_t kVersion = 1;

		constexpr int    kPollSeconds = 3;        // real seconds between checks
		constexpr double kAdvanceStepHours = 0.1; // game hours between ageing steps
		constexpr double kReconcileHours = 1.0;   // game hours between inventory re-checks
		constexpr float  kMergeTolerance = 0.05f; // batches of the same food this close in age are merged
		constexpr double kInvalidTime = -1.0;

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

		template <class T>
		T* Look(RE::FormID a_id)
		{
			auto* dh = RE::TESDataHandler::GetSingleton();
			return dh ? dh->LookupForm<T>(a_id, "LastSeed.esp") : nullptr;
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
			return false;
		}

		bool IsTracked(RE::FormID a_id)
		{
			if (a_id == 0x14 || a_id == d.provisions) {
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
				auto* food = RE::TESForm::LookupByID<RE::TESBoundObject>(item.food);
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
			}
		}

		// ---- save games (SKSE co-save) ----

		void OnSave(SKSE::SerializationInterface* a_intfc)
		{
			std::scoped_lock l(lock);
			if (a_intfc->OpenRecord(kRecordTime, kVersion)) {
				a_intfc->WriteRecordData(lastAdvance);
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
			lastAdvance = kInvalidTime;
			lastReconcile = kInvalidTime;
			std::uint32_t type, version, length;
			while (a_intfc->GetNextRecordInfo(type, version, length)) {
				if (version != kVersion) {
					continue;
				}
				if (type == kRecordTime) {
					a_intfc->ReadRecordData(lastAdvance);
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
			SKSE::log::info("Spoilage: loaded {} batches from the save", batches.size());
		}

		void OnRevert(SKSE::SerializationInterface*)
		{
			std::scoped_lock l(lock);
			batches.clear();
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
			d.cats[i].spoiled = Look<RE::TESBoundObject>(c.spoiled);
			d.cats[i].rate = Look<RE::TESGlobal>(c.rate);
			if (!d.cats[i].list || !d.cats[i].spoiled || !d.cats[i].rate) {
				SKSE::log::error("Spoilage: category '{}' is missing a form (list {}, spoiled {}, rate {})", c.name, !!d.cats[i].list, !!d.cats[i].spoiled, !!d.cats[i].rate);
			}
		}
		d.preserved = Look<RE::BGSListForm>(ids::PreservedList);
		d.spoiledFoods = Look<RE::BGSListForm>(ids::SpoiledFoodsList);
		d.perishedFood = Look<RE::TESBoundObject>(ids::PerishedFood);
		d.iceTeeth = Look<RE::TESBoundObject>(ids::IceWraithTeeth);
		d.iceTeethOld = Look<RE::TESBoundObject>(ids::IceWraithTeethOld);
		d.iceRate = Look<RE::TESGlobal>(ids::IceWraithTeethRate);
		d.enable = Look<RE::TESGlobal>(ids::SpoilageEnable);
		d.remove = Look<RE::TESGlobal>(ids::SpoilageRemove);
		d.speed = Look<RE::TESGlobal>(ids::SpoilTemperatureMulti);
		if (auto* prov = Look<RE::TESObjectREFR>(ids::ProvisionsContainer)) {
			d.provisions = prov->GetFormID();
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

	Stats Snapshot(std::size_t a_maxRows)
	{
		Stats s;
		std::scoped_lock l(lock);
		s.active = active;
		s.speed = speed;
		s.batches = static_cast<int>(batches.size());
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
