"""Draft legal campaign recipes; strength order requires match-based balancing."""
import collections
import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
cards = json.loads((ROOT / 'data/cards.json').read_text(encoding='utf-8'))
leaders = {'realms': [22, 23, 24, 24, 24], 'nilfgaard': [56, 57, 56, 59, 59],
           'monsters': [96, 96, 93, 94, 93], 'skellige': [212, 212, 211, 212, 211]}
# Explicit per-faction priorities are draft composition preferences, not proven AI levels.
priorities = {'realms': {'bond': 4, 'medic': 6, 'spy': 7, 'morale': 2},
              'nilfgaard': {'spy': 9, 'medic': 7, 'bond': 3},
              'monsters': {'muster': 7, 'morale': 3, 'scorch_c': 4},
              'skellige': {'bond': 5, 'berserker': 4, 'mardroeme': 5, 'medic': 5, 'muster': 5}}
recipes = []
for faction, leader_ids in leaders.items():
    pool = [(i, copy) for i, c in enumerate(cards)
            if c['deck'] in (faction, 'neutral') and c['row'] in ('close', 'ranged', 'siege', 'agile')
            for copy in range(int(c['count']))]
    for level, leader_id in enumerate(leader_ids):
        def rating(item):
            c = cards[item[0]]
            ability = c['ability'].split()
            synergy = sum(priorities[faction].get(a, 0) for a in ability)
            return int(c['strength'] or 0) + level * synergy / 4 + (level * 2 if 'hero' in ability else 0)
        ordered = sorted(pool, key=lambda item: (rating(item), cards[item[0]]['name'], item))
        units = [24, 23, 22, 22, 22][level]
        start = round((len(ordered) - units) * level / 4)
        chosen = ordered[start:start + units]
        if faction == 'realms' and level == 4:
            # Preserve full draw/return engines and worthwhile Bond sets rather
            # than letting low printed Spy power lose to unrelated high units.
            required = [item for item in ordered if
                        ('hero' not in cards[item[0]]['ability'].split() and
                         ('spy' in cards[item[0]]['ability'].split() or
                          'medic' in cards[item[0]]['ability'].split() or
                          ('bond' in cards[item[0]]['ability'].split() and
                           int(cards[item[0]]['strength'] or 0) >= 4)))]
            assert len(required) <= units
            chosen = required + [item for item in reversed(ordered) if item not in required][:units-len(required)]
        counts = collections.Counter(i for i, _ in chosen)
        # Avoid flooding weak decks with specials; concentrated advanced decks get utility.
        specials = [[4, 1], [5, 1]] if level == 0 else [[1, 1], [5, 1], [2, 1]]
        # Foltest King searches the remaining deck for Fog. Preserve one
        # legal target instead of generating a permanently unusable leader.
        if faction == 'realms' and level == 0:
            specials = [[9, 1], [5, 1]]
        if level in (2, 3):
            specials.append([10, 1])
        if faction == 'skellige' and level >= 2 and any('berserker' in cards[i]['ability'] for i in counts):
            specials[-1] = [202, 1]
        for index, count in specials:
            counts[index] += count
        entries = sorted(counts.items())
        unit_count = special_count = 0
        for index, count in entries:
            c = cards[index]
            assert 0 < count <= int(c['count'])
            assert c['deck'] in (faction, 'neutral', 'special', 'weather')
            assert c['row'] != 'leader'
            if c['row'] in ('close', 'ranged', 'siege', 'agile'):
                unit_count += count
            else:
                special_count += count
        assert unit_count >= 22 and special_count <= 10
        assert cards[leader_id]['row'] == 'leader' and cards[leader_id]['deck'] == faction
        recipes.append({'id': f'{faction}-{level+1}', 'faction': faction, 'stage': level+1,
                        'leader': leader_id, 'cards': entries, 'units': unit_count,
                        'specials': special_count, 'balance_status': 'unverified-draft'})
# Training benchmark 43908517238 found two Skellige inversions. Preserve the
# five compositions but order them by measured score. This is provisional
# until the independent holdout and newer engine revision both agree.
skellige = [d for d in recipes if d['faction'] == 'skellige']
recipes = [d for d in recipes if d['faction'] != 'skellige']
for stage, source_stage in enumerate((4, 2, 1, 3, 5), 1):
    deck = dict(skellige[source_stage-1])
    deck['id'] = f'skellige-{stage}'
    deck['stage'] = stage
    deck['composition_origin_stage'] = source_stage
    recipes.append(deck)
output = ROOT / 'data/campaign-decks-draft.json'
output.write_text(json.dumps({'schema_version': 1, 'status': 'not-enabled-in-game', 'decks': recipes},
                             ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(f'{len(recipes)} draft recipes: faction, leader, copies, units and specials checks passed')
