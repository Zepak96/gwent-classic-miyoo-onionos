# Gwent Classic for Miyoo Mini Plus / OnionOS

A native port of the Gwent minigame from **The Witcher 3: Wild Hunt** is the project goal, covering the base game, Hearts of Stone, Blood and Wine, and the Gwent additions/changes from the Witcher 3 next-gen update. This is not the standalone `GWENT: The Witcher Card Game` or its online rules. The repository is at an early stage: current code is an engine scaffold and a minimal SDL2 frontend, not a complete playable implementation.

## Status

See [PROGRESS.md](PROGRESS.md) for the audit, implementation status, and remaining work. There is no ARM binary or installation ZIP yet, and Miyoo compatibility has not been confirmed. Rules and card coverage are still being implemented; do not treat the current code as a faithful full game.

## Build

Requirements and commands are in [BUILD.md](BUILD.md). The project requires CMake and a C++17 compiler; the frontend requires SDL2.

## Licenses and assets

This project does not include code or assets from the reference repositories. See [LICENSES.md](LICENSES.md). No CD PROJEKT RED material is included.

## Controls

The SDL2 frontend does not yet have the game UI or complete Miyoo button mapping. SDL_QUIT/Escape closes the desktop window; target controls have not been verified.
