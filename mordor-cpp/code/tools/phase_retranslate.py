"""Re-translate ALL in-scope skipped reference assertions with a targeted view of our API. New tests are fixed specs (expected values verbatim);
failing ones expose code bugs and are kept. usage: phase_retranslate.py <round> [--limit K] [--only n,n]   (GATE=1)"""
import asyncio, collections, glob, json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
args = sys.argv[1:]; sys.argv = sys.argv[:1]
import phase_investigate as pi
import api_index
from cant import ROOT, run_all, est_tokens, parse_files
M = os.path.dirname(ROOT)
src = open(f"{ROOT}/tools/phase_newfuncs.py").read()
ns = {}; exec(src[src.index("RETEST = {"):src.index("SYS = (")], {"pi": pi}, ns); SYS_RE = ns["SYS_RE"].replace("test_engine_re_<id>", "test_engine_rt<id>").replace("engine_re_<id>", "engine_rt<id>")
rnd = args[0]; limit = int(args[args.index("--limit") + 1]) if "--limit" in args else 10**9
only = set(args[args.index("--only") + 1].split(",")) if "--only" in args else None
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
OUT = ("MdrLoader", "Paint", "PortSource", "SpellHelpProbe", "LibrarySearch", "Librarian", "VbConvert")
rem = [c for i, c in cases.items() if i in skipped and not any(o in c.get("api", "") for o in OUT)]
bytopic = collections.defaultdict(list)
for c in rem: bytopic[c.get("topic", "?")].append(c)
batches = []
for topic, cs in sorted(bytopic.items()):
    cs.sort(key=lambda c: c.get("api", ""))
    for k in range(0, len(cs), 8): batches.append((f"{topic.lower()}{k // 8}", cs[k:k + 8]))
done_f = f"{ROOT}/logs/retranslate_done.json"; done = set(json.load(open(done_f))) if os.path.exists(done_f) else set()
tasks = []; support = re.sub(r"//[^\n]*\n", "", open(f"{ROOT}/tests/test_support.h").read())[:1800]
for bid, cs in batches:
    if bid in done or (only and bid not in only) or len(tasks) >= limit: continue
    caserec = "\n".join(f"[{c['id']}] ({c.get('api')}) {c['question']}\n   expected: {c['expected']}" for c in cs)
    meths = list(dict.fromkeys(c.get("api", "").split("(")[0].split(".")[-1].strip() for c in cs))
    toks = pi.tokens_of(caserec)
    def packet(b):
        ref = pi.cs_members([m for m in meths if m][:6], "src/Mordor.Core", b["ref"])
        decl = api_index.relevant_index(" ".join(toks) + " " + " ".join(m[0].lower() + m[1:] for m in meths if m), cap=b["decl"])
        return "\n\n".join([f"BATCH {bid}: use the id `{bid}` in the file name tests/test_engine_rt{bid}.cpp and in test names engine_rt{bid}_...", "REFERENCE C# METHODS\n" + ref, "CASES (reference assertions, expected values are ground truth)\n" + caserec[:b["cases"]],
                            "OUR DECLARATIONS (relevant)\n" + decl, "OUR STATE STRUCTS\n" + api_index.spine_for_header(decl + " " + caserec)[:b["spine"]], "TEST SUPPORT\n" + support])
    budget = dict(ref=7000, cases=9000, decl=8000, spine=5000); user = packet(budget); k = 0
    while est_tokens(SYS_RE, user) > 31000 and k < 14: budget = {a: int(v * .85) for a, v in budget.items()}; user = packet(budget); k += 1
    if os.environ.get("DRY"): print(bid, len(cs), "cases; est", est_tokens(SYS_RE, user)); continue
    tasks.append({"id": bid, "system": SYS_RE, "user": user, "allow": [], "save": f"spec/investigate/rt{rnd}_{bid}.out"})
if os.environ.get("DRY"): sys.exit(0)
print(len(tasks), "batches", flush=True)
asyncio.run(run_all(tasks, f"retranslate{rnd}", 4))
from gate import Gate
gate = Gate(); gbase = gate.run(); results = {}; added = 0; failing_total = 0
for t in tasks:
    p = f"{ROOT}/{t['save']}"
    if not os.path.exists(p): continue
    out = open(p).read(); bid = t["id"]; raw = parse_files(out)
    files = [(k, v) for k, v in raw.items() if re.match(r"tests/test_engine_rt\w+\.cpp$", k)]
    if not files: print(f"{bid:8} no test file"); done.add(bid); continue
    tag = re.match(r"tests/test_engine_rt(\w+)\.cpp$", files[0][0]).group(1); v0 = files[0][1].replace("rt" + tag, "rt" + bid) if tag != bid else files[0][1]
    path = f"{ROOT}/tests/test_engine_rt{bid}.cpp"
    gate.backup(); open(path, "w").write(v0); after = gate.build_and_run(fix_rounds=6)
    if after is None:
        gate.restore()
        try: os.remove(path)
        except OSError: pass
        print(f"{bid:8} BUILD FAILED -> removed", flush=True); done.add(bid); json.dump(sorted(done), open(done_f, "w")); continue
    new = gate.new_failures(gbase, after); own = {k: v for k, v in new.items() if k.startswith(f"engine_rt{bid}_")}; other = {k: v for k, v in new.items() if k not in own}
    if other:
        gate.restore()
        try: os.remove(path)
        except OSError: pass
        print(f"{bid:8} REVERTED (others broke: {list(other)[:3]})", flush=True)
    else:
        gbase = after; ntests = len(re.findall(r"^TEST\(", v0, re.M)); added += ntests; failing_total += len(own)
        results[bid] = {"tests": ntests, "failing": list(own)}
        print(f"{bid:8} +{ntests} tests, {len(own)} failing (exposed)", flush=True)
    done.add(bid); json.dump(sorted(done), open(done_f, "w"))
json.dump(results, open(f"{ROOT}/logs/retranslate_round{rnd}.json", "w"), indent=1)
print("TOTAL added", added, "failing", failing_total, flush=True)
