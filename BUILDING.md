# Building Last Seed 2026

Everything below expects the author's workspace layout and an installed copy of Last Seed 5.3; edit the paths at the top of each tool.

## SKSE plugin (`plugin/` -> `SKSE/Plugins/LastSeed.dll`)

CommonLibSSE-NG plugin, CMake + Ninja + MSVC + vcpkg. Set `VCPKG_ROOT`, run `plugin/build.cmd` (edit the Visual Studio and vcpkg paths at its
top). Output: `plugin/build/release/LastSeed.dll`. It needs [SKSE Menu Framework 3](https://www.nexusmods.com/skyrimspecialedition/mods/120352)
at runtime. One DLL serves both plugin builds: `plugin/src/Ids.cpp` translates FormIDs through `plugin/src/EslMap.h` when `LastSeed.esp`
is loaded as a light plugin.

`plugin/src/GameIds.h` and `plugin/src/McmTable.h` hold Last Seed 5.3's local FormIDs. If a later Last Seed renumbers them, regenerate
`McmTable.h` (below) and update `GameIds.h`.

## The plugin (`src/esp/LastSeed` -> `LastSeed.esp`)

`src/esp/LastSeed` is a [Spriggit](https://github.com/Mutagen-Modding/Spriggit) YAML export of the plugin (Last Seed 5.3 with the General,
Attack Speed Fix and Your Own Thoughts patches merged and the start-up banner removed). Deserialize it with
`spriggit deserialize --InputPath src/esp/LastSeed --OutputPath <somewhere>/LastSeed.esp` (the output file must be called `LastSeed.esp`).
`tools/merge_patches.py` merges patch plugins (serialized the same way) into a copy; it refuses any patch whose masters are not LastSeed.esp's.
`tools/analyze_patches.py` summarises what a set of patch plugins changes.

## Scripts (`src/scripts` -> `Scripts/*.pex`)

`python tools/build_scripts.py` compiles `src/scripts` with the official Papyrus compiler. Compile-time imports come from Last Seed's other
scripts, decompiled once with Champollion into `imports/lastseed-decompiled/` (git-ignored). Three scripts are generated from your own Last
Seed install, then built with the rest (all git-ignored):

- `tools/make_alias_food_monitor.py`: the food monitor steps aside when LastSeed.dll tracks spoilage.
- `tools/make_container_spoil_script.py`: Last Seed's one-shot container spoilage steps aside for the plugin's.
- `tools/make_mcm_script.py`: the SkyUI menu keeps only the Food & Drink Lists page; the profile, preset and food functions the plugin calls stay.

## The settings table (`tools/extract_mcm.py` -> `plugin/src/McmTable.h`)

Reads Last Seed's own menu script, `LastSeed.esp` (Spriggit YAML) and the English strings, and writes every setting of the Gameplay, Needs,
Vitality, Alcohol & Disease, Food Spoilage and Other pages (global, range, default, profile key), the 33 food lists and the Overview lists.
It prints anything it could not turn into a table row.

## ESL build

`python tools/esl_build.py` renumbers the plugin's 1927 records into 000800+, points its Campfire references at the ESL Campfire (map from the
Frostfall 2026 project's `esl` folder), rewrites and recompiles the scripts that hard-code FormIDs, and writes `plugin/src/EslMap.h`. Deserialize
`esl-work/yaml` with Spriggit to `esl-work/esp/LastSeed.esp`, then `python tools/esl_package.py` lays out the ESL mod folder.

## Compatibility patches (`patches/`)

`patches/src` is the Spriggit YAML of the patches that are still needed (listed with their requirements in `patches/patches.json`).
`python tools/patches_esl.py` (after `tools/esl_build.py`) rewrites each for the ESL build: Last Seed and ESL Campfire references renumbered,
Small flag set; `python tools/patches_verify.py` re-reads every result and checks it. `python tools/make_patches_release.py <version>` builds the
two FOMOD zips, one per build (regular: Chesko's original plugins from the Last Seed Development Kit download; ESL: the rewritten ones).

## Release zips

`python tools/make_release.py` builds both zips (regular and ESL) into `release/` from the regular `release-contents/` and the ESL folder.
