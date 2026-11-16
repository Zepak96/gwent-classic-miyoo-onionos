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
- [ ] One action per turn; automatic pass when no cards/leader action remain.
- [ ] Voluntary pass; passed player stays out while the other keeps playing.
- [ ] Round result, gems/health, ties, Nilfgaard tie-break, match result/rematch.
- [ ] Round cleanup, Northern Realms draw, Monsters survivor, Skellige round-three
  graveyard selection and Agile row selection.
- [ ] Leader activation availability resets at each round; White Flame cancels
  the opponent leader; all leader passives apply at the source-defined time.

## Card rules — `abilities.js`, `Row`, `Weather`, `Card`

- [ ] Base score, Hero immunity, weather floor/half-weather, horn and Dandelion.
- [ ] Tight Bond by printed name; Morale boosts other row units.
- [ ] Spy placement and up-to-two draws; Medic selection and Agile row choice.
- [ ] Decoy target selection, returned card, replacement slot and ownership.
- [ ] Muster from hand/deck; next-gen Geralt/Ciri muster and Roach Avenger.
- [ ] Berserker/Mardroeme transformations and Vildkaarl lifecycle.
- [ ] Global Scorch and each row Scorch threshold/targets.
- [ ] Clear Weather, each individual weather type, Skellige Storm, duplicate
  weather behavior and weather-card ownership/discard.
- [ ] Bovine Defense Force and Kambi/Hemdall Avenger triggers.
- [ ] Every leader ability in the source catalog: all Foltest, Emhyr, Eredin,
  Francesca, Crach an Craite and King Bran variants.

## AI — `ControllerAI`

- [ ] One source weighted-random AI; no difficulty selector.
- [ ] Source board maxima, board/grave card counts, per-card weights, weather,
  Scorch, Horn, Mardroeme, Medic, Berserker, Decoy and Agile choices.
- [ ] Source leader weights/actions and exact target selection.
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
