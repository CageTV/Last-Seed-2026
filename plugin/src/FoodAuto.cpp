/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#include "PCH.h"
#include "FoodAuto.h"
#include "Classifier.h"
#include "Ids.h"
#include "McmTable.h"
#include "Settings.h"

namespace FoodAuto
{
	namespace
	{
		constexpr int kCategoryCount = 20;  // mcm::kFoodLists[0..19]: the food and drink kinds Last Seed knows a food by (19 = blood potions)
		constexpr int kIdxNotFood = 20;

		std::vector<RE::AlchemyItem*> tokens;  // the items put on the Not Food list, for OnGameLoaded

		RE::BGSKeyword* Kw(RE::FormID a_id) { return RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSKeyword>(a_id, "Update.esm"); }

		RE::BGSListForm* LsList(int a_index)
		{
			return RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSListForm>(ids::Ls(mcm::kFoodLists[a_index].formId), "LastSeed.esp");
		}

		struct Keywords
		{
			RE::BGSKeyword *meat{}, *fish{}, *seafood{}, *stewSimple{}, *stewComplex{}, *pastrySmall{}, *pastryLarge{}, *vegetable{}, *fruit{}, *bread{},
				*cheese{}, *cheeseBowl{}, *treat{}, *milk{}, *nonAlcohol{}, *skoomaWeak{}, *skoomaStrong{};
			RE::BGSKeyword* alcohol[3]{};  // weak, moderate, strong
			RE::BGSKeyword* hunger[4]{};   // light, medium, filling, hearty
			RE::BGSKeyword* raw{};         // Skyrim's VendorItemFoodRaw
			std::vector<RE::BGSKeyword*> kinds;  // every kind keyword Last Seed's classifier reads: a food with one is already described

			bool Load()
			{
				meat = Kw(0xAF1004);
				fish = Kw(0xAF1007);
				seafood = Kw(0xAF1006);
				stewSimple = Kw(0xAF1012);
				stewComplex = Kw(0xD9FF03);
				pastrySmall = Kw(0xAF1010);
				pastryLarge = Kw(0xD9FF02);
				vegetable = Kw(0xAF1009);
				fruit = Kw(0xAF1008);
				bread = Kw(0xAF100F);
				cheese = Kw(0xAF100A);
				cheeseBowl = Kw(0xAF100B);
				treat = Kw(0xAF1011);
				milk = Kw(0xAF101D);
				nonAlcohol = Kw(0xAF101B);
				skoomaWeak = Kw(0xD9FF06);
				skoomaStrong = Kw(0xD9FF07);
				alcohol[0] = Kw(0xAF101A);
				alcohol[1] = Kw(0xD9FF04);
				alcohol[2] = Kw(0xD9FF05);
				for (int i = 0; i < 4; ++i) {
					hunger[i] = Kw(0xD9FF08 + i);
				}
				raw = RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSKeyword>(0x0A0E56, "Skyrim.esm");
				for (RE::FormID id : { 0xAF1004u, 0xAF1005u, 0xAF1006u, 0xAF1007u, 0xAF1008u, 0xAF1009u, 0xAF100Au, 0xAF100Bu, 0xAF100Fu, 0xAF1010u, 0xAF1011u,
				                       0xAF1012u, 0xAF101Au, 0xAF101Bu, 0xAF101Du, 0xD9FF02u, 0xD9FF03u, 0xD9FF04u, 0xD9FF05u, 0xD9FF06u, 0xD9FF07u, 0xD9FF0Eu }) {
					if (auto* k = Kw(id)) {
						kinds.push_back(k);
					}
				}
				return meat && stewSimple && hunger[0] && hunger[3] && alcohol[0];
			}
		};

		// Survival Mode's warmth and cold effects mark a hot dish (the classifier reads that as "stew-like")
		bool Warm(const RE::AlchemyItem* a_item)
		{
			auto*       dh = RE::TESDataHandler::GetSingleton();
			const auto* warmth = dh->LookupForm<RE::EffectSetting>(0x2EE6, "Update.esm");
			const auto* cold = dh->LookupForm<RE::EffectSetting>(0x2EE5, "Update.esm");
			for (auto* fx : a_item->effects) {
				if (fx && fx->baseEffect && (fx->baseEffect == warmth || fx->baseEffect == cold)) {
					return true;
				}
			}
			return false;
		}

		void Add(RE::AlchemyItem* a_item, RE::BGSKeyword* a_kw)
		{
			if (a_kw && !a_item->HasKeyword(a_kw)) {
				a_item->AddKeyword(a_kw);
			}
		}

		// Drinks the food classifier leaves out on purpose (it only knows Gourmet's foods, ales and wines): skooma, cider, water, coffee, tea...
		bool Drink(const Keywords& k, RE::AlchemyItem* a_item, std::string& a_what)
		{
			const auto words = gclass::Words(a_item->GetName());
			auto       has = [&](std::initializer_list<const char*> a_list) {
                return std::any_of(words.begin(), words.end(), [&](const std::string& w) {
                    return std::any_of(a_list.begin(), a_list.end(), [&](const char* x) { return w == x; });
                });
			};
			RE::BGSKeyword* kw = nullptr;
			if (has({ "skooma", "balmora" })) {
				const bool strong = has({ "double", "distilled", "strong" });
				kw = strong ? k.skoomaStrong : k.skoomaWeak;
				a_what = strong ? "strong skooma" : "weak skooma";
			} else if (has({ "cider", "liquor", "spirits", "jagga" })) {
				kw = k.alcohol[0];
				a_what = "weak alcohol";
			} else if (has({ "water", "coffee", "tea", "juice", "cocoa", "lemonade", "infusion" })) {
				kw = k.nonAlcohol;
				a_what = "non-alcoholic drink";
			}
			if (!kw) {
				return false;
			}
			Add(a_item, kw);
			return true;
		}

		bool Known(RE::AlchemyItem* a_item, RE::BGSListForm* a_notFood, const std::vector<RE::BGSListForm*>& a_lists, const Keywords& k)
		{
			return a_notFood->HasForm(a_item) || std::any_of(a_lists.begin(), a_lists.end(), [&](auto* l) { return l->HasForm(a_item); }) ||
			       std::any_of(k.kinds.begin(), k.kinds.end(), [&](auto* kw) { return a_item->HasKeyword(kw); });
		}

		std::vector<RE::AlchemyItem*> unread;  // foods the classifier could not read, for the count after a save has loaded

		// Items other mods flag as food that are not food, found by a keyword their mod puts on them. Hunting in Skyrim's 73 trap baits ("Deer Musk", "Wolf
		// Chow", "Fox Lure", "Bear Bait" ...) are ingestibles with the FoodItem flag and the vendor-food keyword, and their names carry animal words, so the
		// classifier would take them for raw meat and they would spoil in the player's inventory.
		struct NotFoodKeyword
		{
			const char* plugin;
			unsigned    id;
		};
		constexpr NotFoodKeyword kNotFoodKeywords[] = {
			{ "Hunting in Skyrim.esp", 0x13B64A },  // HISTrapBaitKeyword
		};

		bool HasNotFoodKeyword(RE::TESDataHandler* a_dh, RE::AlchemyItem* a_item)
		{
			static std::vector<RE::BGSKeyword*> resolved;
			static bool                          done = false;
			if (!done) {
				done = true;
				for (const auto& nf : kNotFoodKeywords) {
					if (auto* kw = a_dh->LookupForm<RE::BGSKeyword>(nf.id, nf.plugin)) {
						resolved.push_back(kw);
					}
				}
			}
			return std::any_of(resolved.begin(), resolved.end(), [&](auto* kw) { return a_item->HasKeyword(kw); });
		}

		// The kind and hunger keywords for one classified food; false when the classifier's answer has no Last Seed kind
		// Foods the classifier cannot read from their names, by display name (and plugin, where the name alone could mean something else): Hunterborn's
		// own prepared meals and foraged food ("Morthal Mudders", "Velvet Slaughter", "Edible Root"), Heartland's dishes, a few creature foods. Only
		// foods the classifier left unrecognised are looked up here, and the kind follows what the dish is made of. Without it they would get no
		// hunger value, never spoil, and Last Seed would ask about each the first time it is eaten or stored.
		struct DishRule
		{
			const char*  file;  // lower-case plugin name; the plugin the food comes from
			const char*  name;  // lower-case display name
			gclass::Stat stat;
			gclass::Type type;
			bool         raw;
			bool         drink = false;
		};

		std::string Lower(std::string a_text)
		{
			std::transform(a_text.begin(), a_text.end(), a_text.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
			return a_text;
		}

		bool KnownDish(const std::string& a_file, const std::string& a_name, gclass::Result& a_out)
		{
			using S = gclass::Stat;
			using T = gclass::Type;
			static const DishRule kDishes[] = {
				// Hunterborn.esp
				{ "hunterborn.esp", "edible flower", S::Stamina, T::Vegetable, true },
				{ "hunterborn.esp", "edible root", S::Stamina, T::Vegetable, true },
				{ "hunterborn.esp", "edible mushroom", S::Stamina, T::Vegetable, true },
				{ "hunterborn.esp", "fox herb cutlet", S::Health, T::Meat, false },
				{ "hunterborn.esp", "velvet slaughter", S::Health, T::Fish, false },
				{ "hunterborn.esp", "sweetwolf", S::Health, T::Meat, false },
				{ "hunterborn.esp", "wyrm and chips", S::Health, T::Fish, false },
				{ "hunterborn.esp", "morthal mudders", S::Health, T::Crab, false },
				{ "hunterborn.esp", "ocean's kiss", S::Health, T::Fish, false },
				{ "hunterborn.esp", "farmer's breakfast", S::Stamina, T::Bread, false },
				{ "hunterborn.esp", "wool coat", S::Health, T::Meat, false },
				{ "hunterborn.esp", "fatty fin fry", S::Health, T::Fish, false },
				{ "hunterborn.esp", "predators' price", S::Health, T::Meat, false },
				// Heartland (BSHeartland.esm, BSAssets.esm)
				{ "bsheartland.esm", "agvald's seafood surprise", S::Health, T::Fish, false },
				{ "bsheartland.esm", "mooncake", S::Magicka, T::Dessert, false },
				{ "bsheartland.esm", "sweetcake", S::Magicka, T::Dessert, false },
				{ "bsheartland.esm", "colovian battlecry", S::Stamina, T::Ale, false, true },
				{ "bsassets.esm", "flapjack", S::Stamina, T::Bread, false },
				{ "bsassets.esm", "eyebread", S::Stamina, T::Bread, false },
				// Immersive Creatures
				{ "skyrim immersive creatures special edition.esp", "kwama egg", S::Health, T::Meat, true },
			};
			const std::string file = Lower(a_file);
			const std::string name = Lower(a_name);
			for (const auto& r : kDishes) {
				if (file == r.file && name == r.name) {
					a_out = {};
					a_out.ok = true;
					a_out.why = "known dish";
					a_out.stat = r.stat;
					a_out.type = r.type;
					a_out.raw = r.raw;
					a_out.drink = r.drink;
					a_out.tier = r.raw ? 10 : 25;
					return true;
				}
			}
			return false;
		}

		// Ingestible records that are not food: cooking ingredients and seasonings, creature parts, ritual and token items, and the "buff" items other
		// mods hand out. Filed as not food, so they are never asked about and never spoil.
		bool KnownNotFood(const std::string& a_file, const std::string& a_name)
		{
			static const char* kFiles[] = { "skillsofthewild.esp", "stress and fear.esp", "frostfall.esp", "campfire.esm" };
			static const char* kNames[] = { "salt", "sugar", "seasoning", "oil", "cliffracer tail", "scamp skin", "blood of the hunt", "kollopi essence", "resin distillated",
				"homecooked meal", "sleeping tree sap", "soul husk", "placeholder", "dummy food" };
			static const char* kPrefixes[] = { "itm", "npchuman" };  // complete alchemy & cooking overhaul's engine tokens (ITMFoodEat, NPCHumanDrink...)
			const std::string file = Lower(a_file);
			const std::string name = Lower(a_name);
			for (const char* f : kFiles) {
				if (file == f) {
					return true;
				}
			}
			for (const char* n : kNames) {
				if (name == n) {
					return true;
				}
			}
			for (const char* pre : kPrefixes) {
				if (name.rfind(pre, 0) == 0 && name.size() > std::string_view(pre).size() && std::isupper(static_cast<unsigned char>(a_name[std::string_view(pre).size()]))) {
					return true;
				}
			}
			return false;
		}

		bool Describe(const Keywords& k, RE::AlchemyItem* a_item, const gclass::Result& c, std::string& a_what)
		{
			using T = gclass::Type;
			const int step = c.tier >= 50 ? 2 : c.tier >= 25 ? 1 : 0;
			if (c.drink) {
				const int strength = std::min(2, step + (c.type == T::Wine ? 1 : 0));
				Add(a_item, k.alcohol[strength]);
				a_what = strength == 0 ? "weak alcohol" : strength == 1 ? "moderate alcohol" : "strong alcohol";
				return true;
			}
			RE::BGSKeyword* kind = nullptr;
			bool            hearty = false;
			std::string     name = a_item->GetName();
			std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
			switch (c.type) {
			case T::Meat: kind = k.meat; a_what = "meat"; break;
			case T::Fish: kind = k.fish; a_what = "fish"; break;
			case T::Crab: kind = k.seafood; a_what = "seafood"; break;
			case T::Stew:
			case T::Chowder:
				kind = step == 2 ? k.stewComplex : k.stewSimple;
				hearty = step == 2;
				a_what = "stew";
				break;
			case T::Soup: kind = k.stewSimple; a_what = "soup"; break;
			case T::Pie:
				kind = step == 2 ? k.pastryLarge : k.pastrySmall;
				hearty = step == 2;
				a_what = "pastry";
				break;
			case T::Vegetable: kind = k.vegetable; a_what = "vegetable"; break;
			case T::Fruit: kind = k.fruit; a_what = "fruit"; break;
			case T::Bread: kind = k.bread; a_what = "bread"; break;
			case T::Cheese:
				kind = name.find("bowl") != std::string::npos ? k.cheeseBowl : k.cheese;
				a_what = "cheese";
				break;
			case T::Dessert: kind = k.treat; a_what = "treat"; break;
			default:
				// no type: milk, butter, honey, cooked mushrooms... by the buff the classifier gave it
				if (name.find("milk") != std::string::npos) {
					kind = k.milk;
					a_what = "milk";
				} else if (c.stat == gclass::Stat::Health) {
					kind = k.meat;
					a_what = "meat-like";
				} else if (c.stat == gclass::Stat::Stamina) {
					kind = k.vegetable;
					a_what = "vegetable-like";
				} else if (c.stat == gclass::Stat::Magicka) {
					kind = k.treat;
					a_what = "treat-like";
				}
				break;
			}
			if (!kind) {
				return false;
			}
			Add(a_item, kind);
			if (c.raw) {
				Add(a_item, k.raw);
				a_what = "raw " + a_what;
			}
			const int h = hearty ? 3 : step;
			Add(a_item, k.hunger[h]);
			static constexpr const char* kHunger[] = { "light", "medium", "filling", "hearty" };
			a_what += std::string(", ") + kHunger[h];
			return true;
		}
	}

	Compat::Entry Apply()
	{
		Compat::Entry entry{ "Foods Last Seed does not know: classified automatically", true, false, "" };
		if (!Settings::Get().autoClassifyFoods) {
			entry.detail = "switched off (Last Seed asks about each one)";
			return entry;
		}
		auto* dh = RE::TESDataHandler::GetSingleton();
		Keywords k;
		RE::BGSListForm* notFood = LsList(kIdxNotFood);
		std::vector<RE::BGSListForm*> lists;
		for (int i = 0; i < kCategoryCount; ++i) {
			if (auto* l = LsList(i)) {
				lists.push_back(l);
			}
		}
		if (!dh || !k.Load() || !notFood || lists.size() != kCategoryCount) {
			entry.detail = "Last Seed's keywords or lists not found";
			return entry;
		}

		int classified = 0, notFoodCount = 0, left = 0;
		for (auto* item : dh->GetFormArray<RE::AlchemyItem>()) {
			if (!item || item->IsDeleted() || !item->IsFood() || item->IsPoison()) {
				continue;
			}
			if (Known(item, notFood, lists, k)) {
				continue;  // Last Seed knows it, or will file it by its keywords
			}
			if (HasNotFoodKeyword(dh, item)) {
				notFood->AddForm(item);  // bait, not a meal: never classified, never spoils
				++notFoodCount;
				continue;
			}
			const char*       n = item->GetName();
			const std::string name = (n && n[0]) ? n : "";
			const auto*       file = item->GetFile(0);
			const std::string where = file ? std::string(file->GetFilename()) : "?";

			if (KnownNotFood(where, name)) {
				notFood->AddForm(item);
				++notFoodCount;
				SKSE::log::info("Foods: {} ({}, {:08X}) -> not food (known)", name.empty() ? "(unnamed)" : name, where, item->GetFormID());
				continue;
			}

			gclass::Input in;
			in.name = name;
			const char* ed = item->GetFormEditorID();
			in.editorId = ed ? ed : "";
			in.rawKeyword = k.raw && item->HasKeyword(k.raw);
			in.warmth = Warm(item);
			in.value = item->GetGoldValue();
			auto c = gclass::Classify(in);
			if (!c.ok) {
				KnownDish(where, name, c);
				if (!c.ok && in.rawKeyword) {
					// a raw food from a fishing mod's plugin (Fancy Fishing, the Creation Club fish, ...) is a fish, whatever its species is called
					std::string lowerFile = where;
					std::transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
					if (lowerFile.find("fish") != std::string::npos) {
						c = {};
						c.ok = true;
						c.why = "raw food from a fishing mod";
						c.stat = gclass::Stat::Health;
						c.type = gclass::Type::Fish;
						c.raw = true;
						c.tier = 10;
					}
				}
			}
			std::string what;
			if ((c.ok && Describe(k, item, c, what)) || Drink(k, item, what)) {
				++classified;
				SKSE::log::info("Foods: {} ({}, {:08X}) -> {}", name, where, item->GetFormID(), what);
			} else if (item->GetWeight() <= 0.0f && item->GetGoldValue() <= 0) {
				// no weight, no value, no food words: a token another mod gives the player (CACO's "Not Hungry", "Rested"...), not a food
				notFood->AddForm(item);
				tokens.push_back(item);
				++notFoodCount;
				SKSE::log::info("Foods: {} ({}, {:08X}) -> not food ({})", name.empty() ? "(unnamed)" : name, where, item->GetFormID(), c.why);
			} else {
				++left;
				unread.push_back(item);
				SKSE::log::info("Foods: {} ({}, {:08X}) -> not recognised ({})", name.empty() ? "(unnamed)" : name, where, item->GetFormID(), c.why);
			}
		}
		entry.applied = classified + notFoodCount > 0;
		entry.detail = std::format("{} classified, {} marked as not food, {} not recognised (most are filed by Last Seed's own imports once a save loads)", classified,
			notFoodCount, left);
		return entry;
	}

	void OnGameLoaded()
	{
		auto* notFood = LsList(kIdxNotFood);
		if (!notFood) {
			return;
		}
		int added = 0;
		for (auto* t : tokens) {
			if (!notFood->HasForm(t)) {
				notFood->AddForm(t);
				++added;
			}
		}
		if (added) {
			SKSE::log::info("Foods: {} not-food item(s) put back on Last Seed's Not Food list after loading", added);
		}
		// Last Seed's own imports (CACO, Bruma, Frostfall...) fill its lists from the save, so only now is it known what it would still ask about
		Keywords k;
		std::vector<RE::BGSListForm*> lists;
		for (int i = 0; i < kCategoryCount; ++i) {
			if (auto* l = LsList(i)) {
				lists.push_back(l);
			}
		}
		if (!k.Load()) {
			return;
		}
		int ask = 0;
		for (auto* item : unread) {
			if (!Known(item, notFood, lists, k)) {
				++ask;
				SKSE::log::info("Foods: Last Seed will ask about {} ({:08X}) the first time it is eaten or stored", item->GetName(), item->GetFormID());
			}
		}
		SKSE::log::info("Foods: after loading, {} of {} unrecognised food(s) are still unknown to Last Seed", ask, unread.size());
	}
}
