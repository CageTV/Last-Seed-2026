"""Builds src/scripts/_Seed_AliasFoodMonitor.psc from YOUR installed copy of Last Seed.

Last Seed ships no Papyrus source and its license is unknown, so this repository does not contain a copy of the script. This tool
decompiles the shipped _Seed_AliasFoodMonitor.pex with Champollion, adds the guards that make the alias step aside when
LastSeed.dll tracks food spoilage (LastSeedNative.NativeSpoilage), and writes the result next to our other sources
(git-ignored). Run it once, then tools/build_scripts.py.

Usage: python tools/make_alias_food_monitor.py [path to Last Seed's Scripts folder] [path to Champollion.exe]
"""
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRIPTS = sys.argv[1] if len(sys.argv) > 1 else r"E:\Tabula Rasa\mods\Last Seed\Scripts"
CHAMPOLLION = sys.argv[2] if len(sys.argv) > 2 else r"E:\Tabula Rasa\mods\Skyrim-Claude Code Modder's Toolkit\tools\Champollion\Champollion.exe"
GUARD = "  If LastSeedNative.NativeSpoilage() ; Last Seed 2026: LastSeed.dll tracks spoilage\n    Return \n  EndIf\n"

out = tempfile.mkdtemp()
subprocess.run([CHAMPOLLION, "-p", out, os.path.join(SCRIPTS, "_Seed_AliasFoodMonitor.pex")], check=True, capture_output=True)
src = open(os.path.join(out, "_Seed_AliasFoodMonitor.psc"), encoding="utf-8").read()

src = src.replace("Function addInventoryEventFilters()\n", "Function addInventoryEventFilters()\n" + GUARD, 1)
count = 0


def add(m):
    global count
    count += 1
    return m.group(0) + GUARD


src = re.sub(r"Event OnItemAdded\([^\n]*\)\n", add, src)
src = re.sub(r"Event OnItemRemoved\([^\n]*\)\n", add, src)
assert count == 3, f"expected 3 inventory events, found {count}"
src = src.replace("ScriptName _Seed_AliasFoodMonitor Extends ReferenceAlias\n",
                  "ScriptName _Seed_AliasFoodMonitor Extends ReferenceAlias\n; Last Seed 2026: when LastSeed.dll tracks spoilage (LastSeedNative.NativeSpoilage), this alias stops listening to inventory events.\n", 1)
dest = os.path.join(HERE, "src", "scripts", "_Seed_AliasFoodMonitor.psc")
open(dest, "w", encoding="utf-8", newline="\n").write(src)
print("wrote", dest)
