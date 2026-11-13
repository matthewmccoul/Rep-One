"""Completion pass for the feature factory: replace stubs ('not wired', 'not in this build', ...) with working code. One ant per feature, gated; a rewrite is kept only if
it builds, breaks nothing, and has FEWER stub phrases than before. usage: phase_complete.py [--rounds N] [ids...]   (DRY=1 prints packet sizes)"""
import asyncio, glob, json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
args = sys.argv[1:]; sys.argv = sys.argv[:1]
import phase_investigate as pi
import api_index
from cant import ROOT, run_all, est_tokens, parse_files
from gate import Gate
M = os.path.dirname(ROOT); REF = f"{M}/src/engine-source/OpenDejenol-v0.1.3/src/Mordor.Core"
src = open(f"{ROOT}/tools/phase_features.py").read()
ns = {"pi": pi}; exec(src[src.index("F = {}"):src.index("EXIST =")], ns); F = ns["F"]
STUB = re.compile(r"not wired|not in this build|not connected|not exposed|stub|NEED|cannot yet|not implemented|always refuse|always fail|not available|not yet|unavailable", re.I)
def stubs(text): return len(STUB.findall(text))
SYS = ("You COMPLETE a player-facing feature of a C++17 game core (no I/O, no exceptions, no globals). A first attempt registered the commands and validated arguments but left the real work as stubs that reply 'not wired' or 'not in this build', because the state was not visible to it. "
       "Now you get: the feature, ACCESS HELPERS (access.h: the only way to reach party members, packs, gold, companions, tables and the square under the party, with the layout of the state), the CURRENT FILE, what it said was missing, the declarations and structs it asked for, our relevant module declarations, and the REFERENCE (OpenDejenol C#, ground truth for mechanics). "
       "Rewrite the file so EVERY stub does the real thing: use the access helpers and the existing module functions, mirror the reference mechanics, mutate the real state (gold moves, items change, the party changes). Keep the verbs, screens and help lines. Never answer 'not wired'. Where something truly cannot be done with the types shown, do the rest and say `NEED: name` once. "
       "Output the COMPLETE new file in the same format as the old one, plus ONE small smoke test that checks a REAL effect (e.g. the gold actually moved):\n=== FILE: src/feat_<id>.cpp ===\n...\n=== END FILE ===\n=== FILE: tests/test_feat_<id>.cpp ===\n#include \"flow_support.h\"\n#include \"mordor/access.h\"\nusing namespace flowsup;\nTEST(feat_<id>_<what>) {...}\n=== END FILE ===\nFinish with `STATUS: IMPLEMENTED | PARTIAL` and one line per remaining limitation.\n\n" + pi.CONV)
rounds = int(args[args.index("--rounds") + 1]) if "--rounds" in args else 3
only = [a for a in args if a in F]
STATE_F = f"{ROOT}/logs/complete_state.json"; state = json.load(open(STATE_F)) if os.path.exists(STATE_F) else {}
def save(): json.dump(state, open(STATE_F, "w"), indent=1)
def ref_text(files, maxchars):
    out, per = [], max(2500, maxchars // max(1, len(files)))
    for f in files:
        p = f"{REF}/{f}.cs"
        if os.path.exists(p): out.append(f"// ---- {f}.cs\n" + re.sub(r"\n\s*\n+", "\n", open(p, encoding="utf-8", errors="replace").read())[:per])
    return "\n\n".join(out)
def header_decls(names, maxchars):
    out, seen = [], set()
    for n in names:
        for h in glob.glob(f"{ROOT}/include/mordor/*.h"):
            for m in re.finditer(rf"^[^\n;{{}}]*\b{re.escape(n)}\s*\([^;{{}}]*\)\s*(?:const)?\s*;", open(h).read(), re.M):
                s = m.group(0).strip()
                if s not in seen and sum(map(len, out)) + len(s) < maxchars: seen.add(s); out.append(s)
    return "\n".join(out)
access = open(f"{ROOT}/include/mordor/access.h").read()
support = re.sub(r"//[^\n]*\n", "", open(f"{ROOT}/tests/flow_support.h").read())[:1200]
def last_output(fid):
    fs = sorted(glob.glob(f"{ROOT}/spec/investigate/feat*_{fid}.out"), key=lambda p: int(re.search(r"feat(\d)_", p).group(1))); return open(fs[-1]).read() if fs else ""
FACTS = ("VERIFIED FACTS (use them; do not write NEED for these)\n"
 "- A Ctx for rng and events inside any handler: `Ctx ctx = s.ctx();` (Session has ctx()). ctx.rng is the only random source; ctx.say(code, text, nums, strs) emits an event.\n"
 "- ItemDef has `price` (whole gold, i32), `type`, `cursed`, `specialType`, `spellId`, `id`; Item has `id`, `charges`, `equipped`, `alignment` (ItemAlign 0..3), `slotCursed` (bool), `cursed` (CurseKind). Prices: include/mordor/store_prices.h (storeBuyPrice, storeSellPrice, storeIdentifyFee, storeRealignFee(data, item, steps)).\n"
 "- Character has `location` {x,y,dungeonLevel,area,facing}, `deathState`, `ageDays`, `inventory`-style access via access.h; Inventory.carryIndex -1 nothing, 0..3 companion slot, >=4 body.\n"
 "- A new Session already has the starting pens (10 creatures x2). Pen indices are 0 Good, 1 Neutral, 2 Evil; companion alignment 1 Good, 2 Neutral, 3 Evil.\n"
 "- TESTS: the ONLY way to get a session is `Session s; CHECK(makeSession(s, 1));` (flowsup). It gives 4 members in town (Aldric Human Good Warrior, Lyra Elf Good Mage, Borin Osiri Neutral Thief, Thessa Osiri Neutral Healer, ~1200 gold each). Do not call addMember, sessionInit, newSession or std::exit. Use CHECK only. Set gold as Currency: `static_cast<Currency>(whole_gold) * 10000`.\n")
def packet(fid, b, prev):
    f = F[fid]; cur = open(f"{ROOT}/src/feat_{fid}.cpp").read(); old = last_output(fid)
    asked = [x for x in dict.fromkeys(re.findall(r"[A-Za-z_]\w{3,}", " ".join(re.findall(r"NEED:\s*(.*)", old))))]
    missing = "\n".join(re.findall(r"NEED:.*", old))[:1500] + "\n" + (re.search(r"STATUS:.*", old, re.S).group(0)[:1500] if re.search(r"STATUS:", old) else "")
    toks = pi.tokens_of(f["note"], f["words"])
    parts = [f"FEATURE {fid}: {f['note']}", "ACCESS HELPERS (include/mordor/access.h)\n" + re.sub(r"\n\n+", "\n", access)[:b["acc"]], "CURRENT FILE src/feat_" + fid + ".cpp\n" + cur[:b["cur"]],
             "WHAT THE FIRST ATTEMPT SAID WAS MISSING\n" + missing,
             "DECLARATIONS AND STRUCTS IT ASKED FOR\n" + header_decls(asked, 3000) + "\n\n" + pi.decl_blocks(asked, b["asked"]),
             "OUR MODULE DECLARATIONS (relevant)\n" + api_index.relevant_index(f["words"] + " " + f["note"] + " " + cur[:3000], cap=b["decl"]),
             "STATE STRUCTS\n" + api_index.spine_for_header(f["words"] + " " + cur[:3000])[:b["spine"]],
             "REFERENCE (OpenDejenol, ground truth)\n" + ref_text(f["files"], b["ref"]),
             "SRD PARAGRAPHS\n" + (pi.srd_hits(f["srd"], b["srd"]) or pi.srd_rank(toks, b["srd"])), "TEST SUPPORT\n" + support, FACTS]
    if prev: parts.append("PREVIOUS REWRITE (it did not make it in)\n" + prev["answer"][:2000] + "\nPROBLEM:\n" + prev["problem"][:2000])
    return "\n\n".join(parts)
gate = Gate(); gbase = gate.run()
todo = [fid for fid in F if (not only or fid in only) and os.path.exists(f"{ROOT}/src/feat_{fid}.cpp") and (stubs(open(f"{ROOT}/src/feat_{fid}.cpp").read()) > 0 or os.path.getsize(f"{ROOT}/src/feat_{fid}.cpp") < 1800) and state.get(fid, {}).get("status") != "DONE"]
print(len(todo), "features with stubs:", {fid: stubs(open(f'{ROOT}/src/feat_{fid}.cpp').read()) for fid in todo}, flush=True)
for rnd in range(1, rounds + 1):
    if not todo: break
    tasks = []
    for fid in todo:
        prev = state.get(fid, {}).get("prev"); b = dict(acc=5500, cur=11000, asked=7000, decl=6000, spine=4500, ref=8000, srd=3000); u = packet(fid, b, prev); k = 0
        while est_tokens(SYS, u) > 31000 and k < 14: b = {a: int(v * .85) for a, v in b.items()}; u = packet(fid, b, prev); k += 1
        if os.environ.get("DRY"): print(fid, est_tokens(SYS, u)); continue
        tasks.append({"id": fid, "system": SYS, "user": u, "allow": [], "save": f"spec/investigate/comp{rnd}_{fid}.out"})
    if os.environ.get("DRY"): sys.exit(0)
    print(f"== round {rnd}: {len(tasks)} ants", flush=True)
    asyncio.run(run_all(tasks, f"complete{rnd}", 4))
    still = []
    for t in tasks:
        fid = t["id"]; p = f"{ROOT}/{t['save']}"
        if not os.path.exists(p): still.append(fid); continue
        out = open(p).read(); files = parse_files(out)
        code = files.get(f"src/feat_{fid}.cpp"); test = files.get(f"tests/test_feat_{fid}.cpp")
        if not code: still.append(fid); state[fid] = {"status": "NO_FILE", "prev": {"answer": out, "problem": "No file block for src/feat_%s.cpp. Use the exact block format." % fid}}; save(); print(f"{fid:12} no file"); continue
        old_text = open(f"{ROOT}/src/feat_{fid}.cpp").read(); before, after_n = stubs(old_text), stubs(code)
        gate.backup(); open(f"{ROOT}/src/feat_{fid}.cpp", "w").write(code)
        tp = f"{ROOT}/tests/test_feat_{fid}.cpp"
        if test: open(tp, "w").write(test)
        after = gate.build_and_run(fix_rounds=5)
        if after is None:
            nl = open(f"{ROOT}/logs/ninja_last.txt", errors="replace").read().splitlines() if os.path.exists(f"{ROOT}/logs/ninja_last.txt") else []
            errl = [re.sub(r"^.*?/code/", "", x)[:230] for x in nl if re.search(r"error:|undefined reference|multiple definition", x)][:14]
            prob = "BUILD FAILED. Compiler errors (fix exactly these):\n" + "\n".join(errl) if errl else "BUILD FAILED (no compiler text captured). Keep to declarations shown above."
            gate.restore(); state[fid] = {"status": "BUILD_FAILED", "prev": {"answer": out, "problem": prob}}; still.append(fid); save(); print(f"{fid:12} build failed", flush=True); continue
        new = gate.new_failures(gbase, after); own = {k: v for k, v in new.items() if k.startswith(f"feat_{fid}_")}; other = {k: v for k, v in new.items() if k not in own}
        if other:
            gate.restore(); state[fid] = {"status": "REGRESSED", "prev": {"answer": out, "problem": "Broke: " + "; ".join(f"{k}: {v[0]}" for k, v in list(other.items())[:5])}}; still.append(fid); save(); print(f"{fid:12} REVERTED, broke {list(other)[:3]}", flush=True); continue
        if after_n >= before and before > 0 and after_n > 0 and len(code) < len(old_text) * 1.3:
            gate.restore(); lines = [l.strip() for l in code.split("\n") if STUB.search(l)][:6]
            state[fid] = {"status": "NO_PROGRESS", "prev": {"answer": out, "problem": f"Stub phrases remain ({after_n}, before {before}). Replace these with working code: " + " | ".join(lines)}}; still.append(fid); save(); print(f"{fid:12} no progress ({before} -> {after_n} stub phrases)", flush=True); continue
        if own and rnd < rounds:
            gate.restore(); state[fid] = {"status": "SMOKE_FAILED", "prev": {"answer": out, "problem": "Your smoke test fails: " + "; ".join(f"{k}: {v[0]}" for k, v in list(own.items())[:4])}}; still.append(fid); save(); print(f"{fid:12} smoke fails, retry", flush=True); continue
        if own:
            try: os.remove(tp)
            except OSError: pass
            after = gate.build_and_run(fix_rounds=2) or after
        gbase = after; state[fid] = {"status": "DONE", "stubs_before": before, "stubs_after": after_n, "smoke_dropped": bool(own)}; save()
        print(f"{fid:12} DONE stubs {before} -> {after_n}{' (smoke test dropped)' if own else ''}", flush=True)
    todo = still
print("COMPLETED:", [k for k, v in state.items() if v.get("status") == "DONE"], "| still stubbed:", todo, flush=True)
