"""Checks the ESL patch plugins in esl-work/patches/ESL: each is re-read with Spriggit and must be flagged Small, keep its masters, have its own
records in the light range, and point at Last Seed / Campfire records that exist in the ESL build (or the ESL Campfire).

Usage: python tools/patches_verify.py [path to the Frostfall project's esl folder]
"""
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W = os.path.join(HERE, "esl-work")
ESL_MAPS = sys.argv[1] if len(sys.argv) > 1 else r"E:\WorkSpace\Frostfall-2026-repo\esl"
SPRIGGIT = r"E:/Tabula Rasa/mods/Skyrim-Claude Code Modder's Toolkit/tools/spriggit-cli.sh"
new_ls = set(json.load(open(os.path.join(W, "map_lastseed.json"))).values())
new_cf = set(json.load(open(os.path.join(ESL_MAPS, "map_full.json"))).values())
patches = json.load(open(os.path.join(HERE, "patches", "patches.json"), encoding="utf-8"))
bad = 0
for p in patches:
    name = p["file"][:-4]
    esp = os.path.join(W, "patches", "ESL", p["file"])
    chk = os.path.join(W, "patches", "check", name)
    if os.path.exists(chk):
        shutil.rmtree(chk)
    subprocess.run(["bash", SPRIGGIT, "serialize", "--InputPath", esp, "--OutputPath", chk, "--GameRelease", "SkyrimSE", "--PackageName", "Spriggit.Yaml", "--PackageVersion", "0.41.0"], capture_output=True, text=True)
    problems = []
    try:
        rd = open(os.path.join(chk, "RecordData.yaml"), encoding="utf-8").read()
    except OSError:
        print("FAILED", name, "(could not be re-read)")
        bad += 1
        continue
    if "- Small" not in rd:
        problems.append("not flagged Small")
    orig = open(os.path.join(HERE, "patches", "src", name, "RecordData.yaml"), encoding="utf-8").read()
    if re.findall(r"- Master: (.+)", rd) != re.findall(r"- Master: (.+)", orig):
        problems.append("masters changed")
    own_re = re.compile(r"^\s*(?:- )?FormKey: ([0-9A-F]{6}):" + re.escape(name) + r"\.esp\s*$", re.M | re.I)
    ls_refs, cf_refs, own = set(), set(), set()
    for dp, _, fs in os.walk(chk):
        for fn in fs:
            if fn.endswith(".yaml"):
                t = open(os.path.join(dp, fn), encoding="utf-8", errors="replace").read()
                ls_refs.update(re.findall(r"\b([0-9A-F]{6}):LastSeed\.esp", t))
                cf_refs.update(re.findall(r"\b([0-9A-F]{6}):Campfire\.esm", t))
                own.update(own_re.findall(t))
    if not ls_refs <= new_ls:
        problems.append(f"Last Seed refs not in the ESL build: {sorted(ls_refs - new_ls)[:3]}")
    if not cf_refs <= new_cf:
        problems.append(f"Campfire refs not ESL ids: {sorted(cf_refs - new_cf)[:3]}")
    if any(not 0x800 <= int(i, 16) <= 0xFFF for i in own):
        problems.append("own ids outside the light range")
    print(("ok    " if not problems else "FAILED"), name, "; ".join(problems))
    bad += bool(problems)
print("all good" if not bad else f"{bad} problem(s)")
sys.exit(1 if bad else 0)
