"""Second look at MISSING rows: the C# method body + the C++ function bodies most similar to it (rarity-weighted identifier overlap). usage: second_look.py [DRY]"""
import asyncio, glob, json, math, os, re, sys, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from army import run_all, est
A2 = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/ants2"; CORE = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/build/od/src/Mordor.Core"; C = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/dl/cpp"
PLAT = {"Import/VbLayout.cs","WindowLayout.cs","WaveMix.cs","X87.cs","MazeSelect.cs","CombatSounds.cs","LimitedVersion.cs","WholeRows.cs","FramePause.cs","PlayClock.cs","GameOptions.cs","CharStatFile.cs","Roster.cs"}
u = json.load(open(f"{A2}/notes/mapping_unique.json"))
todo = [x for x in u if x["status"] == "MISSING" and x["file"] not in PLAT]
def body_of(text, name):
    for m in re.finditer(rf"^\s{{4,8}}(?:public|internal|private)[^\n;=]*\b{re.escape(name)}\s*(?:<[^>\n]*>)?\s*\(", text, re.M):
        i = text.find("{", m.end()); semi = text.find(";", m.end()); arrow = text.find("=>", m.end())
        if 0 <= arrow < (i if i >= 0 else 10**9) and (semi > arrow): return text[m.start():semi + 1]
        if i < 0: continue
        d, j = 0, i
        while j < len(text):
            if text[j] == "{": d += 1
            elif text[j] == "}":
                d -= 1
                if d == 0: return text[m.start():j + 1]
            j += 1
    return ""
def words(t):
    out = []
    for w in re.findall(r"[A-Za-z][A-Za-z0-9]*", t):
        for p in re.findall(r"[A-Z]?[a-z]+|[A-Z]+(?![a-z])|\d+", w):
            p = p.lower()
            if len(p) >= 3: out.append(p)
    return out
defre = re.compile(r"^[A-Za-z_][\w:<>,&\*\s]*?\b([A-Za-z_]\w*)\s*\([^;{)]*\)\s*(?:const)?\s*\{", re.M)
cdefs = []
for f in glob.glob(f"{C}/src/*.cpp"):
    t = open(f, errors="replace").read()
    for m in defre.finditer(t):
        n = m.group(1)
        if n in ("if", "for", "while", "switch"): continue
        d, j = 0, m.end() - 1
        while j < len(t):
            if t[j] == "{": d += 1
            elif t[j] == "}":
                d -= 1
                if d == 0: break
            j += 1
        k = t.rfind("\n\n", 0, m.start()); pre = t[k:m.start()] if k >= 0 and m.start() - k < 500 else ""
        txt = pre + t[m.start():j + 1]; cdefs.append((n, os.path.basename(f), txt, set(words(txt))))
df = collections.Counter(w for *_, ws in cdefs for w in ws); N = len(cdefs)
STOP = set("the and for return int var new null true false void static public this else bool double string const auto i32 i64 f64 str vec".split())
jobs = []
for x in todo:
    cls, meth = x["method"].split(".", 1) if "." in x["method"] else ("", x["method"])
    meth = re.sub(r"\(.*", "", meth)
    src = open(f"{CORE}/{x['file']}", encoding="utf-8", errors="replace").read(); b = body_of(src, meth)[:3500]
    q = collections.Counter(w for w in words(b + " " + x["note"] + " " + meth + " " + meth) if w not in STOP)
    sc = []
    for n, f, txt, ws in cdefs:
        s = sum(math.log(1 + N / (1 + df[w])) * min(c, 3) for w, c in q.items() if w in ws) / (1 + len(txt) / 8000)
        if n.lower() == meth.lower(): s += 50
        sc.append((s, n, f, txt))
    sc.sort(key=lambda z: -z[0]); cands = []; used = 0
    for s, n, f, txt in sc[:6]:
        piece = f"// ---- src/{f} :: {n}\n{txt[:1800]}"; cands.append(piece); used += len(piece)
    jobs.append({"row": x, "csharp": b or "(body not found)", "cands": cands})
packets = []; cur = []; size = 0
for j in jobs:
    blk = f"### C# {j['row']['method']} ({j['row']['file']})\n{j['csharp']}\n\n--- C++ candidates for it ---\n" + "\n\n".join(j["cands"]) + "\n"
    if cur and size + len(blk) > 48000: packets.append(cur); cur, size = [], 0
    cur.append((j, blk)); size += len(blk)
if cur: packets.append(cur)
SYS = """You check whether a C++ re-implementation of the 1995 RPG Mordor contains equivalents of C# reference methods that a first pass could not match. For each C# method below you get its source and the bodies of the C++ functions that look most similar. Decide, for each C# method, one block:
### M <ClassName.MethodName exactly as given>
Status: PRESENT | PARTIAL | MISSING
C++: <C++ function name(s) from the candidates, exactly, or NONE>
Note: <one sentence: what matches or what is absent>
PRESENT = a candidate does the same rule (same inputs, same result, allowing different names and structure, including logic folded into a larger function). PARTIAL = part of it, or a simplified version. MISSING = none of the candidates does it. Judge from the code shown; do not invent names. Output only the blocks."""
os.makedirs(f"{A2}/second", exist_ok=True); tasks = []
for k, p in enumerate(packets, 1):
    user = "\n\n".join(b for _, b in p)
    tasks.append({"id": f"s{k:03d}", "system": SYS, "user": user, "out": f"{A2}/second/{k:03d}.md"})
json.dump([[j["row"]["method"] for j, _ in p] for p in packets], open(f"{A2}/notes/second_manifest.json", "w"))
print(len(todo), "missing rows ->", len(tasks), "ants; max est", max(est(t["system"], t["user"]) for t in tasks), "; no C# body for", sum(1 for j in jobs if j["csharp"].startswith("(body")))
if len(sys.argv) > 1:
    for j in jobs:
        if j["row"]["method"] in ("Rules.ChestExp", "CombatEngine.IsOver", "CombatEngine.Won", "Rules.EngageChance"): print(j["row"]["method"], "->", [c.split("\n")[0] for c in j["cands"]])
    sys.exit(0)
asyncio.run(run_all(tasks, f"{A2}/second.log.jsonl", 4))
