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
#include "Game.h"
#include "GameIds.h"

namespace Game
{
	namespace
	{
		Globals g;
		bool    ready = false;

		RE::TESGlobal* Lookup(RE::FormID a_localID, const char* a_name)
		{
			auto* dh = RE::TESDataHandler::GetSingleton();
			auto* form = dh ? dh->LookupForm<RE::TESGlobal>(ids::Ls(a_localID), "LastSeed.esp") : nullptr;
			if (!form) {
				SKSE::log::error("LastSeed.esp global {} ({:06X}) not found", a_name, a_localID);
			}
			return form;
		}

		void DispatchMain(const char* a_function, bool a_withBypassArg)
		{
			SKSE::GetTaskInterface()->AddTask([a_function, a_withBypassArg]() {
				auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
				if (!vm || !g.mainQuest) {
					return;
				}
				const auto handle = vm->GetObjectHandlePolicy()->GetHandleForObject(RE::TESQuest::FORMTYPE, g.mainQuest);
				RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
				auto* args = a_withBypassArg ? RE::MakeFunctionArguments(true) : RE::MakeFunctionArguments();
				const bool ok = vm->DispatchMethodCall2(handle, "_Seed_Main", a_function, args, callback);
				SKSE::log::info("Called _Seed_Main.{}() -> {}", a_function, ok ? "dispatched" : "FAILED (is the _Seed_Main script attached?)");
			});
		}
	}

	void RefreshOldMeters()
	{
		SKSE::GetTaskInterface()->AddTask([]() {
			auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			if (!vm) {
				return;
			}
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
			auto* args = RE::MakeFunctionArguments();
			vm->DispatchStaticCall("LastSeedNative", "RefreshOldMeters", args, callback);
			SKSE::log::info("Called LastSeedNative.RefreshOldMeters()");
		});
	}

	bool Init()
	{
		g.running = Lookup(ids::LastSeedRunning, "LastSeedRunning");
		g.hunger = Lookup(ids::Seed_AttributeHunger, "_Seed_AttributeHunger");
		g.hungerMax = Lookup(ids::Seed_AttributeHungerMax, "_Seed_AttributeHungerMax");
		g.thirst = Lookup(ids::Seed_AttributeThirst, "_Seed_AttributeThirst");
		g.thirstMax = Lookup(ids::Seed_AttributeThirstMax, "_Seed_AttributeThirstMax");
		g.fatigue = Lookup(ids::Seed_AttributeFatigue, "_Seed_AttributeFatigue");
		g.fatigueMax = Lookup(ids::Seed_AttributeFatigueMax, "_Seed_AttributeFatigueMax");
		g.vitality = Lookup(ids::Seed_AttributeVitality, "_Seed_AttributeVitality");
		g.vitalityMax = Lookup(ids::Seed_AttributeVitalityMax, "_Seed_AttributeVitalityMax");
		auto* dh = RE::TESDataHandler::GetSingleton();
		g.startupFinished = dh ? dh->LookupForm<RE::TESGlobal>(ids::Ls(ids::LastSeedStartupFinished), "LastSeed.esp") : nullptr;
		g.frostfallRunning = dh ? dh->LookupForm<RE::TESGlobal>(ids::Frostfall(ids::FrostfallRunning), "Frostfall.esp") : nullptr;
		g.kwCheck = dh ? dh->LookupForm<RE::TESGlobal>(ids::LastSeedRunningKWCheck, "Update.esm") : nullptr;
		g.mainQuest = dh ? dh->LookupForm<RE::TESQuest>(ids::Ls(ids::MainQuest), "LastSeed.esp") : nullptr;
		g.trackingQuest = dh ? dh->LookupForm<RE::TESQuest>(ids::Ls(ids::TrackingQuest), "LastSeed.esp") : nullptr;
		SKSE::log::info("KWCheck global {}, main quest {}, tracking quest {}", g.kwCheck ? "found" : "MISSING", g.mainQuest ? "found" : "MISSING", g.trackingQuest ? "found" : "MISSING");
		ready = g.running && g.hunger && g.thirst && g.fatigue && g.vitality;
		SKSE::log::info("Last Seed globals {}", ready ? "found" : "MISSING - is LastSeed.esp enabled?");
		return ready;
	}

	const Globals& G() { return g; }
	bool           Ready() { return ready; }

	float Value(const RE::TESGlobal* a_global, float a_fallback)
	{
		return a_global ? a_global->value : a_fallback;
	}

	bool IsRunning()
	{
		return ready && static_cast<int>(Value(g.running)) == 2;
	}

	bool StartupFinished()
	{
		return ready && static_cast<int>(Value(g.startupFinished)) == 2;
	}

	bool FrostfallInstalled() { return g.frostfallRunning != nullptr; }

	bool FrostfallRunning()
	{
		return g.frostfallRunning && static_cast<int>(Value(g.frostfallRunning)) == 2;
	}

	void StartLastSeed()
	{
		if (!ready || !g.mainQuest) {
			return;
		}
		g.running->value = 2.0f;
		if (g.kwCheck) {
			g.kwCheck->value = 2.0f;
		}
		// true = bypass the "first start-up" message box; the rest of the start-up runs exactly as from the MCM
		DispatchMain("StartLastSeed", true);
	}

	void StopLastSeed()
	{
		if (!ready || !g.mainQuest) {
			return;
		}
		if (g.kwCheck) {
			g.kwCheck->value = 1.0f;
		}
		DispatchMain("StopLastSeed", false);
	}

	bool EverStarted()
	{
		return g.trackingQuest && g.trackingQuest->GetCurrentStageID() >= 20;
	}
}
