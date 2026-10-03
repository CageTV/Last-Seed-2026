"""Builds the two FOMOD installers for Last Seed's compatibility patches (patches/patches.json) into release/: one for the regular build of
Last Seed 2026 and one for the ESL build. They are separate downloads so nobody installs the wrong one.

  Regular: Chesko's original plugins (from the Last Seed Development Kit download).
  ESL:     the plugins tools/patches_esl.py rewrote for the ESL build (Last Seed and ESL Campfire references renumbered, flagged ESL).

In both, a patch whose mod is not in the load order cannot be ticked.

Usage: python tools/make_patches_release.py <version>
"""
import json
import os
import sys
import xml.dom.minidom
import zipfile
from xml.sax.saxutils import escape, quoteattr

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W = os.path.join(HERE, "esl-work")
OUT = os.path.join(HERE, "release")
DEVKIT = r"E:\Tabula Rasa\mods\Last Seed - Development Kit"
version = sys.argv[1] if len(sys.argv) > 1 else "1.0.0"
patches = json.load(open(os.path.join(HERE, "patches", "patches.json"), encoding="utf-8"))
GROUPS = ["Food and recipe mods", "Wells and buckets", "Loot", "Other"]
OPTIONAL_GROUPS = {"Loot", "Wells and buckets"}  # gameplay choices: available when their mod is there, but never pre-ticked


def plugin_xml(p, source):
    avail = "Optional" if p["group"] in OPTIONAL_GROUPS else "Recommended"
    if p["requires"]:
        inner = "".join(f'<fileDependency file={quoteattr(r)} state="Active"/>' for r in p["requires"])
        td = (
            '<typeDescriptor><dependencyType><defaultType name="NotUsable"/><patterns><pattern>'
            f'<dependencies operator="And">{inner}</dependencies><type name="{avail}"/></pattern></patterns></dependencyType></typeDescriptor>'
        )
    else:
        td = '<typeDescriptor><type name="Optional"/></typeDescriptor>'
    needs = f" Needs: {', '.join(p['requires'])}." if p["requires"] else ""
    return (
        f'<plugin name={quoteattr(p["title"])}><description>{escape(p["description"] + needs)}</description>'
        f'<files><file source={quoteattr(p["file"])} destination={quoteattr(p["file"])}/></files>{td}</plugin>'
    )


def build(variant):
    esl = variant == "ESL"
    label = "Last Seed 2026 (ESL)" if esl else "Last Seed 2026"
    suffix = " (ESL)" if esl else ""
    groups = []
    for g in GROUPS:
        items = [p for p in patches if p["group"] == g]
        if items:
            groups.append(f'<group name={quoteattr(g)} type="SelectAny"><plugins order="Ascending">{"".join(plugin_xml(p, p["file"]) for p in items)}</plugins></group>')
    config = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<config xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://qconsulting.ca/fo3/ModConfig5.0.xsd">'
        f"<moduleName>Last Seed 2026 - Patches{suffix}</moduleName>"
        f'<installSteps order="Explicit"><installStep name="Patches for {escape(label)}"><optionalFileGroups>{"".join(groups)}</optionalFileGroups></installStep></installSteps></config>\n'
    )
    info = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        f"<fomod><Name>Last Seed 2026 - Patches{suffix}</Name><Author>Chesko and Aytrus (patches); CageTV (ESL variants, installer)</Author>"
        f"<Version>{version}</Version><Website>https://github.com/CageTV/Last-Seed-2026</Website>"
        f"<Description>Compatibility patches for {label}.</Description></fomod>\n"
    )
    esl_words = (
        "THIS IS THE ESL VERSION: for 'Last Seed 2026 (ESL)' with the ESL Campfire and Frostfall pair. Do not use it with the regular Last Seed 2026.\n"
        "The regular patches are a separate download (Last Seed 2026 - Patches).\n"
        if esl
        else "THIS IS THE REGULAR VERSION: for 'Last Seed 2026' with the regular Campfire and Frostfall. For the ESL build use 'Last Seed 2026 - Patches (ESL)'.\n"
    )
    readme = f"""Last Seed 2026 - Patches {version}{suffix}

{esl_words}
The compatibility patches for Last Seed that are still needed with Last Seed 2026. Install it with your mod manager (it is a FOMOD) and tick the
patches for the mods you have; a patch whose mod is not in your load order cannot be ticked.

You do NOT need these, their changes are inside Last Seed 2026 now: General Patch, Attack Speed Fix, Your Own Thoughts, Apothecary Patch,
Growl, Travellers of Skyrim, the fishing bait patches (BBD Skyrim Fishing, Hunting in Skyrim: any fishing mod now works), iWant widgets.

Load the patches after LastSeed.esp and after the mod each one patches. Less Food belongs at the end of the load order.
https://github.com/CageTV/Last-Seed-2026
"""
    xml.dom.minidom.parseString(config.encode("utf-8"))
    xml.dom.minidom.parseString(info.encode("utf-8"))
    os.makedirs(OUT, exist_ok=True)
    zpath = os.path.join(OUT, f"Last Seed 2026 - Patches {version}{' ESL' if esl else ''}.zip")
    with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("fomod/info.xml", info)
        z.writestr("fomod/ModuleConfig.xml", config)
        z.writestr("README.txt", readme)
        for p in patches:
            src = os.path.join(W, "patches", "ESL", p["file"]) if esl else os.path.join(DEVKIT, p["file"])
            z.write(src, p["file"])
    print(f"wrote {zpath}: {len(patches)} patches, {os.path.getsize(zpath) // 1024} KB")


build("Regular")
build("ESL")
