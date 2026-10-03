"""Lays out the ESL build of Last Seed 2026 as its own mod folder (nothing outside that folder is touched).

Needs: esl-work/esp/LastSeed.esp and esl-work/pex (tools/esl_build.py + Spriggit), release-contents (the normal build's scripts and interface)
and plugin/build/release/LastSeed.dll. The result is the normal build with the ESL plugin and the scripts whose hard-coded FormIDs were
remapped; it is meant for the ESL Campfire / Frostfall pair and must not be enabled together with "Last Seed 2026".

Usage: python tools/esl_package.py [destination folder, default E:/Tabula Rasa/mods/Last Seed 2026 ESL]
"""
import os
import shutil
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W = os.path.join(HERE, "esl-work")
DEST = sys.argv[1] if len(sys.argv) > 1 else r"E:\Tabula Rasa\mods\Last Seed 2026 ESL"
REL = os.path.join(HERE, "release-contents")

# never write into the original Last Seed or the normal Last Seed 2026 folder
assert os.path.basename(DEST.rstrip("\\/")) not in ("Last Seed", "Last Seed 2026"), "refusing to write into a folder that is not the ESL build's"
if os.path.exists(DEST):
    shutil.rmtree(DEST)
os.makedirs(DEST)


def put(src, rel):
    dst = os.path.join(DEST, rel)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copyfile(src, dst)


put(os.path.join(W, "esp", "LastSeed.esp"), "LastSeed.esp")

# scripts: the normal build's, then the remapped ones over them
for root, _, files in os.walk(os.path.join(REL, "Scripts")):
    for fn in files:
        src = os.path.join(root, fn)
        put(src, os.path.join("Scripts", os.path.relpath(src, os.path.join(REL, "Scripts"))))
for fn in os.listdir(os.path.join(W, "pex")):
    put(os.path.join(W, "pex", fn), os.path.join("Scripts", fn))
for fn in os.listdir(os.path.join(W, "scripts-src")):
    put(os.path.join(W, "scripts-src", fn), os.path.join("Scripts", "Source", fn))

for root, _, files in os.walk(os.path.join(REL, "Interface")):
    for fn in files:
        src = os.path.join(root, fn)
        put(src, os.path.join("Interface", os.path.relpath(src, os.path.join(REL, "Interface"))))

put(os.path.join(HERE, "plugin", "build", "release", "LastSeed.dll"), os.path.join("SKSE", "Plugins", "LastSeed.dll"))

# other mods' files that name Last Seed forms by id, remapped for the ESL ids: same relative path, so they replace the originals when this
# folder sits below the mod that owns them in MO2's list (higher priority number = further down... see the README)
other = os.path.join(W, "other-mods")
if os.path.isdir(other):
    for root, _, files in os.walk(other):
        for fn in files:
            src = os.path.join(root, fn)
            put(src, os.path.relpath(src, other))
            print("remapped file of another mod:", os.path.relpath(src, other))

with open(os.path.join(DEST, "README - ESL build.txt"), "w", encoding="utf-8") as f:
    f.write(
        "Last Seed 2026 - ESL build\n\n"
        "The same mod as 'Last Seed 2026', with LastSeed.esp flagged ESL (every FormID moved into the light range) and the scripts\n"
        "that hard-code Last Seed, Campfire or Frostfall FormIDs rewritten for it.\n\n"
        "* Use it INSTEAD OF 'Last Seed 2026', never together with it.\n"
        "* It is built for the ESL Campfire (CAMPFIRE ESL UPDATED) and Frostfall 2026 (ESL) pair. With the regular Campfire or Frostfall use the\n"
        "  regular 'Last Seed 2026' folder.\n"
        "* Not compatible with saves made with the regular build: start a new game.\n"
        "* It still needs the original 'Last Seed' mod folder below it for the meshes, textures and sounds.\n"
    )
n = sum(len(fs) for _, _, fs in os.walk(DEST))
print(f"wrote {n} files to {DEST}")
