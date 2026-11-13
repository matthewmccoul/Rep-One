"""Stage B: for each non-plumbing C++-only function, one ant with the closest OpenDejenol methods answers one question:
does OpenDejenol contain this same behaviour? usage: stage_b.py <S> [limit]"""
import asyncio, json, math, os, re, sys, collections
S = sys.argv[1]; F = f"{S}/feat"; sys.path.insert(0, f"{S}/ants2/scripts"); from army import run_all, est
C = f"{S}/prune/code"; lim = int(sys.argv[2]) if len(sys.argv) > 2 else None
idx = json.load(open(f"{F}/a_index.json")); cs = json.load(open(f"{F}/cs_index.json"))
STOP = set("the a an of to and or in is for on by with it this that be as at from are not if else return const int i32 i64 u32 f64 double bool void auto static str string var new public private true false null nullptr mordor ctx std".split())
def toks(s):
    s = re.sub(r"([a-z])([A-Z])", r"\1 \2", s)
    return [w for w in re.findall(r"[a-z]{3,}|\d+(?:\.\d+)?", s.lower()) if w not in STOP]
docs = [collections.Counter(toks(c["name"] + " " + c["name"] + " " + c["cls"] + " " + c["text"][:4000])) for c in cs]
df = collections.Counter(w for d in docs for w in d); N = len(docs); avg = sum(sum(d.values()) for d in docs) / N
def bm25(q):
    sc = []
    for i, d in enumerate(docs):
        L = sum(d.values()); s = 0.0
        for w in q:
            if w in d: s += math.log(1 + (N - df[w] + .5) / (df[w] + .5)) * d[w] * 2.2 / (d[w] + 1.2 * (1 - .75 + .75 * L / avg))
        sc.append((s, i))
    return [i for s, i in sorted(sc, reverse=True)[:8] if s > 0]
SYS = """You compare ONE function from a C++ re-implementation of the 1995 RPG "Mordor: The Depths of Dejenol" with methods from OpenDejenol, a C# port of the original game (Mordor.Core = rules, Mordor.App = game screens and flow, Mordor.Modding and Mods = the port's mod system). The candidate methods were picked by keyword search, so the right one may be missing. Answer one question: does any candidate do the same thing as the C++ function?
SAME = a candidate does the same thing (same inputs and outcome; small structural differences are fine).
PARTIAL = a candidate does part of it, or a closely related version with different details.
NOT_IN_CANDIDATES = none of the candidates does it.
Reply with exactly three lines:
VERDICT: <SAME|PARTIAL|NOT_IN_CANDIDATES>
WHERE: <file and Class.Method of the best candidate, or none>
EVIDENCE: <one sentence: what matches or what differs>"""
tasks = []
for tid, dd in sorted(idx.items()):
    p = f"{F}/a/{tid}.md"
    if not os.path.exists(p): continue
    a = open(p).read(); k = re.search(r"KIND:\s*(\w+)", a); what = re.search(r"WHAT:\s*(.*)", a)
    if not k or k.group(1) in ("PLUMBING", "STUB"): continue
    name, rest = dd.split("@"); f, line = rest.rsplit(":", 1)
    lines = open(f"{C}/{f}", errors="replace").read().splitlines(); a0 = int(line) - 1
    pre = []; j = a0 - 1
    while j >= 0 and lines[j].strip().startswith("//") and len(pre) < 10: pre.insert(0, lines[j]); j -= 1
    e = a0; d = 0; seen = False
    while e < len(lines):
        d += lines[e].count("{") - lines[e].count("}"); seen = seen or "{" in lines[e]; e += 1
        if seen and d <= 0: break
    body = "\n".join(pre + lines[a0:e])[:3500]
    cand = bm25(toks(name + " " + name + " " + (what.group(1) if what else "") + " " + body))
    cands = "\n\n".join(f"--- CANDIDATE {n+1}: {cs[i]['file']} {cs[i]['cls']}.{cs[i]['name']}\n{cs[i]['text'][:2200]}" for n, i in enumerate(cand))
    u = f"C++ FUNCTION ({f}):\n{body}\n\nWHAT IT DOES (from an earlier reader): {what.group(1) if what else '?'}\n\nCANDIDATE OPENDEJENOL METHODS:\n{cands}"
    tasks.append({"id": "b" + tid[1:], "system": SYS, "user": u, "out": f"{F}/b/b{tid[1:]}.md", "kind": k.group(1)})
os.makedirs(f"{F}/b", exist_ok=True)
tasks = tasks[:lim] if lim else tasks
if os.environ.get("DUMP"): json.dump(tasks, open(f"{F}/b_tasks.json", "w")); sys.exit(0)
print(len(tasks), "ants; max est", max(est(t["system"], t["user"]) for t in tasks), flush=True)
asyncio.run(run_all(tasks, f"{F}/b.log.jsonl", int(os.environ.get("CONC", "8"))))
