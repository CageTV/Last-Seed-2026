#include "PCH.h"
#include "Ids.h"
#include "NativeFoodLists.h"
#include "Game.h"
#include "McmTable.h"

#include "SKSEMenuFramework.h"

namespace NativeFoodLists
{
	namespace
	{
		using namespace ImGuiMCP;

		constexpr int         kCategoryCount = 21;      // kFoodLists[0..20]: the food and drink categories (20 = not food)
		constexpr int         kIdxPreserved = 21;
		constexpr int         kIdxSalted = 22;
		constexpr int         kIdxHungerFirst = 23;     // light, medium, filling, hearty
		constexpr int         kIdxAlcoholFirst = 27;    // weak, moderate, strong alcohol, weak and strong skooma
		constexpr int         kIdxSystem = 32;          // Last Seed's own foods: not editable
		constexpr int         kTypeAlcohol = 18;        // food type of the alcoholic drinks list (index 17 + 1)
		constexpr RE::FormID  kDataStoreQuest = 0x00B6CD;  // _Seed_FoodDatastoreHandlerQuest

		RE::BGSListForm* List(int a_index)
		{
			auto* dh = RE::TESDataHandler::GetSingleton();
			return (dh && a_index >= 0 && a_index < static_cast<int>(std::size(mcm::kFoodLists))) ? dh->LookupForm<RE::BGSListForm>(ids::Ls(mcm::kFoodLists[a_index].formId), "LastSeed.esp") : nullptr;
		}

		bool OnList(int a_index, const RE::TESForm* a_form)
		{
			auto* list = List(a_index);
			return list && a_form && list->HasForm(a_form);
		}

		// Every form on a list, including those added while playing (Papyrus AddForm).
		std::vector<RE::TESForm*> Members(RE::BGSListForm* a_list)
		{
			std::vector<RE::TESForm*> out;
			if (!a_list) {
				return out;
			}
			a_list->ForEachForm([&](auto&& a_form) {
				out.push_back(compat::Ptr(a_form));
				return RE::BSContainer::ForEachResult::kContinue;
			});
			return out;
		}

		// SeedUtil's global functions through Papyrus, as the SkyUI menu called them.
		template <class... Args>
		void CallSeedUtil(const char* a_function, Args... a_args)
		{
			SKSE::GetTaskInterface()->AddTask([=]() mutable {
				auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
				if (vm) {
					const bool ok = vm->DispatchStaticCall("SeedUtil", a_function, RE::MakeFunctionArguments(std::move(a_args)...), callback);
					SKSE::log::info("Food editor: SeedUtil.{}() -> {}", a_function, ok ? "dispatched" : "FAILED");
				}
			});
		}

		void SetAlcoholType(RE::AlchemyItem* a_food, int a_type)
		{
			SKSE::GetTaskInterface()->AddTask([a_food, a_type]() {
				auto* dh = RE::TESDataHandler::GetSingleton();
				auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				auto* quest = dh ? dh->LookupForm<RE::TESQuest>(ids::Ls(kDataStoreQuest), "LastSeed.esp") : nullptr;
				if (!vm || !quest) {
					return;
				}
				const auto handle = vm->GetObjectHandlePolicy()->GetHandleForObject(RE::TESQuest::FORMTYPE, quest);
				RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
				vm->DispatchMethodCall2(handle, "_Seed_FoodDatastoreHandler", "SetAlcoholType", RE::MakeFunctionArguments(static_cast<RE::AlchemyItem*>(a_food), static_cast<std::int32_t>(a_type)), callback);
			});
		}

		const char* ModName(const RE::TESForm* a_form)
		{
			if (a_form) {
				if (auto* file = a_form->GetFile(0)) {
					return file->GetFilename().data();
				}
			}
			return "?";
		}

		int FirstMembership(const RE::TESForm* a_form, int a_from, int a_count)
		{
			for (int i = 0; i < a_count; ++i) {
				if (OnList(a_from + i, a_form)) {
					return i;
				}
			}
			return -1;
		}
	}

	void Draw()
	{
		if (!Game::Ready() || !Game::IsRunning()) {
			TextDisabled("Last Seed is not running. Start it from the Overview page first.");
			return;
		}
		static int                 listIndex = 0;
		static char                filter[64] = "";
		static RE::FormID          selected = 0;
		static bool                editing = false;
		static int                 lastList = -1;
		static std::vector<RE::TESForm*> members;
		static float               refreshTimer = 0.0f;

		SeparatorText("Food and drink lists");
		const char* labels[kCategoryCount];
		for (int i = 0; i < kCategoryCount; ++i) {
			labels[i] = mcm::kFoodLists[i].label;
		}
		if (Combo("List", &listIndex, labels, kCategoryCount) || lastList != listIndex) {
			lastList = listIndex;
			members = Members(List(listIndex));
			selected = 0;
			editing = false;
		}
		refreshTimer -= GetIO()->DeltaTime;
		if (refreshTimer <= 0.0f) {  // lists change when an edit lands, so look again now and then
			refreshTimer = 1.0f;
			members = Members(List(listIndex));
		}
		InputText("Search", filter, sizeof(filter));

		std::string needle = filter;
		std::ranges::transform(needle, needle.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (BeginChild("##fooditems", ImVec2(0.0f, 240.0f), true)) {
			for (auto* form : members) {
				const char* name = form->GetName();
				if (!name || !name[0]) {
					continue;
				}
				if (!needle.empty()) {
					std::string lower = name;
					std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
					if (lower.find(needle) == std::string::npos) {
						continue;
					}
				}
				PushID(static_cast<int>(form->GetFormID()));
				if (Selectable(name, selected == form->GetFormID())) {
					selected = form->GetFormID();
					editing = false;
				}
				PopID();
			}
		}
		EndChild();

		auto* food = selected ? RE::TESForm::LookupByID(selected) : nullptr;
		if (!food) {
			TextDisabled("Pick a food to see its properties.");
			return;
		}

		Spacing();
		SeparatorText(food->GetName());
		Text("Source mod: %s", ModName(food));
		Text("On these lists:");
		for (int i = 0; i < static_cast<int>(std::size(mcm::kFoodLists)); ++i) {
			if (OnList(i, food)) {
				BulletText("%s", mcm::kFoodLists[i].label);
			}
		}

		auto* potion = food->As<RE::AlchemyItem>();
		if (!potion) {
			return;
		}
		Spacing();
		const bool system = OnList(kIdxSystem, food);
		BeginDisabled(system);
		Checkbox("Edit this item", &editing);
		EndDisabled();
		if (system) {
			TextDisabled("This is one of Last Seed's own items and cannot be edited.");
			return;
		}
		if (!editing) {
			return;
		}

		// Category: which list the food is on. Same calls as the SkyUI menu's list dropdown.
		int category = std::max(0, FirstMembership(food, 0, kCategoryCount));
		if (Combo("Category", &category, labels, kCategoryCount)) {
			CallSeedUtil("ClearFoodType", potion);
			if (category == 20) {
				CallSeedUtil("SetAsNotFood", potion, true);
			} else if (category == 19) {
				CallSeedUtil("setBloodPotion", potion);
			} else {
				CallSeedUtil("SetFoodType", potion, static_cast<std::int32_t>(category + 1));
			}
		}

		const int type = FirstMembership(food, 0, 19) + 1;  // 1..17 food, 18 alcohol, 19 non alcoholic drink; 0 = none of these
		if (type > 0 && type < 18) {
			int hunger = std::max(0, FirstMembership(food, kIdxHungerFirst, 4));
			if (Combo("Hunger restored", &hunger, mcm::kFood_HungerMenu, 4)) {
				CallSeedUtil("clearFoodRestoreAmount", potion);
				CallSeedUtil("setFoodRestoreAmount", potion, static_cast<std::int32_t>(hunger + 1));
			}
			bool preserved = OnList(kIdxPreserved, food);
			if (Checkbox("Preserved (does not spoil)", &preserved)) {
				CallSeedUtil("SetFoodPreserved", potion, preserved);
				if (!preserved) {
					CallSeedUtil("SetFoodSalted", potion, false);
				}
			}
			BeginDisabled(!OnList(kIdxPreserved, food));
			bool salted = OnList(kIdxSalted, food);
			if (Checkbox("Salted", &salted)) {
				CallSeedUtil("SetFoodSalted", potion, salted);
			}
			EndDisabled();
		}
		if (type == kTypeAlcohol) {
			int alcohol = std::max(0, FirstMembership(food, kIdxAlcoholFirst, 5));
			if (Combo("Alcohol type", &alcohol, mcm::kFood_AlcoholMenu, 5)) {
				SetAlcoholType(potion, alcohol + 1);
			}
		}
		TextWrapped("Portions (food that turns into several smaller items) can still be edited in Last Seed's SkyUI menu for now.");
	}
}
