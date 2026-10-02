"""Draw LastSeed.dll's HUD icons: white glyphs on transparent 64x64 PNGs (supersampled 8x for smooth edges).

Same recipe as Frostfall 2026's tools/make_icons.py so the two icon sets look alike.

Usage: python tools/make_icons.py  -> assets/icons/*.png, assets/icons/preview*.png and
       release-contents/Interface/lastseed/icons/*.png
"""
import math
import os
import shutil

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(HERE, "assets", "icons")
RELEASE = os.path.join(HERE, "release-contents", "Interface", "lastseed", "icons")
S = 64
K = 8
N = S * K
W = (255, 255, 255, 255)
CLEAR = (0, 0, 0, 0)


def canvas():
    im = Image.new("RGBA", (N, N), CLEAR)
    return im, ImageDraw.Draw(im)


def finish(im, name):
    im = im.resize((S, S), Image.LANCZOS)
    im.save(os.path.join(OUT, name))
    return im


def line(d, a, b, w):
    d.line([a, b], fill=W, width=w)
    r = w / 2
    for p in (a, b):
        d.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=W)


def drumstick():
    """Hunger: a roast drumstick - plump meat tapering into a short, thick bone with two knobs."""
    im, d = canvas()
    # meat: a big circle (upper right) that tapers down-left into the bone, drawn as circle + tangent polygon
    mx, my, r = N * 0.60, N * 0.38, N * 0.30
    d.ellipse([mx - r, my - r, mx + r, my + r], fill=W)
    tip = (N * 0.30, N * 0.70)  # where meat meets bone
    dx, dy = tip[0] - mx, tip[1] - my
    dist = math.hypot(dx, dy)
    base = math.atan2(dy, dx)
    off = math.acos(r / dist)
    p1 = (mx + r * math.cos(base + off), my + r * math.sin(base + off))
    p2 = (mx + r * math.cos(base - off), my + r * math.sin(base - off))
    d.polygon([tip, p1, p2], fill=W)
    # bone: short and thick, with two knobs at the end
    end = (N * 0.17, N * 0.83)
    line(d, tip, end, int(N * 0.12))
    kr = N * 0.095
    for ddx, ddy in ((-0.055, 0.045), (0.045, 0.075)):
        cx, cy = end[0] + N * ddx, end[1] + N * ddy
        d.ellipse([cx - kr, cy - kr, cx + kr, cy + kr], fill=W)
    # a bite-mark highlight so the meat does not read as a flat disc
    hr = r * 0.62
    d.arc([mx - hr, my - hr, mx + hr, my + hr], 205, 275, fill=CLEAR, width=int(N * 0.055))
    return finish(im, "hunger.png")


def waterskin():
    """Thirst: a waterskin - plump body, wide neck and a stopper, with a band across the body."""
    im, d = canvas()
    cx = N * 0.5
    d.ellipse([cx - N * 0.32, N * 0.34, cx + N * 0.32, N * 0.94], fill=W)  # body
    d.rounded_rectangle([cx - N * 0.13, N * 0.16, cx + N * 0.13, N * 0.48], radius=N * 0.04, fill=W)  # neck
    d.rounded_rectangle([cx - N * 0.18, N * 0.06, cx + N * 0.18, N * 0.20], radius=N * 0.05, fill=W)  # stopper
    d.rectangle([cx - N * 0.34, N * 0.58, cx + N * 0.34, N * 0.64], fill=CLEAR)  # band
    return finish(im, "thirst.png")


def moon():
    """Fatigue: a crescent moon with a small star."""
    im, d = canvas()
    cx, cy, r = N * 0.46, N * 0.52, N * 0.38
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=W)
    off = N * 0.17
    d.ellipse([cx - r + off + N * 0.06, cy - r - off * 0.4, cx + r + off + N * 0.06, cy + r - off * 0.4], fill=CLEAR)
    # star
    sx, sy, sr = N * 0.80, N * 0.22, N * 0.10
    pts = []
    for i in range(10):
        ang = math.radians(-90 + i * 36)
        rad = sr if i % 2 == 0 else sr * 0.42
        pts.append((sx + rad * math.cos(ang), sy + rad * math.sin(ang)))
    d.polygon(pts, fill=W)
    return finish(im, "fatigue.png")


def heart():
    """Vitality: a heart."""
    im, d = canvas()
    r = N * 0.215
    cy = N * 0.36
    lx, rx = N * 0.5 - r * 0.93, N * 0.5 + r * 0.93
    d.ellipse([lx - r, cy - r, lx + r, cy + r], fill=W)
    d.ellipse([rx - r, cy - r, rx + r, cy + r], fill=W)
    # the point: tangent-ish triangle from the lobes down to the tip
    d.polygon([(lx - r * 0.97, cy + r * 0.25), (rx + r * 0.97, cy + r * 0.25), (N * 0.5, N * 0.90)], fill=W)
    return finish(im, "vitality.png")


def main():
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(RELEASE, exist_ok=True)
    icons = [drumstick(), waterskin(), moon(), heart()]
    pv = Image.new("RGBA", (S * 4 * 3 + 50, S * 3 + 20), (110, 118, 128, 255))
    for i, ic in enumerate(icons):
        big = ic.resize((S * 3, S * 3), Image.LANCZOS)
        pv.alpha_composite(big, (10 + i * (S * 3 + 10), 10))
    pv.save(os.path.join(OUT, "preview.png"))
    small = Image.new("RGBA", (200, 40), (110, 118, 128, 255))
    for i, ic in enumerate(icons):
        small.alpha_composite(ic.resize((18, 18), Image.LANCZOS), (10 + i * 45, 11))
    small = small.resize((800, 160), Image.NEAREST)
    small.save(os.path.join(OUT, "preview_ingame_size.png"))
    for name in ("hunger.png", "thirst.png", "fatigue.png", "vitality.png"):
        shutil.copy(os.path.join(OUT, name), os.path.join(RELEASE, name))
    print("wrote", OUT, "and", RELEASE)


if __name__ == "__main__":
    main()
