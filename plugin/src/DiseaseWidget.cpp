/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#include "PCH.h"
#include "DiseaseWidget.h"

namespace DiseaseWidget
{
	namespace
	{
		constexpr const char* kFile = "Data/SKSE/Plugins/DiseaseWidget/Diseases.json";
		constexpr const char* kStages[] = { "Mild", "Moderate", "Severe", "Crippling", "Deadly" };

		struct Family
		{
			const char* name;
			const char* icon;    // one of the widget's icon files
			int         stages;  // 5 = Mild..Deadly, 0 = a single spell
		};

		// Last Seed's diseases (base spell names; the stage spells are "<stage> <name>") and the widget icon that fits each best.
		constexpr Family kFamilies[] = {
			{ "Ataxia", "ataxia", 5 },
			{ "Bone Break Fever", "bone_break_fever", 5 },
			{ "Brain Rot", "brain_rot", 5 },
			{ "Rattles", "rattles", 5 },
			{ "Rockjoint", "rockjoint", 5 },
			{ "Witbane", "witbane", 5 },
			{ "Droops", "droops", 5 },
			{ "Black Heart Blight", "weakened", 5 },
			{ "Ash-Chancre", "generic", 5 },
			{ "Ash Woe Blight", "greenspore", 5 },
			{ "Astral Vapors", "addled", 5 },
			{ "Blood Lung", "generic", 5 },
			{ "Brown Rot", "brown_rot", 5 },
			{ "Chanthrax Blight", "greenspore", 5 },
			{ "Chills", "frostbitten", 5 },
			{ "Collywobbles", "gutworm", 5 },
			{ "Dampworm", "gutworm", 5 },
			{ "Feeble Limb", "weakened", 5 },
			{ "Greenspore", "greenspore", 5 },
			{ "Helljoint", "rockjoint", 5 },
			{ "Red Rage", "addled", 5 },
			{ "Rotbone", "rockjoint", 5 },
			{ "Rust Chancre", "generic", 5 },
			{ "Scalon Sunburn", "generic", 5 },
			{ "Serpiginous Dementia", "brain_rot", 5 },
			{ "Shakes", "rattles", 5 },
			{ "Stomach Rot", "food_poisoning", 5 },
			{ "Swamp Fever", "generic", 5 },
			{ "Ticklebritch", "generic", 5 },
			{ "Tunnel Cough", "generic", 5 },
			{ "Wither", "weakened", 5 },
			{ "Witless Pox", "witbane", 5 },
			{ "Yellow Tick", "generic", 5 },
			{ "Skooma Addiction", "addled", 0 },
			{ "Skooma Withdrawal", "addled", 5 },
			{ "Hungover", "addled", 0 },
		};

		Compat::Entry entry{ "Disease Widget - SKSE: Last Seed's diseases and stages", false, false, "" };

		std::string Id(std::string_view a_name)
		{
			std::string id = "ls_";
			for (char c : a_name) {
				id += std::isalnum(static_cast<unsigned char>(c)) ? static_cast<char>(std::tolower(static_cast<unsigned char>(c))) : '_';
			}
			return id;
		}

		std::string Block()
		{
			std::string out;
			for (const auto& f : kFamilies) {
				if (!out.empty()) {
					out += ",\n";
				}
				std::string names = std::format("\"{}\"", f.name);
				if (f.stages > 0) {
					for (const char* s : kStages) {
						names += std::format(", \"{} {}\"", s, f.name);
					}
				}
				out += std::format("    {{\n      \"id\": \"{}\",\n      \"name\": \"{}\",\n      \"icon\": \"{}\",\n      \"match\": [{}],\n      \"requireDiseaseType\": true\n    }}",
					Id(f.name), f.name, f.icon, names);
			}
			return out;
		}

		// The index of the ']' that closes the "diseases" array, or npos. Skips strings, so brackets inside names do not count.
		std::size_t ArrayEnd(const std::string& a_text)
		{
			const auto key = a_text.find("\"diseases\"");
			if (key == std::string::npos) {
				return std::string::npos;
			}
			const auto open = a_text.find('[', key);
			if (open == std::string::npos) {
				return std::string::npos;
			}
			int  depth = 0;
			bool inString = false;
			for (std::size_t i = open; i < a_text.size(); ++i) {
				const char c = a_text[i];
				if (inString) {
					if (c == '\\') {
						++i;
					} else if (c == '"') {
						inString = false;
					}
					continue;
				}
				if (c == '"') {
					inString = true;
				} else if (c == '[') {
					++depth;
				} else if (c == ']' && --depth == 0) {
					return i;
				}
			}
			return std::string::npos;
		}
	}

	void MergeDefinitions()
	{
		entry.present = GetModuleHandleA("DiseaseWidget.dll") != nullptr;
		if (!entry.present) {
			return;
		}
		std::ifstream in(kFile, std::ios::binary);
		if (!in) {
			entry.detail = "the widget has not written Diseases.json yet: start the game once, then restart (Last Seed adds its entries on the next launch)";
			SKSE::log::info("Disease Widget: {} is not there yet", kFile);
			return;
		}
		std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		in.close();
		if (text.find("\"id\": \"ls_") != std::string::npos || text.find("\"id\":\"ls_") != std::string::npos) {
			entry.applied = true;
			entry.detail = "Last Seed's disease definitions are in Diseases.json";
			SKSE::log::info("Disease Widget: Last Seed's definitions are already in Diseases.json");
			return;
		}
		const auto end = ArrayEnd(text);
		if (end == std::string::npos) {
			entry.detail = "Diseases.json has no \"diseases\" list, so nothing was added";
			SKSE::log::warn("Disease Widget: no \"diseases\" array in {}", kFile);
			return;
		}
		// Is the array empty? Then no comma goes before the first new entry.
		std::size_t last = end;
		while (last > 0 && std::isspace(static_cast<unsigned char>(text[last - 1]))) {
			--last;
		}
		const bool        empty = last > 0 && text[last - 1] == '[';
		const std::string merged = text.substr(0, last) + (empty ? "\n" : ",\n") + Block() + "\n  " + text.substr(end);
		{
			std::error_code ec;
			std::filesystem::copy_file(kFile, std::string(kFile) + ".before-lastseed", std::filesystem::copy_options::skip_existing, ec);
		}
		std::ofstream out(kFile, std::ios::binary | std::ios::trunc);
		if (!out) {
			entry.detail = "Diseases.json could not be written";
			SKSE::log::warn("Disease Widget: cannot write {}", kFile);
			return;
		}
		out << merged;
		out.close();
		entry.applied = true;
		entry.detail = std::format("added {} disease definitions to Diseases.json (the original is kept as Diseases.json.before-lastseed)", std::size(kFamilies));
		SKSE::log::info("Disease Widget: added {} definitions to {}", std::size(kFamilies), kFile);
	}

	void LogActiveDiseases()
	{
		if (!entry.present) {
			return;
		}
		SKSE::GetTaskInterface()->AddTask([]() {
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* effects = player ? player->AsMagicTarget()->GetActiveEffectList() : nullptr;
			if (!effects) {
				return;
			}
			std::string seen;
			int         count = 0;
			for (auto* e : *effects) {
				const auto* spell = e && e->spell ? e->spell->As<RE::SpellItem>() : nullptr;
				if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kDisease) {
					seen += std::format(" [{} {:08X}]", spell->GetName(), spell->GetFormID());
					++count;
				}
			}
			SKSE::log::info("Disease Widget: the player has {} Disease-type active effect(s):{}", count, seen.empty() ? " none" : seen);
		});
	}

	Compat::Entry Status()
	{
		if (!entry.present) {
			entry.detail = "";
		}
		return entry;
	}
}
