import json
import pathlib
import re
import sys
import unicodedata

source = pathlib.Path(sys.argv[1])
output = pathlib.Path(sys.argv[2])
records = json.loads(source.read_text(encoding="utf-8"))

factions = {
    "realms": "NorthernRealms", "nilfgaard": "Nilfgaard", "scoiatael": "Scoiatael",
    "monsters": "Monsters", "skellige": "Skellige"
}

def cpp_string(value):
    return json.dumps(str(value), ensure_ascii=True)

def slug(value):
    raw = unicodedata.normalize("NFKD", value).encode("ascii", "ignore").decode().lower()
    return re.sub(r"[^a-z0-9]+", "_", raw).strip("_") or "card"

def enum_kind(record, ability, row):
    if row == "leader":
        return "Leader"
    if record["deck"] == "weather":
        return "ClearWeather" if "clear" in ability else "Weather"
    if record["deck"] == "special":
        for tag, kind in (("decoy", "Decoy"), ("scorch", "Scorch"),
                          ("horn", "CommanderHorn"), ("mardroeme", "Mardroeme")):
            if tag in ability:
                return kind
    return "Unit"

cards = []
for index, item in enumerate(records):
    name = item["name"].strip()
    ability = item["ability"].split()
    source_row = item["row"]
    row = {"close": "Melee", "ranged": "Ranged", "siege": "Siege"}.get(source_row, "None")
    agile = source_row == "agile" or "agile" in ability
    faction = factions.get(item["deck"], "Neutral")
    kind = enum_kind(item, ability, source_row)
    copies = int(item["count"])
    group = ""
    if "bond" in ability:
        group = name
    elif "muster" in ability:
        group = item.get("muster") or name.split(" - ", 1)[0]
    instances = max(copies, 1)
    for copy_no in range(1, instances + 1):
        card_id = f"ref_{index}_{slug(name)}_{copy_no}"
        power = int(item["strength"]) if item["strength"] else 0
        fields = [
            cpp_string(card_id), cpp_string(name), f"Kind::{kind}", f"Row::{row}",
            f"Faction::{faction}", str(power),
            "true" if "hero" in ability else "false",
            "true" if "spy" in ability else "false",
            "true" if "medic" in ability else "false",
            "true" if agile else "false",
            "true" if "morale" in ability else "false",
            "true" if "bond" in ability else "false",
            "true" if "muster" in ability else "false",
            "true" if "berserker" in ability else "false",
            cpp_string(group), "{" + ", ".join(cpp_string(tag) for tag in ability) + "}",
            str(index), str(copies)
        ]
        cards.append("    Card{" + ", ".join(fields) + "}")

output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(
    '#include "gwent/catalog.hpp"\n'
    'namespace gwent {\n'
    'const std::vector<Card>& referenceCardCatalog() {\n'
    '  static const std::vector<Card> cards = {\n' + ",\n".join(cards) + '\n  };\n'
    '  return cards;\n}\n}\n',
    encoding="utf-8"
)
