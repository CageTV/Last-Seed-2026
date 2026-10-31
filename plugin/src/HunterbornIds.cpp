/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#include "PCH.h"
#include "HunterbornIds.h"

namespace HunterbornIds
{
	namespace
	{
#include "HunterbornEslMap.inc"  // Hunterborn.esp: old local id -> id in the ESL edition

		bool Light()
		{
			static int state = -1;
			if (state < 0) {
				auto*       dh = RE::TESDataHandler::GetSingleton();
				const auto* file = dh ? dh->LookupModByName("Hunterborn.esp") : nullptr;
				if (!file) {
					return false;  // not in the load order (or not known yet): ask again later
				}
				state = file->IsLight() ? 1 : 0;
			}
			return state == 1;
		}
	}

	unsigned Fix(const char* a_plugin, unsigned a_local)
	{
		if (!a_plugin || _stricmp(a_plugin, "Hunterborn.esp") != 0 || !Light()) {
			return a_local;
		}
		const auto* end = std::end(kEslMap);
		const auto* it = std::lower_bound(std::begin(kEslMap), end, a_local & 0xFFFFFF, [](const auto& e, unsigned v) { return e.first < v; });
		return it != end && it->first == (a_local & 0xFFFFFF) ? it->second : a_local;
	}
}
