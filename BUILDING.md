# Building Last Seed 2026

## SKSE plugin (`plugin/` -> `SKSE/Plugins/LastSeed.dll`)

CommonLibSSE-NG plugin built with CMake + Ninja + MSVC and vcpkg (manifest in `plugin/build-local`, the same baseline and
registry as Frostfall 2026's DLL). Set `VCPKG_ROOT`, then run `plugin/build.cmd` (edit the Visual Studio and vcpkg paths at its top
for your install). Output: `plugin/build/release/LastSeed.dll`.

Uses [SKSE Menu Framework 3](https://www.nexusmods.com/skyrimspecialedition/mods/120352) at runtime.

## Icons (`tools/make_icons.py`)

`python tools/make_icons.py` (needs Pillow) redraws the four HUD icons into `assets/icons/` and copies them to
`release-contents/Interface/lastseed/icons/`.

## Layout of a release

Put the outputs in the folder layout of `release-contents/`: `SKSE/Plugins/LastSeed.dll` and `Interface/lastseed/icons/*.png`
(more as later phases add scripts and a plugin), and zip it with the files at the zip's root.

## How the DLL finds Last Seed's data

`plugin/src/GameIds.h` lists the local FormIDs, in `LastSeed.esp`, of the globals it reads. They were read from Last Seed 5.3's
plugin; if a later Last Seed release renumbers them, that is the one file to update.

## Papyrus scripts (`src/scripts` -> `Scripts/*.pex`)

`python tools/build_scripts.py` compiles them with the official Papyrus compiler (edit the paths at its top). It needs Last Seed's
other scripts as compile-time imports: decompile them once into `imports/lastseed-decompiled/` with Champollion (that folder is
git-ignored; Last Seed ships no source and its license is unknown, so none of its code is kept in this repository).
`_Seed_AliasFoodMonitor.psc` is a patched copy of a Last Seed script, so it is not in the repository either: run
`python tools/make_alias_food_monitor.py` to generate it from your own Last Seed install before building.

