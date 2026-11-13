"""Phase 2: spine. One ant per entity turns the merged census lines into include/mordor/state_<entity>.h"""
import asyncio, glob, os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
from cant import ROOT, run_all, est_tokens
CONV = open(f"{ROOT}/spec/CONVENTIONS.md").read()
ENT = {
 "item":      (["Item"], "state_item.h", "struct Item: one item instance (an entry of a pack, a chest, a shop shelf, the ground). Static item data stays in GameData; keep only instance state (item table index, identification level, charges, curse flags, ...)."),
 "companion": (["Companion"], "state_companion.h", "struct Companion: a monster bound to a character (4 slots per character). Static monster data stays in GameData; keep instance state (creature table index `ref`, -1 = empty slot, hits, binding, ...)."),
 "character": (["Character", "GuildMembership", "Membership"], "state_character.h", "struct Character: a player character. #include state_item.h and state_companion.h. It owns `Arr<Item,40> pack` and `Arr<Companion,4> companions` (use the pack size and layout the SRD gives). Group related fields in small nested structs (Vitals, Ailments, GuildMembership, Lost state...) when the SRD groups them."),
 "creature":  (["Creature", "Group", "Encounter", "Combat"], "state_creature.h", "struct Creature (one monster instance in an encounter), struct MonsterGroup (front member plus members, hits rolled on load), struct Encounter (the groups of one fight, aim, round state if the SRD gives any)."),
 "party":     (["Party"], "state_party.h", "struct Party: #include state_character.h. Up to 4 members (`Arr<Character,4> members; i32 size`), leader = first, plus party-level state the SRD gives (gold pooling flags, carried bodies, position if it belongs here)."),
 "dungeon":   (["Square", "Floor", "Area", "Teleporter", "Chute"], "state_dungeon.h", "struct SquareState (mutable per-square state: automap/seen flags, room record with chest, lair, respawn timer...) and struct FloorState (a level's mutable state: Vec<SquareState> by y*width+x, area records, counters). The static map comes from GameData FloorDef; keep only what changes during play."),
 "game":      (["Game", "Library", "LibRecord", "HallRecord", "GuildLogEvent", "LogEntry", "GuildMaster", "GuildStatus", "Guild", "Slot"], "state_game.h", "struct GameState: #include all other state_*.h. Holds the Party, the Encounter (if fighting), `Vec<FloorState> floors`, party position and facing, clocks (ms accumulators), options flags, the town (shelves, pens, bank if not per character), the library, the hall of records (12 slots), message log settings, and every other global the SRD gives. Also `struct Options`."),
}
SYS = ("You write one C++ header from a list of state fields extracted from the SRD of a dungeon RPG. Merge duplicates (same meaning, different names), pick the SRD's names, keep types consistent, give every member a default initializer, use enums for coded values (with the SRD's value names), fixed arrays where the SRD gives a size. All gold amounts are `Currency` (i64, units of 1/10000). Plain data only: no logic beyond tiny inline helpers. Put a short comment with the rule ref on each field. If lines conflict, keep the more specific one and note `// CONFLICT:`. Do not declare static data from GameData.\n\n"
       + CONV + "\nOutput ONLY the file in exactly this form:\n=== FILE: include/mordor/<name>.h ===\n<code>\n=== END FILE ===")
tasks = []
cen = {}
for f in sorted(glob.glob(f"{ROOT}/spec/census/*.txt")):
    txt = open(f).read(); pid = os.path.basename(f)[:-4]
    m = re.search(r"STATE\n(.*?)(?:\nFUNCS|\Z)", txt, re.S)
    for ln in (m.group(1) if m else "").splitlines():
        ln = ln.strip().lstrip("-* ").strip()
        mm = re.match(r"([A-Z][A-Za-z]+)\.", ln)
        if mm: cen.setdefault(mm.group(1), set()).add(ln)
for key, (ents, fname, desc) in ENT.items():
    lines = sorted({l for e in ents for l in cen.get(e, set())})
    # Item-like entities also appear under alternative names in census output; keep exact entity prefix only
    user = f"TARGET: {desc}\nFile name: include/mordor/{fname}\n\nSTATE LINES FROM THE SRD CENSUS ({len(lines)} lines):\n" + "\n".join(lines)
    if est_tokens(SYS, user) > 32_000:   # keep the most informative lines if too many: drop duplicates by field name
        seen, keep = set(), []
        for l in lines:
            k = l.split("|")[0].strip()
            if k in seen: continue
            seen.add(k); keep.append(l)
        user = f"TARGET: {desc}\nFile name: include/mordor/{fname}\n\nSTATE LINES FROM THE SRD CENSUS ({len(keep)} lines, duplicates by field name removed):\n" + "\n".join(keep)
    if os.path.exists(f"{ROOT}/include/mordor/{fname}"): continue
    print(key, len(lines), "lines, est", est_tokens(SYS, user))
    tasks.append({"id": key, "system": SYS.replace("<name>", fname[:-2]), "user": user, "allow": [f"include/mordor/{fname}"], "save": f"spec/spine/{key}.out"})
asyncio.run(run_all(tasks, "spine", 4))
