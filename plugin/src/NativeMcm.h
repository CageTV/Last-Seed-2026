#pragma once

// Last Seed's settings pages, drawn through SKSE Menu Framework instead of the SkyUI MCM. They are built from McmTable.h (generated from
// Last Seed's own MCM script) and do what the MCM does: write the setting's global, write the active profile file (through PapyrusUtil's
// JsonUtil, exactly like the MCM) and let Last Seed's configuration handler apply the changes when the menu is closed.
namespace NativeMcm
{
	// One page of McmTable.h (an index into mcm::kPages), drawn inside a SKSE Menu Framework section.
	void DrawPage(int a_page);

	// The parts of Last Seed's Overview page that are settings: the gameplay preset, the location safety readout and the safe-location
	// toggle. Drawn below the status block of the Overview page.
	void DrawOverviewExtras();

	// Last Seed's settings profiles: pick, rename, reset, and switch automatic saving on or off.
	void DrawProfiles();

	// Once per frame from Hud::Render: notices when the pages stop being drawn (the player closed the menu or changed page),
	// flushes pending profile writes and tells Last Seed to apply the changes.
	void Tick(float a_dt);
}
