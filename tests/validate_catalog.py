import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
manifest = json.loads((root / "cards-manifest.json").read_text(encoding="utf-8"))
cards = json.loads((root / "cards.json").read_text(encoding="utf-8"))
assert len(cards) == manifest["record_count"] == 216
required = {"name", "id", "deck", "row", "strength", "ability", "filename", "count"}
assert all(required <= set(card) for card in cards)
assert all(card["name"].strip() for card in cards)
assert {"realms", "nilfgaard", "scoiatael", "monsters", "skellige"} <= {card["deck"] for card in cards}
assert any(card["name"] == "Roach" for card in cards)
assert any(card["name"] == "Geralt of Rivia" and "muster" in card["ability"].split() and card.get("muster") == "Roach" for card in cards)
assert any(card["name"].startswith("Cirilla Fiona") and "muster" in card["ability"].split() and card.get("muster") == "Roach" for card in cards)
assert all(int(card["count"]) >= 0 for card in cards)
deck_manifest = json.loads((root / "premade-decks-manifest.json").read_text(encoding="utf-8"))
decks = json.loads((root / "premade-decks.json").read_text(encoding="utf-8"))
assert len(decks) == deck_manifest["record_count"] == 10
assert {deck["faction"] for deck in decks} >= {"realms", "nilfgaard", "scoiatael", "monsters", "skellige"}
assert all(0 <= deck["leader"] < len(cards) for deck in decks)
assert all(0 <= index < len(cards) and count > 0 for deck in decks for index, count in deck["cards"])
print(f"Validated {len(cards)} reference card records from {manifest['commit']}.")
