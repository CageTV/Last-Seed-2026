# Last Seed 2026

An update of **Last Seed - Survival Needs and Diseases** (Nexus mod 56393, by Chesko and Aytrus) for Skyrim AE 1.6.1170 and up, built the same way as [Frostfall 2026](https://github.com/CageTV/Frostfall-2026): what can leave Papyrus and the
old SkyUI widgets now runs in an SKSE plugin, and the settings live in SKSE Menu Framework 3.

Last Seed's scripts and plugin are MIT licensed by their authors, so this release carries a modified `LastSeed.esp` and the scripts that
changed. **It does not carry Last Seed's meshes, textures or sounds** (those need the authors' permission): keep the original Last Seed
download installed underneath it.

## What's different

- **HUD:** four flat vertical bars (hunger, thirst, fatigue, vitality) in Frostfall 2026's style, stacked directly under Frostfall's. The old
  SkyUI meters are hidden. A short Last Seed logo plays when it has started.
- **Starts on its own** on a new game, waiting for Frostfall first when it is installed. No sleeping, no MCM, no start-up banner.
- **Native food spoilage** (`LastSeed.dll`): food ages in plain records instead of one Papyrus tracker object per stack. Same categories,
  rot times, spoiled items and settings as Last Seed. Food keeps its age when you move it between yourself, followers and the Provisions
  container, and food you stash in a chest keeps ageing while you are away. A *Spoilage* page shows what is closest to rotting.
- **World containers spoil too**, with a chance you set, a share that is always left fresh, and a reset period after which spoiled food comes
  back fresh and is rolled again. Your own chests, containers you put food into and merchants' stock chests are never touched.
- **Freshness bar** on the item card while you hover food (vanilla/SkyUI, position adjustable for other skins) and a freshness ring on food
  tiles in Grid Inventory (registers itself, nothing to configure).
- **Foods Last Seed does not know** (added by other mods) spoil too, found by Skyrim's own `VendorItemFood` / `VendorItemFoodRaw`
  keywords, with drinks left out. No patch needed for new foods.
- **All of Last Seed's settings are in SKSE Menu Framework 3** (Overview, Gameplay, Primary Needs, Vitality, Alcohol / Skooma / Disease,
  Food Spoilage, Other incl. hotkeys, Profiles, Food & Drink Lists). They write the same game values and profile files as the old menu. The
  SkyUI menu keeps one page: the food portion editor.
- **Patches built in** (no patch plugins, no new masters): General Compatibility, Attack Speed Fix (Unbound Intensity no longer slows time),
  Your Own Thoughts (first-person messages). **Done at run time when the other mod is present:** spoiled food as bait for any fishing mod,
  Growl - Werebeasts of Skyrim, Travellers of Skyrim.
- The **"COMPLETED: LAST SEED" banner** at start-up is gone.

Patches that need another mod as a master, or that move things in the world, are not included and keep working as separate plugins: the
recipe patches (CACO, SAFO, Nordic Cooking, Hunterborn, Falskaar, Skyrim Food Expanded, Bruma, Mealtime, Warm Drinks, Apothecary Food,
Creation Club Fishing, Requiem), the well and bucket patches, Less Food, CFTO and Reliquary of Myth. You can drop the General,
Attack Speed Fix, Your Own Thoughts and Apothecary patch plugins, and `iWant Last Seed Widgets.esp`.

## Two builds

Both are in one FOMOD installer, **Last Seed 2026**, which asks Regular or ESL.

| | **Last Seed 2026** | **Last Seed 2026 (ESL)** |
|---|---|---|
| `LastSeed.esp` | regular plugin, same FormIDs as Last Seed 5.3 | light plugin (ESL), FormIDs renumbered |
| Campfire / Frostfall | regular | the ESL pair (Campfire 2026 ESL, Frostfall 2026 ESL) |
| Existing saves | works with a save that already runs Last Seed | **new game** |

Use one, never both. The same `LastSeed.dll` is in each; it detects which plugin build is loaded.

## Requirements

- The original **Last Seed 5.3** (Nexus 56393) installed underneath (meshes, textures, sounds, the other scripts), and **Campfire** (Nexus 667).
- Skyrim Script Extender (SKSE64) and Address Library for SKSE Plugins.
- [SKSE Menu Framework 3](https://www.nexusmods.com/skyrimspecialedition/mods/120352) (Nexus 120352): the bars and every settings page.
- PapyrusUtil (profiles). SkyUI only for the one remaining menu page. Optional: Frostfall 2026, Grid Inventory.

"Last Seed - Water" (Nexus 58338) is obsolete: Last Seed 4.0 and later include everything it did. Do not install it.

## Install

Install the original Last Seed, then this over it, and let this one overwrite. Order: Campfire, Campfire 2026, Frostfall 2026, then Last Seed 2026. Disable the patch plugins listed above. If you use the
**Drunk or drugged animations OAR** mod with the ESL build, see the ESL section of the readme (its config names two Last Seed effects by id).

## Patches

Last Seed's compatibility patches that are still needed are one FOMOD download, **Last Seed 2026 - Patches**. It first asks which build you use
(Regular or ESL), so you cannot install the wrong plugins, then lists the patches (a patch whose mod is not in your load order cannot be ticked).

## Building

See [BUILDING.md](BUILDING.md).

## Credits and licences

- **Chesko and Aytrus** - Last Seed; Chesko - Campfire and Frostfall. `LastSeed.esp` and the Papyrus scripts here are modified versions of
  theirs, used under their MIT licence (see [LICENSE.txt](LICENSE.txt)).
- The SKSE Menu Framework 3 author - `plugin/include/SKSEMenuFramework.h` is that project's SDK header, unmodified, under its own terms.
- Grid Inventory (Nexus) - `plugin/include/GridInventoryAPI.h` is its extension header, vendored unmodified as the header asks, under Grid
  Inventory's GPL-3.0 with its modding exception.
