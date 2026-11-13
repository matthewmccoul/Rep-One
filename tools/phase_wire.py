"""Wire rules modules into the game layer, one FEATURE per ant, gated. The ant returns replacement game-layer functions and a new flow test.
usage: phase_wire.py <round> [feature ids...]   (GATE=1 recommended; DRY=1 writes packets to spec/investigate/wirepacket_<id>.txt)"""
import asyncio, json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
args = sys.argv[1:]; sys.argv = sys.argv[:1]
import phase_investigate as pi
import api_index
from cant import ROOT, run_all, est_tokens
from triage import find_function
import phase_fixtests as pfx
FEATURES = {
 "gear": dict(funcs=["src/game_town.cpp::townAction"], extra=[], words="equip unequip wield wear remove weapon armor hands slot cursed use item", cs=["Equip", "Unequip", "Wield", "CanEquip", "Hands"], srd=["equip", "unequip", "wield", "hands", "cursed"],
     note="Town commands `equip <member> <pack slot>`, `unequip <member> <slot>` and `use <member> <slot>` (consumables, charged items): equipping respects the item's hands, guild, alignment, strength and curse rules from the items modules; a cursed worn item cannot be removed; the combat A/D is recomputed (totalAD/combatAD). Add them to townAction (also usable in the dungeon via dungeonAction) with replies that name the item."),
 "services": dict(funcs=["src/game_town.cpp::doBank", "src/game_town.cpp::doIdentify", "src/game_town.cpp::doMorgue", "src/game_town.cpp::doPool"], extra=[], words="bank withdraw deposit identify fee morgue raise dead resurrect cost pool gold share", cs=["Withdraw", "Deposit", "Identify", "Raise", "Resurrect", "Morgue", "PoolGold"], srd=["bank", "withdraw", "identify", "morgue", "raise", "resurrect", "pool"],
     note="Town services: the bank (deposit and WITHDRAW with the SRD's rules, balance cap), identification (the fee list AND actually identifying an item, deducting the fee), the morgue (RAISE a dead member for the SRD's cost, with the success rule and the consequences) and pooling gold. doBank, doIdentify, doMorgue, doPool and their dispatch in townAction."),
 "shop": dict(funcs=["src/game_town.cpp::doShop", "src/game_town.cpp::doBuy", "src/game_town.cpp::doSell"], extra=[], words="store shelf buy sell price stock alignment priceOf sellPrice storeSale storeBuy charges identify row", cs=["PriceOf", "SellPrice", "StoreStock", "Count", "ShelfCap", "SoldByCharge"], srd=["store", "shelf", "price", "sell", "ShelfCap", "stock"],
     note="General store: list distinct items with rows numbered 1..N; BUY charges the reference stock-dependent price (floor-0 items at list price, others round_half_even(1.5*P/(ln(shelf+1)+1.15) - ln(charges+1)*0.05*P), shelf = min(count of that item and alignment on the shelf - 1, 9)) and removes one copy from s.town.store; SELL pays the reference SellPrice, puts the item on the shelf (s.town.store grows by one) and reports gold in whole gold with decimals if needed, never '1/10000 units'. Use the module functions that already implement these rules (city1.h, items*.h) rather than re-deriving them."),
 "magic": dict(funcs=["src/game_combat.cpp::combatAction", "src/game_combat.cpp::resolveMemberOrder", "src/game_town.cpp::townAction"], extra=[], codebudget=15000, words="cast spell spells spell points cost reachable guild level list buffer order damage heal resist castCostSp payCastCost listReachableSpellIds spellDamage spellHealing castSpell", cs=["Cast", "CastSpell", "SpellDamage", "SpellHeal", "CanCast", "SpellCost"], srd=["cast", "spell points", "spell cost", "reachable", "spell list"],
     note="Spell casting. (1) `spells <member>` in town lists the spells the member can reach (listReachableSpellIds) with their SP cost (castCostSp). (2) `cast <member> <spell name or id> [group]` works by NAME (case-insensitive, also the numeric id) in combat AND in town for non-combat spells: refuses unknown names and unreachable spells with a message and no cost; otherwise spends castCostSp (payCastCost) and applies the effect through the casting/spell modules (damage spells hit the chosen monster group via spellDamage with the monsters' resistances; healing heals the target; other effects as the modules provide). In combat the cast is that member's order for the round, and the SP are spent when the order resolves. Keep `use <member> <slot>` working."),
 "chests": dict(funcs=["src/loot2.cpp::openChest", "src/game_dungeon.cpp::dungeonAction"], extra=[], codebudget=14000, words="chest open trap gold items place disarm opener countdown applyChestTrap distributeChestGold placeChestItem openChestInFight ChestOpenResult resolveChestTrap goldSharePaid", cs=["OpenChest", "ChestTraps", "Spring", "PlaceItem", "Chest", "Disarm"], srd=["chest", "trap", "disarm", "open", "gold share", "items placed"],
     note="Opening a chest in the dungeon (`open`): after the opening countdown (a few `open` commands) the chest's trap is resolved FIRST (the trap's effect hurts / poisons / diseases / destroys an item / teleports etc. via the trap modules; the reply names the trap and what it did), then the gold is shared among the living party members and the chest's ITEMS are placed in the members' packs (placeChestItem; items that do not fit are lost with a message), and the chest is removed from the square. The reply tells the player what happened: trap, gold per member, item names. A killed opener or a trap that kills/teleports the party stops the payout. Use the loot modules (loot1a.h: openChestInFight/ChestOpenResult, placeChestItem, goldSharePaid, resolveChestTrap) rather than re-deriving rules."),
}
SYS = ("You wire existing, tested rules modules into the GAME LAYER of a C++ game (command handlers that take a Session and a command string and return a reply string). You get an EVIDENCE PACKET: the public game API, the current handlers, the relevant module declarations and state structs, the REFERENCE C# implementation (ground truth for behaviour) and the SRD paragraphs. "
       "Implement the feature completely and faithfully: call the module functions (do not re-implement rules), use the reference's order of random draws, and make the replies informative. Use ONLY functions, members and types shown in the packet. No I/O, globals or exceptions. Keep existing commands and signatures working. "
       "Output (1) one block per changed or new game-layer function, with the COMPLETE function (static helpers may be added inside the block before it):\n=== REPLACE <file> :: <function> ===\n<code>\n=== END REPLACE ===\n"
       "For a function that does not exist yet, use the same block (it is appended to the file). (2) The ACCEPTANCE TESTS in the packet are FIXED by the coordinator: make them pass by changing code only; never output test code. If a test seems wrong, say so in one line `TEST_DOUBT <name>: reason` and still implement the rest.\n"
       "(3) If you lack code you need, put `NEED: name1, name2` (functions or types of OUR code) on its own line and implement what you can. (4) `STATUS: IMPLEMENTED | PARTIAL | BLOCKED` and one line per limitation. Optionally an `=== ERRATUM ch<NN> ===` block when the SRD is wrong or silent.\n\n" + pi.CONV)
rnd = args[0]; only = args[1:]
pf = f"{ROOT}/logs/wire_round{int(rnd)-1}.json"; prev = json.load(open(pf)) if int(rnd) > 1 and os.path.exists(pf) else {}
fidx = pfx.src_function_index(); tasks = []
from gate import Gate as _G
_run = _G().run(); failing_now = {}
for _k in ("srd", "engine"):
    for _n, _l in _run[_k].items():
        for _fid in FEATURES:
            if _n.startswith("flow_" + FEATURES[_fid].get("tests", _fid)): failing_now.setdefault(_fid, []).append(f"{_n}: " + "; ".join(_l[:2]))
flow_sample = open(f"{ROOT}/tests/test_game_flow.cpp").read(); m = re.search(r"TEST\(flow_level_up_in_town_raises_level_and_hits\).*?\n}\n", flow_sample, re.S); sample = m.group(0) if m else ""
support = open(f"{ROOT}/tests/flow_support.h").read()
for fid, f in FEATURES.items():
    if only and fid not in only: continue
    srcs = []
    for ff in f["funcs"]:
        fl, fn = ff.split("::"); t = open(f"{ROOT}/{fl}").read(); loc = find_function(t, fn)
        if loc: srcs.append(f"// ---- {fl} :: {fn}\n{t[loc[0]:loc[1]]}")
    code = "\n\n".join(srcs)
    need = pi.need_tokens(prev[fid].get("answer", "")) if fid in prev else []
    names_f = [x for x in need if x in fidx][:12]
    def packet(b):
        parts = [f"FEATURE {fid}: {f['note']}",
                 "GAME API (game.h)\n" + api_index.digest(f"{ROOT}/include/mordor/game.h")[:b["api"]],
                 "CURRENT HANDLERS\n" + code[:b["code"]],
                 "ACCEPTANCE TESTS (fixed) and flow_support.h\n" + (open(f"{ROOT}/tests/test_flow_{f.get('tests', fid)}.cpp").read()[:6000] if os.path.exists(f"{ROOT}/tests/test_flow_{f.get('tests', fid)}.cpp") else sample) + "\n" + re.sub(r"//[^\n]*\n", "", support)[:1800] + ("\nCURRENTLY FAILING:\n" + "\n".join(failing_now.get(fid, []))[:2500] if failing_now.get(fid) else ""),
                 "STATE STRUCTS\n" + api_index.spine_for_header(code + " " + f["words"])[:b["spine"]],
                 "MODULE DECLARATIONS (relevant)\n" + api_index.relevant_index(code + " " + f["words"] + " " + " ".join(need), cap=b["decl"]),
                 "REQUESTED DEFINITIONS\n" + (pi.decl_blocks(need, 5000) + "\n" + pi.cpp_code(names_f, b["cpp"]) if need else "(none)"),
                 "REFERENCE C# (ground truth)\n" + pi.cs_members(f["cs"], "src/Mordor.Core", b["cs"]) + "\n\n" + pi.cs_rank(pi.tokens_of(f["words"], f["note"]), b["cs2"], 3),
                 "SRD PARAGRAPHS\n" + (pi.srd_hits(f["srd"], b["srd"]) or pi.srd_rank(pi.tokens_of(f["words"]), b["srd"]))]
        if fid in prev: parts.append("PREVIOUS ATTEMPT\n" + prev[fid].get("answer", "")[:2500] + ("\nTESTS OF THE NEW FEATURE THAT STILL FAIL:\n" + "\n".join(prev[fid].get("own_failing", []))[:2500] if prev[fid].get("own_failing") else ""))
        return "\n\n".join(parts)
    budget = dict(api=3500, code=f.get("codebudget", 9000), spine=4500, decl=6000, cpp=6000, cs=8000, cs2=3500, srd=4500); user = packet(budget); k = 0
    while est_tokens(SYS, user) > 31000 and k < 14: budget = {a: int(v * .85) for a, v in budget.items()}; user = packet(budget); k += 1
    print(fid, "est", est_tokens(SYS, user), flush=True)
    if os.environ.get("DRY"): open(f"{ROOT}/spec/investigate/wirepacket_{fid}.txt", "w").write(user); continue
    tasks.append({"id": fid, "system": SYS, "user": user, "allow": [], "save": f"spec/investigate/w{rnd}_{fid}.out"})
if os.environ.get("DRY"): sys.exit(0)
asyncio.run(run_all(tasks, f"wire{rnd}", 3))
gate = gbase = None; results = {}; ownfail = {}
for t in tasks:
    p = f"{ROOT}/{t['save']}"
    if not os.path.exists(p): continue
    out = open(p).read(); fid = t["id"]
    # new (not yet existing) functions: append to the named file
    for mm in re.finditer(r"=== REPLACE (\S+) :: (\w+) ===\n(.*?)\n=== END REPLACE ===", pi.normalise(out), re.S):
        fl, fn, body = mm.groups(); pth = f"{ROOT}/{fl}"
        if os.path.exists(pth) and not find_function(open(pth).read(), fn) and not any(find_function(open(g).read(), fn) for g in __import__("glob").glob(f"{ROOT}/src/*.cpp")):
            tx = open(pth).read(); k2 = tx.rfind("}  // namespace mordor")
            if k2 < 0: k2 = tx.rfind("}  // namespace")
            body = re.sub(r"^```[a-z+]*\n", "", body); body = re.sub(r"\n```\s*$", "", body)
            open(pth, "w").write(tx[:k2] + body + "\n\n" + tx[k2:]) if k2 > 0 else open(pth, "a").write("\n" + body + "\n")
    from cant import parse_files
    def write_tests():
        for pth, body in parse_files(out).items():
            if pth.startswith("tests/test_flow_") and ".." not in pth: open(f"{ROOT}/{pth}", "w").write(body)
    if os.environ.get("GATE"):
        from gate import Gate
        if gate is None: gate = Gate(); gbase = gate.run()
        gate.backup(); n = pi.apply_patches(out)
        after = gate.build_and_run(); new_all = gate.new_failures(gbase, after)
        own = {k: v for k, v in new_all.items() if k.startswith("flow_" + fid)}; new = {k: v for k, v in new_all.items() if k not in own}
        if new: gate.restore(); print(f"  GATE REVERTED {fid}: {list(new)[:6]}", flush=True); n = 0
        else: gbase = after; ownfail[fid] = ["%s: %s" % (k, "; ".join(v[:2])) for k, v in own.items()]; print(f"  gate ok {fid} (own failing tests: {len(own)}): engine failing {len(after['engine'])}, srd failing {len(after['srd'])}, srd tests {after['srd_summary'][0]}", flush=True)
    else: n = pi.apply_patches(out)
    st = re.search(r"STATUS:\s*(\w+)", out); results[fid] = {"status": st.group(1) if st else None, "patches": n, "answer": out, "own_failing": ownfail.get(fid, [])}
    print(f"{fid:12} {results[fid]['status']}  patches {n}")
json.dump(results, open(f"{ROOT}/logs/wire_round{rnd}.json", "w"), indent=1)
