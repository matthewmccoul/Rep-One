"""Port reference functions that our code lacks, with their reference assertions as acceptance tests. One ant per function.
usage: phase_newfuncs.py <round> [ids]   (GATE=1 recommended)   New files only: include/mordor/ref_<id>.h, src/ref_<id>.cpp, tests/test_engine_ref_<id>.cpp"""
import asyncio, glob, json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
args = sys.argv[1:]; sys.argv = sys.argv[:1]
import phase_investigate as pi
import api_index
from cant import ROOT, run_all, est_tokens, parse_files
M = os.path.dirname(ROOT)
FUNCS = {
 "tryclng": dict(api="TryCLng", cls="VbConvert", note="VB CLng parsing of a typed string (e.g. '1E3', '&H10', ' 12', '1D2'): returns whether it parses and the Long value (round half to even)."),
 "banktyped": dict(api="BankTyped", cls="Party", note="Bank deposit/withdraw with a typed amount: the answer (amount, message or refusal) for a character, text and direction."),
 "fn1485": dict(api="Fn1485Price", cls="Rules", note="Spell price for a guild: fn1485's price from level, u4, rate, spell level; also reports the rated level."),
 "respawn": dict(api="RespawnDelay", cls="DungeonState", note="Respawn delay of a dungeon room from level, header area count and tally."),
 "questband": dict(api="QuestItemBand", cls="Rules", note="Item quest band from the quest tier."),
 "splitcoords": dict(api="SplitCoords", cls="Rescue", note="Split a typed coordinate string into numbers."),
 "swingsat": dict(api="SwingsAt", cls="", note="Extra swings of a guild at a guild level (the page table)."),
 "goldtosteal": dict(api="GoldToSteal", cls="CombatEngine", note="How much gold a thief monster steals."),
 "placeof": dict(api="PlaceOf", cls="Librarian", note="The place text for a find: depends on lost level/position, level and x,y."),
 "fn13ed": dict(api="Fn13ED", cls="Rules", note="Experience difference between consecutive levels from the guild and race experience factors."),
 "goldfromtag": dict(api="GoldFromTag", cls="GiveCommand", note="Gold amount from a give-command tag (CLng)."),
 "bankalltext": dict(api="BankAllText", cls="Party", note="The text of the whole balance for a bank 'all' answer."),
 "wordsort": dict(api="WordSort", cls="LibrarySearch", note="Library word ordering comparison."),
}
RETEST = {
 "spellcost": dict(api="Spellbook.Cost", note="Spell point cost of a spell for a caster (guild, level, spell rate): the reference Spellbook.Cost."),
 "maxworn": dict(api="MaxWornOfClass", note="Maximum worn items of a wear class (hands/body/...)."),
 "perceivesq": dict(api="PerceiveSquare", note="Perceiving a square: notice/identify rolls and the certainty report."),
 "theftslot": dict(api="TheftSlot", note="Which pack slot a thief monster steals from."),
 "questtier": dict(api="Rules.QuestTier", note="Quest tier from the guild level (clamp(round_half_even(level/25),1,10))."),
 "lookahead": dict(api="PerceiveAhead.Look", note="Perceiving features ahead: the per-feature perception rolls."),
 "uncurse": dict(api="Uncursing.Refusal", note="Why an uncurse is refused (not cursed / not equipped)."),
 "known": dict(api="Spellbook.Known", note="Which spells a new character knows."),
}
SYS_RE = ("You translate assertions from a reference implementation's unit tests (C#) into C++ tests for a C++ port whose API is shown. You get the reference method, the CASES (question = inputs and call, expected = the reference's expected value, VERBATIM ground truth), and the declarations of OUR code that implement the same rule. "
          "For each case that can be expressed with OUR API (set up a Character/Item/Ctx with the types shown, call our function, compare), write TEST(engine_re_<id>_<what>) using CHECK/CHECK_EQ with the expected value verbatim. For a case that cannot be expressed write only `// SKIP <caseid>: <reason>`. NEVER change an expected value to match our code: if our function disagrees, the test must FAIL (that exposes a bug). "
          "Output exactly one file:\n=== FILE: tests/test_engine_re_<id>.cpp ===\n(#include \"test_support.h\" and the needed module headers; using namespace mordor;)\n=== END FILE ===\nIf you lack a declaration, add `NEED: name1, name2`.\n\n" + pi.CONV)
SYS = ("You port a function from a reference implementation (C#, ground truth) into a C++17 library with no I/O, no exceptions, no globals, and translate the reference's own assertions into C++ tests. You get: the reference function, the extracted CASES (question text = inputs and call, expected = the reference's expected value), our basic types, a sample test file and the test support API. "
       "Write the function with the simplest C++ signature that can express the cases (plain ints, doubles, Str, Currency; use only types shown). Round exactly as the reference (half to even where it says so). Then one test per case that can be expressed: TEST(engine_ref_<id>_<what>) using CHECK/CHECK_EQ with the expected value verbatim; for a case you cannot express write only `// SKIP <caseid>: <reason>`. "
       "Output exactly three files:\n=== FILE: include/mordor/ref_<id>.h ===\n(#pragma once, includes, namespace mordor { declarations with a one-line comment citing the reference })\n=== END FILE ===\n=== FILE: src/ref_<id>.cpp ===\n...\n=== END FILE ===\n=== FILE: tests/test_engine_ref_<id>.cpp ===\n(#include \"test_support.h\" and your header; using namespace mordor;)\n=== END FILE ===\n"
       "If a type or function you need is missing, add a line `NEED: name1, name2` and still output what you can.\n\n" + pi.CONV)
rnd = args[0]; only = args[1:]
pf = f"{ROOT}/logs/newfuncs_round{int(rnd)-1}.json"; prev = json.load(open(pf)) if int(rnd) > 1 and os.path.exists(pf) else {}
cases = {}
for f in glob.glob(f"{M}/testcheck/cases/B_*.jsonl") + glob.glob(f"{M}/testcheck/cases/A_0[123].jsonl"):
    for l in open(f):
        l = l.strip()
        if l.startswith("{"):
            try: o = json.loads(l)
            except Exception: continue
            cases[o["id"]] = o
from gate import Gate
_run = Gate().run(); failing_now = {}
for k in ("srd", "engine"):
    for n, ls in _run[k].items():
        m = re.match(r"engine_ref_(\w+?)_", n)
        if m: failing_now.setdefault(m.group(1), []).append(f"{n}: " + "; ".join(ls[:2]))
tasks = []; support = re.sub(r"//[^\n]*\n", "", open(f"{ROOT}/tests/test_support.h").read())[:2500]
sample = open(f"{ROOT}/tests/test_engine_guilds_5.cpp").read()[:1500]
for fid, f in FUNCS.items():
    if only and fid not in only: continue
    cs = [c for c in cases.values() if c.get("api", "").split("(")[0].strip().endswith(f["api"])]
    if not cs: print(fid, "no cases"); continue
    ref = pi.cs_members([f["api"]], "src/Mordor.Core", 7000)
    caserec = "\n".join(f"[{c['id']}] {c['question']}\n   expected: {c['expected']}" for c in cs)[:9000]
    need = pi.need_tokens(prev[fid].get("answer", "")) if fid in prev else []
    def packet(b):
        parts = [f"FUNCTION {fid}: {f['note']}\nUse the id `{fid}` exactly wherever <id> appears (file names ref_{fid}.h, ref_{fid}.cpp, test_engine_ref_{fid}.cpp, test names engine_ref_{fid}_...).", "REFERENCE C#\n" + ref[:b["ref"]], "CASES (reference assertions)\n" + caserec[:b["cases"]], "OUR TYPES\n" + api_index.spine_digest(include_game=False)[:b["types"]],
                 "REQUESTED DEFINITIONS\n" + (pi.decl_blocks(need, 4000) if need else "(none)"), "TEST SUPPORT\n" + support, "SAMPLE TEST FILE\n" + sample]
        if fid in prev: parts.append("PREVIOUS ATTEMPT (it did not fully pass)\n" + prev[fid].get("answer", "")[:2500] + ("\nFAILING TESTS NOW:\n" + "\n".join(failing_now.get(fid, []))[:2500] if failing_now.get(fid) else ""))
        return "\n\n".join(parts)
    budget = dict(ref=7000, cases=9000, types=4500); user = packet(budget); k = 0
    while est_tokens(SYS, user) > 31000 and k < 14: budget = {a: int(v * .85) for a, v in budget.items()}; user = packet(budget); k += 1
    print(fid, len(cs), "cases; est", est_tokens(SYS, user), flush=True)
    if os.environ.get("DRY"): open(f"{ROOT}/spec/investigate/nfpacket_{fid}.txt", "w").write(user); continue
    tasks.append({"id": fid, "system": SYS, "user": user, "allow": [], "save": f"spec/investigate/n{rnd}_{fid}.out"})
if os.environ.get("DRY"): sys.exit(0)
asyncio.run(run_all(tasks, f"newfuncs{rnd}", 4))
gate = gbase = None; results = {}
for t in tasks:
    p = f"{ROOT}/{t['save']}"
    if not os.path.exists(p): continue
    out = open(p).read(); fid = t["id"]
    raw = parse_files(out); files = {}
    tags = {m.group(1) for k in raw for m in [re.match(r"(?:include/mordor/ref_|src/ref_|tests/test_engine_ref_)(\w+)\.(?:h|cpp)$", k)] if m}
    tag = fid if fid in tags else (sorted(tags)[0] if len(tags) == 1 else None)
    if tag:
        for k, v in raw.items():
            if re.match(r"(include/mordor/ref_|src/ref_|tests/test_engine_ref_)" + re.escape(tag) + r"\.(h|cpp)$", k):
                files[k.replace("ref_" + tag, "ref_" + fid)] = v.replace("ref_" + tag, "ref_" + fid).replace("engine_ref_" + tag + "_", "engine_ref_" + fid + "_")
    if len(files) < 3: print(f"{fid:14} incomplete output ({len(files)} files)"); results[fid] = {"status": "INCOMPLETE", "answer": out, "own_failing": []}; continue
    if os.environ.get("GATE"):
        gate = gate or Gate(); gbase = gbase or gate.run()
        gate.backup()
        for k, v in files.items(): open(f"{ROOT}/{k}", "w").write(v)
        after = gate.build_and_run(); new_all = gate.new_failures(gbase, after)
        own = {k: v for k, v in new_all.items() if k.startswith(f"engine_ref_{fid}_")}; new = {k: v for k, v in new_all.items() if k not in own}
        if new or after is None:
            gate.restore()
            for k in files: 
                try: os.remove(f"{ROOT}/{k}")
                except OSError: pass
            print(f"  GATE REVERTED {fid}: {list(new)[:5]}", flush=True); results[fid] = {"status": "REVERTED", "answer": out, "own_failing": []}; continue
        gbase = after
        results[fid] = {"status": "OK", "answer": out, "own_failing": [f"{k}: " + "; ".join(v[:2]) for k, v in own.items()]}
        print(f"{fid:14} ok; own failing {len(own)}; srd tests {after['srd_summary'][0]}, engine tests {after['engine_summary'][0]}", flush=True)
    else:
        for k, v in files.items(): open(f"{ROOT}/{k}", "w").write(v)
json.dump(results, open(f"{ROOT}/logs/newfuncs_round{rnd}.json", "w"), indent=1)
