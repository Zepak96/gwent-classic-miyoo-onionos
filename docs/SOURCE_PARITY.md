# Gwent Classic source parity audit

This audit uses the unchanged files in `upstream/asundr-gwent-classic/` at
commit `08592d847f2573440a34deb9f32bc855b2544cdd` as the authority. A checked
item requires a native implementation and a repeatable comparison against the
source behavior. A build or screenshot alone does not count as parity.

## Match lifecycle — `Game`, `Player`, `Deck`, `Board`

- [ ] New game, random faction premade opponent, random legal faction leader.
- [x] Ten-card initial hands; source redraw order and at most two redraws.
- [x] First-player coin toss and Scoia'tael's first-player choice. Engine
  regressions cover random first player, player choice, opponent-Scoia random
  choice, and the source behavior when both factions are Scoia'tael. The SDL
  input test now drives the player's left/right choice and confirms it.
- [x] One action per turn; the source `Player.endTurn()` auto-passes only when
  both the hand and Leader action are exhausted. Native engine regressions cover
  exhausted starts, exhausted final action, and Leader as a remaining action.
- [x] Voluntary pass marks the player passed and leaves the opponent acting
  until they pass; native engine regressions cover both passes ending a round.
  Miyoo X hold passes directly after 600 ms; SDL input probes cover hold,
  threshold, and short press behavior.
- [x] Round results, gem losses, score ties, Nilfgaard tie-break, and match
  victory/defeat/draw. Engine regressions cover round wins/losses, a draw on the
  last gem, and a match draw. SDL result-screen checks cover round win/loss/draw,
  Nilfgaard's tie-break, and the drawn-match screen. Rematch behavior remains
  open.
- [x] Round cleanup, Northern Realms draw, Monsters survivor, and Skellige
  round-three graveyard selection/Agile row selection. Regressions verify the
  Northern Realms winner draws, a Monsters player keeps one non-Hero Unit,
  Skellige revives at most two units on round three, AI Agile row selection,
  player Agile row selection, and cleanup interactions with Avenger/Kambi.
- [x] An active Leader ability is available once per match and does not reset
  between rounds, matching the source `disableLeader()` lifecycle; an engine
  regression verifies it remains spent after round two starts.
- [x] White Flame cancels both active Leader abilities and suppresses Invader,
  Treacherous, Daisy and King Bran passives at match setup. Engine regressions
  run White Flame on both player sides against the active leader and each
  passive leader.

## Card rules — `abilities.js`, `Row`, `Weather`, `Card`

- [x] Base score, Hero immunity, weather floor/half-weather, horn and
  Dandelion. `scoreRules()` checks weather floor, horn, Hero immunity,
  Dandelion's row horn and Morale; King Bran regressions check floor-half
  rounding (including odd power) and holder-only scope.
- [x] Tight Bond is counted by printed name, multiplies each matching card by
  the number of Bond-bearing copies, and Morale boosts other row units.
  Regressions cover one and three copies, same name across different metadata
  groups, different names sharing metadata, and non-Bond same-name cards.
- [x] Spy placement and up-to-two draws; Medic selection, random Emhyr revive,
  Hero/noncombat rejection and Agile row choice. Engine tests cover explicit
  targets and RNG-preserving target preview; SDL tests cover Agile selection.
- [x] Decoy uses the specifically selected own non-Hero combat unit, returns
  that same card, and leaves the Decoy in the selected row. Engine regressions
  cover exact target identity, returned hand card, row replacement, and rejection
  of noncombat or opponent-owned targets; Cow removal also exercises Avenger.
- [x] Muster pulls matching cards from hand and deck using the source name
  prefix. Regressions cover Arachas hand/deck copies and next-gen Geralt/Ciri
  summoning Roach while leaving the other hero in hand.
- [x] Berserker/Mardroeme transformations and Vildkaarl gameplay lifecycle.
  Engine regressions cover both transformed card variants, Mardroeme and Ermion
  triggers, Berserkers played after Mardroeme, row ordering, occupied special
  slots, and Mardroeme reset at the next round. The source's one-shot Vildkaarl
  audio marker is handled by the frontend; exact animation/audio timing remains
  under the separate animation audit.
- [x] Berserker/Mardroeme transformation ordering now re-sorts the transformed
  Vildkaarl by the source row comparator (base power, then name); engine
  regression covers the transformed card moving after a 9-strength unit. The
  source's one-shot Vildkaarl marker is also removed after transformation, so a
  later Medic revival receives ordinary placement audio instead of suppressing
  it as a fresh transformation.
- [x] Global Scorch chooses the strongest non-Hero units on both sides and
  removes ties; row Scorch uses the selected enemy row, includes Hero power in
  the 10-point threshold but cannot destroy Heroes. Regression cases cover all
  three rows, exactly 10 versus 9 total strength, preserving own units, and
  global ties/Hero immunity.
- [x] Clear Weather, each individual weather type, Skellige Storm, duplicate
  weather behavior and weather-card ownership/discard. Engine regressions verify
  Frost/Fog/Rain row mapping, Storm's Fog+Rain effects, duplicate Frost discard,
  Clear Weather removing all four active weather cards, and each card going to
  its owner's graveyard.
- [x] Cow removal summons Bovine Defense Force; Kambi removal summons Hemdall
  both during Scorch and round cleanup. Regression cases cover each path.
- [ ] Every leader ability in the source catalog: all Foltest, Emhyr, Eredin,
  Francesca, Crach an Craite and King Bran variants.

## AI — `ControllerAI`

- [ ] One source weighted-random AI; no difficulty selector.
- [ ] Source board maxima, board/grave card counts, per-card weights, weather,
  Scorch, Horn, Mardroeme, Medic, Berserker, Decoy and Agile choices.
- [x] Equal-benefit target ordering matches source AI: Commander’s Horn keeps
  the first row in close/ranged/siege order, and fallback Decoy targets use
  the source `map(...).concat(accumulator)` reverse-row ordering. Fixed-seed
  engine regressions distinguish both native selections from the old port.
- [ ] Source leader weights/actions and exact target selection.
- [x] Pass weighting: AI immediately passes when ahead after the opponent has
  passed, also passes on a Nilfgaard tie, does not force-pass on another
  faction's tie, and does not spend its final gem on an AI pass. Deterministic
  engine regressions cover all four decisions and verify forced passes do not
  consume RNG.
- [x] Hero Morale/Bond weighting preserves the pinned AI helper's treatment of
  Hero power. A seeded Isengrim-vs-five-point choice regression distinguishes
  the source weight from the previously reduced native weight.
- [x] Source AI redraw and discard ordering, including two sequential
  `ControllerAI.redraw()` calls that each recalculate discard order after
  drawing the replacement. Clear Weather joins typed weather cards in the
  source's multi-weather discard pool; a deterministic mulligan regression
  checks the Clear Weather/Biting Frost case. Muster groups and normal cards
  use stable sorting, preserving source order for tied `Card.compare` values;
  an equal-priority regression checks the first two AI discards.
- [ ] No illegal action may be silently converted into a pass.
- [ ] Repeatable simulations cover every premade deck and every leader action.
  Current regression coverage completes 100 seeded matches using source
  premade-deck recipes and leaders; it does not yet prove every possible leader
  action was selected or all AI weights match the source.

## Localization — source card data and UI strings

- [ ] Every source card title and ability title/description matches the official
  Polish game text; English source text remains available in English mode.
- [ ] Every menu, board label, interaction prompt, status, notification, and
  accessibility hint is translated and checked at 640×480.
- [ ] Original card artwork with printed English titles is covered by readable
  localized overlays wherever cards are enlarged or inspected.

## Screens and interactions — `index.html`, `style.css`, `gwent.js`

- [ ] Initial DeckMaker screen; faction description/change; bank and deck arrays;
  copy counts; leader; validation statistics; opponent deck preview/import/clear.
- [ ] Match left/center/right panels and all card piles/counters.
- [ ] Hover/select preview; card descriptions; leader view/activation; grave/deck
  viewers; five-card carousel; confirmation popup; notifications; end screens.
- [ ] Upload/download deck flows replaced only with suitable local-device flows.
- [ ] Music, SFX, notification toggles and persisted settings.
- [ ] All original board/card/status animations and their timing. Partial work is
  recorded in `docs/REFERENCE_ANIMATIONS.md`; this item remains open until every
  source effect is ported and compared.
- [ ] Exact 16:9 source stage scaled to 640x360 on the Miyoo 640x480 display;
  no invented title screen or re-centered/rearranged layout.

## Device acceptance

- [ ] Every source rule/action has a deterministic engine regression case.
- [ ] UI input paths exercise each source interaction, including cancel paths.
- [ ] SDL desktop smoke frames checked at 640x480 for every screen/state.
- [ ] ARM build and package hash audit; physical Miyoo input/audio/gameplay test.

## Status at audit start

All sections above remain open. The current native prototype has selected
engine rules, a subset of screens, and asset loading, but it does not meet the
1:1 acceptance bar. The previous SD package is not a parity release.

## Verified increments on `codex/full-source-parity`

These focused regression checks pass, but they do not close the broader parity
items above:

- Match initialization now follows the source order: each deck is constructed
  by random-position insertion, leader passives and faction setup run before
  the coin toss, and the opening deal alternates between players. Francesca
  Daisy's extra card therefore precedes the ten-card deal as in the reference.
  Human-hand draws use the source base-power/name sort, while AI-hand draws
  insert at index zero. Seeded regressions verify full deck/hand order and RNG
  state for a Daisy start and a normal coin toss. The port still uses xorshift32;
  identical numeric results to browser `Math.random()` are not claimed.
- Combat rows now use the source `Card.compare` insertion order (special type,
  base power, then name) for played units, Muster, Spy placement, Medic, Decoy,
  Avenger summons and Skellige revival. Previously most paths appended to the
  vector, so the visible row order diverged from the browser source and could
  make card-flight/selection positions appear to shift. A three-card Muster
  regression checks ordering across cards deployed from hand and deck. Full
  animation timing and physical-device verification remain open.
- Fixed-row unit audio now uses the card's printed row when callers have no row
  choice; Agile units still use the selected row. This also makes Berserker /
  Mardroeme detection and the placement-audio wait inspect the actual row. A
  frontend input regression checks that a fixed Ranged unit schedules the
  source `common_ranged` cue.
- The match renderer no longer stops weather-card drawing before the fourth
  distinct active weather card. Weather flight destinations and the layout
  share the same source slot calculation; the frontend regression checks slot
  four's board coordinate. The source catalog permits Frost, Fog, Rain, and
  Skellige Storm at once.
- Physical Commander's Horn and Mardroeme cards now remain in explicit row
  special slots until round cleanup, are serialized in save version 5, render
  and animate into the special slot, then move to the graveyard at round end.
  This prevents Crach from returning a still-active special card to the deck.
  Engine/save regressions cover active-card identity, Crach exclusion and the
  eventual graveyard move. Physical Miyoo placement visuals remain to verify.
- Decoy now returns the specifically selected unit, restores its source base
  power, runs Avenger removal effects, inserts the returned card using the
  correct human/AI hand rules, and leaves Decoy in the chosen combat row in its
  source-sorted position. The AI fallback targets one of the maximum-power
  units from any of its own rows, matching `max.rmax`; this target pool differs
  from the global `max.me` set used to calculate Decoy's action weight.
  Regressions cover exact target identity, row order, hand order, and seeded AI
  fallback choices.
- Agile Medic target plus melee/ranged revive row, including Miyoo-style SDL
  directional input and rejection of an incomplete choice.
- Emhyr Invader's random Medic target: ordinary units are revived immediately
  as in the source; if the random result is Agile, the player chooses only its
  row. The preview uses a copied RNG state so it does not consume or alter the
  match's next random result. Engine and SDL regressions cover both outcomes.
- Targeted plays now use the same input/action lock as ordinary plays and AI
  actions, through the end of queued card/effect animations. SDL input
  regression verifies the Medic target-confirm path activates the lock.
- Match initialization namespaces runtime card IDs by player and deck instance.
  The same neutral catalog card may be present in both decks, while the
  renderer tracks card flights and effects by ID. A regression verifies that
  identical catalog copies on opposite sides receive distinct runtime IDs.
  The 100-seed full-match simulations now also assert that all live-zone IDs
  remain unique and every original deck card remains present in exactly one
  zone after each action and after the match ends.
- Mardroeme's persistent row effect, its distinction from special-slot
  occupancy, Ermion activation, Berserker transformation on later plays and
  Muster summons, round reset, and save/load state.
- Source AI Mardroeme row weights have a deterministic target-selection
  regression: two close Berserkers versus two ranged Young Berserkers selects
  ranged using the source `10*n` / `8*n²-2*n` weights. This checks one case and
  does not establish parity for the complete AI heuristic set.
- Generated Avenger tokens use `summoned_` IDs and are omitted from the
  persistent graveyard when destroyed. The reference removes Bovine Defense
  Force/Hemdall from the graveyard 1001 ms later, before another turn can use
  Medic on them; the engine resolves this timer-backed cleanup atomically. A
  Cow → Bovine Defense Force → Scorch regression confirms the token is not
  left as a revive target.
- Transformed Vildkaarl cards tagged `vildkarrl` now play the source Mardroeme
  sound and suppress the ordinary row-placement sound, matching the placed
  audio handler in `abilities.js`.
- Card audio dispatch now follows the reference event split: unit placement
  selects `hero` or the row's `common_*` cue, spies and Berserkers under active
  Mardroeme suppress that cue, Decoy uses its own cue, and weather uses the
  card's source audio tag. `Card.animate()` cues are scheduled separately from
  placement; Mardroeme transformations and triggered Medic/Bond/Muster effects
  receive their own animation cues. Desktop build and CTest pass, but physical
  Miyoo speaker playback and timing still need device verification.
- Targeted source-rule regressions now cover Foltest Lord/Son, Emhyr Imperial/
  Invader, and Francesca Queen/Beautiful/Pureblood/Hope in addition to the
  previously covered leaders. This does not yet cover every leader's effects
  and selection flows.
- Eredin Commander now has a direct close-row leader Horn regression, including
  rejection when that row's special slot is already occupied.
- Scorch regressions now compare source target semantics for global ties across
  both sides and rows, Hero immunity, and row-Scorch's 10-point threshold (Hero
  strength counts toward the threshold but Heroes are not destroyed). This is
  targeted coverage; the full Scorch/leader/AI interaction matrix remains open.
- `ControllerAI.weightWeather()` uses own-side gain minus opponent-side gain.
  The native AI weather weight had the sign reversed, which favored harmful
  weather effects on the AI itself and suppressed beneficial Clear Weather.
  The sign now matches the source, and a seeded 512-sample regression confirms
  Clear Weather receives the expected 7:1 weighted choice over a one-point
  fallback when it restores seven points to the AI's row. Other weather AI
  weights and the full source decision distribution remain open.
- Card-name localization now shows Cirilla's Polish in-game card title instead
  of the full English name and resolves the accented source key for Eredin
  Bréacc Glas's Treacherous card. Catalog validation locks both mappings;
  full Polish title/ability/UI review is still open.
- Crach an Craite now returns graveyard cards using the source Deck's random
  insertion operation, preserving the existing deck when a graveyard is empty
  instead of shuffling all remaining cards. Seeded regressions cover both
  players' added cards and the empty-grave case; source animation and exact
  asynchronous random interleaving remain outside this engine-state check.
- Emhyr White Flame's source initialization suppresses both players' leader
  initialization when either side uses it, including passive Invader,
  Treacherous, Daisy, and King Bran effects; each passive suppression case now
  has a targeted regression.
- Match action/error statuses for saves, mulligan, passes, leader activation,
  target selection, Skellige revival and rejected AI actions now use Polish
  strings in Polish mode. This does not establish full UI localization parity.
- Opponent setup now chooses a random source premade recipe and a random leader
  of that recipe's faction; a deterministic 500-seed regression covers legal
  leaders/decks, repeatability, and reachability of all ten premade recipes.
- After an action, a player with no cards and no available leader ability now
  auto-passes; round start also pre-passes players with no legal action, matching
  the source `Player.endTurn()` and `Game.startRound()` checks.
- The zero-total AI fallback now considers Clear Weather in the same way as the
  source's `card.row === "weather"` check. A regression builds equal, weathered
  rows so Clear Weather has exactly zero raw value, then verifies the AI still
  selects it rather than passing. This closes only this fallback case; full AI
  weight and action distribution parity remains open.
- AI Scorch maxima use each unit's current `Card.power`, which the source
  updates when row effects resolve; this includes weather, Horn, Morale, and
  Tight Bond. A seeded selection regression verifies the maximum comparison
  against weather-adjusted power. Full AI weight parity is still open.
- Francesca Hope's AI weight now preserves the source helper's `-1` result
  when no Agile units are present, and its action weight is no longer clipped
  to zero. A 128-seed regression verifies how that negative weight shifts the
  weighted action interval; other source AI weights remain under audit.
- An AI action rejected by the port is reported; it is not converted into a
  voluntary pass.
- Source `Carousel` choices now use the five-card source geometry, source card
  images, localized descriptions, clamped left/right selection and a matching
  eased movement stagger (200/300/450 ms) with a 100 ms center highlight.
  Medic/Decoy targets, leader choices and Skellige revival use this shared
  view. Broader screen and animation parity remains open.
- Medic, Decoy, Emhyr Relentless and Eredin Bringer of Death now apply the
  source `Card.isUnit()` restriction consistently: Heroes and noncombat cards
  cannot appear in the picker or be accepted by the rules engine. Regression
  coverage checks both picker lists and engine rejection.
- Tight Bond now matches the source row effect exactly: its multiplier is the
  number of same-name Bond cards when that count exceeds one, and the source
  name-keyed multiplier applies to every same-name card. Two- and three-copy
  cases plus differing catalog group metadata have regression coverage.
- Match actions now create concurrent 499 ms card flights from before/after
  zone diffs. This covers multi-card Muster summons and Scorch removals, Medic
  revivals, Spy draws, Decoy swaps, and weather cards sent to the grave; a UI
  regression checks multi-card Muster and Scorch queues. The played-card
  preview now remains pinned to the selected card for the source 1000 ms preview,
  then the 499 ms flight begins. Placement audio and ability overlays are
  scheduled at their source sequence offsets; Medic/Muster, Spy and Scorch
  follow-on flights wait for their animation chains. The state engine still
  resolves immediately instead of awaiting the visual chain, and physical Miyoo
  animation review remains open.
- After a card or leader action, player input and the next AI action are now
  held until the queued source-timed flight/effect sequence and row placement
  wait finish. The SDL input regression checks the 1000 ms preview, 499 ms
  flight, row placement cue, delayed ability overlay and locked-input deadline.
  Turn sounds and gameplay notifications are also held until that lock expires,
  so they no longer begin over the played card's effects. Rule state still
  changes immediately at action selection, and device timing needs physical
  confirmation.
- Large card displays now crop original artwork at its printed gold divider
  and draw localized names and rules as native text. This applies to the card
  inspector, hand preview, target carousel, and deck collection; 640×480 smoke
  captures were visually reviewed. Exact card-image scaling and all remaining
  source UI states still need comparative review.
- Release builds keep `assert`-based engine regression expressions enabled.
  Without this, `NDEBUG` removed state-changing expressions from the test suite
  and the Release test executable crashed. Release CTest now exercises the same
  setup and checks as Debug.

Verification for the current source tree: SDL2 Release desktop build, CTest
2/2, `gwent --input-test`, and `gwent --audio-test assets` with dummy SDL
video/audio. The audio command decoded all 35 SFX and both background tracks;
it does not verify audible output on physical hardware. The card inspector,
deck builder, and target carousel were visually reviewed at 640×480 in
`artifacts/qa/card-details-polish.png`,
`artifacts/qa/deck-builder-card-preview.png`, and
`artifacts/qa/target-carousel.png`. A new card-flight frame is recorded at
`artifacts/qa/source-timed-played-card-preview.png`. Its visual state and source
timing offsets are checked by `gwent --input-test`. Physical-device
input/audio/gameplay still need confirmation.

Mulligan parity now uses the source `Deck.swap()` sequence for human and AI:
each discarded card is inserted with `Deck.addCardRandom()`, the new front card
is drawn immediately, and the replacement is inserted with the source Hand
behavior: the human hand re-sorts when the selected index is zero, while other
indices are retained; the AI hand uses the requested index. The engine uses the
same front-of-deck draw convention for opening hands and later draws. Seeded
regressions lock the order for two human swaps and the AI's two recalculated
discard decisions, reject a third redraw without mutation, and preserve the RNG
result. AI discard ordering includes Clear Weather whenever its hand contains
multiple cards from source row `weather`. Release CTest also passes 100 seeded
complete matches across premade decks, including mulligans and the Skellige
round-setup revive selection. Full interactive device testing remains open.

## Eredin Destroyer of Worlds selection flow

The source leader ability opens a non-cancellable hand carousel for up to two cards, then an optional one-card deck carousel that can be canceled. The discard carousel accepts fewer than two cards when the hand is smaller, and the ability is spent once resolution begins even if the optional draw is canceled. The SDL flow and engine now mirror those cases, including a draw-only choice with an empty hand and an empty activation when both hand and deck are empty. Engine regressions cover the two-discard/no-draw, one-discard/no-draw, and draw-only cases; SDL input regressions cover cancel timing and draw-only selection. Physical Miyoo interaction still requires confirmation.

## Animacja wskrzeszenia Medyka — 16.11.2026

Źródłowy handler `Medic` animuje wybraną kartę, gdy nadal leży w cmentarzu, a dopiero po 1650 ms rozpoczyna jej 499 ms lot do rzędu. SDL nakładał efekt drugi raz również na zagrany egzemplarz Medyka i rysował pierwszą nakładkę w miejscu docelowym. Renderer pokazuje teraz pojedynczy efekt na karcie w cmentarzu, a lot startuje na końcu sekwencji animacji. Regresja wejścia SDL sprawdza liczbę i pozycję nakładki, punkt startu lotu i dokładną granicę czasową. CTest (2/2) i `--input-test` przechodzą; pozostałe efekty i fizyczny test Miyoo pozostają otwarte.

## Board-wide card counts in source AI — 16.11.2026

`ControllerAI.getBoardData()` collects on-board abilities from `board.row[0..2]`, the AI-controlled side in the pinned source layout. Native AI retains board-wide counts for other decisions, while Decoy weighting and target selection use only its own three rows. Seeded regressions compare 256 decisions with and without an opposing Spy and verify that the AI returns its own Spy when both sides have one. This corrects an earlier parity note that incorrectly described the source as scanning all six rows.

Francesca Hope's exhaustive Agile assignment uses the same sorted row insertion
as the pinned source's `Row.addCard()` / `CardContainer.addCardSorted()` path.
Engine regression checks that both combat rows remain sorted after activating
the Leader.

Arachas- Behemoth's default Muster prefix now follows the source
`name.indexOf('-')` behavior, including hyphens with no surrounding space.
The prior catalog generator split only on `" - "`, so it failed to summon
Arachas copies from hand/deck. An engine regression verifies all three copies
move and both source zones empty.

## Berserker transformation row order — 16.11.2026

The pinned `abilities.js` removes each Berserker and adds its Vildkaarl
replacement through `Row.addCard()`, which sorts by printed power and name.
The native engine previously replaced the struct in place. It now resorts the
row after transformation, preserving the runtime card ID for UI selection. The
engine regression confirms the 14-strength Vildkaarl moves after a 9-strength
unit. CTest passed 2/2, desktop CI passed for commit `450237d`, and the ARMHF
OnionOS package was installed and SHA-256 verified on SD (603 files). The SD
backup is `GwentClassic.previous-450237d`. Physical match behavior and the
one-shot Vildkaarl marker lifecycle still required audit at that commit.

Commit `c472e24` now also removes the source's temporary `vildkarrl` tag as
soon as transformation resolves. The regression confirms neither ordinary nor
Young Vildkaarl retains the marker, so a later revival follows the normal
placement-audio path. CTest passed 2/2, desktop CI and ARMHF packaging passed,
and all 603 files on SD match the package. Physical audio/gameplay confirmation
is still open.

## AI tie ordering for Horn and Decoy — 16.11.2026

The pinned source's `ControllerAI.horn()` visits melee, ranged, then siege and
keeps the first row when gains tie. The native implementation used the reverse
order; it now follows the source and a fixed-seed regression checks the melee
choice. The source Decoy fallback prepends each row's candidate list while
iterating melee to siege, yielding siege/ranged/melee target order. The native
fallback now uses that order; a fixed-seed regression checks the siege target.
These tests verify tie/fallback ordering only; the broader AI weighting and
decision audit remains open.

## Round and match result presentation

The result screen previously used `matchWinner` even during `RoundResult`,
where it is still unset. That made every completed round display the loss
stamp; a match draw also displayed defeat. The view now derives a round outcome
from score plus the Nilfgaard tie-break and derives a completed match outcome
from `matchWinner`, displaying the source draw stamp and localized draw title
when appropriate. The frontend result smoke test renders round win, loss, draw,
Nilfgaard tie-break, and match draw frames; `--input-test` checks all outcome
branches. Engine/frontend CI passed for commit `550435f`; the ARMHF package was
installed on SD and all 603 files were SHA-256 verified. Physical-device result
screen confirmation remains open.

## Miyoo card miniatures and rules panel

The match rules panel now renders in the reserved 4:3 display band below the
centered 16:9 board, instead of covering the hand. Compact cards keep the
source ability icons, while the row-class badge uses a short, localized tag
(WR/DY/OB/ZW in Polish; CL/RA/SI/AG in English). The source weapon medallions
are still used on enlarged cards; at 11px on a Miyoo hand their details collapsed
into ambiguous marks. The power badge PNGs contain a large transparent export margin; drawing their full canvas
made the medal half-size and placed its number away from the visible circle.
The renderer now crops that margin before scaling the medal into the number's
24x25 box. The mini row and ability badges have a stronger dark/gold backing.
The large card preview remains unchanged.

The renderer's built-in smoke screenshot is checked in desktop CI; CI must
render the latest badge-crop change before a new visual is treated as verified.
The prior local frame predates this crop and is not evidence for the latest
miniature badge alignment or a physical Miyoo capture.

## AI action RNG ordering

The pinned ControllerAI computes candidate weights, rolls once to select an
action, and only then resolves random targets for the chosen Decoy, Medic, or
Leader. The native port previously resolved those targets while building every
candidate and therefore consumed RNG for actions that were never played. It
now previews candidate legality on copied game state and resolves the chosen
action against the live match only after the weighted ticket. Seeded engine
regressions cover an unchosen Decoy, a chosen Decoy, and an unchosen random
Medic Leader. The Eredin Destroyer weight also follows the source's generic
leader fallback; its discard ordering belongs to action resolution and must not
change the selection weight or consume RNG before the action is chosen.

The full weighted-AI parity audit remains open; these regressions prove RNG
ordering for the covered target paths only.

## Miniature validation — 2026-11-16

Desktop CI run 43879730026 passed for bc33dcc5ebcc3409de106d2b0a6d7543726eeb07.
Its frontend-smoke.bmp was downloaded and visually inspected at native 640x480
and with nearest-neighbor enlargement of the hand. The row labels and common
power rendering are present; two-digit powers use condensed spacing at the
same glyph scale. This verifies that renderer fixture only, not physical-device
readability or every possible buffed power. ARM package run 43880296800 was
started for this same revision; installation and hardware validation remain
pending until the package is available.

## SD update — 2026-11-16, bc33dcc

ARM workflow 43880296800 completed successfully for bc33dcc5ebcc3409de106d2b0a6d7543726eeb07.
Package SHA-256: a73bd9f27c6ece3807aaefba9c7d6824f859819db0974adb54d33e048ae1aae5.
Installed to E: (MIYOO MINI, FAT32) using tools/install_verified_sd.ps1;
all 604 archive files matched their destination SHA-256. The previous game
folder was backed up locally in artifacts/qa/package-bc33dcc/sd-backup-20261116-145649.
This proves copying and file integrity, not successful execution on Miyoo.
Physical startup, input, audio and miniature readability remain to be checked.

## Fractional Horn weights and integer AI tickets

The audit found two differences in the previous SD build bc33dcc. Source
ControllerAI.weightCard preserves Math.max(...rows)/2 for Commander's Horn,
including half-point weights. The old native int return discarded the fraction.
Source common.js randomInt floors the weighted ticket, whereas the old native
selector used a continuous double ticket. Both affected action distribution.

The discrepancy above was corrected in 68a7a67: Horn weights remain double
precision and the weighted ticket is floored like source randomInt(). A seeded
regression evaluates 2,857 seeds with a three-point Horn gain (weight 1.5)
and a one-point unit, checking both selection regions and final RNG state.
Engine CI job 133347163861 passed, including this test. Frontend CI is still
running; SD remains on bc33dcc until a new ARM package is validated.

ARM workflow 43881332283 now builds c2d06b8, which includes the 68a7a67
engine correction. Desktop frontend checks for 68a7a67 are still running.
No newer SD installation is claimed by this entry.
