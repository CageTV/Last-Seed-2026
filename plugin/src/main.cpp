/*
 * Last Seed 2026
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#include "PCH.h"
#include "AutoStart.h"
#include "Game.h"
#include "GridInv.h"
#include "Hud.h"
#include "Compat.h"
#include "Hotkeys.h"
#include "Menu.h"
#include "Settings.h"
#include "Spoilage.h"

namespace
{
	void SetupLog()
	{
		const auto dir = SKSE::log::log_directory();
		if (!dir) {
			return;
		}
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((*dir / "LastSeed.log").string(), true);
		auto log = std::make_shared<spdlog::logger>("LastSeed", std::move(sink));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
		log->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
		spdlog::set_default_logger(std::move(log));
	}

	// ---------- Papyrus: LastSeedNative ----------
	// True while the HUD bars are drawn, so Last Seed's own SkyUI meters can step aside (hooked up in a later phase).
	bool HudBarsActive(RE::StaticFunctionTag*)
	{
		return Menu::Registered() && Settings::Get().hudEnabled;
	}

	bool AutoStartEnabled(RE::StaticFunctionTag*)
	{
		return Settings::Get().autoStart;
	}

	bool NativeSpoilage(RE::StaticFunctionTag*)
	{
		return Spoilage::Active();
	}

	bool NativeWorldSpoilage(RE::StaticFunctionTag*)
	{
		return Spoilage::WorldActive();
	}

	bool RegisterPapyrus(RE::BSScript::IVirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("HudBarsActive", "LastSeedNative", HudBarsActive);
		a_vm->RegisterFunction("AutoStartEnabled", "LastSeedNative", AutoStartEnabled);
		a_vm->RegisterFunction("NativeSpoilage", "LastSeedNative", NativeSpoilage);
		a_vm->RegisterFunction("NativeWorldSpoilage", "LastSeedNative", NativeWorldSpoilage);
		return true;
	}

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kPostLoad:
			Menu::Register();  // SKSE Menu Framework loads after us alphabetically, so register once everything is loaded
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			Game::Init();
			Spoilage::Init();
			Spoilage::Begin();
			Compat::Apply();
			Hotkeys::Init();
			AutoStart::Begin();
			break;
		case SKSE::MessagingInterface::kPostLoadGame:
		case SKSE::MessagingInterface::kNewGame:
			Hotkeys::OnGameLoaded();
			break;
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SetupLog();
	SKSE::log::info("LastSeed.dll 1.0.0, game {}", REL::Module::get().version().string());
	Settings::Get().Load();
	Spoilage::Register();
	SKSE::GetPapyrusInterface()->Register(RegisterPapyrus);
	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
	GridInv::Register();
	return true;
}
