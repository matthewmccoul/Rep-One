"""Candidate duplicate pairs among the running functions (same name, or similar descriptions), then one ant per pair:
do these two functions do the same thing? usage: pairs.py <S> [threshold]"""
import asyncio, collections, json, math, os, re, sys
S = sys.argv[1]; F = f"{S}/feat"; C = f"{S}/prune/code"; TH = float(sys.argv[2]) if len(sys.argv) > 2 else 0.45
sys.path.insert(0, f"{S}/ants2/scripts"); from army import run_all, est
idx = json.load(open(f"{F}/al_index.json"))
items = []
for tid, dd in sorted(idx.items()):
    t = open(f"{F}/al/{tid}.md").read()
    k = re.search(r"KIND:\s*(\w+)", t); w = re.search(r"WHAT:\s*(.*)", t)
    items.append({"id": tid, "defn": dd, "name": dd.split("@")[0], "kind": k.group(1) if k else "?", "what": w.group(1) if w else ""})
STOP = set("the a an of to and or in is for by with it that this be as at from are not its when given returns return which into one value whether true false character's party".split())
def tk(s): return [w for w in re.findall(r"[a-z]{3,}|\d+", s.lower()) if w not in STOP]
docs = [collections.Counter(tk(i["what"])) for i in items]; df = collections.Counter(w for d in docs for w in d); N = len(docs)
vec = []
for d in docs:
    v = {w: c * math.log(N / df[w]) for w, c in d.items()}; n = math.sqrt(sum(x * x for x in v.values())) or 1; vec.append({w: x / n for w, x in v.items()})
pairs = {}
for i in range(N):
    for j in range(i + 1, N):
        s = sum(x * vec[j].get(w, 0) for w, x in vec[i].items())
        if items[i]["name"] == items[j]["name"]: pairs[(i, j)] = ("same name", s)
        elif s >= TH: pairs[(i, j)] = ("similar description", s)
def body(defn):
    name, rest = defn.split("@"); f, line = rest.rsplit(":", 1)
    L = open(f"{C}/{f}", errors="replace").read().splitlines(); a0 = int(line) - 1
    pre = []; j = a0 - 1
    while j >= 0 and L[j].strip().startswith("//") and len(pre) < 8: pre.insert(0, L[j]); j -= 1
    e = a0; d = 0; seen = False
    while e < len(L):
        d += L[e].count("{") - L[e].count("}"); seen = seen or "{" in L[e]; e += 1
        if seen and d <= 0: break
    return "\n".join(pre + L[a0:e])[:5000]
SYS = """You compare TWO functions from the same C++ code base (a re-implementation of the rules of the 1995 RPG "Mordor: The Depths of Dejenol"). Answer one question: do they do the same thing, so that one could replace the other at every call site with at most a change of argument form?
Reply with exactly three lines:
VERDICT: <SAME|SAME_EXCEPT_DETAILS|DIFFERENT>
DIFFERENCES: <none, or the exact behavioural differences (inputs where results differ)>
NOTE: <one sentence>
SAME = identical results for all inputs. SAME_EXCEPT_DETAILS = the same rule, but results differ for some inputs (name them). DIFFERENT = different purposes."""
tasks = []; meta = {}
for (i, j), (why, s) in sorted(pairs.items(), key=lambda kv: -kv[1][1]):
    pid = f"p{i:03d}_{j:03d}"; a, b = items[i], items[j]
    meta[pid] = {"a": a["defn"], "b": b["defn"], "why": why, "score": round(s, 2), "what_a": a["what"], "what_b": b["what"]}
    u = f"FUNCTION A ({a['defn']}):\n{body(a['defn'])}\n\nFUNCTION B ({b['defn']}):\n{body(b['defn'])}"
    tasks.append({"id": pid, "system": SYS, "user": u, "out": f"{F}/pairs/{pid}.md"})
json.dump(meta, open(f"{F}/pairs_meta.json", "w"), indent=0)
print(len(tasks), "pairs:", collections.Counter(m["why"] for m in meta.values()), flush=True)
if os.environ.get("DRY"): sys.exit(0)
asyncio.run(run_all(tasks, f"{F}/pairs.log.jsonl", int(os.environ.get("CONC", "20"))))
