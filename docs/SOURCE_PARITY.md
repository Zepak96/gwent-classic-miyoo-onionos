# Gwent Classic source parity audit

This audit uses the unchanged files in `upstream/asundr-gwent-classic/` at
commit `08592d847f2573440a34deb9f32bc855b2544cdd` as the authority. A checked
item requires a native implementation and a repeatable comparison against the
source behavior. A build or screenshot alone does not count as parity.

## Match lifecycle — `Game`, `Player`, `Deck`, `Board`

- [ ] New game, random faction premade opponent, random legal faction leader.
- [ ] Ten-card initial hands; source redraw order and at most two redraws.
- [ ] First-player coin toss and Scoia'tael's first-player choice.
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
  drawing the replacement.
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

- Agile Medic target plus melee/ranged revive row, including Miyoo-style SDL
  directional input and rejection of an incomplete choice.
- Mardroeme's persistent row effect, its distinction from special-slot
  occupancy, Ermion activation, Berserker transformation on later plays and
  Muster summons, round reset, and save/load state.
- Decoy fallback considers each own row's maximum units as `ControllerAI` does.
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
  regression checks multi-card Muster and Scorch queues. Full source preview,
  all action-specific waits, and physical Miyoo animation review remain open.
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
`artifacts/qa/target-carousel.png`. Physical-device input/audio/gameplay still
need confirmation.

AI mulligan regression: `ControllerAI.redraw()` performs two independent
discard-order decisions, with each discarded card returned to the deck bottom
and the next card drawn from the deck top before the next decision. The port
previously simulated both decisions but returned card IDs for a simultaneous
hand selection; a second choice could then refer to a just-drawn card and be
rejected. The runtime now performs the two swaps sequentially. Release CTest
passes 100 seeded complete matches across premade decks, including mulligans
and the Skellige round-setup revive selection. This is targeted regression
coverage, not a full proof of AI-weight parity.
