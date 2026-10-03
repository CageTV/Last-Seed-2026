"""Builds plugin/src/McmTable.h: every setting on Last Seed's Gameplay, Needs, Vitality, Alcohol & Disease and Food Spoilage MCM pages.

Last Seed is Chesko's mod (MIT for its scripts and plugin; its meshes and textures are not covered, and this tool touches neither). The table is
read from Last Seed's own MCM script (decompiled with Champollion from the installed _Seed_SkyUIConfigPanelScript.pex), the FormIDs of the
settings' globals (LastSeed.esp serialized with Spriggit) and the English strings (Interface/Translations/LastSeed_ENGLISH.txt).

Usage:  python tools/extract_mcm.py <MCM script .psc> <Spriggit YAML folder of LastSeed.esp> <LastSeed_ENGLISH.txt> [--out plugin/src/McmTable.h]
Prints everything it could not turn into a plain table row, so a human can review it before the table is trusted.
"""
import os
import re
import sys

PAGES = [
    ("Gameplay", "PageReset_Gameplay"),
    ("Needs", "PageReset_Needs"),
    ("Vitality", "PageReset_Vitality"),
    ("Alcohol, Skooma & Disease", "PageReset_AlcoholDisease"),
    ("Food Spoilage", "PageReset_FoodSpoilage"),
]


def read(path, enc="utf-8"):
    with open(path, encoding=enc, errors="replace") as f:
        return f.read()


def strip_comment(line):
    i = line.find(" ; #DEBUG_LINE_NO")
    return line[:i] if i >= 0 else line


def function_bodies(text):
    """name -> list of (comment-stripped) lines, for every top level Function/Event."""
    out, cur, name = {}, None, None
    for raw in text.splitlines():
        m = re.match(r"^(?:Function|Event)\s+(\w+)\(", raw)
        if m:
            name, cur = m.group(1), []
            out[name] = cur
            continue
        if re.match(r"^End(?:Function|Event)\b", raw):
            name, cur = None, None
            continue
        if cur is not None:
            cur.append(strip_comment(raw).rstrip())
    return out


def option_blocks(lines):
    """Splits an OnOption* handler into {OID name: [lines]} at each `If/ElseIf option == X`."""
    blocks, cur = {}, None
    for l in lines:
        m = re.match(r"^\s*(?:Else)?If option == (\w+)\s*$", l)
        if m:
            cur = blocks.setdefault(m.group(1), [])
            continue
        if re.match(r"^\s*(?:Else|EndIf)\s*$", l) and cur is not None and l.startswith("  Else") is False:
            pass
        if cur is not None:
            cur.append(l)
    return blocks


def load_arrays(body):
    """Arrays assigned in loadArrays: name -> list of $keys / strings."""
    arrays = {}
    for l in body:
        m = re.match(r"^\s*(\w+) = new String\[(\d+)\]", l)
        if m:
            arrays[m.group(1)] = [None] * int(m.group(2))
            continue
        m = re.match(r'^\s*(\w+)\[(\d+)\] = "(.*)"\s*$', l)
        if m and m.group(1) in arrays:
            arrays[m.group(1)][int(m.group(2))] = m.group(3)
    return arrays


def globals_from_yaml(folder):
    out = {}
    gdir = os.path.join(folder, "Globals")
    for fn in os.listdir(gdir):
        t = read(os.path.join(gdir, fn))
        eid = re.search(r"^EditorID:\s*(\S+)", t, re.M)
        fk = re.search(r"^FormKey:\s*([0-9A-Fa-f]{6}):(\S+)", t, re.M)
        data = re.search(r"^Data:\s*(\S+)", t, re.M)
        typ = re.search(r"^MutagenObjectType:\s*(\w+)", t, re.M)
        if eid and fk:
            out[eid.group(1).lower()] = (int(fk.group(1), 16), fk.group(2), data.group(1) if data else "0", typ.group(1) if typ else "")
    return out


def translations(path):
    out = {}
    for l in read(path, "utf-16").splitlines():
        if "\t" in l and l.startswith("$"):
            k, v = l.split("\t", 1)
            out[k.strip()] = v.strip()
    return out


def fl(x):
    t = f"{x:g}"
    if '.' not in t and 'e' not in t:
        t += '.0'
    return t + 'f'


def cstr(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    out_path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "plugin", "src", "McmTable.h")
    if "--out" in sys.argv:
        out_path = sys.argv[sys.argv.index("--out") + 1]
        args = [a for a in args if a != out_path]
    if len(args) != 3:
        raise SystemExit(__doc__)
    psc, yaml_dir, trans_path = args
    funcs = function_bodies(read(psc))
    arrays = load_arrays(funcs["loadArrays"])
    gl = globals_from_yaml(yaml_dir)
    tr = translations(trans_path)
    problems = []

    def label(key):
        if key.startswith("$"):
            if key not in tr:
                problems.append(f"no English string for {key}")
            return tr.get(key, key)
        return key

    sel = option_blocks(funcs["OnOptionSelect"])
    sopen = option_blocks(funcs["OnOptionSliderOpen"])
    sacc = option_blocks(funcs["OnOptionSliderAccept"])
    mopen = option_blocks(funcs["OnOptionMenuOpen"])
    macc = option_blocks(funcs["OnOptionMenuAccept"])
    odef = option_blocks(funcs["OnOptionDefault"])

    def resolve(prop):
        g = gl.get(prop.lower())
        if not g:
            problems.append(f"global {prop} not found in the ESP")
            return None
        if g[1].lower() != "lastseed.esp":
            problems.append(f"global {prop} lives in {g[1]} (FormKey {g[0]:06X}), not LastSeed.esp")
        return g

    pages_out = []
    used_arrays = {}
    for title, fname in PAGES:
        rows = []
        for l in funcs[fname]:
            s = l.strip()
            m = re.match(r'^Self\.AddHeaderOption\("([^"]+)", \d+\)$', s)
            if m:
                rows.append(("header", label(m.group(1))))
                continue
            if s.startswith("Self.SetCursorPosition(1)"):
                rows.append(("column",))
                continue
            if s in ("Self.AddEmptyOption()", "Self.SetCursorFillMode(Self.TOP_TO_BOTTOM)", "Return") or s.startswith("If LastSeedRunning") or s.startswith("Self.AddTextOption(\"$LastSeedNotRunning") or s == "EndIf":
                continue
            m = re.match(r'^(\w+) = Self\._Seed_AddToggleOption\((\w+), "([^"]+)"\)$', s)
            if m:
                rows.append(("toggle", m.group(1), m.group(2), label(m.group(3))))
                continue
            m = re.match(r'^(\w+) = Self\.AddSliderOption\("([^"]+)", (\w+)\.getValue\(\), "([^"]*)", \d+\)$', s)
            if m:
                rows.append(("slider", m.group(1), m.group(3), label(m.group(2)), m.group(4)))
                continue
            m = re.match(r'^(\w+) = Self\.AddMenuOption\("([^"]+)", (\w+)\[(\w+)\.GetValueInt\(\)( - (\d+))?\], \d+\)$', s)
            if m:
                rows.append(("menu", m.group(1), m.group(4), label(m.group(2)), m.group(3), int(m.group(6) or 0)))
                continue
            problems.append(f"{title}: unparsed line: {s}")
        pages_out.append((title, rows))

    # ---- emit ----
    lines = []
    A = lines.append
    A("// GENERATED by tools/extract_mcm.py from Last Seed's own MCM script, LastSeed.esp and English strings. Do not edit by hand.")
    A("// Last Seed - Survival Needs and Diseases is by Chesko and Aytrus; its scripts and plugin are MIT licensed (the meshes and textures are not part of this).")
    A("#pragma once")
    A("")
    A("#include <iterator>")
    A("")
    A("namespace mcm")
    A("{")
    A("	enum class Kind { Header, Column, Toggle, Slider, Menu };")
    A("")
    A("	// One row of a page. formId is the local FormID of the setting's global in LastSeed.esp (0 for headers and column breaks).")
    A("	// Toggles hold 1 (off) or 2 (on). A menu stores its list position plus menuBase. Sliders are saved to the profile as integers.")
    A("	struct Entry")
    A("	{")
    A("		Kind                kind;")
    A("		const char*         label;")
    A("		unsigned int        formId;")
    A("		const char*         profileKey;  // key in the profile file (\"\" when the setting is not saved to profiles)")
    A("		float               def, min, max, step;")
    A("		const char*         format;      // slider display format, e.g. {1} = one decimal")
    A("		const char* const*  options;     // menu entries")
    A("		int                 optionCount;")
    A("		int                 menuBase;    // value stored for the first menu entry")
    A("	};")
    A("")
    # menu arrays first
    menu_arrays = {}
    for title, rows in pages_out:
        for r in rows:
            if r[0] == "menu":
                menu_arrays[r[4]] = arrays.get(r[4])
    for name, vals in menu_arrays.items():
        if vals is None:
            problems.append(f"menu list {name} not found in loadArrays")
            continue
        A(f"	inline constexpr const char* kList_{name}[] = {{")
        for v in vals:
            A(f"		{cstr(label(v) if v else '')},")
        A("	};")
    A("")
    page_ids = []
    for pi, (title, rows) in enumerate(pages_out):
        A(f"	inline constexpr Entry kPage{pi}[] = {{")
        for r in rows:
            if r[0] == "header":
                A(f"		{{ Kind::Header, {cstr(r[1])}, 0, \"\", 0.0f, 0.0f, 0.0f, 0.0f, \"\", nullptr, 0, 0 }},")
            elif r[0] == "column":
                A("		{ Kind::Column, \"\", 0, \"\", 0, 0, 0, 0, \"\", nullptr, 0, 0 },")
            elif r[0] == "toggle":
                _, oid, prop, lab = r
                g = resolve(prop)
                key = ""
                b = sel.get(oid)
                if b is None:
                    problems.append(f"toggle {oid} has no OnOptionSelect branch")
                else:
                    m = re.search(r'_Seed_OnOptionSelect\(' + re.escape(prop) + r', ' + oid + r', "([^"]*)"\)', "\n".join(b))
                    if m:
                        key = m.group(1)
                    else:
                        problems.append(f"toggle {oid}: unusual OnOptionSelect: {' | '.join(x.strip() for x in b)}")
                    extra = [x.strip() for x in b if x.strip() and "_Seed_OnOptionSelect" not in x]
                    if extra:
                        problems.append(f"toggle {oid} ({lab}): extra effects on select: {' | '.join(extra)}")
                A(f"		{{ Kind::Toggle, {cstr(lab)}, 0x{g[0]:06X}, {cstr(key)}, 1, 1, 2, 1, \"\", nullptr, 0, 1 }},")
            elif r[0] == "slider":
                _, oid, prop, lab, fmt = r
                g = resolve(prop)
                o = "\n".join(sopen.get(oid, []))
                dm = re.search(r"SetSliderDialogDefaultValue\(([^)]*)\)", o)
                rm = re.search(r"SetSliderDialogRange\(([^,]*),\s*([^)]*)\)", o)
                im = re.search(r"SetSliderDialogInterval\(([^)]*)\)", o)

                def num(s):
                    s = s.replace(" as Float", "").strip()
                    return float(s)

                if not (dm and rm and im):
                    problems.append(f"slider {oid} ({lab}): range not found")
                    d, mn, mx, st = 0, 0, 0, 1
                else:
                    d, mn, mx, st = num(dm.group(1)), num(rm.group(1)), num(rm.group(2)), num(im.group(1))
                key = ""
                b = sacc.get(oid, [])
                m = re.search(r'SaveSettingToCurrentProfile\("([^"]*)"', "\n".join(b))
                if m:
                    key = m.group(1)
                else:
                    problems.append(f"slider {oid} ({lab}): no profile key")
                extra = [x.strip() for x in b if x.strip() and not re.search(r"(\.setValue\(value\)|SetSliderOptionValue|SaveSettingToCurrentProfile)", x)]
                if extra:
                    problems.append(f"slider {oid} ({lab}): extra effects on accept: {' | '.join(extra)}")
                A(f"		{{ Kind::Slider, {cstr(lab)}, 0x{g[0]:06X}, {cstr(key)}, {fl(d)}, {fl(mn)}, {fl(mx)}, {fl(st)}, {cstr(fmt)}, nullptr, 0, 0 }},")
            elif r[0] == "menu":
                _, oid, prop, lab, arr, base = r
                g = resolve(prop)
                key = ""
                b = macc.get(oid, [])
                m = re.search(r'SaveSettingToCurrentProfile\("([^"]*)"', "\n".join(b))
                if m:
                    key = m.group(1)
                else:
                    problems.append(f"menu {oid} ({lab}): no profile key")
                m = re.search(r"\.SetValueInt\(index( \+ (\d+))?\)", "\n".join(b))
                accept_base = int(m.group(2) or 0) if m else base
                if accept_base != base:
                    problems.append(f"menu {oid}: shown with offset {base} but stored with offset {accept_base}")
                dm = re.search(r"SetMenuDialogDefaultIndex\((\d+)\)", "\n".join(mopen.get(oid, [])))
                extra = [x.strip() for x in b if x.strip() and not re.search(r"(SetMenuOptionValue|SetValueInt|SaveSettingToCurrentProfile)", x)]
                if extra:
                    problems.append(f"menu {oid} ({lab}): extra effects on accept: {' | '.join(extra)}")
                n = len(arrays.get(arr) or [])
                A(f"		{{ Kind::Menu, {cstr(lab)}, 0x{g[0]:06X}, {cstr(key)}, {fl(int(dm.group(1)) if dm else 0)}, 0.0f, 0.0f, 0.0f, \"\", kList_{arr}, {n}, {accept_base} }},")
        A("	};")
        A("")
        page_ids.append((title, f"kPage{pi}"))
    A("	struct Page")
    A("	{")
    A("		const char*  title;")
    A("		const Entry* entries;")
    A("		int          count;")
    A("	};")
    A("	inline constexpr Page kPages[] = {")
    for title, ident in page_ids:
        A(f"		{{ {cstr(title)}, {ident}, static_cast<int>(std::size({ident})) }},")
    A("	};")
    A("}")
    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    total = sum(1 for _, rows in pages_out for r in rows if r[0] in ("toggle", "slider", "menu"))
    print(f"wrote {out_path}: {len(pages_out)} pages, {total} settings")
    for p in problems:
        print("  REVIEW:", p)
    print(f"{len(problems)} item(s) to review")


if __name__ == "__main__":
    main()
