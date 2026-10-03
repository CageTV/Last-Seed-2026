#include "PCH.h"
#include "Ids.h"
#include "EslMap.h"

namespace ids
{
	namespace
	{
		// Is the plugin loaded as a light (ESL) plugin? -1 = not known yet (data not loaded, or the plugin is not there).
		int Light(const char* a_plugin, std::atomic<int>& a_cache)
		{
			const int cached = a_cache.load();
			if (cached >= 0) {
				return cached;
			}
			auto* dh = RE::TESDataHandler::GetSingleton();
			auto* file = dh ? dh->LookupModByName(a_plugin) : nullptr;
			if (!file) {
				return -1;
			}
			const int light = file->IsLight() ? 1 : 0;
			a_cache = light;
			SKSE::log::info("{} is loaded as a {} plugin", a_plugin, light ? "light (ESL)" : "regular");
			return light;
		}

		template <std::size_t N>
		RE::FormID Translate(const eslmap::Pair (&a_table)[N], RE::FormID a_local, const char* a_plugin)
		{
			const RE::FormID local = a_local & 0x00FFFFFF;
			const auto*      end = std::end(a_table);
			const auto*      it = std::lower_bound(std::begin(a_table), end, local, [](const eslmap::Pair& a_p, RE::FormID a_id) { return a_p.from < a_id; });
			if (it != end && it->from == local) {
				return it->to;
			}
			static std::atomic<bool> warned{ false };
			if (!warned.exchange(true)) {
				SKSE::log::error("{}: no light-plugin id known for {:06X} (further misses are not logged)", a_plugin, local);
			}
			return a_local;
		}
	}

	RE::FormID Ls(RE::FormID a_local)
	{
		static std::atomic<int> cache{ -1 };
		return Light("LastSeed.esp", cache) == 1 ? Translate(eslmap::kLastSeed, a_local, "LastSeed.esp") : a_local;
	}

	RE::FormID Frostfall(RE::FormID a_local)
	{
		static std::atomic<int> cache{ -1 };
		return (Light("Frostfall.esp", cache) == 1 && std::size(eslmap::kFrostfall) > 0) ? Translate(eslmap::kFrostfall, a_local, "Frostfall.esp") : a_local;
	}
}
