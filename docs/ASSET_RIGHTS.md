# Asset provenance and redistribution record

This file records what the repository itself establishes and distinguishes it
from permission reported privately by the project owner. It is not a legal
opinion. Bundling an asset in the reference project, or crediting its creator,
does not by itself document a separate redistribution grant for that asset.

| Material | Provenance | Evidence currently recorded | Redistribution status |
| --- | --- | --- | --- |
| Port code, card records, ability text | `asundr/gwent-classic`, pinned commit `08592d847f2573440a34deb9f32bc855b2544cdd` | Upstream `LICENSE` and attribution in [`third_party/gwent-classic/NOTICE.md`](../third_party/gwent-classic/NOTICE.md) | MIT with Commons Clause v1.0; preserve notices and the no-sale condition. |
| Card art, board art, UI art, SVG | Files copied from the pinned `asundr/gwent-classic` source | Inventory in [`assets/gwent-classic/ASSET_ORIGIN.md`](../assets/gwent-classic/ASSET_ORIGIN.md); the source repository includes a general software license | Project owner reports private CD PROJEKT RED authorization for CDPR game assets in this non-commercial Miyoo mod. The permission correspondence is not included here, and the source repository does not provide a per-file art license. |
| 35 gameplay/menu SFX | Included in the pinned source repository; its README credits RandomPianist for *many* edited effects | Source README attribution is preserved in [`assets/gwent-classic/ASSET_ORIGIN.md`](../assets/gwent-classic/ASSET_ORIGIN.md); the upstream license file is retained | The source does not map individual files to contributors or state a separate SFX license. Attribution is recorded, but an asset-specific redistribution grant is not substantiated by the repository. |
| `menu.mp3`, `match.mp3` | Supplied by the project owner | Names, intended use, and owner-supplied provenance are recorded in [`assets/gwent-classic/ASSET_ORIGIN.md`](../assets/gwent-classic/ASSET_ORIGIN.md) | The owner reports CDPR authorization for CDPR game assets. The repository does not include the correspondence or separate track-level rights documentation. |
| Six rasterized UI icons | Generated from the upstream SVG files with CairoSVG 2.9.1 | Reproducible generation process and source SVGs are documented in [`assets/gwent-classic/ASSET_ORIGIN.md`](../assets/gwent-classic/ASSET_ORIGIN.md) | Derivative of upstream UI art; follows the upstream asset status above. |

## Public release evidence

The repository preserves the upstream code notice and the sound-effect
attribution it found. The project owner has stated that CD PROJEKT RED gave
permission to use CDPR game assets for this non-commercial device mod. That is
an owner-reported permission; the private correspondence is not independently
reviewable from this repository. No separate license or permission record for
the RandomPianist-attributed sound effects was found in the pinned source
materials.

The public project must keep this distinction in its release notes and must
not describe all bundled media as covered by the source code license. This
record should be updated if written permission, a license, replacement media,
or per-file author data becomes available.
