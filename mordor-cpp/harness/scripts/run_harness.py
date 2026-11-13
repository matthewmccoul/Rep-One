"""Run harness ants, build/run each group on both sides, compare. usage: run_harness.py <S> <H> [gids|all] [rounds]"""
import asyncio, json, os, re, subprocess, sys, collections
S, H = sys.argv[1:3]; sys.path.insert(0, f"{S}/ants2/scripts"); from army import run_all
sel = sys.argv[3] if len(sys.argv) > 3 else "all"; rounds = int(sys.argv[4]) if len(sys.argv) > 4 else 3
tasks = {t["id"]: t for t in json.load(open(f"{H}/tasks.json"))}; man = json.load(open(f"{H}/manifest.json"))
gids = list(tasks) if sel == "all" else sel.split(",")
FILE = re.compile(r"=== FILE: (\S+) ===\n(.*?)\n=== END FILE ===", re.S)
def strip_fence(b): b = re.sub(r"^```[a-zA-Z+#]*\n", "", b); return re.sub(r"\n```\s*$", "", b)
def compare(g):
    core = dict(l.split("\t", 1) for l in open(f"{g}/core.txt").read().splitlines() if "\t" in l)
    res = collections.defaultdict(lambda: [0, 0, []])
    for l in open(f"{g}/cpp.txt").read().splitlines():
        parts = l.split("\t", 2)
        if len(parts) < 3: continue
        name, k, v = parts; cv = core.get(k)
        if cv is None or cv.startswith("EXC:"): continue
        r = res[name]; r[1] += 1
        if v == cv: r[0] += 1
        elif len(r[2]) < 3: r[2].append(f"case {k}: core {cv} | c++ {v}")
    excs = sum(1 for v in core.values() if v.startswith("EXC:"))
    return {n: {"match": a, "of": b, "examples": ex} for n, (a, b, ex) in res.items()}, len(core), excs
state_f = f"{H}/state.json"; state = json.load(open(state_f)) if os.path.exists(state_f) else {}
pending = [g for g in gids if state.get(g, {}).get("status") not in ("DONE", "NO_REFERENCE")]
START = int(os.environ.get('START_ROUND', '1'))
for rnd in range(START, START + rounds):
    if not pending: break
    batch = []
    for gid in pending:
        t = dict(tasks[gid]); t["out"] = f"{H}/groups/{gid}/ant_r{rnd}.md"
        if state.get(gid, {}).get("problem"): t["user"] = t["user"] + "\n\nYOUR PREVIOUS FILES FAILED. Fix exactly this:\n" + state[gid]["problem"][:3500] + "\n\nPREVIOUS FILES:\n" + state[gid].get("prev", "")[:6000]
        batch.append(t)
    asyncio.run(run_all(batch, f"{H}/harness.log.jsonl", 5))
    nxt = []
    for gid in pending:
        g = f"{H}/groups/{gid}"; p = f"{g}/ant_r{rnd}.md"
        if not os.path.exists(p): nxt.append(gid); continue
        out = open(p).read()
        if "NO_REFERENCE" in out and "=== FILE" not in out: state[gid] = {"status": "NO_REFERENCE", "why": out.strip()[:400]}; continue
        files = {k: strip_fence(v) for k, v in FILE.findall(out)}
        if not all(k in files for k in ("cases.json", "Case.cs", "case.cpp")) or any("/" in k or ".." in k for k in files):
            state[gid] = {"status": "BAD_OUTPUT", "problem": "Missing one of the three file blocks.", "prev": out[:6000]}; nxt.append(gid); continue
        for k, v in files.items(): open(f"{g}/{k}", "w").write(v + "\n")
        r = subprocess.run([f"{H}/scripts/run_group.sh", g], capture_output=True, text=True, timeout=1200).stdout.strip()
        if r != "OK":
            log = ""
            for lf in ("cs_build.log", "cpp_build.log", "core_err.log", "cpp_err.log"):
                if os.path.exists(f"{g}/{lf}"): log += "\n".join(l for l in open(f"{g}/{lf}", errors="replace").read().splitlines() if re.search(r"error|Exception|undefined|Unhandled", l))[:1800]
            state[gid] = {"status": r, "problem": f"{r}\n{log}", "prev": "\n".join(f"=== FILE: {k} ===\n{v}\n=== END FILE ===" for k, v in files.items())}
            nxt.append(gid); print(gid, r, flush=True); continue
        res, n, excs = compare(g)
        if n == 0 or excs * 2 > n:
            err = open(f"{g}/core_err.log", errors="replace").read()[:2500]
            state[gid] = {"status": "CORE_EXC", "problem": f"The reference (Case.cs) threw on {excs} of {n} cases. First exception:\n{err}", "prev": "\n".join(f"=== FILE: {k} ===\n{v}\n=== END FILE ===" for k, v in files.items())}
            nxt.append(gid); print(gid, "CORE_EXC", excs, "of", n, flush=True); continue
        unreach = re.findall(r"UNREACHABLE:\s*(.*)", out); ref = re.findall(r"REFERENCE:\s*(.*)", out)
        state[gid] = {"status": "DONE", "cases": n, "core_exceptions": excs, "copies": res, "unreachable": unreach, "reference": ref[-1].strip() if ref else ""}
        print(gid, "DONE", n, "cases;", {k: f"{v['match']}/{v['of']}" for k, v in res.items()}, flush=True)
    json.dump(state, open(state_f, "w"), indent=1); pending = nxt
json.dump(state, open(state_f, "w"), indent=1)
c = collections.Counter(v["status"] for v in state.values()); print("STATUS", dict(c), flush=True)
