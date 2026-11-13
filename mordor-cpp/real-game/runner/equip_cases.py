"""Equip refusal: real game vs Mordor.Core vs our C++. Per case: edit save (unloaded), load, Equip pack slot 1,
read the message pane, exit and unload, read the save to see whether the item ended up equipped."""
import json, sys
sys.path.insert(0, "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/real/runner"); from realgame import Game, S, start, crashed
data = json.load(open(f"{S}/prune/code/data/combat-data.json")); ref = {it["id"]: i for i, it in enumerate(data["items"])}
G = f"{S}/harness/groups/g017"; cases = json.load(open(f"{G}/cases.json"))
core = dict(l.split("\t", 1) for l in open(f"{G}/core.txt").read().splitlines() if "\t" in l)
flow = dict(l.split("\t", 1) for l in open(f"{S}/harness/groups/zz_equipflow/core.txt").read().splitlines() if "\t" in l)
cpp = {l.split("\t")[1]: l.split("\t")[2] for l in open(f"{G}/cpp.txt").read().splitlines() if l.startswith("equipProblem\t")}
pick = [int(x) for x in sys.argv[1].split(",")] if len(sys.argv) > 1 else range(len(cases))
HUMAN = [(4, 17), (4, 18), (4, 18), (6, 17), (5, 18), (6, 18)]
out = open(f"{S}/real/runner/equip_run.out", "a")
g = Game(); start(g)                       # main menu, nothing loaded
for k in pick:
    a = cases[k]["args"]
    r = {"case": k, "args": a, "core_action": flow.get(str(k)), "core_fn": core.get(str(k)), "cpp_fn": cpp.get(str(k))}
    if any(not (lo <= a[f"s{i}"] <= hi) for i, (lo, hi) in enumerate(HUMAN)): r["real"] = "UNREACHABLE: stat outside the Human range"
    else:
        g.set({**{f"Stats({i})": a[f"s{i}"] for i in range(6)}, "Pack(1).Ref": ref[a["id"]], "Pack(1).Id": a["id"], "Pack(1).Alignment": 2,
               "Pack(1).Charges": 0, "Pack(1).Equipped": 0, "Pack(1).IdLevel": 3, "Pack(1).Cursed": 0})
        g.load_and_run(); before = g.messages(f"eqc{k}_a"); g.equip_slot(2); after = g.messages(f"eqc{k}_b")
        if crashed(g): r["real"] = "CRASH"; start(g)
        else:
            r["real"] = after[-1] if after and after != before else "(no new message)"
            g.city_to_menu(); g.unload()
            r["equipped_after"] = g.read().get("Pack(1).Equipped")
            if r["real"] == "(no new message)": r["real"] = "(equipped)" if r["equipped_after"] not in ("0", None) else "(silent, not equipped)"
            r["match"] = r["real"] == r["core_action"]
    print(json.dumps(r), file=out, flush=True); print(json.dumps(r), flush=True)
