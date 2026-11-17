import json,pathlib,sys
recipes=json.loads(pathlib.Path(sys.argv[1]).read_text(encoding='utf-8-sig'))['decks']
people=json.loads(pathlib.Path(sys.argv[2]).read_text(encoding='utf-8-sig'))['opponents']
by_id={x['id']:x for x in people}
assert len(by_id)==len(people)==len(recipes)==20
factions={'realms':'NorthernRealms','nilfgaard':'Nilfgaard','monsters':'Monsters','skellige':'Skellige'}
assert set(by_id)=={d['id'] for d in recipes}
assert {(d['faction'],d['stage']) for d in recipes}=={(f,s) for f in factions for s in range(1,6)}
quote=lambda x:json.dumps(x,ensure_ascii=True)
rows=[]
for d in recipes:
    o=by_id[d['id']]
    assert o.get('faction',d['faction'])==d['faction'] and o.get('stage',d['stage'])==d['stage']
    assert d['id']==f"{d['faction']}-{d['stage']}"
    for key in ('name_pl','name_en','intro_pl','intro_en','portrait'): assert isinstance(o[key],str) and o[key]
    cards=','.join('{%d,%d}'%tuple(pair) for pair in d['cards'])
    faction='Faction::'+factions[d['faction']]
    recipe='{'+quote(d['id'])+','+faction+','+str(d['leader'])+',{'+cards+'}}'
    rows.append('{'+','.join([quote(d['id']),faction,str(d['stage'])]+[quote(o[k]) for k in ('name_pl','name_en','intro_pl','intro_en','portrait')]+[recipe])+'}')
target=pathlib.Path(sys.argv[3]);target.parent.mkdir(parents=True,exist_ok=True)
target.write_text('#include "gwent/campaign.hpp"\nnamespace gwent {\nconst std::vector<CampaignNode>& campaignNodes(){\nstatic const std::vector<CampaignNode> nodes={'+','.join(rows)+'};\nreturn nodes;\n}\n}\n',encoding='utf-8')
