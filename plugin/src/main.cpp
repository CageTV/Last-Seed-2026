#include "PCH.h"
#include "AutoStart.h"
#include "Game.h"
#include "Hud.h"
#include "Menu.h"
#include "Settings.h"

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

	bool RegisterPapyrus(RE::BSScript::IVirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("HudBarsActive", "LastSeedNative", HudBarsActive);
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
			AutoStart::Begin();
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
	SKSE::log::info("LastSeed.dll 0.2.0, game {}", REL::Module::get().version().string());
	Settings::Get().Load();
	SKSE::GetPapyrusInterface()->Register(RegisterPapyrus);
	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
	return true;
}
