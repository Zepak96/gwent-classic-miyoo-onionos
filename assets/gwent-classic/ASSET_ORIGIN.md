# Reference asset provenance

These files are copied from `asundr/gwent-classic` at commit
`08592d847f2573440a34deb9f32bc855b2544cdd`.

- `img/`: 531 files — the Gwent board, 216 small card images, 221 large card
  images, faction backs, interface icons, overlays, and notifications.
- `sfx/`: 35 MP3 sound effects included in the reference repository.
- `svg/`: 6 interface icons.
- `img/icons/ui_*.png`: 6 transparent 256x256 PNG renderings of the six SVG
  icons, generated with CairoSVG 2.9.1. The source SVG files remain included;
  PNGs are used by the SDL renderer, which does not decode SVG at runtime.
  Regenerate them with `python tools/convert_svg_icons.py` after installing
  CairoSVG.

The reference README credits RandomPianist for many of its edited sound
effects. This attribution is preserved here. Its music is streamed from
YouTube by the reference browser game; there are no music tracks in this
repository to port.

The project owner reports that a CD PROJEKT RED representative authorized
using CDPR game assets in this non-commercial Miyoo Mini Plus mod. The private
correspondence is not redistributed here. That report does not claim to grant
rights to third-party works; the SFX attribution above remains applicable.

Upstream source: https://github.com/asundr/gwent-classic
Upstream README: https://github.com/asundr/gwent-classic/blob/08592d847f2573440a34deb9f32bc855b2544cdd/README.md
