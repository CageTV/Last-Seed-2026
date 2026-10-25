"""Builds ONE FOMOD installer for Last Seed 2026 that covers the regular and the ESL build, with a second choice for the SKSE library the DLL is built on
(release/Last Seed 2026 <version>.zip).

Step 1 picks Regular or ESL (LastSeed.esp differs, and the ESL build adds six rebuilt scripts). Step 2 picks the DLL: "new" (alandtse's
CommonLibSSE-NG, Skyrim 1.6.1170 and newer) or "older" (CharmedBaryon's, Skyrim VR and 1.6.1130 and older). The logo, icons and the nine scripts
that are identical are installed always. Everything except the DLL and the scripts in CHANGED is taken from the published 1.0.0 regular and ESL zips,
so it is byte-identical.
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
DLL_NAME = "SKSE/Plugins/LastSeed.dll"
DLLS = {"new": os.path.join(HERE, "..", "ng-build", "build", "release", "out", "LastSeed", "LastSeed.dll"),
        "older": os.path.join(HERE, "plugin", "build", "release", "LastSeed.dll")}
for k, d in DLLS.items():
    assert os.path.isfile(d), f"missing {k} DLL: {d}"
NEW_VER = "1.6.1170.0"
reg = zipfile.ZipFile(os.path.join(R, "Last Seed 2026 1.0.0.zip"))
esl = zipfile.ZipFile(os.path.join(R, "Last Seed 2026 1.0.0 ESL.zip"))
files = lambda z: {n for n in z.namelist() if not n.endswith("/")}
md5 = lambda z, n: hashlib.md5(z.read(n)).hexdigest()
A, B = files(reg), files(esl)
assert not (A - B), A - B
SPECIAL = {"LastSeed.esp", "README.txt", DLL_NAME}
shared = sorted((A & B) - SPECIAL)
assert all(md5(reg, n) == md5(esl, n) for n in shared)

# Scripts changed since 1.0.0: published path -> {where: source file}. "common" goes to both builds; "regular" / "esl" to that build only
# (the ESL build's copy has Last Seed's FormIDs renumbered by tools/esl_build.py). Run tools/build_scripts.py and tools/esl_build.py first.
RC = os.path.join(HERE, "release-contents", "Scripts")
EW = os.path.join(HERE, "esl-work")
CHANGED = {
    # 1.1.3: a Campfire tent or a bedroll by the fire is a safe place to sleep (no FormIDs: the same in both builds)
    "Scripts/_Seed_FatigueSystem.pex": {"common": os.path.join(RC, "_Seed_FatigueSystem.pex")},
    "Scripts/Source/_Seed_FatigueSystem.psc": {"common": os.path.join(RC, "Source", "_Seed_FatigueSystem.psc")},
    # 1.2.0: CACO's food is imported without the Add Now / Add Later question
    "Scripts/_Seed_Compatibility.pex": {"regular": os.path.join(RC, "_Seed_Compatibility.pex"), "esl": os.path.join(EW, "pex", "_Seed_Compatibility.pex")},
    "Scripts/Source/_Seed_Compatibility.psc": {"regular": os.path.join(RC, "Source", "_Seed_Compatibility.psc"),
                                               "esl": os.path.join(EW, "scripts-src", "_Seed_Compatibility.psc")},
}
for n, srcs in CHANGED.items():
    for where, p in srcs.items():
        assert os.path.isfile(p), f"missing {where} {n}: {p}"
shared = [n for n in shared if n not in CHANGED]
esl_only = sorted(B - A)

config = f"""<config xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://qconsulting.ca/fo3/ModConfig5.0.xsd">
  <moduleName>Last Seed 2026</moduleName>
  <requiredInstallFiles>
    <folder source="common" destination=""/>
    <file source="README.txt" destination="Last Seed 2026 README.txt"/>
    <file source="LICENSE.txt" destination="Last Seed 2026 LICENSE.txt"/>
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
    <installStep name="Game version">
      <optionalFileGroups order="Explicit">
        <group name="Which LastSeed.dll do you want?" type="SelectExactlyOne">
          <plugins order="Explicit">
            <plugin name="Skyrim 1.6.1170 and newer (SE / AE)">
              <description>The new build. Works on Skyrim SE/AE 1.6.1170 and on every later version, including 1.7.x. Pre-selected when the installer sees a game version of 1.6.1170 or newer.</description>
              <conditionFlags><flag name="dll">new</flag></conditionFlags>
              <typeDescriptor><dependencyType><defaultType name="Optional"/><patterns>
                <pattern><dependencies><gameDependency version="{NEW_VER}"/></dependencies><type name="Recommended"/></pattern>
              </patterns></dependencyType></typeDescriptor>
            </plugin>
            <plugin name="Skyrim VR, or 1.6.1130 and older">
              <description>The older build, for Skyrim VR and for SE/AE 1.6.1130 and older. Pre-selected when the installer sees an older game version or Skyrim VR. On 1.6.1170 either build works.</description>
              <conditionFlags><flag name="dll">older</flag></conditionFlags>
              <typeDescriptor><dependencyType><defaultType name="Recommended"/><patterns>
                <pattern><dependencies><gameDependency version="{NEW_VER}"/></dependencies><type name="Optional"/></pattern>
              </patterns></dependencyType></typeDescriptor>
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
      <pattern>
        <dependencies><flagDependency flag="dll" value="new"/></dependencies>
        <files><folder source="new" destination=""/></files>
      </pattern>
      <pattern>
        <dependencies><flagDependency flag="dll" value="older"/></dependencies>
        <files><folder source="older" destination=""/></files>
      </pattern>
    </patterns>
  </conditionalFileInstalls>
</config>
"""
info = (f"<fomod><Name>Last Seed 2026</Name><Author>CageTV (Last Seed by Chesko and Aytrus)</Author><Version>{version}</Version>"
        "<Website>https://github.com/CageTV/Last-Seed-2026</Website>"
        "<Description>Last Seed 2026, regular and ESL, with a choice of LastSeed.dll for your game version, in one installer.</Description></fomod>\n")
xml.dom.minidom.parseString(config)
xml.dom.minidom.parseString(info)

rr = reg.read("README.txt").decode("utf-8")
er = esl.read("README.txt").decode("utf-8")
esl_note = er.split("\n", 1)[1].strip()
readme = (f"Last Seed 2026 {version}\n\nOne installer for both builds: it asks Regular or ESL, then which LastSeed.dll (new: Skyrim 1.6.1170 and newer; older: Skyrim VR or 1.6.1130 and older; both work on 1.6.1170). New in 1.2.0: foods of other mods that Last Seed does not know are classified automatically when the game starts, instead of asking you about each one (switch: Compatibility on the settings page); items that only look like food (CACO's hunger-status tokens) are marked as not food; CACO's food is imported without the Add Now / Add Later question; a Campfire tent or a bedroll by the fire counts as a safe place to sleep. New in 1.1.2: support for Disease Widget - SKSE (when its DLL is installed, Last Seed adds its diseases and their stages to the widget's list, so they get the right icons). New in 1.1.1: an Interface Settings page (Focus and other notifications, eating and drinking animations, effects). The text below is the regular build's README, "
          "then the ESL build's README (differences only matter if you pick ESL).\n\n" + rr.split("\n", 1)[1].strip() +
          "\n\n\n=== ESL BUILD ===\n\n" + esl_note + "\n")

out = os.path.join(R, f"Last Seed 2026 {version}.zip")
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    z.writestr("fomod/info.xml", info)
    z.writestr("fomod/ModuleConfig.xml", config)
    z.writestr("README.txt", readme)
    z.write(os.path.join(HERE, "LICENSE.txt"), "LICENSE.txt")
    for k, d in DLLS.items():
        z.write(d, f"{k}/{DLL_NAME}")
    for n in shared:
        z.writestr("common/" + n, reg.read(n))
    z.writestr("regular/LastSeed.esp", reg.read("LastSeed.esp"))
    z.writestr("esl/LastSeed.esp", esl.read("LastSeed.esp"))
    for n in esl_only:
        if n not in CHANGED:
            z.writestr("esl/" + n, esl.read(n))
    for n, srcs in CHANGED.items():
        for where, p in srcs.items():
            z.write(p, f"{where}/{n}")
# verify: what each choice installs equals the published zips, apart from the DLL and the changed scripts (which must be the files listed)
chk = zipfile.ZipFile(out)
for name, src, pre in (("regular", reg, "regular/"), ("esl", esl, "esl/")):
    got = {n[len("common/"):]: chk.read(n) for n in chk.namelist() if n.startswith("common/")}
    got.update({n[len(pre):]: chk.read(n) for n in chk.namelist() if n.startswith(pre)})
    want = {n: src.read(n) for n in files(src) - {"README.txt", DLL_NAME} - set(CHANGED)}
    for n, srcs in CHANGED.items():
        p = srcs.get("common") or srcs.get(name)
        if p:
            want[n] = open(p, "rb").read()
    assert got == want, (name, sorted(set(got) ^ set(want)), sorted(k for k in got.keys() & want.keys() if got[k] != want[k]))
print(f"wrote {out}: {len(shared)} shared, {len(esl_only) + 1} ESL, 1 regular plugin, {len(CHANGED)} changed scripts; both builds verified against 1.0.0 "
      f"apart from the DLL and those scripts; {os.path.getsize(out) // 1024} KB")
