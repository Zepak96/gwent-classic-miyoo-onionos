"""Summarize paired-seed campaign results without claiming unmeasured difficulty."""
import csv
import pathlib
import sys

source = pathlib.Path(sys.argv[1])
target = pathlib.Path(sys.argv[2])
rows = list(csv.DictReader(source.open(encoding='utf-8-sig')))
assert len(rows) == 20, 'Incomplete benchmark: expected twenty deck results'
by_faction = {}
for row in rows:
    faction, stage = row['deck'].rsplit('-', 1)
    row['stage'] = int(stage)
    for field in ('matches', 'wins', 'draws', 'losses'):
        row[field] = int(row[field])
    assert row['matches'] > 0 and row['wins'] + row['draws'] + row['losses'] == row['matches']
    row['score'] = (row['wins'] + row['draws'] / 2) / row['matches']
    by_faction.setdefault(faction, []).append(row)
assert set(by_faction) == {'realms', 'nilfgaard', 'monsters', 'skellige'}
lines = ['# Campaign draft balance report', '',
         'Each score is (wins + 0.5 × draws) / matches against the source controls.',
         'These point estimates describe this fixed test set, not universal difficulty.', '',
         '| Deck | Matches | Wins | Draws | Losses | Score |',
         '| --- | ---: | ---: | ---: | ---: | ---: |']
inversions = []
for faction in ('realms', 'nilfgaard', 'monsters', 'skellige'):
    ordered = sorted(by_faction[faction], key=lambda row: row['stage'])
    assert [row['stage'] for row in ordered] == [1, 2, 3, 4, 5]
    assert len({row['matches'] for row in ordered}) == 1
    for row in ordered:
        lines.append(f"| {row['deck']} | {row['matches']} | {row['wins']} | {row['draws']} | {row['losses']} | {row['score']:.3f} |")
    for left, right in zip(ordered, ordered[1:]):
        if right['score'] <= left['score']:
            inversions.append(f"- {left['deck']} → {right['deck']}: {left['score']:.3f} → {right['score']:.3f}; progression is not established.")
lines += ['', '## Stages requiring redesign', '']
lines += inversions or ['No non-increasing point estimates in this run. A larger independent seed set is still required before release.']
target.write_text('\n'.join(lines) + '\n', encoding='utf-8')
print(f'{len(rows)} results summarized; {len(inversions)} non-increasing stage transitions')
