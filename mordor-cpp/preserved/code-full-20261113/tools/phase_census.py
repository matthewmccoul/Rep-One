"""Phase 1: state census. One ant per SRD part lists the game state and rule functions it needs."""
import asyncio, json, os, sys
sys.path.insert(0, os.path.dirname(__file__))
from cant import ROOT, run_all
SYS = """You read one part of a rules document (SRD) for a dungeon RPG that will be implemented in C++ with shared plain-data structs. List every piece of game STATE this part reads or writes, and every rule FUNCTION it describes. Use the names the text uses (CurHits, DeathState, AilmentSteps...).
Entities: Character (a player character), Companion (a monster bound to a character), Creature (a monster instance in an encounter or square), Item (an item instance), Party, Square (one dungeon square), Floor (a dungeon level), Game (anything global: clocks, options, flags). Static tables already exist as GameData (races, guilds, spells, items, monsters, floors: fields exactly as the data file: race min,max,size,alignMask,bonusPoints,maxAge,expFactor,resistances; guild reqStats,avgHits,hitsCapLevel,lateHits,adEvery,adMaxLevel,attPerLevel,defPerLevel,goldFactor,itemUseFactor,abilities,expFactor,questPercent,spellRate,spellMax; spell level,category,effect,required,...; item att,def,price,swings,type,guilds,statsRequired,statsMod,cursed,...; monster att,def,hits,levelFound,resistances,stats,...). Do not list static table fields.
Output exactly two sections and nothing else:
STATE
Entity.fieldName | type (i32, i64, bool, Str, Arr<T,N>, Vec<T> or an enum) | meaning, range, default | rule ref
FUNCS
Name(args) -> result | one-line purpose | rule ref
One line per item; be exact about sizes (e.g. 40 pack slots) and units."""
parts = json.load(open(f"{ROOT}/spec/parts.json"))
tasks = []
for p in parts:
    out = f"spec/census/{p['id']}.txt"
    if os.path.exists(f"{ROOT}/{out}"): continue
    tasks.append({"id": p["id"], "system": SYS, "user": open(f"{ROOT}/spec/parts/{p['id']}.md").read(), "save": out})
print(len(tasks), "census ants")
asyncio.run(run_all(tasks, "census", 6))
