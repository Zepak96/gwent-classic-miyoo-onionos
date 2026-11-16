# Reference asset provenance

These files are copied from `asundr/gwent-classic` at commit
`08592d847f2573440a34deb9f32bc855b2544cdd`.

- `img/`: 531 files — the Gwent board, 216 small card images, 221 large card
  images, faction backs, interface icons, overlays, and notifications.
- `sfx/`: 35 MP3 sound effects included in the reference repository.
- `music/`: two MP3 background tracks supplied by the project owner: the Blood
  and Wine main-menu theme (`menu.mp3`) and the Gwent match theme
  (`match.mp3`). These are not part of the pinned reference repository.
- `svg/`: 6 interface icons.
- `img/icons/ui_*.png`: 6 transparent 256x256 PNG renderings of the six SVG
  icons, generated with CairoSVG 2.9.1. The source SVG files remain included;
  PNGs are used by the SDL renderer, which does not decode SVG at runtime.
  Regenerate them with `python tools/convert_svg_icons.py` after installing
  CairoSVG.

The reference README credits RandomPianist for many of its edited sound
effects. This attribution is preserved here. Its music is streamed from
YouTube by the reference browser game; the project owner supplied separate
Witcher 3 music tracks for this port.

This inventory records where the files came from. The pinned source repository
does not provide per-file media rights information; the source README's SFX
attribution above is preserved.
The pinned source README credits RandomPianist for many edited SFX but does not
identify per-file authors or a separate sound-effect license. See
[`docs/ASSET_RIGHTS.md`](../../docs/ASSET_RIGHTS.md) for the full provenance
and redistribution record; the upstream code license is not presented as a
blanket license for every included media file.

Upstream source: https://github.com/asundr/gwent-classic
Upstream README: https://github.com/asundr/gwent-classic/blob/08592d847f2573440a34deb9f32bc855b2544cdd/README.md
