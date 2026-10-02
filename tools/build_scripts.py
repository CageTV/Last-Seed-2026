"""Compile Last Seed 2026's scripts in src/scripts with the official Papyrus compiler (from the Creation Kit).

Import order matters: our sources first, then Chesko's MIT shared sources (Campfire, CheskoPapyrusShared), the SkyUI SDK, SKSE,
then vanilla. Edit the paths below for your install. Output goes to build/scripts, and the .pex files are copied to
release-contents/Scripts. Usage: python tools/build_scripts.py
"""
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME = r"E:\Tabula Rasa\Stock Game"
MODS = r"E:\Tabula Rasa\mods"
FROSTFALL = r"E:\WorkSpace\Frostfall-2026-repo"       # https://github.com/CageTV/Frostfall-2026 (imports/chesko-shared, MIT)
CAMPFIRE = r"E:\WorkSpace\Frostfall-Modernized\upstream-chesko\Scripts\Source"  # https://github.com/chesko256/Campfire (MIT)
SKYUI = r"E:\WorkSpace\Frostfall-Modernized\imports\skyui"                      # SkyUI SDK sources (compile time only)

COMPILER = os.path.join(GAME, "Papyrus Compiler", "PapyrusCompiler.exe")
FLAGS = os.path.join(GAME, "Data", "Source", "Scripts", "TESV_Papyrus_Flags.flg")
SRC = os.path.join(HERE, "src", "scripts")
IMPORTS = [
    SRC,
    os.path.join(HERE, "imports", "stubs"),                                  # compile-only stubs for Last Seed's own types
    CAMPFIRE,
    os.path.join(FROSTFALL, "imports", "chesko-shared"),
    os.path.join(FROSTFALL, "imports", "stubs"),                             # compile-only stubs for optional third-party types
    SKYUI,
    os.path.join(MODS, "Skyrim Script Extender (SKSE64)", "Scripts", "Source"),
    os.path.join(MODS, "PapyrusUtil SE - Modders Scripting Utility Functions", "Scripts", "Source"),
    os.path.join(MODS, "powerofthree's Papyrus Extender", "Source", "scripts"),
    os.path.join(MODS, "FileAccess Interface for Skyrim SE Scripts - FISSES", "scripts", "source"),
    os.path.join(GAME, "Data", "Source", "Scripts"),                         # vanilla
]


def main():
    out = os.path.join(HERE, "build", "scripts")
    os.makedirs(out, exist_ok=True)
    for p in [COMPILER, FLAGS] + IMPORTS:
        if not os.path.exists(p):
            raise SystemExit(f"missing: {p}")
    log, rc = "", 0
    for name in sorted(n for n in os.listdir(SRC) if n.lower().endswith(".psc")):
        cmd = [COMPILER, name, f"-f={FLAGS}", "-i=" + ";".join(IMPORTS), f"-o={out}"]
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, cwd=SRC, timeout=120)
            log += (r.stdout or "") + (r.stderr or "")
            rc = rc or r.returncode
        except subprocess.TimeoutExpired:
            subprocess.run(["taskkill", "/F", "/IM", "PapyrusCompiler.exe"], capture_output=True)
            log += f"{name}: compilation failed (compiler hang, killed after 120 s)\n"
            rc = 1
    with open(os.path.join(HERE, "build", "compile.log"), "w", encoding="utf-8") as f:
        f.write(log)
    errors = [l for l in log.splitlines() if ".psc(" in l or "compilation failed" in l.lower()]
    pexs = [n for n in os.listdir(out) if n.lower().endswith(".pex")]
    print(f"compiler exit {rc}: {len(pexs)} .pex; {len(errors)} error lines (full log: build/compile.log)")
    for l in errors[:25]:
        print("  " + l)
    if rc == 0 and not errors:
        dest = os.path.join(HERE, "release-contents", "Scripts")
        os.makedirs(os.path.join(dest, "Source"), exist_ok=True)
        for n in pexs:
            shutil.copy2(os.path.join(out, n), os.path.join(dest, n))
        for n in os.listdir(SRC):
            if n.lower().endswith(".psc"):
                shutil.copy2(os.path.join(SRC, n), os.path.join(dest, "Source", n))
        print("copied to release-contents/Scripts")
    sys.exit(0 if rc == 0 and not errors else 1)


if __name__ == "__main__":
    main()
