"""Run phase_wire.py rounds until each feature is IMPLEMENTED with no failing own tests, or the round limit is reached.
usage: wire_driver.py <first_round> <max_round> <feature ids...>   (always GATE=1; only one gate process may run at a time)"""
import json, os, subprocess, sys
ROOT = "/tmp/claude-0/-home-user-Rep-One/17a752bb-9e88-5884-a5bf-3b7fc4ab8c87/scratchpad/mordor/code"
first, last, ids = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3:]
for r in range(first, last + 1):
    if not ids: break
    print(f"== round {r}: {ids}", flush=True)
    subprocess.run(["python3", "-I", f"{ROOT}/tools/phase_wire.py", str(r)] + ids, cwd=ROOT, env={**os.environ, "GATE": "1"})
    try: res = json.load(open(f"{ROOT}/logs/wire_round{r}.json"))
    except Exception: continue
    ids = [i for i in ids if not (res.get(i, {}).get("status") == "IMPLEMENTED" and not res.get(i, {}).get("own_failing"))]
print("remaining:", ids, flush=True)
