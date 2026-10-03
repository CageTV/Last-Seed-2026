"""Builds src/scripts/_Seed_SkyUIConfigPanelScript.psc from YOUR installed copy of Last Seed.

Last Seed's SkyUI menu script by Chesko and Aytrus (MIT licensed scripts). Its settings now live in LastSeedmenu pages drawn by LastSeed.dll through SKSE Menu
Framework, so this tool decompiles the shipped _Seed_SkyUIConfigPanelScript.pex with Champollion and changes only these things:

  * the SkyUI menu lists a single page, "Food & Drink Lists" (its portion editor has no replacement yet); every other page is gone,
    including Effects & HUD and Meters, which the new HUD replaces
  * the script version goes up by one, and OnVersionUpdate rebuilds the page list, so saves that already registered the old menu pick this up
  * nothing else: the profile, preset and food functions stay exactly as shipped, because LastSeed.dll calls them (SwitchToProfile,
    GenerateDefaultProfile, SaveAllSettings)

Usage: python tools/make_mcm_script.py [path to Last Seed's Scripts folder] [path to Champollion.exe]
"""
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRIPTS = sys.argv[1] if len(sys.argv) > 1 else r"E:\Tabula Rasa\mods\Last Seed\Scripts"
CHAMPOLLION = sys.argv[2] if len(sys.argv) > 2 else r"E:\Tabula Rasa\mods\Skyrim-Claude Code Modder's Toolkit\tools\Champollion\Champollion.exe"

out = tempfile.mkdtemp()
subprocess.run([CHAMPOLLION, "-p", out, os.path.join(SCRIPTS, "_Seed_SkyUIConfigPanelScript.pex")], check=True, capture_output=True)
src = open(os.path.join(out, "_Seed_SkyUIConfigPanelScript.psc"), encoding="utf-8").read()

# 1. the page list
pages = re.search(r"  Pages = new String\[11\][^\n]*\n(?:  Pages\[\d+\] = [^\n]*\n){11}", src)
assert pages, "the Pages block was not found"
src = src.replace(pages.group(0), '  Pages = new String[1] ; Last Seed 2026: every other page moved to LastSeed.dll\n  Pages[0] = "$LastFoodListPage"\n', 1)

# 2. version bump so the page list is rebuilt on existing saves
ver = re.search(r"Int Function GetVersion\(\)\n  Return (\d+)[^\n]*\n", src)
assert ver, "GetVersion was not found"
src = src.replace(ver.group(0), f"Int Function GetVersion()\n  Return {int(ver.group(1)) + 1} ; Last Seed 2026\n", 1)
upd = re.search(r"Function OnVersionUpdate\(Int a_version\)\n  ; Empty function\nEndFunction\n", src)
assert upd, "OnVersionUpdate was not found (or no longer empty)"
src = src.replace(upd.group(0), "Function OnVersionUpdate(Int a_version)\n  Self.loadArrays() ; Last Seed 2026: rebuild the page list\nEndFunction\n", 1)

dest = os.path.join(HERE, "src", "scripts", "_Seed_SkyUIConfigPanelScript.psc")
open(dest, "w", encoding="utf-8", newline="\n").write(src)
print("wrote", dest)
