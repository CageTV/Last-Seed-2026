/*
 * Last Seed 2026
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
 */
#include "PCH.h"
#include "GridInv.h"
#include "Settings.h"
#include "Spoilage.h"

#include "GridInventoryAPI.h"  // vendored verbatim from Grid Inventory (GPL-3.0 with its modding exception), as the header asks

namespace GridInv
{
	namespace
	{
		// 0xRRGGBBAA, as the extension ABI asks: green when fresh, through amber, to red as the food nears spoiling.
		std::uint32_t Tint(float a_rotted)
		{
			const float fresh = std::clamp(1.0f - a_rotted, 0.0f, 1.0f);
			float       r, g;
			if (fresh > 0.5f) {
				const float k = (1.0f - fresh) * 2.0f;
				r = 120.0f + 115.0f * k;
				g = 205.0f - 25.0f * k;
			} else {
				const float k = fresh * 2.0f;
				r = 225.0f + 20.0f * (1.0f - k);
				g = 70.0f + 110.0f * k;
			}
			return (static_cast<std::uint32_t>(r) << 24) | (static_cast<std::uint32_t>(g) << 16) | (70u << 8) | 0xF2u;
		}

		// Render thread, once per visible tile per frame: a hash lookup and nothing more.
		bool GetOverlay(void*, const GridInvAPI::ItemKey* a_key, GridInvAPI::Overlay* a_out)
		{
			if (!a_key || !a_out || !Settings::Get().freshnessBar || !Spoilage::Active()) {
				return false;
			}
			const auto f = Spoilage::FreshnessOf(a_key->owner, a_key->base);
			if (!f.valid) {
				return false;
			}
			a_out->count = 1;
			a_out->badges[0].iconId = 0;                // the host's own empty socket
			a_out->badges[0].tintRGBA = Tint(f.rotted); // drawn as a ring around it
			a_out->badges[0].state = GridInvAPI::kBadgeEmpty;
			return true;
		}

		std::uint32_t GetTooltipLines(void*, const GridInvAPI::ItemKey*, GridInvAPI::TooltipLine*, std::uint32_t)
		{
			return 0;  // Grid Inventory's tooltip does not ask extensions for lines
		}

		std::uint32_t OfferDrop(void*, const GridInvAPI::DropQuery*)
		{
			return GridInvAPI::kDropReject;
		}

		GridInvAPI::Provider provider{
			sizeof(GridInvAPI::Provider), GridInvAPI::kABIVersion, "Last Seed", nullptr, GetOverlay, GetTooltipLines, OfferDrop
		};

		// Not filtered by sender, so only our own message types may be acted on (see the header).
		void OnApiMessage(SKSE::MessagingInterface::Message* a_msg)
		{
			if (!a_msg || a_msg->type != GridInvAPI::kMsgHostReady || !a_msg->data || a_msg->dataLen < sizeof(GridInvAPI::HostReady)) {
				return;
			}
			const auto* ready = static_cast<const GridInvAPI::HostReady*>(a_msg->data);
			if (ready->abiVersion != GridInvAPI::kABIVersion) {
				SKSE::log::warn("Grid Inventory announced extension ABI {} (this build speaks {}); not registering", ready->abiVersion, GridInvAPI::kABIVersion);
				return;
			}
			const bool sent = SKSE::GetMessagingInterface()->Dispatch(GridInvAPI::kMsgRegisterProvider, &provider, sizeof(provider), GridInvAPI::kHostPluginName);
			SKSE::log::info("Grid Inventory found: registered as a freshness provider ({})", sent ? "sent" : "NOT delivered");
		}
	}

	void Register()
	{
		SKSE::GetMessagingInterface()->RegisterListener(nullptr, OnApiMessage);
	}
}
