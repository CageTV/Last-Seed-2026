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

		// The bar sits just under the divider below the item's name, which is the one part of the card that never moves relative to
		// the card (descriptions grow and shrink below it). The card's own height is read from the menu movie; the divider is a fixed
		// distance below the top of its clip. Horizontal extent: the divider's, as fractions of the screen (measured at 2560x1440).
		constexpr const char* kCardClip = "_root.Menu_mc.itemCardFadeHolder";
		constexpr float       kDividerBelowTop = 51.0f;  // stage units (the stage is 720 high) from the clip's _y to the divider
		constexpr float       kGapBelowDivider = 2.5f;   // stage units
		constexpr float       kFallbackTop = 0.6775f;    // when the card clip can't be read
		constexpr float       kThickness = 4.0f;         // pixels at 1080p

		struct Hover
		{
			bool                                  valid = false;
			float                                 rotted = 0.0f;
			bool                                  haveCard = false;
			float                                 clipTop = 0.0f;  // fraction of the stage height
			std::chrono::steady_clock::time_point at{};
		};

		std::mutex        lock;
		Hover             hover;
		std::atomic<bool> pending{ false };

		// Where the item card's clip starts, as a fraction of the stage height.
		bool ClipTop(const RE::GPtr<RE::GFxMovieView>& a_view, float& a_out)
		{
			if (!a_view) {
				return false;
			}
			const auto   frame = a_view->GetVisibleFrameRect();
			const double fh = frame.bottom - frame.top;
			RE::GFxValue clip, y;
			if (fh <= 1.0 || !a_view->GetVariable(&clip, kCardClip) || !clip.IsObject() || !clip.GetMember("_y", &y) || !y.IsNumber()) {
				return false;
			}
			a_out = static_cast<float>((y.GetNumber() - frame.top) / fh);
			return true;
		}

		// Game thread: which item is under the cursor, and how fresh is it.
		void Update()
		{
			static RE::FormID lastLogged = 0;
			Hover             next;
			if (auto* ui = RE::UI::GetSingleton()) {
				RE::ItemList*              list = nullptr;
				RE::GPtr<RE::GFxMovieView> view;
				if (ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME)) {
					if (auto menu = ui->GetMenu<RE::InventoryMenu>()) {
						list = menu->GetRuntimeData().itemList;
						view = menu->uiMovie;
					}
				} else if (ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME)) {
					if (auto menu = ui->GetMenu<RE::ContainerMenu>()) {
						list = menu->GetRuntimeData().itemList;
						view = menu->uiMovie;
					}
				}
				if (auto* item = list ? list->GetSelectedItem() : nullptr; item && item->data.objDesc && item->data.objDesc->object) {
					RE::FormID           owner = 0x14;
					RE::TESObjectREFRPtr ref;
					if (RE::LookupReferenceByHandle(item->data.owner, ref) && ref) {
						owner = ref->GetFormID();
					}
					const auto food = item->data.objDesc->object->GetFormID();
					const auto f = Spoilage::FreshnessOf(owner, food);
					if (f.valid) {
						next.valid = true;
						next.rotted = f.rotted;
						next.haveCard = ClipTop(view, next.clipTop);
						if (food != lastLogged) {  // once per food hovered
							lastLogged = food;
							SKSE::log::info("Freshness: {:08X} in {:08X} is {:.0f}% rotted; card clip {}", food, owner, f.rotted * 100.0f, next.haveCard ? "found" : "NOT found");
						}
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

		const auto& s = Settings::Get();
		const float x0 = a_displayW * s.freshLeft;
		const float x1 = a_displayW * std::min(1.0f, s.freshLeft + s.freshWidth);
		const float y = a_displayH * ((h.haveCard ? h.clipTop + (kDividerBelowTop + kGapBelowDivider) / 720.0f : kFallbackTop) + s.freshOffsetY / 720.0f);
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
