const fs = require('fs');
const vm = require('vm');
const assert = require('assert');
const path = require('path');
const root = path.resolve(__dirname, '..');
const source = fs.readFileSync(path.join(root, 'upstream/asundr-gwent-classic/gwent.js'), 'utf8');
const context = vm.createContext({ console, game: { doubleSpyPower: false }, isNumber: x => typeof x === 'number' });
vm.runInContext('class CardContainer {}\n' +
  source.slice(source.indexOf('class ControllerAI {'), source.indexOf('class Player {')) + '\n' +
  source.slice(source.indexOf('class Row extends'), source.indexOf('class Weather extends')) +
  '\nglobalThis.AI=ControllerAI;globalThis.SourceRow=Row;', context);
const records = JSON.parse(fs.readFileSync(path.join(root, 'data/cards.json'), 'utf8'));
function card(name, holder) {
  const record = records.find(c => c.name === name);
  assert(record, name);
  return {name, holder, row:record.row, basePower:Number(record.strength || 0),
    hero:record.ability.split(' ').includes('hero'), abilities:record.ability.split(' ').filter(Boolean),
    id(){return this.name;}, isUnit(){return !this.hero;}, isSpecial(){return false;}};
}
const cases=[['Blue Stripes Commando',0,4],['Blue Stripes Commando',4,12],
  ['Kaedweni Siege Expert',0,1],['Kaedweni Siege Expert',5,2],
  ['Dandelion',0,2],['Dandelion',5,7],['Isengrim Faoiltiarna',5,11]];
for(const [name, allyPower, expected] of cases) {
  const owner={}; const candidate=card(name,owner); const row=Object.create(context.SourceRow.prototype);
  row.cards=[];row.effects={weather:false,halfWeather:false,bond:{},morale:0,horn:0,mardroeme:0};
  row.findCards=predicate=>row.cards.filter(predicate);
  if(allyPower) {
    let ally;
    if(candidate.abilities.includes('bond'))ally=card(name,owner);
    else ally={holder:owner,basePower:allyPower,hero:false,abilities:[],id:()=> 'control',isUnit:()=>true};
    row.cards.push(ally);row.updateState(ally,true);
  }
  const ai=new context.AI(owner);
  const weight=ai.weightRowChange(candidate,row);
  assert.strictEqual(weight,expected,`${name}, ally ${allyPower}`);
  assert.strictEqual(row.cards.length,allyPower?1:0,'source weight does not insert candidate');
  console.log(`${name}, ally=${allyPower}: source weight=${weight}`);
}
for(const [health,difference,expected] of [[2,10,10],[2,31,100],[1,31,0]]) {
  const ai=new context.AI({health,total:0,handsize:4,opponent:()=>({total:difference,handsize:4})});
  assert.strictEqual(ai.weightPass(),expected);
  console.log(`4 cards, health=${health}, behind=${difference}: source pass weight=${expected}`);
}
