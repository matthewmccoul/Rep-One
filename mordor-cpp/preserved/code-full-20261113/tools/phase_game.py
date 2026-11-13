"""Phase 7: game layer. Glue ants implement include/mordor/game.h in src/game_*.cpp by calling the rules modules. usage: phase_game.py [task ...]"""
import asyncio, glob, json, os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
from cant import ROOT, run_all, est_tokens
import api_index
from partsplit import condense, header_view
CONV = open(f"{ROOT}/spec/CONVENTIONS.md").read()
GAME_H = open(f"{ROOT}/include/mordor/game.h").read()
SYS = ("You are a glue-code ant. The rules of a dungeon RPG are implemented as ~70 C++ modules (headers in include/mordor). You implement part of the GAME LAYER declared in include/mordor/game.h: "
       "thin orchestration that calls the existing rules functions listed in the API INDEX and the state structs shown, following the SPEC given. Prefer calling the rules modules over re-implementing rules; "
       "where several modules declare similar functions choose the one whose signature matches the data you have (read the signatures carefully) and never invent functions or struct members that are not shown: if something you need does not exist, implement the small piece locally (static helper) or reduce the feature and add `// SIMPLIFIED:`. "
       "Randomness only through ctx.rng, text through the returned Str (and ctx.say events). No I/O, no globals. Define every function you are assigned exactly once, with exactly the signature in game.h. Keep the file under 800 lines.\n\n" + CONV +
       "\nOutput ONLY files in this form:\n=== FILE: src/<name>.cpp ===\n<code>\n=== END FILE ===")
TASKS = {
 "setup": dict(files="src/game_setup.cpp", funcs="sessionInit, defaultStats, creationPointsLeft, createCharacter, currentScreen",
   srd=["c02p1", "c03p1"], words="createBase creationPool raiseStat recalcSP maxSP joinGuild hitsOnLevelUp levelThresholdXp newGame clockSetup recalcSpellPoints performJoin joinParty eligibleToJoin statCeiling generateBaseStat",
   spec="sessionInit: parse the JSON with loadGameDataFromJson, reseed s.rng, reset s.game to GameState(), build s.game.floors (one FloorState per GameData floor: dungeonLevel=levelNum, squares sized width*height from the floor fields, areas copied from FloorDef areas; use whatever FloorState/SquareState/Area members really exist), call the module that initialises a new game and the clocks if one exists (newGame, clockSetup). "
        "defaultStats: min(race.max[i], race.min[i]+5) per stat. creationPointsLeft: SRD ch.2 R3 (30 + bonusPoints - sum(stat - min)). "
        "createCharacter: validate name (non-empty, no '|'), race/guild/alignment allowed (races.h guildAllowed, alignmentAllowed or the GameData masks), stats within race min..max and creation points >= 0, party not full (4); then build the Character exactly per SRD ch.2 R5: stats, MaxHits=CurHits=15, one guild membership of the chosen guild at level 1, experience 0, 1500 gold on hand, age 16 years (kAgeStartDays), combat setting 1, area -1, location dungeonLevel 0, then recalc spell points with refill, set xpRequired; apply the guild join (joinGuild or the lower-level pieces) only if it can be done without extra state; append to s.game.party (members[size++], size++). "
        "currentScreen: GameOver if every party member is dead and size>0; Combat if game.inCombat; Dungeon if the party's dungeonLevel > 0; Town if size>0 and in town; NewGame if the party is empty."),
 "view": dict(files="src/game_view.cpp", funcs="describeParty, describeCharacter, describeSquare, renderAutomap, describeFight",
   srd=["c15p1"], words="statWithMods totalAD combatAD effectiveStat guildTitle townSwitchOrder squareIsRock classifyEdge canMoveFacing mapping explored automap markExplored facing monsterName",
   spec="Pure text rendering of the state, no side effects. describeParty: one line per member: index, name, race name (GameData races), active guild name and level, HP cur/max, SP cur/max, status words (dead/stoned/poisoned/diseased/paralysed if the state says so), gold on hand. "
        "describeCharacter: name, race, alignment, age in years, guilds with levels and experience, six stats (natural and with mods via statWithMods/effectiveStat), attack/defence (totalAD/combatAD if signatures fit), resistances if easily available, pack items by name (GameData items) with slot numbers, companions. "
        "describeSquare: in town (party dungeonLevel 0) a menu listing the town actions of game.h; in the dungeon: level, x,y, facing name (N/E/S/W), the four edges (wall/door/open using classifyEdge or the floor flag bits FLAG_WALL_*/FLAG_DOOR_*), and notable features on the square (stairs up/down, pit, teleporter, chute, chest) from the FloorDef field flags. "
        "renderAutomap: ASCII grid of radius r around the party on the current floor: '#' wall, '+' door, '.' known open square, ' ' unknown, '^'/'v' stairs, '@' the party with an arrow for facing; use the floor's SquareState seen/explored flags if they exist, else show squares within 1 of the party. "
        "describeFight: list each monster group of game.encounter with count and name (GameData monsters via ref/index) and which group is aimed at."),
 "town": dict(files="src/game_town.cpp", funcs="townAction",
   srd=["c16p1", "c16p2"], words="restInCity arriveInTown makeLevel joinGuild guildJoinCost goldForNextLevel storeSale storeBuy withdrawFromVault townCharge coverShortfall templeMorgueFill poolGoldToSelected partyDepositGold identifyFee uncurse levelUpGoldCost canLevelUp raise",
   spec="townAction parses `command` (first word + args, 0-based or 1-based member indices as written in game.h, document in the reply) and performs: rest (restInCity for every member, report days aged and hits restored), guild (list each member's guilds with the xp needed/gold for the next level via the guild functions), levelup <member> (makeLevel/levelUp for the active guild with the proper checks; report success/refusal reason), join <member> <guild>, "
        "shop (list the store stock with prices from GameData items price), buy <n> <member> (charge via townCharge or reduce the member's gold on hand; put the item in the first free pack slot 1..39), sell <slot> <member> (sale price rule from ch.16: half of price, via storeSale if the signature fits), bank deposit/withdraw <gold> (member 0's wallet goldOnHand/goldInBank in Currency units: gold*10000), equip/unequip <member> <slot> (use the items module equip functions), identify, morgue/raise (list dead members; raise via the death module if available), pool (pool gold), descend (leave town to dungeon level 1: set party location, call arriveInTown/leaveArea counterparts as needed). "
        "Unknown command: return false. Always fill `reply`. If a feature's module call cannot be made with the data at hand, do the simple arithmetic yourself and add `// SIMPLIFIED:`."),
 "dungeon": dict(files="src/game_dungeon.cpp", funcs="dungeonAction, plus a static generateEncounterFor(...) helper",
   srd=["c15p1", "c11p1"], words="applyMoveKey tryMoveForward canMoveFacing classifyEdge applySquareEffects perceiveSquare markExplored mapSquareOnAutomap takeChute applyPitFall rollLair rollGroupCount rollGroupSize rollMonsterHits rollEncounterSpread startFight onMove engagementTick hostileRoll climbToTownAlive openChest rollMonsterDrops generateEncounterChest",
   spec="dungeonAction: forward/back (tryMoveForward / applyMoveKey with MoveKey::Up/Down), left/right turn (applyMoveKey MoveKey::Left/Right), then run the arrival effects: applySquareEffects (pits, chutes, teleporters, stairs), markExplored/mapSquareOnAutomap, and the ENCOUNTER roll (SRD ch.11 R7 and ch.15): if the square's area id > 0, not rock, and no monsters here, roll the lair/class/monster/group count/size/hits using the roll* functions and GameData monsters (levelFound window, chance), fill game.encounter and the square's chest record (generateEncounterChest), set game.inCombat via startFight and report the monsters met. "
        "stairs: use stairs up/down when the square has them (level change via leaveArea/arrive; climbing to level 0 is climbToTownAlive); open: open a chest on the square if one exists (openChest) and report loot; search: perceiveSquare; pool: poolGoldToSelected; map: return renderAutomap-equivalent is NOT needed (the front end calls renderAutomap). "
        "After every successful move advance time by 1000 ms with advanceTime. Report what happened in `reply`. Unknown command: return false."),
 "combat": dict(files="src/game_combat.cpp", funcs="combatAction, advanceTime",
   srd=["c06p1", "c07p1"], words="combatTick startFight endFight endCombat playerRound characterAct playClockTick ailmentTick orderFromSetting retarget meleeHold payFightGold giveExperience markRoomCleared onPartyWiped removePartyMember openChestInFight",
   spec="combatAction: parse fight [group] | defend | cast <member> <spell> [group] | use <member> <slot> | open. Set every living member's standing order (game.party members' `order` / combat setting: Fight with the aimed group, Defend, or Spell) then run ONE combat round with combatTick (SRD ch.6: the round walks the party, then companions, then monsters). If the fight ends (no foes) call endFight/endCombat so gold, XP and chests are settled and game.inCombat is cleared; if the whole party is dead, set the game over state (onPartyWiped). Fill `reply` from the events produced since the call began (s.log.events: join their `text` fields) plus a one-line summary. "
        "advanceTime(s, ms): accumulate ms; every 1000 ms call combatTick only when in combat, every 250 ms the play clock tick (playClockTick), every 4000 ms ailmentTick; keep the remainders in game state if a member exists for them, else in a static-free way (use the GameState clock members)."),
 "save": dict(files="src/game_save.cpp", funcs="saveSession, loadSession",
   srd=["c20p1"], words="save character stat file format MDATA4 import",
   spec="A simple, human-readable, lossless-enough text format (line based `key=value`, lists as `a,b,c`) for the mutable state: the RNG is NOT saved; save every member's name, race, sex, alignment, active guild, level/experience/age, vitals, wallet, stats, per-guild memberships, pack item ids/charges/flags, companions, ailments, location; party size; the clocks; for each floor the explored/seen flags and area respawn data you can reach. loadSession parses it back into a fresh Session state (keep data tables), returning false+err on a malformed text. Round-trip must be exact for what you save. Only use members that exist; list in a comment what is NOT saved."),
}
want = sys.argv[1:] or list(TASKS)
spine = api_index.spine_digest(include_game=True)
tasks = []
for k in want:
    t = TASKS[k]
    if os.path.exists(f"{ROOT}/{t['files'].split(',')[0].strip()}") and not os.environ.get("FORCE"): continue
    srd = "\n\n".join(condense(open(f"{ROOT}/spec/parts/{p}.md").read()) for p in t["srd"])
    idx = api_index.relevant_index(t["words"] + " " + t["spec"], cap=26000)
    def mk(srd_text, idx_text):
        return f"TASK: implement {t['funcs']} in {t['files']}\n\nSPEC\n{t['spec']}\n\nGAME LAYER HEADER include/mordor/game.h\n{header_view(GAME_H)}\n\nSTATE STRUCTS\n{spine}\n\nAPI INDEX (rules modules)\n{idx_text}\n\nSRD CONTEXT\n{srd_text}"
    user = mk(srd, idx)
    while est_tokens(SYS, user) > 31_000 and len(srd) > 4000:   # shrink SRD context first, then the index
        srd = srd[: int(len(srd) * 0.8)]; user = mk(srd, idx)
    while est_tokens(SYS, user) > 31_000 and len(idx) > 6000:
        idx = idx[: int(len(idx) * 0.85)]; user = mk(srd, idx)
    print(k, "est", est_tokens(SYS, user))
    tasks.append({"id": k, "system": SYS, "user": user, "allow": [f"src/game_{k}", "src/game_"], "save": f"spec/game/{k}.out"})
if os.environ.get("DRY"): sys.exit(0)
asyncio.run(run_all(tasks, "game", 3))
