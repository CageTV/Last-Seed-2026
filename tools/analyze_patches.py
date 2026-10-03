"""Summarises Last Seed's compatibility patches (serialized with Spriggit, one folder per patch) so they can be sorted into what LastSeed.dll can
do at run time and what has to stay a plugin.

For each patch: its masters, then its records grouped by where they come from (a new record of the patch itself, or an override of a record
from Skyrim.esm / LastSeed.esp / another mod) and by record type.

Usage: python tools/analyze_patches.py <folder with one Spriggit YAML folder per patch> [patch name filter]
"""
import collections
import os
import re
import sys


def read(p):
    with open(p, encoding="utf-8", errors="replace") as f:
        return f.read()


def analyse(folder, patch):
    masters = re.findall(r"- Master: (\S.*)", read(os.path.join(folder, "RecordData.yaml")))
    counts = collections.Counter()
    samples = {}
    for root, _, files in os.walk(folder):
        for fn in files:
            if not fn.endswith(".yaml") or fn in ("RecordData.yaml",):
                continue
            t = read(os.path.join(root, fn))
            fk = re.search(r"^FormKey:\s*([0-9A-Fa-f]{6}):(.+)$", t, re.M)
            ty = re.search(r"^MutagenObjectType:\s*(\w+)", t, re.M)
            if not fk:
                continue
            owner = fk.group(2).strip()
            kind = "NEW" if owner.lower() == (patch + ".esp").lower() else f"override of {owner}"
            typ = ty.group(1) if ty else os.path.basename(root)
            eid = re.search(r"^EditorID:\s*(\S+)", t, re.M)
            counts[(kind, typ)] += 1
            samples.setdefault((kind, typ), []).append(eid.group(1) if eid else fk.group(1))
    return masters, counts, samples


def main():
    folder = sys.argv[1]
    flt = sys.argv[2].lower() if len(sys.argv) > 2 else ""
    for name in sorted(os.listdir(folder)):
        d = os.path.join(folder, name)
        if not os.path.isdir(d) or flt not in name.lower():
            continue
        masters, counts, samples = analyse(d, name)
        print(f"\n=== {name}")
        print("  masters:", ", ".join(masters))
        for (kind, typ), n in sorted(counts.items()):
            ex = ", ".join(samples[(kind, typ)][:4])
            print(f"  {n:4d} {typ:22s} {kind}   e.g. {ex}")


if __name__ == "__main__":
    main()
