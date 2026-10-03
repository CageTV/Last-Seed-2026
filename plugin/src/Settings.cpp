#include "PCH.h"
#include "Settings.h"

namespace
{
	const std::filesystem::path kPath = "Data/SKSE/Plugins/LastSeed.ini";
	const std::filesystem::path kFrostfallPath = "Data/SKSE/Plugins/Frostfall.ini";

	std::string Trim(std::string_view a_s)
	{
		const auto b = a_s.find_first_not_of(" \t\r\n");
		if (b == std::string_view::npos) {
			return {};
		}
		const auto e = a_s.find_last_not_of(" \t\r\n");
		return std::string(a_s.substr(b, e - b + 1));
	}

	std::string Lower(std::string a_s)
	{
		std::transform(a_s.begin(), a_s.end(), a_s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return a_s;
	}

	using KeyValues = std::unordered_map<std::string, std::string>;

	// Reads an INI into lower-cased key -> value (section headers are ignored; every key here is unique).
	bool ReadIni(const std::filesystem::path& a_path, KeyValues& a_kv)
	{
		std::ifstream in(a_path);
		if (!in) {
			return false;
		}
		std::string line;
		while (std::getline(in, line)) {
			const auto t = Trim(line);
			if (t.empty() || t[0] == ';' || t[0] == '#' || t[0] == '[') {
				continue;
			}
			const auto eq = t.find('=');
			if (eq != std::string::npos) {
				a_kv[Lower(Trim(std::string_view(t).substr(0, eq)))] = Trim(std::string_view(t).substr(eq + 1));
			}
		}
		return true;
	}

	void GetBool(const KeyValues& a_kv, const char* a_key, bool& a_v)
	{
		if (auto it = a_kv.find(a_key); it != a_kv.end()) {
			a_v = it->second == "1" || Lower(it->second) == "true";
		}
	}

	void GetInt(const KeyValues& a_kv, const char* a_key, int& a_v)
	{
		if (auto it = a_kv.find(a_key); it != a_kv.end()) {
			try { a_v = std::stoi(it->second); } catch (...) {}
		}
	}

	void GetFloat(const KeyValues& a_kv, const char* a_key, float& a_v)
	{
		if (auto it = a_kv.find(a_key); it != a_kv.end()) {
			try { a_v = std::stof(it->second); } catch (...) {}
		}
	}
}

Settings& Settings::Get()
{
	static Settings instance;
	return instance;
}

void Settings::Load()
{
	KeyValues kv;
	if (!ReadIni(kPath, kv)) {
		SKSE::log::info("{} not found; using defaults", kPath.string());
		return;
	}

	GetBool(kv, "bautostart", autoStart);
	GetBool(kv, "bnativespoilage", nativeSpoilage);
	GetBool(kv, "bworldspoilage", worldSpoilage);
	GetBool(kv, "bfreshnessbar", freshnessBar);
	GetBool(kv, "bbaitcompat", baitCompat);
	GetBool(kv, "bkeywordfoods", keywordFoods);
	GetFloat(kv, "frawfoodhours", rawFoodHours);
	GetFloat(kv, "fotherfoodhours", otherFoodHours);
	GetFloat(kv, "ffreshleft", freshLeft);
	GetFloat(kv, "ffreshwidth", freshWidth);
	GetFloat(kv, "ffreshoffsety", freshOffsetY);
	GetFloat(kv, "fcontainerresetdays", containerResetDays);
	GetFloat(kv, "fworldspoilchance", worldSpoilChance);
	GetFloat(kv, "fworldfreshshare", worldFreshShare);
	GetBool(kv, "bwaitforfrostfall", waitForFrostfall);
	GetBool(kv, "bhudenabled", hudEnabled);
	GetInt(kv, "idisplaymode", displayMode);
	GetInt(kv, "ifillmode", fillMode);
	GetInt(kv, "ilayout", layout);
	GetBool(kv, "bstackunderfrostfall", stackUnderFrostfall);
	GetBool(kv, "bmatchfrostfall", matchFrostfall);
	GetFloat(kv, "fstackgap", stackGap);
	GetFloat(kv, "fposx", posX);
	GetFloat(kv, "fposy", posY);
	GetFloat(kv, "fscale", scale);
	GetFloat(kv, "fopacity", opacity);
	GetFloat(kv, "fbarlength", barLength);
	GetFloat(kv, "fbarthickness", barThickness);
	GetFloat(kv, "fspacing", spacing);
	GetFloat(kv, "fcontextualseconds", contextualSeconds);
	GetBool(kv, "bshowhunger", showHunger);
	GetBool(kv, "bshowthirst", showThirst);
	GetBool(kv, "bshowfatigue", showFatigue);
	GetBool(kv, "bshowvitality", showVitality);
	GetBool(kv, "bshowicons", showIcons);

	containerResetDays = std::clamp(containerResetDays, 1.0f, 365.0f);
	rawFoodHours = std::clamp(rawFoodHours, 1.0f, 2000.0f);
	otherFoodHours = std::clamp(otherFoodHours, 1.0f, 2000.0f);
	freshLeft = std::clamp(freshLeft, 0.0f, 1.0f);
	freshWidth = std::clamp(freshWidth, 0.02f, 1.0f);
	freshOffsetY = std::clamp(freshOffsetY, -200.0f, 200.0f);
	worldSpoilChance = std::clamp(worldSpoilChance, 0.0f, 100.0f);
	worldFreshShare = std::clamp(worldFreshShare, 0.0f, 100.0f);
	displayMode = std::clamp(displayMode, 0, 1);
	fillMode = std::clamp(fillMode, 0, 1);
	layout = std::clamp(layout, 0, 1);
	stackGap = std::clamp(stackGap, 0.0f, 200.0f);
	posX = std::clamp(posX, 0.0f, 1.0f);
	posY = std::clamp(posY, 0.0f, 1.0f);
	scale = std::clamp(scale, 0.5f, 3.0f);
	opacity = std::clamp(opacity, 0.1f, 1.0f);
	barLength = std::clamp(barLength, 30.0f, 400.0f);
	barThickness = std::clamp(barThickness, 3.0f, 30.0f);
	spacing = std::clamp(spacing, 2.0f, 40.0f);
	contextualSeconds = std::clamp(contextualSeconds, 1.0f, 60.0f);
	SKSE::log::info("Loaded {}", kPath.string());
}

void Settings::Save() const
{
	std::ofstream out(kPath, std::ios::trunc);
	if (!out) {
		SKSE::log::error("Could not write {}", kPath.string());
		return;
	}
	out << "; LastSeed.dll settings. Edit them in game from the SKSE Menu Framework page (Last Seed).\n\n";
	out << "[General]\n";
	out << "iVersion=" << kVersion << "\n";
	out << "bAutoStart=" << (autoStart ? 1 : 0) << "\n";
	out << "bWaitForFrostfall=" << (waitForFrostfall ? 1 : 0) << "\n";
	out << "bNativeSpoilage=" << (nativeSpoilage ? 1 : 0) << "\n";
	out << "bWorldSpoilage=" << (worldSpoilage ? 1 : 0) << "\n";
	out << "bFreshnessBar=" << (freshnessBar ? 1 : 0) << "\n";
	out << "bBaitCompat=" << (baitCompat ? 1 : 0) << "\n";
	out << "bKeywordFoods=" << (keywordFoods ? 1 : 0) << "\n";
	out << std::format("fRawFoodHours={:.1f}\nfOtherFoodHours={:.1f}\n", rawFoodHours, otherFoodHours);
	out << std::format("fFreshLeft={:.4f}\nfFreshWidth={:.4f}\nfFreshOffsetY={:.1f}\n", freshLeft, freshWidth, freshOffsetY);
	out << std::format("fContainerResetDays={:.1f}\n", containerResetDays);
	out << std::format("fWorldSpoilChance={:.1f}\nfWorldFreshShare={:.1f}\n\n", worldSpoilChance, worldFreshShare);
	out << "[HUD]\n";
	out << "bHudEnabled=" << (hudEnabled ? 1 : 0) << "\n";
	out << "iDisplayMode=" << displayMode << "\n";
	out << "iFillMode=" << fillMode << "\n";
	out << "iLayout=" << layout << "\n";
	out << "bStackUnderFrostfall=" << (stackUnderFrostfall ? 1 : 0) << "\n";
	out << "bMatchFrostfall=" << (matchFrostfall ? 1 : 0) << "\n";
	out << std::format("fStackGap={:.1f}\n", stackGap);
	out << std::format("fPosX={:.4f}\nfPosY={:.4f}\nfScale={:.3f}\nfOpacity={:.3f}\n", posX, posY, scale, opacity);
	out << std::format("fBarLength={:.1f}\nfBarThickness={:.1f}\nfSpacing={:.1f}\nfContextualSeconds={:.1f}\n", barLength, barThickness, spacing, contextualSeconds);
	out << "bShowHunger=" << (showHunger ? 1 : 0) << "\n";
	out << "bShowThirst=" << (showThirst ? 1 : 0) << "\n";
	out << "bShowFatigue=" << (showFatigue ? 1 : 0) << "\n";
	out << "bShowVitality=" << (showVitality ? 1 : 0) << "\n";
	out << "bShowIcons=" << (showIcons ? 1 : 0) << "\n";
}

void Settings::ResetHudLayout()
{
	const Settings d;
	displayMode = d.displayMode;
	fillMode = d.fillMode;
	layout = d.layout;
	stackUnderFrostfall = d.stackUnderFrostfall;
	matchFrostfall = d.matchFrostfall;
	stackGap = d.stackGap;
	posX = d.posX;
	posY = d.posY;
	scale = d.scale;
	opacity = d.opacity;
	barLength = d.barLength;
	barThickness = d.barThickness;
	spacing = d.spacing;
	contextualSeconds = d.contextualSeconds;
	showHunger = showThirst = showFatigue = showVitality = showIcons = true;
}

FrostfallLayout FrostfallLayout::Read()
{
	FrostfallLayout f;
	// Frostfall's HUD only exists while Frostfall.dll is loaded; its INI may not exist yet (defaults apply until it is first saved).
	f.loaded = GetModuleHandleW(L"Frostfall.dll") != nullptr;
	if (!f.loaded) {
		return f;
	}
	KeyValues kv;
	if (!ReadIni(kFrostfallPath, kv)) {
		return f;
	}
	GetBool(kv, "bhudenabled", f.hudEnabled);
	GetInt(kv, "ilayout", f.layout);
	GetFloat(kv, "fposx", f.posX);
	GetFloat(kv, "fposy", f.posY);
	GetFloat(kv, "fscale", f.scale);
	GetFloat(kv, "fbarlength", f.barLength);
	GetFloat(kv, "fbarthickness", f.barThickness);
	GetFloat(kv, "fspacing", f.spacing);
	GetBool(kv, "bshowicons", f.showIcons);
	f.layout = std::clamp(f.layout, 0, 1);
	f.posX = std::clamp(f.posX, 0.0f, 1.0f);
	f.posY = std::clamp(f.posY, 0.0f, 1.0f);
	f.scale = std::clamp(f.scale, 0.5f, 3.0f);
	f.barLength = std::clamp(f.barLength, 30.0f, 400.0f);
	f.barThickness = std::clamp(f.barThickness, 3.0f, 30.0f);
	f.spacing = std::clamp(f.spacing, 2.0f, 40.0f);
	return f;
}
