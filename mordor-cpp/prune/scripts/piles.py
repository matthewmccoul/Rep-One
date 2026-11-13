import json, re, collections, itertools, sys
W = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/prune"; inv = json.load(open(f"{W}/notes/inventory.json"))
for i, f in enumerate(inv): f["id"] = i
byname = collections.defaultdict(list)
for f in inv: byname[f["name"]].append(f)
def toks(b):
    b = re.sub(r"//[^\n]*", "", b); body = b[b.find("{"):]
    return re.findall(r"[A-Za-z_]\w*|\d+(?:\.\d+)?|[^\sA-Za-z_\d]", body)
def shingles(t, k=5): return {tuple(t[i:i + k]) for i in range(max(0, len(t) - k + 1))}
sh = {f["id"]: shingles(toks(f["body"])) for f in inv}
edges = collections.defaultdict(set)   # (a,b) -> reasons
def calls_each_other(a, b): return b["name"] in a["calls"] or a["name"] in b["calls"]
# (a) same name in more than one definition
for n, fs in byname.items():
    if len(fs) > 1:
        for a, b in itertools.combinations(fs, 2): edges[(a["id"], b["id"])].add("same_name")
# (b) same Core method mapped to several C++ functions
mp = json.load(open("/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/ants2/notes/mapping_final.json"))
for r in mp:
    if r["status"] in ("PRESENT", "PARTIAL") and len(r.get("cpp") or []) >= 2:
        fs = [f for n in r["cpp"] for f in byname.get(n, [])]
        for a, b in itertools.combinations(fs, 2):
            if a["name"] != b["name"] and not calls_each_other(a, b): edges[(min(a["id"], b["id"]), max(a["id"], b["id"]))].add("core:" + r["method"])
# (c) near-identical bodies under different names (>= 6 lines, shingle Jaccard >= 0.6)
big = [f for f in inv if f["lines"] >= 6]
idx = collections.defaultdict(set)
for f in big:
    for s in list(sh[f["id"]])[:400]: idx[s].add(f["id"])
cand = collections.Counter()
for s, ids in idx.items():
    if len(ids) < 12:
        for a, b in itertools.combinations(sorted(ids), 2): cand[(a, b)] += 1
for (a, b), c in cand.items():
    A, B = sh[a], sh[b]
    if A and B and inv[a]["name"] != inv[b]["name"]:
        j = len(A & B) / len(A | B)
        if j >= 0.6 and not calls_each_other(inv[a], inv[b]): edges[(a, b)].add(f"similar:{j:.2f}")
# (d) explicit source-variant names
VAR = re.compile(r"^(wiki|sheet|archive|engine|published|costProduct|port)[A-Z]")
for f in inv:
    if VAR.match(f["name"]): edges[(f["id"], f["id"])].add("variant_name")
# union-find clusters
par = {}
def find(x):
    par.setdefault(x, x)
    while par[x] != x: par[x] = par[par[x]]; x = par[x]
    return x
for (a, b) in edges: par[find(a)] = find(b)
cl = collections.defaultdict(set)
for (a, b) in edges: cl[find(a)].update({a, b})
piles = sorted(cl.values(), key=lambda s: -len(s))
out = []
for k, s in enumerate(piles, 1):
    reasons = collections.Counter(r.split(":")[0] for (a, b), rs in edges.items() if a in s for r in rs)
    out.append({"pile": k, "members": sorted(s), "reasons": dict(reasons)})
json.dump({"piles": out, "edges": {f"{a},{b}": sorted(r) for (a, b), r in edges.items()}}, open(f"{W}/notes/piles.json", "w"), indent=0)
sizes = collections.Counter(len(p["members"]) for p in out)
print("piles", len(out), "functions in piles", sum(len(p["members"]) for p in out), "size histogram", sorted(sizes.items()))
print("reason totals", collections.Counter(r for p in out for r in p["reasons"]))
for p in out[:8]: print(p["pile"], len(p["members"]), p["reasons"], sorted({inv[i]["name"] for i in p["members"]})[:12])
