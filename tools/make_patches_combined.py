"""Builds ONE FOMOD installer for Last Seed's compatibility patches (patches/patches.json) that covers both builds of Last Seed 2026.

Page 1 asks which build is installed (regular or ESL); page 2 lists the patches, grouped as before. A patch whose mod is not in the load
order cannot be ticked. The chosen build decides which plugin is installed:

  Regular: Chesko's original plugins (from the Last Seed Development Kit download).
  ESL:     the plugins tools/patches_esl.py rewrote for the ESL build (Last Seed and ESL Campfire references renumbered, flagged ESL).

This replaces make_patches_release.py's two separate zips; that script is kept for the 1.0.0 releases.

Usage: python tools/make_patches_combined.py <version>
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
# the regular plugins are Chesko's originals exactly as shipped in the 1.0.0 regular patches zip (the Development Kit download they came from is no longer installed)
REGULAR_ZIP = os.path.join(HERE, "release", "Last Seed 2026 - Patches 1.0.0.zip")
version = sys.argv[1] if len(sys.argv) > 1 else "1.1.0"
patches = json.load(open(os.path.join(HERE, "patches", "patches.json"), encoding="utf-8"))
GROUPS = ["Food and recipe mods", "Wells and buckets", "Loot", "Other"]
OPTIONAL_GROUPS = {"Loot", "Wells and buckets"}  # gameplay choices: available when their mod is there, but never pre-ticked


def key(p):
    return "p_" + "".join(c for c in p["file"].lower() if c.isalnum())


def plugin_xml(p):
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
        f'<conditionFlags><flag name="{key(p)}">on</flag></conditionFlags>{td}</plugin>'
    )


build_step = (
    '<installStep name="Last Seed 2026 build"><optionalFileGroups><group name="Which Last Seed 2026 do you use?" type="SelectExactlyOne"><plugins order="Explicit">'
    '<plugin name="Regular"><description>Last Seed 2026 with the regular Campfire and Frostfall. Installs the original patch plugins.</description>'
    '<conditionFlags><flag name="build">regular</flag></conditionFlags><typeDescriptor><type name="Recommended"/></typeDescriptor></plugin>'
    '<plugin name="ESL"><description>Last Seed 2026 (ESL) with the ESL Campfire and Frostfall pair. Installs the patch plugins rewritten for the ESL build.</description>'
    '<conditionFlags><flag name="build">esl</flag></conditionFlags><typeDescriptor><type name="Optional"/></typeDescriptor></plugin>'
    "</plugins></group></optionalFileGroups></installStep>"
)
groups = []
for g in GROUPS:
    items = [p for p in patches if p["group"] == g]
    if items:
        groups.append(f'<group name={quoteattr(g)} type="SelectAny"><plugins order="Ascending">{"".join(plugin_xml(p) for p in items)}</plugins></group>')
patch_step = f'<installStep name="Patches"><optionalFileGroups>{"".join(groups)}</optionalFileGroups></installStep>'

patterns = []
for p in patches:
    for build in ("regular", "esl"):
        patterns.append(
            f'<pattern><dependencies operator="And"><flagDependency flag="build" value="{build}"/><flagDependency flag="{key(p)}" value="on"/></dependencies>'
            f'<files><file source={quoteattr(build + chr(92) + p["file"])} destination={quoteattr(p["file"])}/></files></pattern>'
        )

config = (
    '<?xml version="1.0" encoding="UTF-8"?>\n'
    '<config xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://qconsulting.ca/fo3/ModConfig5.0.xsd">'
    "<moduleName>Last Seed 2026 - Patches</moduleName>"
    f'<installSteps order="Explicit">{build_step}{patch_step}</installSteps>'
    f'<conditionalFileInstalls><patterns>{"".join(patterns)}</patterns></conditionalFileInstalls></config>\n'
)
info = (
    '<?xml version="1.0" encoding="UTF-8"?>\n'
    "<fomod><Name>Last Seed 2026 - Patches</Name><Author>Chesko and Aytrus (patches); CageTV (ESL variants, installer)</Author>"
    f"<Version>{version}</Version><Website>https://github.com/CageTV/Last-Seed-2026</Website>"
    "<Description>Compatibility patches for Last Seed 2026, regular and ESL, in one installer.</Description></fomod>\n"
)
readme = f"""Last Seed 2026 - Patches {version}

One installer for both builds of Last Seed 2026. It first asks which build you use (regular, or ESL with the ESL Campfire and Frostfall pair),
then which patches you want. A patch whose mod is not in your load order cannot be ticked. This replaces the two separate 1.0.0 downloads
(Patches and Patches ESL): the plugins are the same.

The compatibility patches for Last Seed that are still needed with Last Seed 2026. Tick the patches for the mods you have.

You do NOT need these, their changes are inside Last Seed 2026 now: General Patch, Attack Speed Fix, Your Own Thoughts, Apothecary Patch,
Growl, Travellers of Skyrim, the fishing bait patches (BBD Skyrim Fishing, Hunting in Skyrim: any fishing mod now works), iWant widgets.

Load the patches after LastSeed.esp and after the mod each one patches. Less Food belongs at the end of the load order.
https://github.com/CageTV/Last-Seed-2026
"""
for name, text in (("config", config), ("info", info)):
    xml.dom.minidom.parseString(text.encode("utf-8"))
os.makedirs(OUT, exist_ok=True)
zpath = os.path.join(OUT, f"Last Seed 2026 - Patches {version}.zip")
regular = zipfile.ZipFile(REGULAR_ZIP)
with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
    z.writestr("fomod/info.xml", info)
    z.writestr("fomod/ModuleConfig.xml", config)
    z.writestr("README.txt", readme)
    for p in patches:
        z.writestr("regular/" + p["file"], regular.read(p["file"]))
        z.write(os.path.join(W, "patches", "ESL", p["file"]), "esl/" + p["file"])
print(f"wrote {zpath}: {len(patches)} patches x 2 builds, {os.path.getsize(zpath) // 1024} KB")
