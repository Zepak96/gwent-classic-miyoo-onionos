# Gwent Classic for Miyoo Mini Plus / OnionOS

A native 1:1 port of the Gwent minigame from **The Witcher 3: Wild Hunt** to Miyoo Mini Plus is the project goal, covering the base game, Hearts of Stone, Blood and Wine, and the Gwent additions/changes from the Witcher 3 next-gen update. The pinned Gwent Classic repository is the source of truth; this is not the standalone `GWENT: The Witcher Card Game` or its online rules. The current SDL interface is a prototype and does not reproduce the reference layout, so it is not a faithful port or an OnionOS release.

## Status

See [PROGRESS.md](PROGRESS.md) and [docs/REFERENCE_UI_PORT.md](docs/REFERENCE_UI_PORT.md) for the reference audit and remaining work. An OnionOS ARM build and SD package have been produced and tested against the card contents, but the current code is still incomplete and has not been launched on the physical console.

Current 640×480 smoke frames: [deck editor](artifacts/qa/deck-builder-card-preview.png), [deck selection](artifacts/qa/deckselect-menu.png), [card-choice carousel](artifacts/qa/target-carousel.png), and [card inspector](artifacts/qa/card-details-polish.png). These are desktop smoke-test renders, not captures from a physical Miyoo.

## Build

Requirements and commands are in [BUILD.md](BUILD.md). The project requires CMake and a C++17 compiler; the frontend requires SDL2.

## Licenses and assets

All card images, interface graphics, and SFX present in the pinned reference repository are included under `assets/gwent-classic/`; the card art is used by the current prototype. Two owner-supplied MP3 tracks loop as menu and match background music. See [LICENSES.md](LICENSES.md) and the asset provenance file.

## Controls

The application starts in the deck editor, matching the `DeckMaker` screen in the reference repository. Escape/B opens deck selection. There, left/right changes the deck, Y changes language, A/Start begins a match, R resumes the last save, X/B opens the editor, P selects a reference deck, and [ / ] changes the user-deck slot. In the editor, left/right switches between the card bank and deck, up/down selects a card, A adds or removes a copy depending on the active panel, B removes a copy in the deck panel, L selects a leader, Y changes faction, Select saves a valid deck, and Start begins the match. X opens the text card inspector. User decks use three save slots.

The AI uses one weighted `ControllerAI` model from the reference repository; there are no separate difficulty levels. With Scoia'tael, left/right chooses who starts and A confirms. During mulligan, left/right selects a card, A marks it, Start/Y confirms or skips, and X opens the card inspector. The counter shows remaining redraws out of two. In a match, left/right selects a card, up/down selects a row, A plays, Y activates the leader, Select requests guarded passing, Start opens pause, and X shows card details. Pause can resume, save/load, change language, disable audio, or return to deck selection. See [docs/MIYOO_INPUT.md](docs/MIYOO_INPUT.md) for the Miyoo button mapping.

SDL Mini for Miyoo is built with a patch mapping physical **MENU** (`KEY_HOME`) to `SDLK_HOME`; the app saves the match and exits. Physical controls and audio still need confirmation on the console.
