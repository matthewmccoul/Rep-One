"""Stage C: for each C++ function stage B could not find in OpenDejenol, three ants each ask ONE different question.
Q1 second search (query from the plain-language description and comments, not code names), Q2 does the SRD describe it,
Q3 original 1995 game or added by a re-implementation. Plus a script pass over the function's own comment tags.
usage: stage_c.py <S> [limit]"""
import asyncio, json, math, os, re, sys, collections
S = sys.argv[1]; F = f"{S}/feat"; sys.path.insert(0, f"{S}/ants2/scripts"); from army import run_all, est
C = f"{S}/prune/code"; lim = int(sys.argv[2]) if len(sys.argv) > 2 else None
rows = [r for r in json.load(open(f"{F}/b_rows.json")) if r["verdict"] == "NOT_IN_CANDIDATES"]
cs = json.load(open(f"{F}/cs_index.json"))
srd = open(f"{S}/srd_art.txt").read()
chunks = [srd[i:i + 1800] for i in range(0, len(srd), 1500)]
STOP = set("the a an of to and or in is for on by with it this that be as at from are not if else return const int i32 i64 u32 f64 double bool void auto static str string var new public private true false null nullptr mordor ctx std function returns given character".split())
def toks(s):
    s = re.sub(r"([a-z])([A-Z])", r"\1 \2", s)
    return [w for w in re.findall(r"[a-z]{3,}|\d+(?:\.\d+)?", s.lower()) if w not in STOP]
class BM:
    def __init__(self, texts):
        self.d = [collections.Counter(toks(t)) for t in texts]; self.df = collections.Counter(w for d in self.d for w in d)
        self.N = len(self.d); self.avg = sum(sum(d.values()) for d in self.d) / self.N
    def top(self, q, k):
        sc = []
        for i, d in enumerate(self.d):
            L = sum(d.values()); s = 0.0
            for w in set(q):
                if w in d: s += math.log(1 + (self.N - self.df[w] + .5) / (self.df[w] + .5)) * d[w] * 2.2 / (d[w] + 1.2 * (.25 + .75 * L / self.avg))
            sc.append((s, i))
        return [i for s, i in sorted(sc, reverse=True)[:k] if s > 0]
bm_cs = BM([c["cls"] + " " + c["text"][:4000] for c in cs]); bm_srd = BM(chunks)
def body(defn):
    name, rest = defn.split("@"); f, line = rest.rsplit(":", 1)
    L = open(f"{C}/{f}", errors="replace").read().splitlines(); a0 = int(line) - 1
    pre = []; j = a0 - 1
    while j >= 0 and L[j].strip().startswith("//") and len(pre) < 10: pre.insert(0, L[j]); j -= 1
    e = a0; d = 0; seen = False
    while e < len(L):
        d += L[e].count("{") - L[e].count("}"); seen = seen or "{" in L[e]; e += 1
        if seen and d <= 0: break
    hdr = "\n".join(l for l in L[:10] if l.startswith("//"))
    return f, hdr, "\n".join(pre + L[a0:e])[:3500], "\n".join(pre)
def piece_kind(n):
    return ("Mordor.Core" if n <= 22 else "port audit/fidelity notes" if n <= 34 else "port docs/mod system" if n <= 41 else
            "Mordor.App (game screens)" if n <= 54 else "port self-check tests" if n <= 68 else "spell spreadsheet/play guide" if n <= 81 else
            "research notes/manual" if n <= 84 else "community wiki" if n <= 96 else "wiki+forum archive" if n == 97 else "forum archive")
SYS1 = """You compare ONE function from a C++ re-implementation of the 1995 RPG "Mordor: The Depths of Dejenol" with methods from OpenDejenol, a C# port of the original game (Mordor.Core = rules, Mordor.App = screens and game flow, Mordor.Modding/Mods = the port's mod system). An earlier search found nothing; these candidates come from a different search. Answer one question: does any candidate do the same thing, or contain the same rule inline inside a bigger method?
Reply with exactly three lines:
VERDICT: <SAME|PARTIAL|NOT_IN_CANDIDATES>
WHERE: <file and Class.Method, or none>
EVIDENCE: <one sentence>"""
SYS2 = """You check ONE function from a C++ re-implementation of the 1995 RPG "Mordor: The Depths of Dejenol" against excerpts from the Mordor SRD, a rules document compiled from many sources. Citation tags like [c18] name the source piece. Answer one question: does the SRD describe the mechanic this function implements?
Reply with exactly three lines:
VERDICT: <DESCRIBED|PARTLY|NOT_IN_EXCERPTS>
QUOTE: <the shortest SRD phrase or formula that matches, with its [cN] tags, or none>
NOTE: <one sentence: what matches or what the function adds beyond the SRD>"""
SYS3 = """You judge ONE function from a C++ re-implementation of the 1995 RPG "Mordor: The Depths of Dejenol" (a Windows dungeon crawler written in Visual Basic). Answer one question: is the behaviour this function implements something the original 1995 game does, or something a re-implementation added (a text-console command, a convenience, a test or debug aid, a mod hook, save-file plumbing of its own, an invented placeholder value)?
Reply with exactly two lines:
ORIGIN: <ORIGINAL_GAME|ADDED_BY_REIMPLEMENTATION|UNSURE>
REASON: <one sentence pointing at the evidence in the code or comments>"""
tasks = []; meta = {}
for r in rows[:lim] if lim else rows:
    f, hdr, b, pre = body(r["defn"])
    q = toks(r["what"] + " " + pre)
    srdx = "\n\n".join(f"--- SRD EXCERPT {n+1}:\n{chunks[i]}" for n, i in enumerate(bm_srd.top(toks(r["what"] + " " + b), 6)))
    csx = "\n\n".join(f"--- CANDIDATE {n+1}: {cs[i]['file']} {cs[i]['cls']}.{cs[i]['name']}\n{cs[i]['text'][:2000]}" for n, i in enumerate(bm_cs.top(q, 8)))
    fn = f"C++ FUNCTION ({f}; file header: {hdr})\n{b}\n\nWHAT IT DOES (from an earlier reader): {r['what']}"
    tags = sorted({int(x) for x in re.findall(r"\bc(\d{1,3})\b", pre + "\n" + hdr) if 1 <= int(x) <= 107})
    meta[r["id"]] = {"defn": r["defn"], "kind": r["kind"], "what": r["what"], "tags": tags, "tag_sources": sorted({piece_kind(t) for t in tags}),
                     "markers": sorted(set(re.findall(r"\b(UNKNOWN|ASSUMED|ASSUMPTION|CONFLICT|SIMPLIFIED|PLACEHOLDER|INVENTED|TODO|FIXED)\b", pre)))}
    for q_, sys_, u in (("1", SYS1, f"{fn}\n\nCANDIDATE OPENDEJENOL METHODS:\n{csx}"), ("2", SYS2, f"{fn}\n\nSRD EXCERPTS:\n{srdx}"), ("3", SYS3, fn)):
        tasks.append({"id": f"c{q_}_{r['id']}", "system": sys_, "user": u, "out": f"{F}/c/q{q_}_{r['id']}.md"})
os.makedirs(f"{F}/c", exist_ok=True); json.dump(meta, open(f"{F}/c_meta.json", "w"), indent=0)
print(len(rows), "functions,", len(tasks), "ants; max est", max(est(t["system"], t["user"]) for t in tasks), flush=True)
asyncio.run(run_all(tasks, f"{F}/c.log.jsonl", int(os.environ.get("CONC", "8"))))
