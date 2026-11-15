# Card catalog

`cards.json` is a mechanical-data extraction of `card_dict` from `asundr/gwent-classic` commit `08592d847f2573440a34deb9f32bc855b2544cdd`. It keeps the reference card names, IDs, faction keys, rows, strength, ability keys, art filenames as identifiers, copy counts, and muster names. The art files themselves are not included in this JSON extraction.

`premade-decks.json` extracts all 10 reference starter/opponent deck recipes from `decks.js`, including card dictionary indexes, copy counts, faction, and leader index. Its source and commit are recorded in `premade-decks-manifest.json`.

The extracted card data retains the source project's MIT + Commons Clause v1.0 terms and attribution. See `../third_party/gwent-classic/LICENSE` and `../third_party/gwent-classic/NOTICE.md`. The `filename` field is an upstream asset key; it does not imply that an asset file exists in this port.

This is the chosen parity dataset for this port. The native engine must implement its abilities, leaders, factions, and card limits rather than silently dropping unsupported entries. Next-gen details will be compared against the pinned reference revision and Witcher 3 behavior.
