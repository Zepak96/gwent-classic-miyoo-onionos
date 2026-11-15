#!/usr/bin/env python3
"""Rasterize the pinned reference SVG UI icons for the SDL image loader."""

import pathlib

import cairosvg

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "gwent-classic" / "svg"
OUTPUT = ROOT / "assets" / "gwent-classic" / "img" / "icons"
ICONS = ("exit", "music", "notification", "sfx", "trash", "view")

OUTPUT.mkdir(parents=True, exist_ok=True)
for icon in ICONS:
    cairosvg.svg2png(
        url=str(SOURCE / f"{icon}.svg"),
        write_to=str(OUTPUT / f"ui_{icon}.png"),
        output_width=256,
        output_height=256,
    )
