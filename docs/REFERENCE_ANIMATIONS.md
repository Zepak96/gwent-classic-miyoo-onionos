# Reference animation parity

Canonical source: `upstream/asundr-gwent-classic/` at
`08592d847f2573440a34deb9f32bc855b2544cdd`. Animation images are the original
files under `assets/gwent-classic/img/icons/anim_*.png`.

## Source behavior inventory

| Effect | Source implementation | Source timing / behavior | Port state |
| --- | --- | --- | --- |
| Card movement between containers | `translateTo()` → `translate()` in `gwent.js` | 499 ms transform; row spacing and source/destination geometry affect endpoint | Native card flight now uses 499 ms; endpoint geometry still needs source-layout parity |
| Ability overlays: Horn, Spy, Muster, Morale, Bond | `Card.animate()` in `gwent.js`; `abilities.js` calls it | 50 ms setup, 300 ms fade-in, 1000 ms hold, 300 ms fade-out; source `anim_<ability>.png` | Native overlay uses original PNG and nominal timing on the played card; Bond must animate every matching row card and Muster needs its source ordering |
| Medic revive overlay | `abilities.js` Medic handler | `anim_medic` runs on the selected revived unit before it moves from graveyard to its combat row | Not ported; deliberately not attached to the played Medic card because that would animate the wrong card |
| Scorch overlay | `Card.scorch()` in `gwent.js` | 50 ms setup; fade-in over 300 ms; fade-out starts 1300 ms after fade-in starts and runs 300 ms; cover sizing | Not ported; engine removes all targets immediately, so the UI must preserve target positions before mutation |
| Weather row overlay | `Row.addOverlay()` / `removeOverlay()` in `gwent.js` | Fade in/out over 500 ms | Native alpha transition added and previous overlay type retained for fade-out; multiple simultaneous row overlay composition still needs comparison |
| Card preview fade | `style.css` | 300 ms opacity transition | Not ported |
| Five-card carousel movement | `style.css` and `ui.queueCarousel()` in `gwent.js` | 250 ms base; each adjacent/outer card is staggered by −50/+50/+200 ms; center card has a separate 50 ms glow transition | Not ported |
| Notifications | `ui.notification()` in `gwent.js` | 150 ms fade-in, timed hold (requested 1200 ms for turn/round notices), 150 ms fade-out | Not ported as timed source notifications |
| Round/game end screen | `gwent.js` | End screen fades in over 300 ms | Not ported |
| Other source CSS transitions | `style.css` | Includes 100 ms opacity, color, border, and shadow changes plus 250/500 ms layout transitions | Not ported comprehensively |

## Verification status

This file is deliberately an inventory, not a completion claim. The native
implementation currently reuses the source card animation PNGs and ports a
subset of their timing. It does not yet reproduce every ability's affected
cards, ordering, source container geometry, carousel, timed notifications,
leader/end-screen transitions, or all CSS selection/hover states. Therefore
the animation parity checkbox in `docs/SOURCE_PARITY.md` remains unchecked.
