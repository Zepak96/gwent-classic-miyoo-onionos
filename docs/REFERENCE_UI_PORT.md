# Reference UI port map

## Authority

The visual and interaction source of truth is `asundr/gwent-classic` at commit
`08592d847f2573440a34deb9f32bc855b2544cdd`. The current native screen in
`src/main.cpp` is an original SDL prototype, not a faithful rendering of that
source. Its generated smoke frames are not acceptance evidence for a 1:1 port.

## Source files and responsibilities

- `index.html`: exact DOM hierarchy and all visible/overlay UI elements.
- `style.css`: geometry, typography, board composition, overlays, animations,
  hover states, card sizes, menu layout, and 1920:1080 viewport ratio.
- `gwent.js`: game flow, UI transitions, card presentation, selection,
  notifications, modal/carousel flows, match end screen, and settings.
- `common.js`, `cards.js`, `decks.js`, `abilities.js`, `factions.js`: shared
  helpers and the reference card, deck, ability, and faction behavior/data.
- `img/`, `svg/`, and `sfx/`: source presentation assets. The pinned repository
  does not contain music files; its README says the browser game streams music
  from YouTube.

## Reference screens and elements

### Deck customization (the original initial screen)

`DeckMaker` is constructed at startup. There is no title splash screen in the
reference. The source screen is a full-screen darkened overlay containing:

- faction title and description;
- upload deck, change faction, and download deck options;
- scrollable card collection and current-deck card arrays with copy counters;
- leader card selection;
- deck statistics with their original icons;
- opponent deck preview/upload/clear controls;
- start-game button and music/SFX controls.

The port may replace browser file dialogs with device file selection and map
mouse actions to Miyoo buttons. It must retain the source elements, ordering,
copy rules, and visual geometry.

### Match screen

The source has three fixed-width panels over `img/board.jpg`:

- left (26.5%): exit control, both leader cards and activation indicator,
  profile frames, names/deck names, hand counts, gems, side score totals,
  pass control, and weather-card container;
- center (52.5%): opponent rows, player rows, per-row score/special/weather
  areas, and the hand;
- right (21%): opponent/player graveyards and deck piles/counters.

Source overlays and states that must also be ported include card previews and
descriptions, the five-card carousel, confirmation popups, notification bar,
round/game end table and result art, faction/leader abilities, and settings.
They are part of the game UI, not optional decoration.

## Miyoo coordinate transform

The CSS reference uses a 1920x1080 (16:9) stage whose width is 100vw and whose
height is `100vw * 1080 / 1920`. At 640 pixels wide, the stage is exactly
640x360. Preserve that stage and its original top-left anchoring on the Miyoo
Mini Plus 640x480 display; the remaining 120 vertical pixels are outside the
reference stage. CSS `vw` dimensions therefore scale by exactly one third from
the 1920-wide reference. Do not stretch the 16:9 composition to 4:3.

## Port rule

Keep the reference ordering, art, labels, score presentation, board zones,
states, transitions, card selection/preview flow, notifications, and end
screens. Adapt only rendering/input/persistence mechanisms that require a
native Miyoo implementation. Every implementation deviation must be recorded
as a known gap until verified against the pinned source. The planned arcade
campaign belongs after the reference game is complete and must not replace or
re-layout the source game's screens.
