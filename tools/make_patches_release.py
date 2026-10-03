"""Builds the FOMOD installer for Last Seed's compatibility patches (patches/patches.json) into release/.

The zip holds both variants of every patch: Regular/ (Chesko's original plugins, from the Last Seed Development Kit download) and ESL/
(tools/patches_esl.py, for the ESL build of Last Seed 2026). The installer first asks which build of Last Seed 2026 is installed, then lists the
patches; a patch whose mod is not in the load order cannot be ticked.

Usage: python tools/make_patches_release.py <version>
"""
import json
import os
import sys
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


def dependency(requires):
    if not requires:
        return None
    inner = "".join(f'<fileDependency file={quoteattr(r)} state="Active"/>' for r in requires)
    return f'<dependencies operator="And">{inner}</dependencies>'


def plugin_xml(i, p):
    dep = dependency(p["requires"])
    avail = "Optional" if p["group"] in OPTIONAL_GROUPS else "Recommended"
    if dep:
        td = (
            '<typeDescriptor><dependencyType><defaultType name="NotUsable"/><patterns><pattern>'
            f'{dep}<type name="{avail}"/></pattern></patterns></dependencyType></typeDescriptor>'
        )
    else:
        td = f'<typeDescriptor><type name="Optional"/></typeDescriptor>'
    needs = f" Needs: {', '.join(p['requires'])}." if p["requires"] else ""
    return (
        f'<plugin name={quoteattr(p["title"])}><description>{escape(p["description"] + needs)}</description>'
        f'<conditionFlags><flag name="p{i}">On</flag></conditionFlags>{td}</plugin>'
    )


steps = []
steps.append(
    '<installStep name="Last Seed 2026 build"><optionalFileGroups>'
    '<group name="Which build of Last Seed 2026 do you have?" type="SelectExactlyOne"><plugins order="Explicit">'
    '<plugin name="Regular"><description>For the regular Last Seed 2026 (regular Campfire and Frostfall).</description>'
    '<conditionFlags><flag name="build">Regular</flag></conditionFlags><typeDescriptor><type name="Recommended"/></typeDescriptor></plugin>'
    '<plugin name="ESL"><description>For Last Seed 2026 (ESL), built for the ESL Campfire and Frostfall pair.</description>'
    '<conditionFlags><flag name="build">ESL</flag></conditionFlags><typeDescriptor><type name="Optional"/></typeDescriptor></plugin>'
    "</plugins></group></optionalFileGroups></installStep>"
)
groups_xml = []
for g in GROUPS:
    items = [(i, p) for i, p in enumerate(patches) if p["group"] == g]
    if items:
        groups_xml.append(
            f'<group name={quoteattr(g)} type="SelectAny"><plugins order="Ascending">{"".join(plugin_xml(i, p) for i, p in items)}</plugins></group>'
        )
steps.append(f'<installStep name="Patches"><optionalFileGroups>{"".join(groups_xml)}</optionalFileGroups></installStep>')

patterns = []
for i, p in enumerate(patches):
    for build in ("Regular", "ESL"):
        patterns.append(
            f'<pattern><dependencies operator="And"><flagDependency flag="build" value="{build}"/><flagDependency flag="p{i}" value="On"/></dependencies>'
            f'<files><file source={quoteattr(build + chr(92) + p["file"])} destination={quoteattr(p["file"])}/></files></pattern>'
        )

config = (
    '<?xml version="1.0" encoding="UTF-8"?>\n'
    '<config xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://qconsulting.ca/fo3/ModConfig5.0.xsd">'
    "<moduleName>Last Seed 2026 - Patches</moduleName>"
    f'<installSteps order="Explicit">{"".join(steps)}</installSteps>'
    f'<conditionalFileInstalls><patterns>{"".join(patterns)}</patterns></conditionalFileInstalls></config>\n'
)
info = (
    '<?xml version="1.0" encoding="UTF-8"?>\n<fomod><Name>Last Seed 2026 - Patches</Name><Author>Chesko and Aytrus (patches); CageTV (ESL variants, installer)</Author>'
    f"<Version>{version}</Version><Website>https://github.com/CageTV/Last-Seed-2026</Website>"
    "<Description>Compatibility patches for Last Seed 2026, regular and ESL.</Description></fomod>\n"
)
readme = f"""Last Seed 2026 - Patches {version}

The compatibility patches for Last Seed that are still needed with Last Seed 2026, with a regular and an ESL version of each and an installer
that picks the right one. Install it with your mod manager (it is a FOMOD): choose the build of Last Seed 2026 you use, then tick the patches
for the mods you have; a patch whose mod is not in your load order cannot be ticked.

You do NOT need these, their changes are inside Last Seed 2026 now: General Patch, Attack Speed Fix, Your Own Thoughts, Apothecary Patch,
Growl, Travellers of Skyrim, the fishing bait patches (BBD Skyrim Fishing, Hunting in Skyrim: any fishing mod now works), iWant widgets.

Load the patches after LastSeed.esp and after the mod each one patches. Less Food belongs at the end of the load order.
https://github.com/CageTV/Last-Seed-2026
"""
# well-formed?
import xml.dom.minidom

xml.dom.minidom.parseString(config.encode("utf-8"))
xml.dom.minidom.parseString(info.encode("utf-8"))

os.makedirs(OUT, exist_ok=True)
zpath = os.path.join(OUT, f"Last Seed 2026 - Patches {version}.zip")
with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
    z.writestr("fomod/info.xml", info)
    z.writestr("fomod/ModuleConfig.xml", config)
    z.writestr("README.txt", readme)
    for p in patches:
        z.write(os.path.join(DEVKIT, p["file"]), f"Regular/{p['file']}")
        z.write(os.path.join(W, "patches", "ESL", p["file"]), f"ESL/{p['file']}")
print(f"wrote {zpath}: {len(patches)} patches x 2 variants, {os.path.getsize(zpath) // 1024} KB")
