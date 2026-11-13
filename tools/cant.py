"""Code-ant runner: one headless Haiku call per task, hard 35k input limit, file extraction, full logging.
Tasks: dict(id, system, user, allow=[path prefixes ant may write], save=<raw output path or None>)"""
import asyncio, json, os, re, sys, time
sys.path.insert(0, "/tmp/claude-0/-home-user-Rep-One/17a752bb-9e88-5884-a5bf-3b7fc4ab8c87/scratchpad/mordor/scripts")
from army import ask, prompt_tokens, HARD_LIMIT
ROOT = "/tmp/claude-0/-home-user-Rep-One/17a752bb-9e88-5884-a5bf-3b7fc4ab8c87/scratchpad/mordor/code"
OVERHEAD = 1300
CPT = 2.0          # conservative chars/token for mixed code and markdown (measured: code+SRD prompts ran ~2.0)
FILE_RE = re.compile(r"=== FILE: (\S+) ===\n(.*?)\n=== END FILE ===", re.S)

def est_tokens(*texts): return int(sum(len(t) for t in texts) / CPT) + OVERHEAD

def parse_files(text):
    out = {}
    for m in FILE_RE.finditer(text):
        body = m.group(2)
        body = re.sub(r"^```[a-z+]*\n", "", body); body = re.sub(r"\n```\s*$", "", body)
        out[m.group(1)] = body + "\n"
    return out

async def exact_tokens(system, user):
    """Exact prompt size via a real call (the answer is capped to 'ok')."""
    res, err = await ask(system, user + "\n\n(Size check only: reply with the single word ok.)", effort="low")
    return prompt_tokens(res) if res and not res.get("is_error") else None

async def run_one(t, sem, phase, logf):
    est = est_tokens(t["system"], t["user"])
    rec = {"phase": phase, "id": t["id"], "est": est}
    if est > 27_000 and t.get("preflight", True):
        async with sem:
            ex = await exact_tokens(t["system"], t["user"])
        rec["exact_preflight"] = ex
        if ex is not None and ex > HARD_LIMIT - 1000:
            rec.update(status="REFUSED_EXACT", error=f"exact {ex}"); logf.write(json.dumps(rec) + "\n"); logf.flush(); print("REFUSED_EXACT", t["id"], ex, flush=True); return rec
        est = ex if ex is not None else est
    if est > HARD_LIMIT - 1000:
        rec.update(status="REFUSED_TOO_BIG"); logf.write(json.dumps(rec) + "\n"); logf.flush(); print("REFUSED", t["id"], est, flush=True); return rec
    async with sem:
        t0 = time.time()
        for a in (1, 2, 3):
            try: res, err = await ask(t["system"], t["user"])
            except Exception as ex: res, err = None, repr(ex)
            if res and not res.get("is_error") and (res.get("result") or "").strip(): break
            await asyncio.sleep(20 * a)
    rec["seconds"] = round(time.time() - t0, 1)
    if not (res and not res.get("is_error")):
        rec.update(status="FAILED", error=str(err)[:200]); logf.write(json.dumps(rec) + "\n"); logf.flush(); return rec
    u = res["usage"]; its = [x for x in (u.get("iterations") or []) if x.get("type", "message") == "message"]; pt = prompt_tokens(res)
    text = res["result"]
    if t.get("save"):
        os.makedirs(os.path.dirname(f"{ROOT}/{t['save']}"), exist_ok=True); open(f"{ROOT}/{t['save']}", "w").write(text)
    written, rejected = [], []
    if t.get("allow") is not None:
        for path, body in parse_files(text).items():
            if any(path.startswith(p) for p in t["allow"]) and ".." not in path:
                os.makedirs(os.path.dirname(f"{ROOT}/{path}"), exist_ok=True); open(f"{ROOT}/{path}", "w").write(body); written.append(path)
            else: rejected.append(path)
    rec.update(status="ok", prompt_tokens=pt, over_limit=pt > HARD_LIMIT, requests=len(its), out=u["output_tokens"], cost=res["total_cost_usd"], written=written, rejected=rejected)
    logf.write(json.dumps(rec) + "\n"); logf.flush()
    return rec

async def run_all(tasks, phase, conc=6):
    os.makedirs(f"{ROOT}/logs", exist_ok=True)
    logf = open(f"{ROOT}/logs/{phase}.log.jsonl", "a"); sem = asyncio.Semaphore(conc)
    recs = await asyncio.gather(*[run_one(t, sem, phase, logf) for t in tasks])
    ok = [r for r in recs if r.get("status") == "ok"]
    print(f"[{phase}] {len(ok)}/{len(recs)} ok | max prompt {max([r['prompt_tokens'] for r in ok], default=0)} | over35k {sum(1 for r in ok if r['over_limit'])} | multi-request {sum(1 for r in ok if r['requests']>1)} | cost ${sum(r['cost'] for r in ok):.3f}", flush=True)
    return recs

def ant_cost_total():
    import glob
    t = 0.0
    for f in glob.glob(f"{ROOT}/logs/*.log.jsonl"):
        for l in open(f):
            try: t += json.loads(l).get("cost", 0) or 0
            except Exception: pass
    return t
