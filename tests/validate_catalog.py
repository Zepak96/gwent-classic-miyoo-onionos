import ast
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
manifest = json.loads((root / "cards-manifest.json").read_text(encoding="utf-8"))
cards = json.loads((root / "cards.json").read_text(encoding="utf-8"))
ability_text = json.loads((root / "ability-text.json").read_text(encoding="utf-8"))
card_names_pl = json.loads((root / "card-names-pl.json").read_text(encoding="utf-8"))
ability_text_pl = json.loads((root / "ability-text-pl.json").read_text(encoding="utf-8"))
assert len(cards) == manifest["record_count"] == 216
# Parse the JSON string assembled by the pinned upstream cards.js and compare
# every field/order, rather than relying only on a local record-count manifest.
source_file = root.parent / "upstream" / "asundr-gwent-classic" / "cards.js"
source_fragments = []
for line in source_file.read_text(encoding="utf-8").splitlines():
    fragment = line.strip()
    if fragment.startswith("var card_dict") or fragment == ");":
        continue
    if fragment.startswith("+ "):
        fragment = fragment[2:].strip()
    source_fragments.append(ast.literal_eval(fragment))
source_cards = json.loads("".join(source_fragments))
assert cards == source_cards, "Card records/order differ from pinned upstream cards.js"
required = {"name", "id", "deck", "row", "strength", "ability", "filename", "count"}
assert all(required <= set(card) for card in cards)
assert all(card["name"].strip() for card in cards)
assert {"realms", "nilfgaard", "scoiatael", "monsters", "skellige"} <= {card["deck"] for card in cards}
assert any(card["name"] == "Roach" for card in cards)
assert any(card["name"] == "Geralt of Rivia" and "muster" in card["ability"].split() and card.get("muster") == "Roach" for card in cards)
assert any(card["name"].startswith("Cirilla Fiona") and "muster" in card["ability"].split() and card.get("muster") == "Roach" for card in cards)
assert all(int(card["count"]) >= 0 for card in cards)
card_abilities = {tag for card in cards for tag in card["ability"].split()}
assert card_abilities - set(ability_text) == {"vildkarrl"}
assert all(item["name"] for item in ability_text.values())
assert all("description" in item for item in ability_text.values())
assert {card["name"].strip() for card in cards} <= set(card_names_pl)
assert all(card_names_pl[card["name"].strip()].strip() for card in cards)
assert set(ability_text) == set(ability_text_pl)
assert all(item["name"].strip() and item["description"].strip() for item in ability_text_pl.values())
asset_root = root.parent / "assets" / "gwent-classic" / "img"
for icon in ("ui_exit", "ui_music", "ui_notification", "ui_sfx", "ui_trash", "ui_view"):
    assert (asset_root / "icons" / f"{icon}.png").is_file(), f"Missing rasterized UI icon: {icon}"
sfx_root = root.parent / "assets" / "gwent-classic" / "sfx"
assert len(list(sfx_root.glob("*.mp3"))) == 35
for card in cards:
    image = f"{card['deck']}_{card['filename']}.jpg"
    assert (asset_root / "sm" / image).is_file(), f"Missing small card image: {image}"
    assert (asset_root / "lg" / image).is_file(), f"Missing large card image: {image}"
deck_manifest = json.loads((root / "premade-decks-manifest.json").read_text(encoding="utf-8"))
decks = json.loads((root / "premade-decks.json").read_text(encoding="utf-8"))
assert len(decks) == deck_manifest["record_count"] == 10
assert {deck["faction"] for deck in decks} >= {"realms", "nilfgaard", "scoiatael", "monsters", "skellige"}
assert all(0 <= deck["leader"] < len(cards) for deck in decks)
assert all(0 <= index < len(cards) and count > 0 for deck in decks for index, count in deck["cards"])
print(f"Validated {len(cards)} reference card records and their small/large art from {manifest['commit']}.")
