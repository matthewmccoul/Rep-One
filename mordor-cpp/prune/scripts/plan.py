"""Coordinator's cut plan from evidence + call-graph facts. Rules:
 copies = members the evidence ant put in the same group (>= 2 members)
 keeper = a member that MATCHES the reference (prefer live = has callers outside tests, then most callers);
          if none matches: the live member (most callers); if none live: the one with most test refs, then longest
 loser actions: no non-test callers -> DELETE (tests on it retargeted or removed);
                live loser whose behaviour equals keeper (both MATCH) -> FORWARD / redirect callers  [phase 1]
                live loser that DIFFERS from a matching keeper -> phase 2 (mechanic change, golden will move)
                live loser and keeper both non-matching -> phase 3 (needs a reference-faithful fix first; not cut now)"""
import json, re, sys, glob, os, collections
W = sys.argv[1]
inv = json.load(open(f"{W}/notes/inventory.json")); meta = json.load(open(f"{W}/notes/evidence_meta.json"))
plans = []; stats = collections.Counter()
for pid, m in sorted(meta.items()):
    p = f"{W}/out/{pid}.md"
    if not os.path.exists(p): stats["no_evidence"] += 1; continue
    t = open(p).read()
    groups = {}
    for g, mem in re.findall(r"(?m)^\s*(G\d+):\s*([m\d,\s]+?)\s*\|", t):
        groups[g] = [int(x) for x in re.findall(r"m(\d+)", mem)]
    verdict = {}
    for k, v, why in re.findall(r"(?m)^\s*m(\d+)\s*\|\s*group\s*G\d+\s*\|\s*vs reference:\s*(MATCHES|DIFFERS|NO_REFERENCE)\s*\|?\s*(.*)$", t):
        verdict[int(k)] = (v, why.strip())
    members = m["members"]
    for g, ks in groups.items():
        ks = [k for k in ks if 1 <= k <= len(members)]
        if len(ks) < 2: continue
        fs = [dict(inv[members[k - 1]], k=k, verdict=verdict.get(k, ("?", ""))[0], why=verdict.get(k, ("?", ""))[1]) for k in ks]
        for f in fs: f["live"] = len(f["callers"]) > 0 or f["app_refs"] > 0
        match = [f for f in fs if f["verdict"] == "MATCHES"]
        pool = match or [f for f in fs if f["live"]] or fs
        keeper = sorted(pool, key=lambda f: (not f["live"], -len(f["callers"]), -f["test_refs"], -f["lines"]))[0]
        acts = []
        for f in fs:
            if f is keeper: continue
            if not f["live"]: a, ph = "DELETE", 1
            elif f["verdict"] == "MATCHES" and keeper["verdict"] == "MATCHES": a, ph = "FORWARD", 1
            elif keeper["verdict"] == "MATCHES": a, ph = "REPLACE_WITH_KEEPER", 2
            else: a, ph = "HOLD", 3
            acts.append({"name": f["name"], "file": f["file"], "line": f["line"], "lines": f["lines"], "verdict": f["verdict"], "why": f["why"][:200],
                         "callers": f["callers"], "test_refs": f["test_refs"], "action": a, "phase": ph})
            stats[f"phase{ph}_{a}"] += 1
        rule = re.search(rf"(?m)^\s*{g}:[^|]*\|\s*rule:\s*(.*)$", t)
        plans.append({"pile": pid, "group": g, "rule": rule.group(1).strip() if rule else "", "keeper": {"name": keeper["name"], "file": keeper["file"], "line": keeper["line"],
                      "verdict": keeper["verdict"], "live": keeper["live"], "callers": len(keeper["callers"])}, "losers": acts})
json.dump(plans, open(f"{W}/notes/plan.json", "w"), indent=1)
print("copy groups:", len(plans), "| losers:", sum(len(p["losers"]) for p in plans), "|", dict(stats))
print("loser lines by phase:", {ph: sum(l["lines"] for p in plans for l in p["losers"] if l["phase"] == ph) for ph in (1, 2, 3)})
