# Last Seed 2026

An update layer for **Last Seed - Survival Needs and Diseases** (Nexus mod 56393, by Chesko and Aytrus), modernizing it for
Skyrim Special Edition / Anniversary Edition 1.6.1170 and up, in the same way [Frostfall 2026](https://github.com/CageTV/Frostfall-2026)
does for Frostfall.

> **Status: early development (Phase 1a).** The HUD bars are tested in game and sit correctly under Frostfall's. Hiding the old SkyUI meters, auto-start and the SMF3 settings migration are next.

**This is an update layer, not a full mod.** It contains only our own files. It contains none of Last Seed's plugins, scripts,
meshes, textures or sounds, which belong to their authors: you get them from the original download, which stays installed.

## Goals

- Move what can be moved out of Papyrus and the old SkyUI widget system into SKSE plugins, for stability.
- **HUD:** replace Last Seed's four SkyUI meters (one at the top left, three along the bottom of the screen) with flat vertical bars
  in the style of Frostfall 2026's, stacked directly below Frostfall's group. Hunger, thirst, fatigue and vitality.
- **Settings** in SKSE Menu Framework 3, as Frostfall 2026 does; Last Seed starts on its own on a new game.
- Include the Anniversary Edition content and compatibility pieces in one place.
- A light-plugin (ESL) variant, built after the normal one is tested.

## What's here now (Phase 1a)

- `plugin/` - **LastSeed.dll**, a CommonLibSSE-NG SKSE plugin. It reads Last Seed's own need values (plain game globals:
  `_Seed_AttributeHunger`, `_Seed_AttributeThirst`, `_Seed_AttributeFatigue`, `_Seed_AttributeVitality`, 0 to 120, vitality 0 to 160)
  and draws four bars through SKSE Menu Framework 3's HUD API. It has a settings page (Last Seed > Overview / HUD) and, while
  Frostfall.dll's HUD is on, places its group directly below Frostfall's, using Frostfall's bar size, spacing and layout.
  Last Seed's own SkyUI meters are not touched yet.
- `tools/make_icons.py` - draws the four HUD icons (`assets/icons`, `release-contents/Interface/lastseed/icons`).

## Requirements

- Last Seed 5.3 (Nexus 56393), the original release, and Campfire (Nexus 667) which it needs.
- Skyrim Script Extender (SKSE64) and Address Library for SKSE Plugins.
- [SKSE Menu Framework 3](https://www.nexusmods.com/skyrimspecialedition/mods/120352) (Nexus 120352) - for the bars and the settings page.
- Optional: Frostfall 2026, whose bars this group stacks under.

"Last Seed - Water" (Nexus 58338) is obsolete: Last Seed 4.0 and later include everything it did. Do not install it.

## Building

See [BUILDING.md](BUILDING.md).

## Credits

- Chesko and Aytrus - Last Seed. Chesko - Campfire and Frostfall.
- The SKSE Menu Framework 3 author - `plugin/include/SKSEMenuFramework.h` is that project's SDK header, included unmodified from
  the Frostfall 2026 repository so the plugin builds; see the SKSE Menu Framework 3 page for its terms.
