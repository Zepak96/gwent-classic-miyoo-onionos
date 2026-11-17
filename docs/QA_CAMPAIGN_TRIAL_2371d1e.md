# Northern Realms campaign trial — installed 2026-11-17

Installed source revision: `2371d1e96bf46fe9329990964345b8aaff03176f`.
Only the Northern Realms tower is exposed in this trial. Other faction data is
retained for development, but its tabs cannot be selected in this version.

Five stages: Talar, Ves, Vernon Roche, Sigismund Dijkstra, Filippa Eilhart.
All five generated portraits were inspected. The current 640×480 campaign
smoke frame was inspected: portrait, ladder, dialogue and controls occupy
separate regions. The achievement counter is limited to 0–5 for this trial.

Validation:

- Desktop CI run `44077403053`: engine and frontend jobs successful.
- Includes deck legality, progression and persistence tests; campaign UI smoke
  checks menu entry, hidden faction navigation, locked-stage rejection, match
  start with the prescribed leader, no loss reward, unique win reward,
  next-stage unlock and saved progress reload.
- ARM build run `44077402716`: successful, archive verified by CI.
- Package binary inspected as 32-bit ARM ELF. All five tower portraits and
  `menu.mp3` / `match.mp3` are present in the archive.
- Archive SHA-256:
  `a3d123e843a1c27455bd0e4cd5c3238ee1f8cd58869eb3c37711a17d57f2d3e9`.
- Installed binary SHA-256:
  `1d0910cb125756af60e99df98c27507e1412c0bfa454eb0e6e32fd1bf0ca01e4`.
- Installed on `E:` after verifying `MIYOO MINI` / FAT32 target identity.
  `tools/install_verified_sd.ps1` verified SHA-256 of all 617 installed files.
- Prior installation backup:
  `artifacts/qa/package-2371d1e/sd-backup-20261117-125153`.
  Existing saves were not replaced.

Controls: choose Campaign in the main menu and confirm B. Up/down selects an
opponent; B opens the encounter, B again starts the match. A returns; Y opens
the player's deck editor. Win progression is automatic.

This is a device-test release, not proof that all historical gameplay bugs are
resolved. Actual rendering, audio and a full campaign playthrough on Miyoo
remain to be confirmed on hardware.
