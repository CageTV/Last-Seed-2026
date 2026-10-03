"""Builds the two release zips into release/ (git-ignored): the regular build and the ESL build.

Regular: release-contents/ (LastSeed.esp, Scripts, Interface) + plugin/build/release/LastSeed.dll.
ESL:     the 'Last Seed 2026 ESL' mod folder tools/esl_package.py wrote (ESL LastSeed.esp, remapped scripts), without the corrected copy of another
         mod's config (that file belongs to the Drunk or drugged animations OAR mod and is not ours to redistribute; the readme says what to change).

Usage: python tools/make_release.py <version> [--no-logo]
"""
import os
import shutil
import sys
import zipfile

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REL = os.path.join(HERE, "release-contents")
OUT = os.path.join(HERE, "release")
ESL_DIR = r"E:\Tabula Rasa\mods\Last Seed 2026 ESL"
version = sys.argv[1] if len(sys.argv) > 1 else "1.0.0"
skip_logo = "--no-logo" in sys.argv
DLL = os.path.join(HERE, "plugin", "build", "release", "LastSeed.dll")
OAR_PREFIX = os.path.join("meshes", "actors")

REGULAR_README = """Last Seed 2026 {v}

An update of Last Seed - Survival Needs and Diseases (Nexus 56393, by Chesko and Aytrus) for Skyrim SE/AE 1.6.1170 and up.
Install the original Last Seed 5.3 first (meshes, textures, sounds, the other scripts) and Campfire, then this over it.

Needs: SKSE64, Address Library, SKSE Menu Framework 3 (Nexus 120352), PapyrusUtil. Optional: Frostfall 2026, Grid Inventory.

You can disable these plugins, their changes are inside LastSeed.esp now: Last Seed - General Patch, Last Seed - Attack Speed Fix Patch,
Last Seed - Your Own Thoughts Patch, Last Seed - Apothecary Patch, and iWant Last Seed Widgets.esp.
Recipe patches (CACO, SAFO, Nordic Cooking, Hunterborn, Falskaar, Skyrim Food Expanded, Bruma, Mealtime, Warm Drinks, Apothecary Food,
Creation Club Fishing, Requiem), the well and bucket patches, Less Food, CFTO and Reliquary of Myth are separate and still work.

Settings: SKSE Menu Framework 3 (the menu key you set there) > Last Seed. https://github.com/CageTV/Last-Seed-2026
"""


def add(z, src, arc):
    z.write(src, arc.replace("\\", "/"))


def regular(path):
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        add(z, os.path.join(REL, "LastSeed.esp"), "LastSeed.esp")
        add(z, DLL, os.path.join("SKSE", "Plugins", "LastSeed.dll"))
        for root, _, files in os.walk(os.path.join(REL, "Scripts")):
            for fn in files:
                src = os.path.join(root, fn)
                add(z, src, os.path.join("Scripts", os.path.relpath(src, os.path.join(REL, "Scripts"))))
        for root, _, files in os.walk(os.path.join(REL, "Interface")):
            for fn in files:
                if skip_logo and fn.lower() == "lastseed_logo.png":
                    continue
                src = os.path.join(root, fn)
                add(z, src, os.path.join("Interface", os.path.relpath(src, os.path.join(REL, "Interface"))))
        z.writestr("README.txt", REGULAR_README.format(v=version))


def esl(path):
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        for root, _, files in os.walk(ESL_DIR):
            for fn in files:
                src = os.path.join(root, fn)
                rel = os.path.relpath(src, ESL_DIR)
                if rel.startswith(OAR_PREFIX) or fn == "meta.ini" or fn == "README - ESL build.txt":
                    continue
                if skip_logo and fn.lower() == "lastseed_logo.png":
                    continue
                if rel == os.path.join("SKSE", "Plugins", "LastSeed.dll"):
                    src = DLL
                add(z, src, rel)
        text = REGULAR_README.format(v=version + " (ESL)").replace(
            "Install the original Last Seed 5.3",
            "THE ESL BUILD: for the ESL Campfire (CAMPFIRE ESL UPDATED, Nexus 193472) and Frostfall 2026 (ESL) pair, new game only, never together\n"
            "with the regular build. Install the original Last Seed 5.3")
        text += (
            "\nIf you use the 'Drunk or drugged animations OAR' mod: its Drunken/config.json names two Last Seed effects by id. With this build change\n"
            "the two formID values 10880 -> 9A0 and 10881 -> 9A1 (hex, the light-plugin ids of _Seed_DrunkEffect3 and 4).\n")
        z.writestr("README.txt", text)


os.makedirs(OUT, exist_ok=True)
a = os.path.join(OUT, f"Last Seed 2026 {version}.zip")
b = os.path.join(OUT, f"Last Seed 2026 {version} ESL.zip")
regular(a)
esl(b)
for p in (a, b):
    with zipfile.ZipFile(p) as z:
        print(f"{os.path.basename(p)}: {len(z.namelist())} files, {os.path.getsize(p) // 1024} KB")
