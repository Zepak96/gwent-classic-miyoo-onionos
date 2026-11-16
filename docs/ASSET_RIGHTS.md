# Asset provenance and redistribution record

This file records the provenance and licensing terms that can be verified from
the checked-in source materials. It is not a legal opinion. Bundling an asset
in the reference project, or crediting its creator, does not by itself
establish a separate redistribution license for that asset.

| Material | Provenance | Evidence currently recorded | Redistribution status |
| --- | --- | --- | --- |
| Port code, card records, ability text | `asundr/gwent-classic`, pinned commit `08592d847f2573440a34deb9f32bc855b2544cdd` | Upstream `LICENSE` and attribution in [`third_party/gwent-classic/NOTICE.md`](../third_party/gwent-classic/NOTICE.md) | MIT with Commons Clause v1.0; preserve notices and the no-sale condition. |
| Card art, board art, UI art, SVG | Files copied from the pinned `asundr/gwent-classic` source | Inventory in [`assets/gwent-classic/ASSET_ORIGIN.md`](../assets/gwent-classic/ASSET_ORIGIN.md); the source repository includes a general software license | The pinned source does not specify per-file media licensing terms. The source license is not treated as a blanket art license. |
| 35 gameplay/menu SFX | Included in the pinned source repository; its README credits RandomPianist for *many* edited effects | Source README attribution is preserved in [`assets/gwent-classic/ASSET_ORIGIN.md`](../assets/gwent-classic/ASSET_ORIGIN.md); the upstream license file is retained | The source does not map individual files to contributors or state a separate SFX license. Attribution is recorded; the repository does not specify asset-level redistribution terms. |
| `menu.mp3`, `match.mp3` | Supplied by the project owner | Names, intended use, and owner-supplied provenance are recorded in [`assets/gwent-classic/ASSET_ORIGIN.md`](../assets/gwent-classic/ASSET_ORIGIN.md) | Track-level licensing terms are not recorded in this repository. |
| Six rasterized UI icons | Generated from the upstream SVG files with CairoSVG 2.9.1 | Reproducible generation process and source SVGs are documented in [`assets/gwent-classic/ASSET_ORIGIN.md`](../assets/gwent-classic/ASSET_ORIGIN.md) | Derivative of upstream UI art; follows the upstream asset status above. |

## Public release evidence

The repository preserves the upstream code notice and the sound-effect
attribution it found. The pinned source materials do not provide per-file
media licensing terms for the bundled sound effects and artwork.

The public project must not describe all bundled media as covered by the
source code license. This record should be updated when verifiable per-file
media licensing terms or author data become available.
