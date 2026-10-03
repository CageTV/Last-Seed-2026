#include "PCH.h"
#include "Freshness.h"
#include "Settings.h"
#include "Spoilage.h"

#include "SKSEMenuFramework.h"

namespace Freshness
{
	namespace
	{
		using namespace ImGuiMCP;

		// Where the bar goes, as fractions of the screen: between the "weight / value" row and the description of the item card
		// (measured on the user's inventory at 2560x1440).
		constexpr float kLeft = 0.6675f;
		constexpr float kRight = 0.8610f;
		constexpr float kTop = 0.7745f;
		constexpr float kThickness = 4.0f;  // pixels at 1080p

		struct Hover
		{
			bool                           valid = false;
			float                          rotted = 0.0f;
			std::chrono::steady_clock::time_point at{};
		};

		std::mutex        lock;
		Hover             hover;
		std::atomic<bool> pending{ false };

		// Game thread: which item is under the cursor, and how fresh is it.
		void Update()
		{
			Hover next;
			if (auto* ui = RE::UI::GetSingleton()) {
				RE::ItemList* list = nullptr;
				if (ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME)) {
					if (auto menu = ui->GetMenu<RE::InventoryMenu>()) {
						list = menu->GetRuntimeData().itemList;
					}
				} else if (ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME)) {
					if (auto menu = ui->GetMenu<RE::ContainerMenu>()) {
						list = menu->GetRuntimeData().itemList;
					}
				}
				if (auto* item = list ? list->GetSelectedItem() : nullptr; item && item->data.objDesc && item->data.objDesc->object) {
					RE::FormID owner = 0x14;
					RE::TESObjectREFRPtr ref;
					if (RE::LookupReferenceByHandle(item->data.owner, ref) && ref) {
						owner = ref->GetFormID();
					}
					const auto f = Spoilage::FreshnessOf(owner, item->data.objDesc->object->GetFormID());
					if (f.valid) {
						next.valid = true;
						next.rotted = f.rotted;
					}
				}
			}
			next.at = std::chrono::steady_clock::now();
			{
				std::scoped_lock l(lock);
				hover = next;
			}
			pending = false;
		}

		bool CardMenuOpen()
		{
			auto* ui = RE::UI::GetSingleton();
			return ui && (ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME) || ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME));
		}

		ImU32 Col(float a_r, float a_g, float a_b, float a_a)
		{
			return IM_COL32(static_cast<int>(a_r), static_cast<int>(a_g), static_cast<int>(a_b), static_cast<int>(std::clamp(a_a, 0.0f, 1.0f) * 255.0f));
		}
	}

	void Draw(ImDrawList* a_dl, float a_displayW, float a_displayH)
	{
		if (!Settings::Get().freshnessBar || !Spoilage::Active() || !CardMenuOpen()) {
			return;
		}
		if (!pending.exchange(true)) {
			SKSE::GetTaskInterface()->AddTask(Update);
		}
		Hover h;
		{
			std::scoped_lock l(lock);
			h = hover;
		}
		if (!h.valid || std::chrono::steady_clock::now() - h.at > std::chrono::milliseconds(400)) {
			return;
		}

		const float x0 = a_displayW * kLeft;
		const float x1 = a_displayW * kRight;
		const float y = a_displayH * kTop;
		const float t = std::max(2.0f, kThickness * a_displayH / 1080.0f);
		const float fresh = std::clamp(1.0f - h.rotted, 0.0f, 1.0f);

		ImDrawListManager::AddRectFilled(a_dl, ImVec2(x0, y), ImVec2(x1, y + t), Col(10, 12, 16, 0.6f), 0.0f, 0);
		if (fresh <= 0.0f) {
			return;
		}
		// green -> amber -> red as the food rots
		float r, g;
		if (fresh > 0.5f) {
			const float k = (1.0f - fresh) * 2.0f;  // 0 at full, 1 at half
			r = 120.0f + 115.0f * k;
			g = 205.0f - 25.0f * k;
		} else {
			const float k = fresh * 2.0f;  // 1 at half, 0 at empty
			r = 225.0f + 20.0f * (1.0f - k);
			g = 70.0f + 110.0f * k;
		}
		ImDrawListManager::AddRectFilled(a_dl, ImVec2(x0, y), ImVec2(x0 + (x1 - x0) * fresh, y + t), Col(r, g, 70.0f, 0.95f), 0.0f, 0);
	}
}
