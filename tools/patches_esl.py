"""Builds the ESL variants of Last Seed's compatibility patches (patches/patches.json) for the ESL build of Last Seed 2026.

The patches name Last Seed's records by FormID (they override them or point at them), and the ESL build renumbered those, so each patch is
rewritten: every NNNNNN:LastSeed.esp reference gets the new id (esl-work/map_lastseed.json, written by tools/esl_build.py), every
NNNNNN:Campfire.esm reference gets the ESL Campfire's id, file and cell-folder names follow, the patch is flagged Small (ESL), and its own
records must already sit in the light range (they do: Chesko's patches are numbered from 000800). Anything it cannot map is an error.

Needs: tools/esl_build.py run first; patches/src (Spriggit YAML of the patches); Spriggit (tools/spriggit-cli.sh of the toolkit).
Usage: python tools/patches_esl.py [path to the Frostfall project's esl folder]
Output: esl-work/patches/ESL/<patch>.esp
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
lsmap = json.load(open(os.path.join(W, "map_lastseed.json")))
cmap = json.load(open(os.path.join(ESL_MAPS, "map_full.json")))
patches = json.load(open(os.path.join(HERE, "patches", "patches.json"), encoding="utf-8"))

ref_ls = re.compile(r"\b([0-9A-Fa-f]{6}):LastSeed\.esp")
ref_cf = re.compile(r"\b([0-9A-Fa-f]{6}):Campfire\.esm")


def read(p):
    with open(p, encoding="utf-8", errors="replace", newline="") as f:
        return f.read()


def write(p, t):
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, "w", encoding="utf-8", newline="") as f:
        f.write(t)


out_esp = os.path.join(W, "patches", "ESL")
os.makedirs(out_esp, exist_ok=True)
failed = 0
for p in patches:
    name = p["file"][:-4]
    src = os.path.join(HERE, "patches", "src", name)
    work = os.path.join(W, "patches", "yaml", name)
    if os.path.exists(work):
        shutil.rmtree(work)
    bad = []
    own_re = re.compile(r"^\s*(?:- )?FormKey: ([0-9A-F]{6}):" + re.escape(name) + r"\.esp\s*$", re.M | re.I)
    own = set()
    n_ls = n_cf = 0

    def sub(t):
        global n_ls, n_cf

        def a(m):
            global n_ls
            k = m.group(1).upper()
            if k not in lsmap:
                bad.append(("LastSeed.esp", k))
                return m.group(0)
            n_ls += 1
            return lsmap[k] + ":LastSeed.esp"

        def b(m):
            global n_cf
            k = m.group(1).upper()
            if k not in cmap:
                bad.append(("Campfire.esm", k))
                return m.group(0)
            n_cf += 1
            return cmap[k] + ":Campfire.esm"

        return ref_cf.sub(b, ref_ls.sub(a, t))

    for dp, _, fs in os.walk(src):
        for fn in fs:
            s = os.path.join(dp, fn)
            d = os.path.join(work, os.path.relpath(s, src))
            if fn.endswith(".yaml"):
                t = read(s)
                own.update(own_re.findall(t))
                write(d, sub(t))
            else:
                os.makedirs(os.path.dirname(d), exist_ok=True)
                shutil.copyfile(s, d)
    outside = [i for i in own if not 0x800 <= int(i, 16) <= 0xFFF]
    if bad or outside:
        print(f"FAILED {name}: unmapped {sorted(set(bad))[:5]}, own ids outside the light range {outside[:5]}")
        failed += 1
        continue
    # names that carry an old id
    for pattern, mp in ((re.compile(r"^(.* - )([0-9A-F]{6})(_LastSeed\.esp(?:\.yaml)?)$"), lsmap), (re.compile(r"^(.* - )([0-9A-F]{6})(_Campfire\.esm(?:\.yaml)?)$"), cmap)):
        for dp, dns, fs in os.walk(work, topdown=False):
            for nme in fs + dns:
                m = pattern.match(nme)
                if m and m.group(2) in mp:
                    os.rename(os.path.join(dp, nme), os.path.join(dp, m.group(1) + mp[m.group(2)] + m.group(3)))
    # the Small flag
    rd = os.path.join(work, "RecordData.yaml")
    t = read(rd)
    if "- Small" not in t:
        nl = "\r\n" if "\r\n" in t else "\n"
        assert "ModHeader:" + nl in t, "ModHeader not found"
        write(rd, t.replace("ModHeader:" + nl, "ModHeader:" + nl + "  Flags:" + nl + "  - Small" + nl, 1))
    target = os.path.join(out_esp, p["file"])
    r = subprocess.run(["bash", SPRIGGIT, "deserialize", "--InputPath", work, "--OutputPath", target], capture_output=True, text=True)
    ok = os.path.exists(target)
    print(f"{'ok    ' if ok else 'FAILED'} {name}: {n_ls} Last Seed refs, {n_cf} Campfire refs, {len(own)} own records")
    failed += not ok
print("done" if not failed else f"{failed} patch(es) failed")
sys.exit(1 if failed else 0)
