import json,pathlib,sys
source=pathlib.Path(sys.argv[1]); target=pathlib.Path(sys.argv[2]); target.parent.mkdir(parents=True,exist_ok=True)
factions={'realms':'NorthernRealms','nilfgaard':'Nilfgaard','monsters':'Monsters','skellige':'Skellige'}
rows=[]
for d in json.loads(source.read_text(encoding='utf-8'))['decks']:
    entries=','.join('{%d,%d}'%tuple(c) for c in d['cards'])
    rows.append('{'+json.dumps(d['id'])+',Faction::'+factions[d['faction']]+','+str(d['leader'])+',{'+entries+'}}')
target.write_text('static const std::vector<ReferenceDeck> campaignDrafts={'+','.join(rows)+'};\n',encoding='utf-8')
