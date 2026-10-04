/*
 * Last Seed 2026
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
 */
#include "PCH.h"
#include "Ids.h"
#include "Compat.h"
#include "GameIds.h"
#include "Settings.h"

namespace Compat
{
	namespace
	{
		std::vector<Entry> status;

		RE::TESForm* LastSeedForm(RE::FormID a_local)
		{
			auto* dh = RE::TESDataHandler::GetSingleton();
			return dh ? dh->LookupForm(ids::Ls(a_local), "LastSeed.esp") : nullptr;
		}

		bool Loaded(const char* a_plugin)
		{
			auto* dh = RE::TESDataHandler::GetSingleton();
			return dh && dh->LookupModByName(a_plugin) != nullptr;
		}

		// Effects live on the game's heap and are freed by the game, so new ones come from its allocator.
		RE::Effect* NewEffect(RE::EffectSetting* a_base, float a_magnitude, std::uint32_t a_area, std::uint32_t a_duration)
		{
			void* mem = RE::malloc(sizeof(RE::Effect));
			if (!mem) {
				return nullptr;
			}
			auto* e = new (mem) RE::Effect();
			e->baseEffect = a_base;
			e->effectItem.magnitude = a_magnitude;
			e->effectItem.area = a_area;
			e->effectItem.duration = a_duration;
			return e;
		}

		// ---- fishing bait ----
		// Bait lists are found by what is on them: the insects and wings every Skyrim fishing mod uses as bait.
		void FishingBait()
		{
			static constexpr RE::FormID kSignature[] = { 0x0E4F0C, 0x0BB956, 0x04DA73, 0x0A9195, 0x0727DE, 0x0727DF, 0x0727E0 };  // dartwings, torchbug, bee, moth wings
			Entry entry{ "Fishing mods: spoiled food as bait", false, false, "" };
			if (!Settings::Get().baitCompat) {
				entry.detail = "switched off";
				status.push_back(entry);
				return;
			}
			auto* dh = RE::TESDataHandler::GetSingleton();
			if (!dh) {
				return;
			}
			std::vector<RE::TESForm*> toAdd;
			if (auto* perished = LastSeedForm(ids::PerishedFood)) {
				toAdd.push_back(perished);
			}
			for (const std::size_t category : { 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u }) {  // the meat, small game, fish and seafood categories
				if (auto* spoiled = LastSeedForm(ids::FoodCategories[category].spoiled)) {
					toAdd.push_back(spoiled);
				}
			}
			int lists = 0;
			for (auto* list : dh->GetFormArray<RE::BGSListForm>()) {
				if (!list || list->forms.size() > 40) {
					continue;
				}
				int hits = 0;
				for (const auto id : kSignature) {
					if (auto* form = RE::TESForm::LookupByID(id); form && list->HasForm(form)) {
						++hits;
					}
				}
				if (hits < 5) {
					continue;
				}
				int added = 0;
				for (auto* form : toAdd) {
					if (!list->HasForm(form)) {
						list->AddForm(form);
						++added;
					}
				}
				++lists;
				entry.present = true;
				entry.detail += std::format("{}{:08X}", entry.detail.empty() ? "" : ", ", list->GetFormID());
				SKSE::log::info("Compat: bait list {:08X} found, {} spoiled foods added", list->GetFormID(), added);
			}
			entry.applied = lists > 0;
			entry.detail = lists > 0 ? std::format("{} bait list(s): {}", lists, entry.detail) : "no fishing mod bait list found";
			status.push_back(entry);
		}

		// ---- Growl - Werebeasts of Skyrim ----
		void Growl()
		{
			Entry entry{ "Growl - Werebeasts of Skyrim", Loaded("Growl - Werebeasts of Skyrim.esp"), false, "" };
			if (!entry.present) {
				status.push_back(entry);
				return;
			}
			auto* dh = RE::TESDataHandler::GetSingleton();
			auto* feed = dh ? dh->LookupForm<RE::SpellItem>(0x0EC357, "Skyrim.esm") : nullptr;  // the werewolf feeding spell
			auto* a = dh ? dh->LookupForm<RE::EffectSetting>(0x0D0288, "Growl - Werebeasts of Skyrim.esp") : nullptr;
			auto* b = dh ? dh->LookupForm<RE::EffectSetting>(0x0D028C, "Growl - Werebeasts of Skyrim.esp") : nullptr;
			if (!feed || !a || !b || feed->effects.empty()) {
				entry.detail = "records not found";
				status.push_back(entry);
				return;
			}
			bool already = false;
			for (auto* e : feed->effects) {
				if (e && (e->baseEffect == a || e->baseEffect == b)) {
					already = true;  // someone (the old patch) did it already
				}
			}
			if (already) {
				entry.applied = true;
				entry.detail = "already patched";
			} else {
				if (auto* first = feed->effects[0]; first && first->effectItem.magnitude == 50.0f) {
					first->effectItem.magnitude = 100.0f;  // as the patch does
				}
				if (auto* e1 = NewEffect(a, 0.2f, 0, 30)) {
					feed->effects.push_back(e1);
				}
				if (auto* e2 = NewEffect(b, 100.0f, 0, 0)) {
					feed->effects.push_back(e2);
				}
				entry.applied = true;
				entry.detail = "feeding spell gets Growl's effects";
			}
			status.push_back(entry);
		}

		// ---- Travellers of Skyrim ----
		void Travellers()
		{
			Entry entry{ "Travellers of Skyrim", Loaded("TravellersOfSkyrim.esm"), false, "" };
			if (!entry.present) {
				status.push_back(entry);
				return;
			}
			auto* dh = RE::TESDataHandler::GetSingleton();
			auto* cure = dh ? dh->LookupForm<RE::SpellItem>(0x007FBD, "TravellersOfSkyrim.esm") : nullptr;  // m0tos_ApothecaryCureDiseaseAbility
			auto* ours = LastSeedForm(0x3A902F) ? LastSeedForm(0x3A902F)->As<RE::EffectSetting>() : nullptr;
			if (!cure || !ours || cure->effects.empty()) {
				entry.detail = "records not found";
				status.push_back(entry);
				return;
			}
			if (cure->effects.size() == 1 && cure->effects[0]) {
				cure->effects[0]->baseEffect = ours;
				entry.applied = true;
				entry.detail = "apothecaries cure with Last Seed's disease cure";
			} else {
				entry.detail = "unexpected effect list, left alone";
			}
			status.push_back(entry);
		}
	}

	void Apply()
	{
		status.clear();
		FishingBait();
		Growl();
		Travellers();
		for (const auto& e : status) {
			SKSE::log::info("Compat: {} - {}{}", e.name, e.present ? (e.applied ? "applied" : "present") : "not installed", e.detail.empty() ? "" : " (" + e.detail + ")");
		}
	}

	std::vector<Entry> Status() { return status; }
}
