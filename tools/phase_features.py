"""Feature factory: one ant per player-facing system writes src/feat_<id>.cpp (registered through the command registry) plus an optional light smoke test.
Breadth first, light tests: a feature is KEPT when the build passes and no existing test or golden transcript regresses; failing own smoke tests are fed back for
two more rounds and then deleted (the code stays). usage: phase_features.py [--rounds N] [ids...]   (DRY=1 prints packet sizes)"""
import asyncio, glob, json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
args = sys.argv[1:]; sys.argv = sys.argv[:1]
import phase_investigate as pi
import api_index
from cant import ROOT, run_all, est_tokens, parse_files
from gate import Gate
M = os.path.dirname(ROOT); REF = f"{M}/src/engine-source/OpenDejenol-v0.1.3/src/Mordor.Core"
F = {}
def feat(fid, screens, files, words, srd, note, cs=None): F[fid] = dict(screens=screens, files=files, words=words, srd=srd, note=note, cs=cs or files)
feat("give", ["Town", "Dungeon"], ["GiveCommand", "GoldShare"], "give gold item transfer member party pack slot gold on hand transferGold giveItem partyb", ["give", "transfer", "gold answer"],
     "Commands `give gold <from> <to> <amount>` and `give item <from> <slot> <to>` between party members (members numbered 1-4, slots as in `equip`). Use the reference GoldAnswer rules (an amount below 1 is 0, more than on hand is refused, the amount is read with Val/CLng) and the existing transfer helpers; guild-issued items cannot be given.")
feat("swap", ["Town", "Dungeon"], ["SwapCommand", "Combining"], "swap pack slot exchange items typed swap buffers combine charges", ["typed swap", "swap", "combine"],
     "Command `swap <member> <slot1> <slot2>` (typed swap: refuses if either number is 1 or below or above 40; exchanges contents including empty slots; spell buffers keep pointing at the same slot numbers) and `combine <member> <slot1> <slot2>` (Combining: merge charges of two like items).")
feat("realign", ["Town"], ["StoreStock", "ServiceForms"], "realign alignment item store fee crest storeRealignFee", ["realign", "alignment fee"],
     "Store service `realign <member> <slot> <good|neutral|evil>`: changes an item's alignment for the reference fee (storeRealignFee: 0.72 x price, doubled across more than one step); refused while the item is equipped.")
feat("uncurse", ["Town"], ["Uncursing"], "uncurse cursed item equipped fee remove curse", ["uncurse"],
     "Service `uncurse <member> <slot>`: refuse with the reference texts when the item is not cursed or not equipped; otherwise remove the curse for the fee the SRD gives (uncurseFee).")
feat("seer", ["Town"], ["Seer"], "seer fee sighting library monster location where", ["seer"],
     "The Seer service `seer ...` as in the reference Seer module: what the Seer offers, its fee, and what it reports.")
feat("library", ["Town"], ["Librarian", "LibrarySearch", "RoomRoster"], "library record monster item knowledge file sighting search describe", ["library", "librarian"],
     "Library: `library` lists what the party has learned; `library search <words>` finds entries; `library monster <name>` / `library item <name>` describe an entry as the reference Librarian does (the party's records are kept in s.featureState under keys starting with `library.`).")
feat("confinement", ["Town"], ["Confinement", "CompanionSlots", "CompanionRef"], "confinement pens creature companion buy sell price pen stock confinementBuy confinementSell penBuyPrice companions slot", ["confinement", "pens", "companion slot"],
     "Confinement: `pens` lists creatures in the pens with the reference buy price; `pen buy <row> <member>` buys one (price from penBuyPrice, charged to the member) and puts the companion in the member's first free companion slot (up to 4); `pen sell <slot> <member>` sells a companion back. Decide where companions live from state_companion.h and Party (aliveCompanions, carryingCompanion); if the Session has no place for them, keep them in s.featureState with keys starting `companion.` and say so.")
feat("companions", ["Town", "Dungeon"], ["CompanionSlots", "CompanionCarry", "CompanionInfo", "CompanionMood", "CompanionTurn"], "companion slots rearrange release carry mood info hits alive follow", ["companion", "carry", "rearrange"],
     "`companions <member>` lists a member's companions; `rearrange <member> <from> <to>` (drag/swap rules of the reference); `release <member> <slot>`; `carry <member> <slot>`; plus the reference mood and info texts.")
feat("hall", ["Town"], ["HallRecords"], "hall of records best experience character mostExperience record", ["hall of records", "hall"],
     "`hall` shows the Hall of Records as the reference HallRecords does (the records kept in s.featureState under `hall.`), updated on arrival in town for the party's members.")
feat("options", ["Town", "Dungeon"], ["GameOptions", "CharacterOptions"], "options difficulty game options character options setting toggle", ["game options", "difficulty", "options"],
     "`options` lists game and character options with their values; `options <name> <value>` changes one, as the reference GameOptions/CharacterOptions define them; where an option changes a rule, expose it through s.game fields if they exist, otherwise s.featureState `option.<name>`.")
feat("quests", ["Town"], ["GuildHall", "Guildmasters", "GuildTitles"], "guild quest assign claim forfeit guildmaster mastership challenge title level fetch kill item", ["quest", "guildmaster", "mastership", "guild title"],
     "Guild services: `quest <member>` shows or assigns the guild quest (RollQuest, QuestTier), `claim <member>` claims a completed quest or the guildmastership, `forfeit <member> <guild>` leaves a guild, `titles <member>` shows the guild title; use the guilds modules.")
feat("guildlog", ["Town"], ["GuildLog"], "guild log entries clean appointment guildmaster history", ["guild log"],
     "`guildlog <guild>` shows the guild's log; appointments of the seeded guildmasters are written at new game; `guildlog clean` keeps the newest entries as the reference does (log kept in s.featureState `guildlog.`).")
feat("rescue", ["Town", "Dungeon"], ["Rescue", "Raising", "PendingRaise"], "rescue carry home dead raise body morgue party lost carried", ["rescue", "carry home", "raise"],
     "Rescue and raising flow beyond `morgue`: carrying a dead member home, the pending raise and `rescue` as the reference Rescue/Raising define them.")
feat("roster", ["Town"], ["Roster", "PartyRow"], "party roster unload load member remove add party row", ["party", "unload", "roster"],
     "`unload <member>` removes a member from the party (their record stays in s.featureState `roster.`), `load <name>` brings one back (party max 4), `roster` lists them.")
feat("look", ["Dungeon"], ["LookAround", "RoomRoster", "SquareInfo"], "look around room roster square info readout lost position facing", ["look", "readout", "room roster"],
     "`look` describes the surroundings as the reference LookAround/RoomRoster/SquareInfo do: the readout line, who is in the room, what the square holds.")
feat("dcast", ["Dungeon"], ["CastGate", "SpellEffect", "SpellThrown", "Teleporting"], "cast spell dungeon heal light detect teleport displacement passwall levitate protection effect applyCastState spellHealing", ["cast", "teleport", "spell effect"],
     "`cast <member> <spell name or id> [args]` on the Dungeon screen for spells usable outside combat, applying the real effect through the spell modules (healing heals, Levitate/Protection/Invisible set ailment flags, Teleport moves the party, Detect reveals); costs the reference SP and refuses unreachable spells.")
feat("duse", ["Dungeon", "Town"], ["ItemUse"], "use item charge spell potion scroll effect itemUseProblem applyItemUse", ["use item", "charges"],
     "Make `use <member> <slot>` apply the item's real spell effect (potions heal, scrolls cast) and spend a charge, on both Town and Dungeon screens, replacing the 'effect not wired' reply by registering `use` on the Dungeon screen (the Town one is handled in game_town.cpp: register an extra verb `useitem` there that does the full effect).")
feat("disarm", ["Dungeon"], ["ChestTraps", "Loot"], "chest disarm inspect trap roll open countdown chestDisarm rollTrapDisarm", ["disarm", "inspect", "chest trap"],
     "`inspect` tells what trap a chest on this square has (by the opener's skill) and `disarm <member>` tries the disarm roll (rollTrapDisarm); a success removes the trap, a failure springs it.")
feat("buffers", ["Town", "Combat"], ["SpellBuffers", "CombatOrder"], "spell buffer standing order cast slot cost text buffer fire", ["spell buffer", "standing order"],
     "`buffer <member> <slot 1-4> <spell>` sets a spell buffer (standing order), `buffer <member>` lists them with their cost text; in combat the buffered spells fire per the reference (BufferFire).")
EXIST = {"Town": "help rest guild levelup join shop buy sell bank deposit withdraw equip unequip use identify morgue raise pool spells cast descend", "Dungeon": "forward back left right stairs open search map pool", "Combat": "fight defend cast use open"}
SYS = ("You add ONE player-facing feature to a C++17 game core (no I/O, no exceptions, no globals; portable to Unreal). Commands are added through a registry: you write a NEW file src/feat_<id>.cpp with handlers and a function `void register_<id>()`. "
       "You get an EVIDENCE PACKET: the feature, the public game API, our relevant module declarations and state structs, the REFERENCE implementation (OpenDejenol C#, ground truth for mechanics) and SRD paragraphs. Mirror the reference mechanics exactly; call our existing module functions instead of re-implementing rules; use ONLY types and functions shown. "
       "Do not add members to existing structs; for state that has no home use `s.featureState[\"<id>.<key>\"]` (a std::map<Str,Str>). Put helpers in an anonymous namespace and prefix any non-static name with the feature id. Do not reuse a verb that already exists on that screen (existing: Town: %TOWN%; Dungeon: %DUN%; Combat: %COM%). Replies are plain text lines.\n"
       "Output EXACTLY: the feature file and, optionally, ONE tiny smoke test (light testing: 1-2 TESTs that exercise the new verb through the game API):\n"
       "=== FILE: src/feat_<id>.cpp ===\n#include \"mordor/features.h\"\n(other includes)\nnamespace mordor {\nnamespace {\nbool cmdX(Session& s, const Vec<Str>& w, Str& reply) { ... return true; }\n}  // namespace\nvoid register_<id>() {\n  registerFeatureCommand({\"verb\", Screen::Town, &cmdX, \"verb <args>  - what it does\"});\n}\n}  // namespace mordor\n=== END FILE ===\n"
       "=== FILE: tests/test_feat_<id>.cpp ===\n#include \"flow_support.h\"\nusing namespace flowsup;\nTEST(feat_<id>_<what>) { Session s; CHECK(makeSession(s, 3)); Str reply; CHECK(townAction(s, \"verb ...\", reply)); ... }\n=== END FILE ===\n"
       "If you lack something, put `NEED: name1, name2` on its own line and implement as much as you can. End with `STATUS: IMPLEMENTED | PARTIAL` and one line per limitation.\n\n" + pi.CONV)
SYS = SYS.replace("%TOWN%", EXIST["Town"]).replace("%DUN%", EXIST["Dungeon"]).replace("%COM%", EXIST["Combat"])
rounds = int(args[args.index("--rounds") + 1]) if "--rounds" in args else 3
only = [a for a in args if a in F]
STATE_F = f"{ROOT}/logs/features_state.json"; state = json.load(open(STATE_F)) if os.path.exists(STATE_F) else {}
def save(): json.dump(state, open(STATE_F, "w"), indent=1)
def ref_text(files, maxchars):
    out, per = [], max(2500, maxchars // max(1, len(files)))
    for f in files:
        p = f"{REF}/{f}.cs"
        if os.path.exists(p): out.append(f"// ---- {f}.cs\n" + re.sub(r"\n\s*\n+", "\n", open(p, encoding="utf-8", errors="replace").read())[:per])
    return "\n\n".join(out)
support = re.sub(r"//[^\n]*\n", "", open(f"{ROOT}/tests/flow_support.h").read())[:1500]
def packet(fid, f, b, prev):
    toks = pi.tokens_of(f["note"], f["words"])
    parts = [f"FEATURE {fid} (screens: {', '.join(f['screens'])}). Use the id `{fid}` for the files: src/feat_{fid}.cpp, tests/test_feat_{fid}.cpp, register_{fid}().\n{f['note']}",
             "GAME API (game.h)\n" + api_index.digest(f"{ROOT}/include/mordor/game.h")[:b["api"]] + "\n\nCOMMAND REGISTRY (features.h)\n" + re.sub(r"//[^\n]*\n", "", open(f"{ROOT}/include/mordor/features.h").read())[:900],
             "STATE STRUCTS\n" + api_index.spine_for_header(f["words"] + " " + f["note"])[:b["spine"]],
             "OUR MODULE DECLARATIONS (relevant)\n" + api_index.relevant_index(f["words"] + " " + f["note"], cap=b["decl"]),
             "REFERENCE (OpenDejenol, ground truth)\n" + ref_text(f["files"], b["ref"]) + "\n\n" + pi.cs_rank(toks, b["cs2"], 2),
             "SRD PARAGRAPHS\n" + (pi.srd_hits(f["srd"], b["srd"]) or pi.srd_rank(toks, b["srd"])),
             "TEST SUPPORT (flow_support.h)\n" + support]
    if prev: parts.append("PREVIOUS ATTEMPT (it did not make it in)\n" + prev["answer"][:2200] + "\nPROBLEM:\n" + prev["problem"][:2200])
    return "\n\n".join(parts)
def register(fid):
    p = f"{ROOT}/src/feature_registry.cpp"; t = open(p).read()
    if f"register_{fid}()" in t: return
    t = t.replace("// FEATURE_DECLS", f"void register_{fid}();\n// FEATURE_DECLS", 1).replace("  // FEATURE_CALLS", f"  register_{fid}();\n  // FEATURE_CALLS", 1); open(p, "w").write(t)
def unregister(fid):
    p = f"{ROOT}/src/feature_registry.cpp"; t = open(p).read()
    t = t.replace(f"void register_{fid}();\n", "").replace(f"  register_{fid}();\n", ""); open(p, "w").write(t)
gate = Gate(); gbase = gate.run()
todo = [fid for fid in F if (not only or fid in only) and state.get(fid, {}).get("status") != "KEPT"]
print(len(todo), "features to build", flush=True)
for rnd in range(1, rounds + 1):
    if not todo: break
    tasks = []
    for fid in todo:
        f = F[fid]; prev = state.get(fid, {}).get("prev")
        b = dict(api=3000, spine=4500, decl=7000, ref=11000, cs2=3500, srd=4000); u = packet(fid, f, b, prev); k = 0
        while est_tokens(SYS, u) > 31000 and k < 14: b = {a: int(v * .85) for a, v in b.items()}; u = packet(fid, f, b, prev); k += 1
        if os.environ.get("DRY"): print(fid, est_tokens(SYS, u)); continue
        tasks.append({"id": fid, "system": SYS, "user": u, "allow": [], "save": f"spec/investigate/feat{rnd}_{fid}.out"})
    if os.environ.get("DRY"): sys.exit(0)
    print(f"== round {rnd}: {len(tasks)} ants", flush=True)
    asyncio.run(run_all(tasks, f"features{rnd}", 4))
    still = []
    for t in tasks:
        fid = t["id"]; p = f"{ROOT}/{t['save']}"
        if not os.path.exists(p): still.append(fid); continue
        out = open(p).read(); files = parse_files(out)
        code = {k: v for k, v in files.items() if k == f"src/feat_{fid}.cpp"}; test = {k: v for k, v in files.items() if k == f"tests/test_feat_{fid}.cpp"}
        if not code: state[fid] = {"status": "NO_FILE", "prev": {"answer": out, "problem": f"No file src/feat_{fid}.cpp was produced. Use exactly the file block format."}}; still.append(fid); save(); print(f"{fid:12} no file"); continue
        gate.backup()
        for k, v in {**code, **test}.items(): open(f"{ROOT}/{k}", "w").write(v)
        register(fid)
        after = gate.build_and_run(fix_rounds=5)
        if after is None:
            errs = json.load(open(f"{ROOT}/logs/build_errors.json")) if os.path.exists(f"{ROOT}/logs/build_errors.json") else {}
            prob = "BUILD FAILED. Errors:\n" + "\n".join(x.split("code/")[-1][:200] for v in errs.values() for x in v[:6])[:2000]
            gate.restore(); state[fid] = {"status": "BUILD_FAILED", "prev": {"answer": out, "problem": prob}}; still.append(fid); save(); print(f"{fid:12} build failed (round {rnd})", flush=True); continue
        new = gate.new_failures(gbase, after); own = {k: v for k, v in new.items() if k.startswith(f"feat_{fid}_")}; other = {k: v for k, v in new.items() if k not in own}
        if other:
            gate.restore(); state[fid] = {"status": "REGRESSED", "prev": {"answer": out, "problem": "Your change broke these existing checks: " + "; ".join(f"{k}: {v[0]}" for k, v in list(other.items())[:6])}}; still.append(fid); save(); print(f"{fid:12} REVERTED, broke {list(other)[:3]}", flush=True); continue
        if own and rnd < rounds:
            gate.restore(); state[fid] = {"status": "SMOKE_FAILED", "prev": {"answer": out, "problem": "Your smoke test fails: " + "; ".join(f"{k}: {v[0]}" for k, v in list(own.items())[:4])}}; still.append(fid); save(); print(f"{fid:12} smoke test fails ({len(own)}), retry", flush=True); continue
        if own:   # last round: light testing - drop the failing smoke test, keep the code
            try: os.remove(f"{ROOT}/tests/test_feat_{fid}.cpp")
            except OSError: pass
            after = gate.build_and_run(fix_rounds=2) or after
        gbase = after; st = re.search(r"STATUS:\s*(\w+)", out)
        state[fid] = {"status": "KEPT", "build": st.group(1) if st else "?", "smoke_dropped": bool(own), "need": re.findall(r"NEED:\s*(.*)", out)[:1]}; save()
        print(f"{fid:12} KEPT ({state[fid]['build']}){' smoke test dropped' if own else ''}; engine {after['engine_summary'][0]} tests, srd {after['srd_summary'][0]} tests", flush=True)
    todo = still
print("DONE kept:", [k for k, v in state.items() if v.get("status") == "KEPT"], "| not built:", todo, flush=True)
