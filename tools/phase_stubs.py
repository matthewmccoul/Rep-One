"""Implement stubbed / placeholder functions faithfully from the reference C# and the SRD (gated).
usage: phase_stubs.py <round> [stub ids...]   (GATE=1 recommended; DRY=1 writes packets only)"""
import asyncio, json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
args = sys.argv[1:]; sys.argv = sys.argv[:1]
import phase_investigate as pi
from cant import ROOT, run_all, est_tokens
from triage import find_function
STUBS = {
 "store_stock": dict(file="src/city3a.cpp", funcs=["storeItemDraw", "restockStore", "replenishStoreAfterPurchase"], cs=["Restock", "Replenish", "StoreStock", "Draw"], srd=["restock", "replenish", "store stock", "shelf", "shelves"], decls=["Shelf", "ItemDef", "TownStock", "Ctx"],
     note="Store stock: draw an item for a floor, restock the shelves from the dungeon floors, replenish after a purchase. Implement faithfully from the reference StoreStock."),
 "library_rec": dict(file="src/city3a.cpp", funcs=["libraryRestockRecord", "mergeLegacyLibraryRecord"], cs=["Librarian", "Restock", "Merge", "Legacy"], srd=["library", "legacy", "merge", "restock"], decls=["LibraryRecord", "IdLevel"],
     note="Library record restock and legacy merge; faithful from the reference Librarian."),
 "companion_pick": dict(file="src/monsters1a.cpp", funcs=["pickCompanionMonster"], cs=["CompanionType", "CompanionSpawnMode", "CompanionId", "Companion"], srd=["companion chain", "companion monster", "spawn mode", "R21"], decls=["CompanionChain", "CompanionPick", "MonsterDef"],
     note="Encounter companion chain: pick the companion monster for a leader from its companion type, spawn mode and id; the stub always restarts the encounter."),
 "modding": dict(file="src/modding_missing.cpp", funcs=["modDisplayTitle", "declaredToggleDefault", "resolveChoice"], cs=["DisplayTitle", "Toggle", "Choice", "Manifest"], srd=["mod manifest", "toggle", "choice", "display title"], decls=["ModRef", "ModTag"],
     note="Modding API routines that were left neutral: display title of a manifest, declared default of a toggle, resolve a stored choice. The first is in modding_missing.cpp, the others in modding_missing2.cpp."),
 "guild_placeholders": dict(file="src/guilds1_a.cpp", funcs=["guildExperiencePenalty", "raceExpMod", "guildGoldParameter", "spellRate"], cs=["ExpFactor", "ExpMod", "GoldFactor", "Rate", "Penalty"], srd=["experience penalty", "race modifier", "gold parameter", "spell rate", "RACE_MOD", "GGF"], decls=["GuildDef", "RaceDef", "SpellDef"],
     note="Placeholder accessors in guilds1_a.cpp that return neutral values (guild experience penalty, race experience modifier, guild gold parameter, spell rate). The data file has the real fields (see the types); wire each accessor to the right field and rule from the reference."),
 "casting_accessors": dict(file="src/casting2.cpp", funcs=["guildRate", "castUseOf", "castCostOf", "castCurSpellPointsOf", "castAntiMagicOf", "castExtinguisherOf", "castCombatSpellOf", "castSpellLevelOf"], cs=["CanCast", "SpellUse", "AntiMagic", "Extinguisher", "SpellRate"], srd=["anti-magic", "extinguisher", "combat spell", "spell points", "cast refusal"], decls=["CastSituation", "GuildDef", "SpellDef", "Ctx"],
     note="Accessors in casting2.cpp that return placeholders because CastSituation and GuildDef lack some members. Decide, from the reference, where each value really comes from (the spell table, the character, the square) and implement the accessors, or say what the caller must pass."),
 "chest_traps": dict(file="src/loot1a.cpp", funcs=["resolveChestTrap"], cs=["Spring", "ChestTraps", "Destructible", "ResistancePercent", "ClearSpellEffects", "AgeBy"], srd=["chest trap", "poison gas", "disease spores", "exploding", "fire trap", "slime", "fate", "blackout", "fear", "withering", "trap 7"], decls=["ChestTrapResult", "ChestTrap", "Party", "Character", "Companion", "Ailments"],
     note="Implement ALL ten chest traps of the reference ChestTraps.Spring in loot1a.cpp resolveChestTrap(ctx, party, openerIndex, trap): 1 poison gas and 2 disease spores (already derived-resistance, via chestAfflictParty), 3 exploding and 4 fire: EVERY party member takes (int)(Rnd*(level*mult*ln(level+2)))+1 with mult 4 (explosion) or 6 (fire; fire reduces by ResistancePercent(fire)/2*0.01 of the damage), CurHits = round_half_even(CurHits - dmg), level = the opener's dungeon level; 5 slime: destroys one pack item per the reference (slot roll, Destructible); 6 fate: rerolls hits and spell points; 7 teleporter: a random non-rock non-teleporter square on the level (set r.teleported and the party position through what is available); 8 blackout: clears running spell effects; 9 fear: companions not bound for life leave; 10 withering: the opener ages 28 days. Finally every member's hits below 1 become 0. The old body used a constant d = 0 and only hurt the opener."),
}
SYS = ("You implement stubbed or placeholder C++ functions faithfully. You get an EVIDENCE PACKET: our current stub source, the type definitions it uses, the REFERENCE C# implementation (ground truth) and the SRD paragraphs. "
       "Write complete replacement functions that do what the reference does (same order of random draws, same rounding), using ONLY members and functions shown in the packet. Keep every signature. No I/O, globals or exceptions. "
       "If you lack a type or function you need, give no guess for that function; end with one line `NEED: name1, name2` (the next round includes them). Output one block per function:\n"
       "=== REPLACE <file> :: <function> ===\n<complete function>\n=== END REPLACE ===\n"
       "then `STATUS: IMPLEMENTED | PARTIAL | BLOCKED` and one line per function saying what is faithful and what is still unknown, and optionally\n=== ERRATUM ch<NN> ===\n<text>\n=== END ERRATUM ===\nwhen the SRD is wrong or silent about something you had to take from the reference.\n\n" + pi.CONV)
rnd = args[0]; only = args[1:]
pf = f"{ROOT}/logs/stubs_round{int(rnd)-1}.json"; prev = json.load(open(pf)) if int(rnd) > 1 and os.path.exists(pf) else {}
import phase_fixtests as pfx
fidx = pfx.src_function_index(); tasks = []
for sid, s in STUBS.items():
    if only and sid not in only: continue
    srcs = []
    for fn in s["funcs"]:
        for f in [s["file"], "src/modding_missing2.cpp"]:
            p = f"{ROOT}/{f}"
            if os.path.exists(p):
                t = open(p).read(); loc = find_function(t, fn)
                if loc: srcs.append(f"// ---- {f} :: {fn}\n{t[loc[0]:loc[1]]}"); break
    head = open(f"{ROOT}/{s['file']}").read().split("namespace mordor")[0][:2500]
    need_names = []
    if sid in prev:
        need_names = pi.need_tokens(prev[sid].get("answer", ""))
    names_f = [x for x in need_names if x in fidx][:12]
    def packet(b):
        parts = [f"STUB {sid}: {s['note']}", "FILE HEADER (what the earlier author says is blocked)\n" + head, "OUR STUBS\n" + "\n\n".join(srcs),
                 "TYPES\n" + pi.decl_blocks(s["decls"] + need_names, b["decl"]),
                 "REFERENCE C# (ground truth)\n" + pi.cs_members(s["cs"], "src/Mordor.Core", b["cs"]) + "\n\n" + pi.cs_rank(pi.tokens_of(" ".join(s["cs"]), s["note"]), b["cs2"], 2),
                 "OUR RELATED CODE\n" + (pi.cpp_code(names_f, b["cpp"]) if names_f else "(none requested)"),
                 "SRD PARAGRAPHS\n" + (pi.srd_hits(s["srd"], b["srd"]) or pi.srd_rank(pi.tokens_of(" ".join(s["srd"])), b["srd"]))]
        if sid in prev: parts.append("PREVIOUS ATTEMPT\n" + prev[sid].get("answer", "")[:2500])
        return "\n\n".join(parts)
    budget = dict(decl=7000, cs=12000, cs2=5000, cpp=7000, srd=6000); user = packet(budget); k = 0
    while est_tokens(SYS, user) > 31000 and k < 12: budget = {a: int(v * .85) for a, v in budget.items()}; user = packet(budget); k += 1
    print(sid, "est", est_tokens(SYS, user), flush=True)
    if os.environ.get("DRY"): open(f"{ROOT}/spec/investigate/stubpacket_{sid}.txt", "w").write(user); continue
    tasks.append({"id": sid, "system": SYS, "user": user, "allow": [], "save": f"spec/investigate/s{rnd}_{sid}.out"})
if os.environ.get("DRY"): sys.exit(0)
asyncio.run(run_all(tasks, f"stubs{rnd}", 4))
gate = gbase = None; results = {}
for t in tasks:
    p = f"{ROOT}/{t['save']}"
    if not os.path.exists(p): continue
    out = open(p).read(); sid = t["id"]
    if os.environ.get("GATE"):
        from gate import Gate
        if gate is None: gate = Gate(); gbase = gate.run()
        gate.backup(); n = pi.apply_patches(out)
        if n:
            after = gate.build_and_run(); new = gate.new_failures(gbase, after)
            if new: gate.restore(); print(f"  GATE REVERTED {sid}: {list(new)[:5]}", flush=True); n = 0
            else: gbase = after; print(f"  gate ok {sid}: engine failing {len(after['engine'])}, srd failing {len(after['srd'])}", flush=True)
    else: n = pi.apply_patches(out)
    st = re.search(r"STATUS:\s*(\w+)", out); results[sid] = {"status": st.group(1) if st else None, "patches": n, "answer": out}
    print(f"{sid:16} {results[sid]['status']}  patches {n}")
json.dump(results, open(f"{ROOT}/logs/stubs_round{rnd}.json", "w"), indent=1)
