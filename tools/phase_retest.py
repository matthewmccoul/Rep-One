"""Re-translate skipped reference assertions whose function exists in our code, as FIXED acceptance tests (tests/test_engine_re_<id>.cpp).
Failing re-tests are kept (they expose code bugs) and later fixed with phase_investigate clusters re_<id>. usage: phase_retest.py <round> [ids]   (GATE=1)"""
import asyncio, glob, json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
args = sys.argv[1:]; sys.argv = sys.argv[:1]
import phase_investigate as pi
import api_index
from cant import ROOT, run_all, est_tokens, parse_files
M = os.path.dirname(ROOT)
src = open(f"{ROOT}/tools/phase_newfuncs.py").read()
ns = {}; exec(src[src.index("RETEST = {"):src.index("SYS = (")].replace("pi.CONV", "pi.CONV"), {"pi": pi}, ns)
RETEST, SYS_RE = ns["RETEST"], ns["SYS_RE"]
rnd = args[0]; only = args[1:]
cases = {}
for f in glob.glob(f"{M}/testcheck/cases/B_*.jsonl") + glob.glob(f"{M}/testcheck/cases/A_0[123].jsonl"):
    for l in open(f):
        l = l.strip()
        if l.startswith("{"):
            try: o = json.loads(l)
            except Exception: continue
            cases[o["id"]] = o
skipped = set()
for f in glob.glob(f"{ROOT}/tests/test_engine_*.cpp"):
    for l in open(f):
        m = re.match(r"\s*//\s*SKIP (B_\d+#\d+)", l)
        if m: skipped.add(m.group(1))
tasks = []
for rid, f in RETEST.items():
    if only and rid not in only: continue
    meth = f["api"].split(".")[-1]
    cs = [c for c in cases.values() if c["id"] in skipped and c.get("api", "").split("(")[0].strip().endswith(f["api"])]
    if not cs: print(rid, "no skipped cases"); continue
    ref = pi.cs_members([meth], "src/Mordor.Core", 6000)
    caserec = "\n".join(f"[{c['id']}] {c['question']}\n   expected: {c['expected']}" for c in cs)
    cam = meth[0].lower() + meth[1:]
    decl = api_index.relevant_index(cam + " " + f["note"] + " " + meth, cap=7000)
    def packet(b):
        return "\n\n".join([f"RULE {rid}: {f['note']}\nUse the id `{rid}` in the file name test_engine_re_{rid}.cpp and in test names engine_re_{rid}_...", "REFERENCE C#\n" + ref[:b["ref"]], "CASES (reference assertions, expected values are ground truth)\n" + caserec[:b["cases"]],
                            "OUR DECLARATIONS\n" + decl[:b["decl"]], "OUR STATE STRUCTS\n" + api_index.spine_for_header(decl + " " + caserec)[:b["spine"]], "TEST SUPPORT\n" + re.sub(r"//[^\n]*\n", "", open(f"{ROOT}/tests/test_support.h").read())[:2000]])
    budget = dict(ref=6000, cases=9000, decl=7000, spine=5000); user = packet(budget); k = 0
    while est_tokens(SYS_RE, user) > 31000 and k < 14: budget = {a: int(v * .85) for a, v in budget.items()}; user = packet(budget); k += 1
    print(rid, len(cs), "cases; est", est_tokens(SYS_RE, user), flush=True)
    if os.environ.get("DRY"): continue
    tasks.append({"id": rid, "system": SYS_RE, "user": user, "allow": [], "save": f"spec/investigate/t{rnd}_{rid}.out"})
if os.environ.get("DRY"): sys.exit(0)
asyncio.run(run_all(tasks, f"retest{rnd}", 4))
from gate import Gate
gate = Gate(); gbase = gate.run(); results = {}
for t in tasks:
    p = f"{ROOT}/{t['save']}"
    if not os.path.exists(p): continue
    out = open(p).read(); rid = t["id"]; raw = parse_files(out)
    files = {k: v for k, v in raw.items() if re.match(r"tests/test_engine_re_\w+\.cpp$", k)}
    if not files: print(f"{rid:12} no test file"); continue
    (k0, v0), = list(files.items())[:1]
    v0 = v0.replace(re.match(r"tests/test_engine_re_(\w+)\.cpp$", k0).group(1), rid) if re.match(r"tests/test_engine_re_(\w+)\.cpp$", k0).group(1) != rid else v0
    gate.backup(); open(f"{ROOT}/tests/test_engine_re_{rid}.cpp", "w").write(v0)
    after = gate.build_and_run()
    if after is None: gate.restore(); os.remove(f"{ROOT}/tests/test_engine_re_{rid}.cpp"); print(f"{rid:12} BUILD FAILED, removed"); continue
    new = gate.new_failures(gbase, after); own = {k: v for k, v in new.items() if k.startswith(f"engine_re_{rid}_")}; other = {k: v for k, v in new.items() if k not in own}
    if other: gate.restore(); os.remove(f"{ROOT}/tests/test_engine_re_{rid}.cpp"); print(f"{rid:12} REVERTED (other tests broke)"); continue
    gbase = after; results[rid] = {"own_failing": list(own)}
    print(f"{rid:12} tests added; failing re-tests (code bugs exposed): {len(own)}  {list(own)[:6]}", flush=True)
json.dump(results, open(f"{ROOT}/logs/retest_round{rnd}.json", "w"), indent=1)
