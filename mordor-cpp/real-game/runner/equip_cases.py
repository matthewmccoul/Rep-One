"""Equip refusal message: real game vs Mordor.Core vs our C++ (harness group g017 cases that differed)."""
import json, sys, time
sys.path.insert(0, "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/real/runner"); from realgame import Game, S, start, crashed
data = json.load(open(f"{S}/prune/code/data/combat-data.json")); ref = {it["id"]: i for i, it in enumerate(data["items"])}
G = f"{S}/harness/groups/g017"; cases = json.load(open(f"{G}/cases.json"))
core = dict(l.split("\t", 1) for l in open(f"{G}/core.txt").read().splitlines() if "\t" in l)
cpp = {l.split("\t")[1]: l.split("\t")[2] for l in open(f"{G}/cpp.txt").read().splitlines() if l.startswith("equipProblem\t")}
pick = [int(x) for x in sys.argv[1].split(",")] if len(sys.argv) > 1 else range(len(cases))
g = Game(); results = []
HUMAN = [(4, 17), (4, 18), (4, 18), (6, 17), (5, 18), (6, 18)]   # natural stat ranges shown on the real creation screen
start(g); loaded = False
for k in pick:
    a = cases[k]["args"]
    if a["id"] not in ref: continue
    if any(not (lo <= a[f"s{i}"] <= hi) for i, (lo, hi) in enumerate(HUMAN)):
        r = {"case": k, "args": a, "real": "UNREACHABLE: a stat outside the Human range", "core": core.get(str(k)), "cpp": cpp.get(str(k))}
        results.append(r); print(json.dumps(r), flush=True); continue
    if loaded: g.city_to_menu(); g.unload()
    else: pass
    g.set({**{f"Stats({i})": a[f"s{i}"] for i in range(6)}, "Pack(1).Ref": ref[a["id"]], "Pack(1).Id": a["id"], "Pack(1).Alignment": 2,
           "Pack(1).Charges": 0, "Pack(1).Equipped": 0, "Pack(1).IdLevel": 3, "Pack(1).Cursed": 0})
    g.load_and_run(); loaded = True; before = g.messages(f"eqc{k}_a")
    g.equip_slot(2); after = g.messages(f"eqc{k}_b")
    if crashed(g):
        r = {"case": k, "args": a, "real": "CRASH", "core": core.get(str(k)), "cpp": cpp.get(str(k))}; results.append(r); print(json.dumps(r), flush=True)
        start(g); loaded = False; continue
    new = after[-1] if after and after != before else "(no new message)"
    r = {"case": k, "args": a, "real": new, "core": core.get(str(k)), "cpp": cpp.get(str(k))}
    results.append(r); print(json.dumps(r), flush=True)
json.dump(results, open(f"{S}/real/runner/equip_results.json", "w"), indent=1)
