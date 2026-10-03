"""Builds ONE FOMOD installer for Last Seed 2026 that covers the regular and the ESL build (release/Last Seed 2026 1.1.0.zip).

The files both builds share (DLL, logo and icons, the nine scripts that are identical) are installed always; LastSeed.esp differs, and the ESL
build adds six rebuilt scripts. All files are taken from the published 1.0.0 regular and ESL zips, so the content is byte-identical to them.
Usage: python tools/make_release_combined.py [version]
"""
import hashlib
import os
import sys
import xml.dom.minidom
import zipfile

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
R = os.path.join(HERE, "release")
version = sys.argv[1] if len(sys.argv) > 1 else "1.1.0"
reg = zipfile.ZipFile(os.path.join(R, "Last Seed 2026 1.0.0.zip"))
esl = zipfile.ZipFile(os.path.join(R, "Last Seed 2026 1.0.0 ESL.zip"))
files = lambda z: {n for n in z.namelist() if not n.endswith("/")}
md5 = lambda z, n: hashlib.md5(z.read(n)).hexdigest()
A, B = files(reg), files(esl)
assert not (A - B), A - B
SPECIAL = {"LastSeed.esp", "README.txt"}
shared = sorted((A & B) - SPECIAL)
assert all(md5(reg, n) == md5(esl, n) for n in shared)
esl_only = sorted(B - A)

config = """<config xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://qconsulting.ca/fo3/ModConfig5.0.xsd">
  <moduleName>Last Seed 2026</moduleName>
  <requiredInstallFiles>
    <folder source="common" destination=""/>
    <file source="README.txt" destination="Last Seed 2026 README.txt"/>
  </requiredInstallFiles>
  <installSteps order="Explicit">
    <installStep name="Build">
      <optionalFileGroups>
        <group name="Which Last Seed 2026 do you want?" type="SelectExactlyOne">
          <plugins order="Explicit">
            <plugin name="Regular">
              <description>For the regular Campfire (Nexus 667) and Frostfall 2026. Works with an existing save.</description>
              <conditionFlags><flag name="build">regular</flag></conditionFlags>
              <typeDescriptor><type name="Recommended"/></typeDescriptor>
            </plugin>
            <plugin name="ESL">
              <description>LastSeed.esp as a light (ESL) plugin, for the ESL builds of Campfire 2026 and Frostfall 2026. New game only. Never mix it with the regular build.</description>
              <conditionFlags><flag name="build">esl</flag></conditionFlags>
              <typeDescriptor><type name="Optional"/></typeDescriptor>
            </plugin>
          </plugins>
        </group>
      </optionalFileGroups>
    </installStep>
  </installSteps>
  <conditionalFileInstalls>
    <patterns>
      <pattern>
        <dependencies><flagDependency flag="build" value="regular"/></dependencies>
        <files><folder source="regular" destination=""/></files>
      </pattern>
      <pattern>
        <dependencies><flagDependency flag="build" value="esl"/></dependencies>
        <files><folder source="esl" destination=""/></files>
      </pattern>
    </patterns>
  </conditionalFileInstalls>
</config>
"""
info = (f"<fomod><Name>Last Seed 2026</Name><Author>CageTV (Last Seed by Chesko and Aytrus)</Author><Version>{version}</Version>"
        "<Website>https://github.com/CageTV/Last-Seed-2026</Website>"
        "<Description>Last Seed 2026, regular and ESL, in one installer.</Description></fomod>\n")
xml.dom.minidom.parseString(config)
xml.dom.minidom.parseString(info)

rr = reg.read("README.txt").decode("utf-8")
er = esl.read("README.txt").decode("utf-8")
esl_note = er.split("\n", 1)[1].strip()
readme = (f"Last Seed 2026 {version}\n\nOne installer for both builds: it asks Regular or ESL. The text below is the regular build's README, "
          "then the ESL build's README (differences only matter if you pick ESL).\n\n" + rr.split("\n", 1)[1].strip() +
          "\n\n\n=== ESL BUILD ===\n\n" + esl_note + "\n")

out = os.path.join(R, f"Last Seed 2026 {version}.zip")
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    z.writestr("fomod/info.xml", info)
    z.writestr("fomod/ModuleConfig.xml", config)
    z.writestr("README.txt", readme)
    for n in shared:
        z.writestr("common/" + n, reg.read(n))
    z.writestr("regular/LastSeed.esp", reg.read("LastSeed.esp"))
    z.writestr("esl/LastSeed.esp", esl.read("LastSeed.esp"))
    for n in esl_only:
        z.writestr("esl/" + n, esl.read(n))
# verify: what each choice installs equals the published zips
chk = zipfile.ZipFile(out)
for name, src, pre in (("regular", reg, "regular/"), ("esl", esl, "esl/")):
    got = {n[len("common/"):]: chk.read(n) for n in chk.namelist() if n.startswith("common/")}
    got.update({n[len(pre):]: chk.read(n) for n in chk.namelist() if n.startswith(pre)})
    want = {n: src.read(n) for n in files(src) - {"README.txt"}}
    assert got == want, name
print(f"wrote {out}: {len(shared)} shared, {len(esl_only) + 1} ESL, 1 regular plugin; both builds verified byte-identical to 1.0.0; {os.path.getsize(out) // 1024} KB")
