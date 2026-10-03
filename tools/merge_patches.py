"""Merges Last Seed compatibility patches that need no master beyond LastSeed.esp and the base game into a copy of LastSeed.esp.

Everything works on Spriggit YAML folders (serialize the plugins first with tools/spriggit-cli.sh, deserialize the result afterwards):
every record a patch contains replaces the same record (same FormKey) in the output tree; later patches win over earlier ones. A patch that
has a master the base plugin does not have is refused, so a merged LastSeed.esp can never gain a dependency.

Usage: python tools/merge_patches.py <base yaml folder> <output yaml folder> <patch yaml folder> [<patch yaml folder> ...]
"""
import os
import re
import shutil
import sys


def read(p):
    with open(p, encoding="utf-8", errors="replace") as f:
        return f.read()


def masters(folder):
    return re.findall(r"^\s*- Master: (\S.*?)\s*$", read(os.path.join(folder, "RecordData.yaml")), re.M)


def records(folder):
    """{(id, plugin): path} for every record file in a Spriggit tree."""
    out = {}
    for root, _, files in os.walk(folder):
        for fn in files:
            if not fn.endswith(".yaml") or fn == "RecordData.yaml":
                continue
            p = os.path.join(root, fn)
            m = re.search(r"^FormKey:\s*([0-9A-Fa-f]{6}):(.+)$", read(p), re.M)
            if m:
                out[(m.group(1).upper(), m.group(2).strip().lower())] = p
    return out


def main():
    if len(sys.argv) < 4:
        raise SystemExit(__doc__)
    base, out, *patches = sys.argv[1:]
    if os.path.exists(out):
        raise SystemExit(f"{out} exists: remove it first (this tool never overwrites)")
    shutil.copytree(base, out)
    base_masters = {m.lower() for m in masters(base)} | {"lastseed.esp"}
    index = records(out)
    applied = 0
    for patch in patches:
        extra = [m for m in masters(patch) if m.lower() not in base_masters]
        if extra:
            raise SystemExit(f"{os.path.basename(patch)} needs {extra}, which LastSeed.esp does not: refusing to merge it")
        added = replaced = 0
        for key, src in records(patch).items():
            if key in index:
                shutil.copyfile(src, index[key])
                replaced += 1
            else:
                rel = os.path.relpath(src, patch)
                dest = os.path.join(out, rel)
                os.makedirs(os.path.dirname(dest), exist_ok=True)
                shutil.copyfile(src, dest)
                index[key] = dest
                added += 1
        print(f"{os.path.basename(patch)}: {replaced} records replaced, {added} added")
        applied += replaced + added
    # say so in the plugin description
    rd = os.path.join(out, "RecordData.yaml")
    t = read(rd)
    m = re.search(r"^  Description: (.*)$", t, re.M)
    if m and "Last Seed 2026" not in m.group(1):
        t = t.replace(m.group(0), m.group(0).rstrip().rstrip("'\"") + " (Last Seed 2026 build with the General, Attack Speed and first-person message patches merged.)", 1)
        open(rd, "w", encoding="utf-8", newline="\n").write(t)
    print(f"{applied} records applied in total")


if __name__ == "__main__":
    main()
