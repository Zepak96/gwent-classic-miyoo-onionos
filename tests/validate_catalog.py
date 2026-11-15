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
print(f"Validated {len(cards)} reference card records from {manifest['commit']}.")
