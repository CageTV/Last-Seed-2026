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

		// Fallback place for the bar, as fractions of the screen, used when the item card's own position can't be read.
		constexpr float kLeft = 0.6675f;
		constexpr float kRight = 0.8610f;
		constexpr float kTop = 0.7745f;
		constexpr float kThickness = 4.0f;  // pixels at 1080p

		struct Hover
		{
			bool                                  valid = false;
			float                                 rotted = 0.0f;
			bool                                  haveCard = false;
			float                                 cardL = 0.0f, cardT = 0.0f, cardR = 0.0f, cardB = 0.0f;  // fractions of the screen
			std::chrono::steady_clock::time_point at{};
		};

		std::mutex        lock;
		Hover             hover;
		std::atomic<bool> pending{ false };

		// The item card's rectangle in the menu movie, as fractions of the visible stage. Adds up _x/_y along the parent chain; scale is
		// assumed to be 100% (true for SkyUI and the skins built on it).
		bool CardRect(RE::ItemCard* a_card, Hover& a_out, double& a_x, double& a_y, double& a_w, double& a_h)
		{
			if (!a_card || !a_card->view || !a_card->obj.IsObject()) {
				return false;
			}
			const auto frame = a_card->view->GetVisibleFrameRect();
			const double fw = frame.right - frame.left;
			const double fh = frame.bottom - frame.top;
			if (fw <= 1.0 || fh <= 1.0) {
				return false;
			}
			RE::GFxValue cur = a_card->obj;
			RE::GFxValue v;
			a_w = a_h = a_x = a_y = 0.0;
			if (cur.GetMember("_width", &v) && v.IsNumber()) {
				a_w = v.GetNumber();
			}
			if (cur.GetMember("_height", &v) && v.IsNumber()) {
				a_h = v.GetNumber();
			}
			for (int i = 0; i < 12; ++i) {
				if (cur.GetMember("_x", &v) && v.IsNumber()) {
					a_x += v.GetNumber();
				}
				if (cur.GetMember("_y", &v) && v.IsNumber()) {
					a_y += v.GetNumber();
				}
				RE::GFxValue parent;
				if (!cur.GetMember("_parent", &parent) || !parent.IsObject()) {
					break;
				}
				cur = parent;
			}
			if (a_w <= 1.0 || a_h <= 1.0) {
				return false;
			}
			a_out.cardL = static_cast<float>((a_x - frame.left) / fw);
			a_out.cardT = static_cast<float>((a_y - frame.top) / fh);
			a_out.cardR = static_cast<float>((a_x + a_w - frame.left) / fw);
			a_out.cardB = static_cast<float>((a_y + a_h - frame.top) / fh);
			return true;
		}

		// Diagnostics: asks the menu movie for the card clip under the usual names and logs what answers, once per menu session.
		void ProbeCard(RE::GPtr<RE::GFxMovieView> a_view)
		{
			static bool probed = false;
			if (probed || !a_view) {
				return;
			}
			probed = true;
			const auto frame = a_view->GetVisibleFrameRect();
			SKSE::log::info("Freshness: stage {:.0f},{:.0f} to {:.0f},{:.0f}", frame.left, frame.top, frame.right, frame.bottom);
			for (const char* path : { "_root.Menu_mc.ItemCard_mc", "_root.Menu_mc.itemCard", "_root.Menu_mc.ItemCard", "_root.Menu_mc.itemCardFadeHolder",
					 "_root.Menu_mc.ItemCardFadeHolder_mc", "_root.Menu_mc.ItemCard_mc.ItemCardMovie", "_level0.Menu_mc.ItemCard_mc" }) {
				RE::GFxValue v;
				if (!a_view->GetVariable(&v, path) || !v.IsObject()) {
					SKSE::log::info("Freshness: probe {} -> not found", path);
					continue;
				}
				const auto num = [&](const char* m) {
					RE::GFxValue n;
					return (v.GetMember(m, &n) && n.IsNumber()) ? n.GetNumber() : -9999.0;
				};
				SKSE::log::info("Freshness: probe {} -> _x {:.0f} _y {:.0f} _width {:.0f} _height {:.0f} _xscale {:.0f} _visible {}", path, num("_x"), num("_y"), num("_width"),
					num("_height"), num("_xscale"), num("_visible"));
			}
		}

		// Game thread: which item is under the cursor, and how fresh is it.
		void Update()
		{
			static RE::FormID lastLogged = 0;
			Hover             next;
			if (auto* ui = RE::UI::GetSingleton()) {
				RE::ItemList* list = nullptr;
				RE::ItemCard* card = nullptr;
				if (ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME)) {
					if (auto menu = ui->GetMenu<RE::InventoryMenu>()) {
						list = menu->GetRuntimeData().itemList;
						card = menu->GetRuntimeData().itemCard;
						ProbeCard(menu->uiMovie);
					}
				} else if (ui->IsMenuOpen(RE::ContainerMenu::MENU_NAME)) {
					if (auto menu = ui->GetMenu<RE::ContainerMenu>()) {
						list = menu->GetRuntimeData().itemList;
						card = menu->GetRuntimeData().itemCard;
						ProbeCard(menu->uiMovie);
					}
				}
				if (auto* item = list ? list->GetSelectedItem() : nullptr; item && item->data.objDesc && item->data.objDesc->object) {
					RE::FormID owner = 0x14;
					RE::TESObjectREFRPtr ref;
					if (RE::LookupReferenceByHandle(item->data.owner, ref) && ref) {
						owner = ref->GetFormID();
					}
					const auto food = item->data.objDesc->object->GetFormID();
					const auto f = Spoilage::FreshnessOf(owner, food);
					if (f.valid) {
						next.valid = true;
						next.rotted = f.rotted;
					}
					double x = 0, y = 0, w = 0, h = 0;
					next.haveCard = CardRect(card, next, x, y, w, h);
					if (food != lastLogged) {  // once per item hovered, so the log shows what the bar is working from
						lastLogged = food;
						SKSE::log::info("Freshness: hovering {:08X} owner {:08X}: tracked {}, rotted {:.2f} [{}]; card clip _x/_y/_w/_h {:.0f}/{:.0f}/{:.0f}/{:.0f}", food, owner, f.valid, f.rotted,
							Spoilage::DebugState(owner, food), x, y, w, h);
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

		float x0 = a_displayW * kLeft;
		float x1 = a_displayW * kRight;
		float y = a_displayH * kTop;
		if (h.haveCard) {  // along the top edge of the item card, inside it
			const float inset = (h.cardR - h.cardL) * 0.04f;
			x0 = a_displayW * (h.cardL + inset);
			x1 = a_displayW * (h.cardR - inset);
			y = a_displayH * h.cardT + 8.0f * a_displayH / 1080.0f;
		}
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
