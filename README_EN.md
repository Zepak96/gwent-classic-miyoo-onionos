# Gwent Classic for Miyoo Mini Plus / OnionOS

A native 1:1 port of the Gwent minigame from **The Witcher 3: Wild Hunt** to Miyoo Mini Plus is the project goal, covering the base game, Hearts of Stone, Blood and Wine, and the Gwent additions/changes from the Witcher 3 next-gen update. The pinned Gwent Classic repository is the source of truth; this is not the standalone `GWENT: The Witcher Card Game` or its online rules. The current SDL interface is a prototype and does not reproduce the reference layout, so it is not a faithful port or an OnionOS release.

## Status

See [PROGRESS.md](PROGRESS.md) and [docs/REFERENCE_UI_PORT.md](docs/REFERENCE_UI_PORT.md) for the reference audit and remaining work. An OnionOS ARM build and SD package have been produced and tested against the card contents, but the current code is still incomplete and has not been launched on the physical console.

## Build

Requirements and commands are in [BUILD.md](BUILD.md). The project requires CMake and a C++17 compiler; the frontend requires SDL2.

## Licenses and assets

All card images, interface graphics, and SFX present in the pinned reference repository are included under `assets/gwent-classic/`; the card art is used by the current prototype. Music tracks are absent from the reference repository. See [LICENSES.md](LICENSES.md) and the asset provenance file.

## Controls

At the title screen, Enter/X opens deck selection. There, left/right changes the reference deck, Z changes language, and up/down cycles the AI difficulty: **Easy** randomly chooses among legal moves and may make strategic mistakes; **Normal** uses the reference repository's weighted AI; **Hard** chooses the legal move with the highest heuristic value. Difficulty and language are saved in device settings. Enter/X starts a match, R resumes the last save, B opens the deck editor, P switches back to a reference deck, and [ / ] selects one of three user deck slots. In the editor, up/down selects cards from the active faction and neutral pool, Enter/A adds a copy, Backspace/B removes one, L/X switches to leader selection, F/Y changes faction, S/SELECT saves a valid deck, and START/Escape returns without saving. With Scoia'tael, left/right chooses who starts and Enter confirms. During mulligan, left/right selects a card, Space/X marks it, and Enter confirms. In a match, left/right selects a card, up/down selects a row, Enter/X plays, L uses the leader, and Backspace passes. S/SELECT saves the match; O/START loads the save. Arrows and Enter select Decoy/Medic targets. At the start of round three, Skellige lets the player choose two units from the graveyard; up/down chooses the row for Agile units. Escape returns to the menu. SDL GameController mapping is implemented, but physical Miyoo controls and device behavior still need verification.
