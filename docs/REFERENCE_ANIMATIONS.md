# Reference animation parity

Canonical source: `upstream/asundr-gwent-classic/` at
`08592d847f2573440a34deb9f32bc855b2544cdd`. Animation images are the original
files under `assets/gwent-classic/img/icons/anim_*.png`.

## Source behavior inventory

| Effect | Source implementation | Source timing / behavior | Port state |
| --- | --- | --- | --- |
| Card movement between containers | `translateTo()` → `translate()` in `gwent.js` | 499 ms transform; row spacing and source/destination geometry affect endpoint | Native card flight now uses 499 ms; endpoint geometry still needs source-layout parity |
| Ability overlays: Horn, Spy, Muster, Morale, Bond | `Card.animate()` in `gwent.js`; `abilities.js` calls it | 50 ms setup, 300 ms fade-in, 1000 ms hold, 300 ms fade-out; source `anim_<ability>.png` | Original PNG and timing are used. Newly summoned Muster copies and every matching Bond card now get their own overlay; movement/order timing is still not source-exact |
| Medic revive overlay | `abilities.js` Medic handler | `anim_medic` runs on the selected revived unit before it moves from graveyard to its combat row | Original PNG/timing now target the revived unit; the source graveyard-to-row travel is not yet reproduced |
| Scorch overlay | `Card.scorch()` in `gwent.js` | 50 ms setup; fade-in over 300 ms; fade-out starts 1300 ms after fade-in starts and runs 300 ms; cover sizing | Original PNG/timing now run concurrently on every unit moved to a graveyard, using pre-action row coordinates. Scorch card flight and row-transition sequencing remain incomplete |
| Weather row overlay | `Row.addOverlay()` / `removeOverlay()` in `gwent.js` | Fade in/out over 500 ms | Native alpha transition added and previous overlay type retained for fade-out; multiple simultaneous row overlay composition still needs comparison |
| Card preview fade | `style.css` | 300 ms opacity transition | Not ported |
| Five-card carousel movement | `style.css` and `ui.queueCarousel()` in `gwent.js` | 250 ms base; each adjacent/outer card is staggered by −50/+50/+200 ms; center card has a separate 50 ms glow transition | Not ported |
| Notifications | `ui.notification()` in `gwent.js` | 150 ms fade-in, 900 ms hold, 150 ms fade-out (1200 ms total) | Source `notif_*.png` art is shown in a queued center banner with matching timing; turn/round/pass/coin events are wired. Notification copy is localized; exact source text typography/layout still needs comparison |
| Round/game end screen | `gwent.js` | End screen fades in over 300 ms | Not ported |
| Other source CSS transitions | `style.css` | Includes 100 ms opacity, color, border, and shadow changes plus 250/500 ms layout transitions | Not ported comprehensively |

## Verification status

This file is deliberately an inventory, not a completion claim. The native
implementation reuses original animation PNGs and now covers more of the
source ability targets, card flight, weather fades, and timed notification
events. It does not yet reproduce source card movement for every affected card,
exact asynchronous sequencing, source container geometry, carousel, card
preview fade, leader/end-screen transitions, or all CSS selection/hover states.
Therefore the animation parity checkbox in
`docs/SOURCE_PARITY.md` remains unchecked.
