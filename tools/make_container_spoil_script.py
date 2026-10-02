"""Builds src/scripts/_Seed_ContainerSpoilScript.psc from YOUR installed copy of Last Seed.

Last Seed ships no Papyrus source and its license is unknown, so this repository does not contain a copy of the script. This tool
decompiles the shipped _Seed_ContainerSpoilScript.pex with Champollion, adds a guard so the script steps aside when LastSeed.dll
spoils world containers itself (LastSeedNative.NativeWorldSpoilage), and writes the result next to our other sources (git-ignored).
Run it once, then tools/build_scripts.py.

Usage: python tools/make_container_spoil_script.py [path to Last Seed's Scripts folder] [path to Champollion.exe]
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRIPTS = sys.argv[1] if len(sys.argv) > 1 else r"E:\Tabula Rasa\mods\Last Seed\Scripts"
CHAMPOLLION = sys.argv[2] if len(sys.argv) > 2 else r"E:\Tabula Rasa\mods\Skyrim-Claude Code Modder's Toolkit\tools\Champollion\Champollion.exe"
GUARD = "  If LastSeedNative.NativeWorldSpoilage() ; Last Seed 2026: LastSeed.dll spoils world containers\n    Return \n  EndIf\n"

out = tempfile.mkdtemp()
subprocess.run([CHAMPOLLION, "-p", out, os.path.join(SCRIPTS, "_Seed_ContainerSpoilScript.pex")], check=True, capture_output=True)
src = open(os.path.join(out, "_Seed_ContainerSpoilScript.psc"), encoding="utf-8").read()
assert "Event OnCellLoad()\n" in src, "OnCellLoad not found"
src = src.replace("Event OnCellLoad()\n", "Event OnCellLoad()\n" + GUARD, 1)
dest = os.path.join(HERE, "src", "scripts", "_Seed_ContainerSpoilScript.psc")
open(dest, "w", encoding="utf-8", newline="\n").write(src)
print("wrote", dest)
