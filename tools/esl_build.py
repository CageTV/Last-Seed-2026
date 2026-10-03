"""Builds the ESL variant of Last Seed 2026 (the same plugin with every FormID moved into the light range, built to work with the ESL
Campfire / Frostfall pair).

What it does, in order (each step prints what it did; anything it cannot map is an error):
  1. numbers Last Seed's own records 000800, 000801, ... in order of their old FormID (a light plugin only has room for 2048)
  2. rewrites the Spriggit YAML in src/esp/LastSeed into esl-work/yaml: every NNNNNN:LastSeed.esp reference gets the new id, every
     NNNNNN:Campfire.esm reference gets the id it has in the ESL Campfire (esl map of the Frostfall project), file and cell-folder names follow,
     and the plugin is flagged Small (ESL)
  3. rewrites every Papyrus script that hard-codes a LastSeed.esp / Campfire.esm / Frostfall.esp FormID (GetFormFromFile) and compiles it
  4. writes plugin/src/EslMap.h, the old -> new table LastSeed.dll uses to find its forms when LastSeed.esp is light
  5. rewrites the other mods' files that point at Last Seed's forms by id (the Open Animation Replacer drunk-walk config)
After this, run Spriggit deserialize on esl-work/yaml to LastSeed.esp (tools/esl_build.py prints the command) and tools/esl_package.py.

Usage: python tools/esl_build.py [path to the Frostfall project's esl folder]
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
SRC_YAML = os.path.join(HERE, "src", "esp", "LastSeed")
OUT_YAML = os.path.join(W, "yaml")

cmap = json.load(open(os.path.join(ESL_MAPS, "map_full.json")))        # Campfire.esm old id -> ESL id
fmap = json.load(open(os.path.join(ESL_MAPS, "map_frostfall.json")))   # Frostfall.esp old id -> ESL id


def read(p):
    with open(p, encoding="utf-8", errors="replace", newline="") as f:
        return f.read()


def write(p, t):
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, "w", encoding="utf-8", newline="") as f:
        f.write(t)


# ---- 1. the map of Last Seed's own records
defn = re.compile(r"^\s*(?:- )?FormKey: ([0-9A-F]{6}):LastSeed\.esp\s*$", re.M)
own = set()
for dp, _, fs in os.walk(SRC_YAML):
    for fn in fs:
        if fn.endswith(".yaml"):
            own.update(defn.findall(read(os.path.join(dp, fn))))
ids = sorted(own, key=lambda x: int(x, 16))
if len(ids) > 0x800:
    sys.exit(f"too many records for a light plugin: {len(ids)} (limit 2048)")
lsmap = {old: f"{0x800 + i:06X}" for i, old in enumerate(ids)}
os.makedirs(W, exist_ok=True)
json.dump(lsmap, open(os.path.join(W, "map_lastseed.json"), "w"), indent=0)
print(f"1. {len(ids)} records -> {lsmap[ids[0]]} .. {lsmap[ids[-1]]} ({0x800 - len(ids)} free)")

# ---- 2. the YAML
if os.path.exists(OUT_YAML):
    shutil.rmtree(OUT_YAML)
ref_ls = re.compile(r"\b([0-9A-Fa-f]{6}):LastSeed\.esp")
ref_cf = re.compile(r"\b([0-9A-Fa-f]{6}):Campfire\.esm")
bad = []


def sub_yaml(t, where):
    def a(m):
        k = m.group(1).upper()
        if k not in lsmap:
            bad.append((where, "LastSeed.esp", k))
            return m.group(0)
        return lsmap[k] + ":LastSeed.esp"

    def b(m):
        k = m.group(1).upper()
        if k not in cmap:
            bad.append((where, "Campfire.esm", k))
            return m.group(0)
        return cmap[k] + ":Campfire.esm"

    return ref_cf.sub(b, ref_ls.sub(a, t))


changed = 0
for dp, _, fs in os.walk(SRC_YAML):
    for fn in fs:
        s = os.path.join(dp, fn)
        d = os.path.join(OUT_YAML, os.path.relpath(s, SRC_YAML))
        os.makedirs(os.path.dirname(d), exist_ok=True)
        if fn.endswith(".yaml"):
            t = read(s)
            t2 = sub_yaml(t, s)
            changed += t != t2
            write(d, t2)
        else:
            shutil.copyfile(s, d)
print(f"2. YAML files rewritten: {changed}")
if bad:
    print("UNMAPPED:", sorted(set(bad))[:20])
    sys.exit(1)

# file and folder names carry the old id: "Name - OLDID_LastSeed.esp[.yaml]" and "... OLDID_Campfire.esm"
for pattern, mp in ((re.compile(r"^(.* - )([0-9A-F]{6})(_LastSeed\.esp(?:\.yaml)?)$"), lsmap), (re.compile(r"^(.* - )([0-9A-F]{6})(_Campfire\.esm(?:\.yaml)?)$"), cmap)):
    for dp, dns, fs in os.walk(OUT_YAML, topdown=False):
        for n in fs + dns:
            m = pattern.match(n)
            if m and m.group(2) in mp:
                os.rename(os.path.join(dp, n), os.path.join(dp, m.group(1) + mp[m.group(2)] + m.group(3)))

# the Small (ESL) flag
rd = os.path.join(OUT_YAML, "RecordData.yaml")
t = read(rd)
if "- Small" not in t:
    nl = "\r\n" if "\r\n" in t else "\n"
    assert "ModHeader:" + nl in t, "ModHeader not found"
    t = t.replace("ModHeader:" + nl, "ModHeader:" + nl + "  Flags:" + nl + "  - Small" + nl, 1)
    write(rd, t)

# interior cells are filed by the last two digits of their FormID: re-home them after the renumbering
cells = os.path.join(OUT_YAML, "Cells")
if os.path.isdir(cells):
    tmpl = {}
    for b in os.listdir(cells):
        bp = os.path.join(cells, b)
        if os.path.isdir(bp) and os.path.exists(os.path.join(bp, "GroupRecordData.yaml")):
            tmpl["block"] = read(os.path.join(bp, "GroupRecordData.yaml"))
            for s in os.listdir(bp):
                if os.path.isdir(os.path.join(bp, s)):
                    tmpl["sub"] = read(os.path.join(bp, s, "GroupRecordData.yaml"))
    moves = []
    for b in os.listdir(cells):
        bp = os.path.join(cells, b)
        if not os.path.isdir(bp):
            continue
        for s in os.listdir(bp):
            sp = os.path.join(bp, s)
            if not os.path.isdir(sp):
                continue
            for c in os.listdir(sp):
                cp = os.path.join(sp, c)
                if os.path.isdir(cp):
                    fid = int(re.match(r"FormKey: ([0-9A-F]{6}):", read(os.path.join(cp, "RecordData.yaml")).splitlines()[0]).group(1), 16)
                    nb, nsub = str(fid % 10), str((fid // 10) % 10)
                    if (nb, nsub) != (b, s):
                        moves.append((cp, nb, nsub, c))
    for cp, nb, nsub, c in moves:
        dst = os.path.join(cells, nb, nsub)
        os.makedirs(dst, exist_ok=True)
        gb = os.path.join(cells, nb, "GroupRecordData.yaml")
        if not os.path.exists(gb):
            write(gb, re.sub(r"BlockNumber: \d+", f"BlockNumber: {nb}", tmpl["block"]))
        gs = os.path.join(dst, "GroupRecordData.yaml")
        if not os.path.exists(gs):
            write(gs, re.sub(r"BlockNumber: \d+", f"BlockNumber: {nsub}", tmpl["sub"]))
        shutil.move(cp, os.path.join(dst, c))
    for b in os.listdir(cells):
        bp = os.path.join(cells, b)
        if os.path.isdir(bp):
            for s in os.listdir(bp):
                sp = os.path.join(bp, s)
                if os.path.isdir(sp) and not [x for x in os.listdir(sp) if os.path.isdir(os.path.join(sp, x))]:
                    shutil.rmtree(sp)
            if not [x for x in os.listdir(bp) if os.path.isdir(os.path.join(bp, x))]:
                shutil.rmtree(bp)
    print(f"   cells re-homed: {len(moves)}")

# ---- 3. scripts
R1 = re.compile(r'(\(\s*)(0x[0-9A-Fa-f]+|\d+)(\s*,\s*")(LastSeed\.esp|Campfire\.esm|Frostfall\.esp)(")', re.I)
R2 = re.compile(r'"(\d+)___(Campfire\.esm|Frostfall\.esp|LastSeed\.esp)"', re.I)
maps = {"lastseed.esp": lsmap, "campfire.esm": cmap, "frostfall.esp": fmap}
SRC_DIRS = [os.path.join(HERE, "src", "scripts"), os.path.join(HERE, "imports", "lastseed-decompiled")]
sd = os.path.join(W, "scripts-src")
if os.path.exists(sd):
    shutil.rmtree(sd)
os.makedirs(sd)
unmapped = []
hits = {}


def remap_id(token, plugin, fn):
    hexed = token.lower().startswith("0x")
    v = (int(token, 16) if hexed else int(token)) & 0xFFFFFF  # GetFormFromFile ignores the load-order byte, decimal ids may still carry it
    new = maps[plugin.lower()].get(f"{v:06X}")
    if new is None:
        unmapped.append((fn, plugin, token))
        return token
    hits[fn] = hits.get(fn, 0) + 1
    return ("0x" + new) if hexed else str(int(new, 16))


done = set()
for d in SRC_DIRS:  # our own sources win over the decompiled ones of the same name
    for fn in sorted(os.listdir(d)):
        if not fn.lower().endswith(".psc") or fn.lower() in done:
            continue
        t = read(os.path.join(d, fn))
        t2 = R1.sub(lambda m: m.group(1) + remap_id(m.group(2), m.group(4), fn) + m.group(3) + m.group(4) + m.group(5), t)
        t2 = R2.sub(lambda m: '"' + remap_id(m.group(1), m.group(2), fn) + "___" + m.group(2) + '"', t2)
        done.add(fn.lower())
        if t2 != t:
            write(os.path.join(sd, fn), t2)
print(f"3. scripts with ids to rewrite: {len(hits)}: {', '.join(f'{k} ({v})' for k, v in sorted(hits.items()))}")
if unmapped:
    print("UNMAPPED script ids:", unmapped)
    sys.exit(1)

sys.path.insert(0, os.path.join(HERE, "tools"))
import build_scripts as b  # noqa: E402

pex = os.path.join(W, "pex")
if os.path.exists(pex):
    shutil.rmtree(pex)
os.makedirs(pex)
failed = []
for fn in sorted(os.listdir(sd)):
    imports = [sd] + [p for p in b.IMPORTS if p != b.SRC]
    r = subprocess.run([b.COMPILER, fn, f"-f={b.FLAGS}", "-i=" + ";".join(imports), f"-o={pex}"], capture_output=True, text=True, cwd=sd, timeout=180)
    if r.returncode != 0 or not os.path.exists(os.path.join(pex, fn[:-4] + ".pex")):
        failed.append((fn, (r.stdout + r.stderr)[-600:]))
print(f"   compiled {len(os.listdir(pex))} of {len(os.listdir(sd))}")
for fn, log in failed:
    print("   FAILED", fn, log)

# ---- 4. the DLL's table
rows = sorted((int(k, 16), int(v, 16)) for k, v in lsmap.items())
frost = [(int(k, 16), int(v, 16)) for k, v in fmap.items() if k.upper() == "06DCFB"]  # FrostfallRunning, the one Frostfall form the DLL reads
lines = ["// GENERATED by tools/esl_build.py. Old local FormID -> ESL local FormID, used when LastSeed.esp (or Frostfall.esp) is loaded as a light plugin.", "#pragma once", "", "namespace eslmap", "{",
         "\tstruct Pair { unsigned int from, to; };", "\tinline constexpr Pair kLastSeed[] = {"]
for a, c in rows:
    lines.append(f"\t\t{{ 0x{a:06X}, 0x{c:06X} }},")
lines += ["\t};", "\tinline constexpr Pair kFrostfall[] = {"]
for a, c in frost:
    lines.append(f"\t\t{{ 0x{a:06X}, 0x{c:06X} }},")
lines += ["\t};", "}", ""]
write(os.path.join(HERE, "plugin", "src", "EslMap.h"), "\n".join(lines))
print(f"4. plugin/src/EslMap.h: {len(rows)} Last Seed ids, {len(frost)} Frostfall id")

# ---- 5. other mods' files that name Last Seed forms by id
# The Open Animation Replacer drunk-walk config names Last Seed's drunk effects ("formID" is a hex string: 10880 = 0x010880 = _Seed_DrunkEffect3).
OAR_REL = os.path.join("meshes", "actors", "character", "animations", "OpenAnimationReplacer", "Drunk animations", "Drunken", "config.json")
OAR = os.path.join(r"E:\Tabula Rasa\mods", "Drunk or drugged animations OAR", OAR_REL)
if os.path.exists(OAR):
    t = read(OAR)
    n = 0
    missing = []

    def f(m):
        global n
        old = f"{int(m.group(2), 16):06X}"
        if old not in lsmap:
            missing.append(old)
            return m.group(0)
        n += 1
        return m.group(1) + f"{int(lsmap[old], 16):X}" + m.group(3)

    t2 = re.sub(r'("pluginName":\s*"LastSeed\.esp",\s*"formID":\s*")([0-9A-Fa-f]+)(")', f, t)
    if missing:
        print("UNMAPPED OAR ids:", missing)
        sys.exit(1)
    write(os.path.join(W, "other-mods", OAR_REL), t2)
    print(f"5. OAR drunk-walk config: {n} Last Seed ids remapped")
print("done. Next: spriggit deserialize esl-work/yaml -> LastSeed.esp, then tools/esl_package.py")
