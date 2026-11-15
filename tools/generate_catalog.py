import json
import pathlib
import re
import sys
import unicodedata

source = pathlib.Path(sys.argv[1])
premade_source = pathlib.Path(sys.argv[2])
ability_source = pathlib.Path(sys.argv[3])
output = pathlib.Path(sys.argv[4])
records = json.loads(source.read_text(encoding="utf-8"))
premade = json.loads(premade_source.read_text(encoding="utf-8"))
ability_text = json.loads(ability_source.read_text(encoding="utf-8"))

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

def card_description(record):
    ability = record["ability"].split()
    hero = bool(ability and ability[0] == "hero")
    if hero:
        ability = ability[1:]
    parts = []
    if record["row"] == "agile":
        parts.append(ability_text.get("agile", {}).get("description", ""))
    parts.extend(ability_text.get(tag, {}).get("description", "") for tag in reversed(ability))
    if hero:
        parts.append(ability_text.get("hero", {}).get("description", ""))
    return "".join(parts).strip()

def card_ability_title(record):
    ability = record["ability"].split()
    hero = bool(ability and ability[0] == "hero")
    if hero:
        ability = ability[1:]
    if record["row"] == "leader":
        return "Leader Ability"
    if ability:
        return ability_text.get(ability[-1], {}).get("name", ability[-1])
    if record["row"] == "agile":
        return ability_text.get("agile", {}).get("name", "agile")
    if hero:
        return ability_text.get("hero", {}).get("name", "hero")
    return ""

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
    '  return cards;\n}\n'
    'const std::vector<ReferenceDeck>& referencePremadeDecks() {\n'
    '  static const std::vector<ReferenceDeck> decks = {\n' + ",\n".join(
        '    ReferenceDeck{' + cpp_string(f"reference_{i + 1}") + ', Faction::' + factions[item["faction"]] +
        ', ' + str(item["leader"]) + ', {' + ', '.join('{' + str(index) + ', ' + str(count) + '}' for index, count in item["cards"]) + '}}'
        for i, item in enumerate(premade)
    ) + '\n  };\n'
    '  return decks;\n}\n'
    'const std::string& referenceCardImage(int sourceIndex) {\n'
    '  static const std::vector<std::string> images = {\n' + ",\n".join(
        '    ' + cpp_string(item["deck"] + "_" + item["filename"] + ".jpg")
        for item in records
    ) + '\n  };\n'
    '  static const std::string empty;\n'
    '  return sourceIndex >= 0 && sourceIndex < static_cast<int>(images.size()) ? images[static_cast<std::size_t>(sourceIndex)] : empty;\n'
    '}\n'
    'const std::string& referenceCardAbilityTitle(int sourceIndex) {\n'
    '  static const std::vector<std::string> titles = {\n' + ",\n".join(
        '    ' + cpp_string(card_ability_title(item)) for item in records
    ) + '\n  };\n'
    '  static const std::string empty;\n'
    '  return sourceIndex >= 0 && sourceIndex < static_cast<int>(titles.size()) ? titles[static_cast<std::size_t>(sourceIndex)] : empty;\n'
    '}\n'
    'const std::string& referenceCardDescription(int sourceIndex) {\n'
    '  static const std::vector<std::string> descriptions = {\n' + ",\n".join(
        '    ' + cpp_string(card_description(item)) for item in records
    ) + '\n  };\n'
    '  static const std::string empty;\n'
    '  return sourceIndex >= 0 && sourceIndex < static_cast<int>(descriptions.size()) ? descriptions[static_cast<std::size_t>(sourceIndex)] : empty;\n'
    '}\n}\n',
    encoding="utf-8"
)
