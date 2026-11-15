# Gwent Classic for Miyoo Mini Plus / OnionOS

A native 1:1 port of the Gwent minigame from **The Witcher 3: Wild Hunt** to Miyoo Mini Plus is the project goal, covering the base game, Hearts of Stone, Blood and Wine, and the Gwent additions/changes from the Witcher 3 next-gen update. The pinned Gwent Classic repository is the gameplay reference; this is not the standalone `GWENT: The Witcher Card Game` or its online rules. The 640x480 SDL prototype has AI matches, reference decks, mulligan, and a deck editor with three saved slots, but it is not a complete implementation or OnionOS release.

## Status

See [PROGRESS.md](PROGRESS.md) for the audit, implementation status, and remaining work. There is no ARM binary or installation ZIP yet, and Miyoo compatibility has not been confirmed. Rules and card coverage are still being implemented; do not treat the current code as a faithful full game.

## Build

Requirements and commands are in [BUILD.md](BUILD.md). The project requires CMake and a C++17 compiler; the frontend requires SDL2.

## Licenses and assets

This project does not include code or assets from the reference repositories. See [LICENSES.md](LICENSES.md). No CD PROJEKT RED material is included.

## Controls

At the title screen, Enter/X opens deck selection. There, left/right changes the reference deck, Z changes language, Enter/X starts a match, R resumes the last save, B opens the deck editor, P switches back to a reference deck, and [ / ] selects one of three user deck slots. In the editor, up/down selects cards from the active faction and neutral pool, Enter/A adds a copy, Backspace/B removes one, L/X switches to leader selection, F/Y changes faction, S/SELECT saves a valid deck, and START/Escape returns without saving. The opponent uses one “Classic” AI mode based on the reference repository's weighted decisions; the reference does not define Easy/Normal/Hard settings. With Scoia'tael, left/right chooses who starts and Enter confirms. During mulligan, left/right selects a card, Space/X marks it, and Enter confirms. In a match, left/right selects a card, up/down selects a row, Enter/X plays, L uses the leader, and Backspace passes. S/SELECT saves the match; O/START loads the save. Arrows and Enter select Decoy/Medic targets. At the start of round three, Skellige lets the player choose two units from the graveyard; up/down chooses the row for Agile units. Escape returns to the menu. SDL GameController mapping is implemented, but physical Miyoo controls and device behavior still need verification.
