# Reference animation parity

Canonical source: `upstream/asundr-gwent-classic/` at
`08592d847f2573440a34deb9f32bc855b2544cdd`. Animation images are the original
files under `assets/gwent-classic/img/icons/anim_*.png`.

## Source behavior inventory

| Effect | Source implementation | Source timing / behavior | Port state |
| --- | --- | --- | --- |
| Played-card preview, flight and placement sequence | `Player.playCardAction()` / `Row.addCard()` / `Row.playPlacementAudio()` in `gwent.js` | Show the chosen card for 1000 ms; move it for 499 ms; ordinary unit placement audio then waits 1000 ms before placed callbacks | The native view keeps the played card in its preview during the 1000 ms selection preview, starts its 499 ms flight afterward, schedules the row cue on arrival, and starts standard row ability overlays after the source audio wait. Player input and the next AI action stay locked through scheduled card/effect flights and the row's trailing wait. Spy and Berserker-under-Mardroeme skip the placement audio wait. The rules engine and turn state still resolve immediately, so this remains a visual/audio timeline approximation, not fully sequential source execution |
| Card movement between containers | `translateTo()` → `translate()` in `gwent.js`; `.card` in `style.css` | 499 ms transform with CSS `ease`; row spacing and source/destination geometry affect endpoint | Native flights use the 499 ms cubic-bezier curve. A game-state diff queues all cards moved by an action, including concurrent Muster summons, Scorch removals, Medic revivals, Spy draws, Decoy swaps, and weather-to-grave moves. Follow-on flights now wait for the corresponding source ability animation: Spy draw; Medic/Muster target sequence; and Scorch target sequence. Original source/destination positions and full asynchronous turn ordering are not yet reproduced exactly |
| Row card insertion/removal spacing | `CardContainer.resizeCardContainer()` / `Row.resize()` in `gwent.js`; `.card` in `style.css` | Cards are centered; per-card side margins update as the row count changes and transition over 250 ms with CSS `ease` | Source row width, centering, overlap formula and 250 ms eased spacing transition are ported; source row/card geometry still needs side-by-side verification |
| Ability overlays: Horn, Spy, Muster, Morale, Bond | `Card.animate()` in `gwent.js`; `abilities.js` calls it | 50 ms setup, 300 ms fade-in, 1000 ms hold, 300 ms fade-out; background-size transitions use CSS `ease` over 250 ms from 40→100%, 100→80%, then 80→40% | Original PNG, fade timing, CSS ease curve and 250 ms size transitions are ported. Muster's overlay is limited to the played card, as in source; matching Bond cards animate concurrently. Preview/flight/placement ordering is scheduled, while immediate rules-state resolution remains a known difference |
| Medic revive overlay | `abilities.js` Medic handler | `anim_medic` runs on the selected revived unit before its 499 ms graveyard-to-row travel | Original PNG/timing runs on the revived unit; the movement queue now holds the card at its graveyard position for the overlay sequence, then flies it to the selected row |
| Scorch overlay | `Card.scorch()` in `gwent.js` | 50 ms setup; fade-in over 300 ms; hold until 1300 ms after fade-in starts; 300 ms fade-out with cover sizing; affected cards move to the grave afterward | Original PNG is shown on the played Scorch card, followed by target overlays and concurrent target flights to the grave. Row-specific Scorch overlays run before their target flights. Preview and all asynchronous event timings still need side-by-side verification |
| Weather row overlay | `Row.addOverlay()` / `removeOverlay()` in `gwent.js`; `.frost`, `.fog`, `.rain` in `style.css` | Fade in/out over 500 ms; Biting Frost artwork belongs to melee, Fog to ranged, Rain to siege; Skellige Storm applies Fog and Rain to its affected rows | Native alpha transition is 500 ms and the overlay now follows the affected row, so a Fog/Rain card cannot leak its artwork onto unrelated rows. Exact timing has SDL regression coverage; multiple simultaneous row overlay composition still needs comparison |
| Exit-game control hover | `style.css` | 300 ms opacity transition from 20% to 100% | Not ported; the CSS transition is on `#exit-game`, not the card preview |
| Five-card carousel movement | `style.css` and `ui.queueCarousel()` in `gwent.js` | 250 ms base; each adjacent/outer card is staggered by −50/+50/+200 ms; center glow transitions over 100 ms; nudge uses CSS `ease-in` | Five-card source layout is used for target, leader and Skellige revival choices; directional input gets staggered 200/300/450 ms `ease-in` movement and a 100 ms center highlight pulse. Exact DOM hover behavior is mapped to Miyoo directional input |
| Notifications | `ui.notification()` in `gwent.js` | 150 ms fade-in, 900 ms hold, 150 ms fade-out (1200 ms total) | Source `notif_*.png` art is shown in a queued center banner with matching timing; turn/round/pass/coin events are wired. Notification copy is localized; exact source text typography/layout still needs comparison |
| Round/game end screen | `gwent.js` `showEndScreen()` | Result overlay fades from opacity 0 to 1 over 300 ms | Timed 300 ms alpha fade added to result shade, source result art and labels; starts on result phase and resumes from saved result state. Exact source result composition still needs comparison |
| Other source CSS transitions | `style.css` | Includes 100 ms opacity, color, border, and shadow changes plus 250/500 ms layout transitions | Not ported comprehensively |

## Verification status

This file is deliberately an inventory, not a completion claim. The native
implementation reuses original animation PNGs and now covers more of the
source ability targets and scale easing, scheduled preview/flight/placement
timing, weather fades, timed notification events, and the result-screen fade.
The game state still resolves actions immediately rather than awaiting the full
source animation chain. It also does not reproduce all source container
geometry, carousel, exit-game hover fade, leader transitions, or all CSS
selection/hover states.
Therefore the animation parity checkbox in
`docs/SOURCE_PARITY.md` remains unchecked.
