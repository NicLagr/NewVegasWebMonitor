#!/usr/bin/env python3
"""
Convert the Consistent Pip-Boy Icons DDS textures (staged in icon-src/) into
downscaled WebP under public/icons/pipboy/, preserving each icon's in-game
texture path (everything after the `textures/` segment, lowercased).

A Fallout: New Vegas item form stores its inventory icon as a path relative to
Textures/, e.g. "Interface\\Icons\\PipBoyImages\\Weapons\\weapons_katana.dds".
The plugin reports that path per item; the app loads the matching
  public/icons/pipboy/interface/icons/pipboyimages/weapons/weapons_katana.webp

Icons by ItsMeJesusHChrist (Consistent Pip-Boy Icons), used with permission
(credit required). Raw DDS in icon-src/ is gitignored; only the WebP we ship
are committed.

Usage:  python3 scripts/convert-dds-icons.py
Requires: Pillow  (pip install Pillow)
"""
import os
import sys
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC_DIRS = ["Weapons", "Apparel", "Items", "DLC's Items"]
OUT_BASE = os.path.join(ROOT, "public", "icons", "pipboy")
SIZE = 128  # icons render at <=48px; 128 covers hi-dpi
QUALITY = 82


def rel_after_textures(path: str):
    """Return the path relative to the `textures/` segment, lowercased,
    forward slashes — i.e. the in-game icon path. None if no textures segment."""
    parts = path.replace("\\", "/").split("/")
    lower = [p.lower() for p in parts]
    if "textures" not in lower:
        return None
    i = lower.index("textures")
    return "/".join(lower[i + 1:])


def main():
    total = 0
    manifest = []
    for variant in SRC_DIRS:
        base = os.path.join(ROOT, "icon-src", variant)
        if not os.path.isdir(base):
            print(f"  (skip missing {variant})")
            continue
        for dirpath, _dirs, files in os.walk(base):
            for fn in files:
                if not fn.lower().endswith(".dds"):
                    continue
                src = os.path.join(dirpath, fn)
                rel = rel_after_textures(src)
                if not rel:
                    continue
                out = os.path.join(OUT_BASE, rel[:-4] + ".webp")  # .dds -> .webp
                os.makedirs(os.path.dirname(out), exist_ok=True)
                try:
                    im = Image.open(src).convert("RGBA")
                    if im.width != SIZE or im.height != SIZE:
                        im = im.resize((SIZE, SIZE), Image.LANCZOS)
                    im.save(out, "WEBP", quality=QUALITY, method=6)
                    manifest.append(rel[:-4])  # relative path, no extension
                    total += 1
                except Exception as e:  # noqa: BLE001
                    print(f"  !! {src}: {e}", file=sys.stderr)
    # Manifest of available icon paths, so the app can fall back to type icons
    # for items whose icon path we didn't convert (e.g. modded items).
    import json
    manifest = sorted(set(manifest))
    with open(os.path.join(OUT_BASE, "manifest.json"), "w") as fh:
        json.dump(manifest, fh)
    print(f"Converted {total} icons ({len(manifest)} in manifest) -> {os.path.relpath(OUT_BASE, ROOT)}")


if __name__ == "__main__":
    main()
