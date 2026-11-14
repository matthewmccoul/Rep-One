"""Ant runner: one headless claude -p Haiku call per packet; 35k hard input limit; per-request logging; resumable."""
import asyncio, json, os, time
MODEL = os.environ.get("ANT_MODEL", "claude-haiku-5-5"); HARD = int(os.environ.get("ANT_HARD", "35000")); CPT = 2.0; OVERHEAD = 1_300
def est(*t): return int(sum(len(x) for x in t) / CPT) + OVERHEAD
async def ask(system, user, effort="medium", timeout=900):
    env = dict(os.environ, DISABLE_PROMPT_CACHING="1", CLAUDE_CODE_MAX_OUTPUT_TOKENS="100000")
    line = json.dumps({"type": "user", "message": {"role": "user", "content": [{"type": "text", "text": user}]}})
    args = ["claude", "-p", "--input-format", "stream-json", "--output-format", "stream-json", "--verbose", "--model", MODEL,
            "--no-session-persistence", "--tools", "", "--system-prompt", system, "--effort", effort]
    p = await asyncio.create_subprocess_exec(*args, stdin=asyncio.subprocess.PIPE, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE, cwd="/tmp", env=env)
    out, err = await asyncio.wait_for(p.communicate(line.encode()), timeout)
    res = None
    for raw in out.decode("utf-8", "replace").splitlines():
        try: ev = json.loads(raw)
        except Exception: continue
        if isinstance(ev, dict) and ev.get("type") == "result": res = ev
    return res, err.decode("utf-8", "replace")[-300:]
def per_request(res):
    its = [i for i in ((res.get("usage") or {}).get("iterations") or []) if i.get("type", "message") == "message"]
    per = [i.get("input_tokens", 0) + i.get("cache_creation_input_tokens", 0) + i.get("cache_read_input_tokens", 0) for i in its]
    if not per:
        u = res["usage"]; per = [u["input_tokens"] + u.get("cache_creation_input_tokens", 0) + u.get("cache_read_input_tokens", 0)]
    return per
async def run_one(t, sem, logf):
    out = t["out"]
    if os.path.exists(out): return
    e = est(t["system"], t["user"]); rec = {"id": t["id"], "est": e}
    if e > HARD - 2000:
        rec["status"] = "REFUSED_TOO_BIG"; logf.write(json.dumps(rec) + "\n"); logf.flush(); print("REFUSED", t["id"], e, flush=True); return
    async with sem:
        t0 = time.time()
        for a in (1, 2, 3):
            try: res, err = await ask(t["system"], t["user"])
            except Exception as ex: res, err = None, repr(ex)
            if res and not res.get("is_error") and (res.get("result") or "").strip(): break
            await asyncio.sleep(20 * a)
    rec["seconds"] = round(time.time() - t0, 1); rec["attempt"] = a
    if res and not res.get("is_error"):
        per = per_request(res); u = res["usage"]
        rec.update(status="ok", requests=per, prompt_tokens=max(per), over_limit=max(per) > HARD, out=u.get("output_tokens"), cost=res.get("total_cost_usd"))
        open(out, "w").write(res["result"])
    else: rec.update(status="FAILED", error=str(err)[:200])
    logf.write(json.dumps(rec) + "\n"); logf.flush()
    print(f"{t['id']}: {rec['status']} {rec.get('prompt_tokens','-')} in / {rec.get('out','-')} out / {rec['seconds']}s", flush=True)
async def run_all(tasks, log, conc=4):
    logf = open(log, "a"); sem = asyncio.Semaphore(conc)
    await asyncio.gather(*[run_one(t, sem, logf) for t in tasks])
