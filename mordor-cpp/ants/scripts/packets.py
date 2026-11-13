"""Build one packet per Mordor.Core file (split if large): the Core code + C++ candidates found by (a) the C++ source citing the Core name, (b) name-token similarity."""
import glob, json, os, re, collections
CORE = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/build/od/src/Mordor.Core"; C = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/dl/cpp"; A2 = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/ants2"
idx = json.load(open(f"{A2}/notes/cpp_index.json"))
byname = collections.defaultdict(list)
for d in idx: byname[d["name"]].append(d)
def toks(n): return {w.lower() for w in re.findall(r"[A-Z]?[a-z]+|[A-Z]+(?![a-z])|\d+", n) if len(w) >= 3 or (len(w) == 2 and w.isupper())}
dtoks = {d["name"]: toks(d["name"]) for d in idx}
norm = {d["name"]: d["name"].lower() for d in idx}
# C++ definitions with spans, to attribute citations of Core names to C++ functions
defre = re.compile(r"^[A-Za-z_][\w:<>,&\*\s]*?\b([A-Za-z_]\w*)\s*\([^;{)]*\)\s*(?:const)?\s*\{", re.M)
cdefs = []
for f in glob.glob(f"{C}/src/*.cpp"):
    t = open(f, errors="replace").read()
    for m in defre.finditer(t):
        n = m.group(1)
        if n in ("if", "for", "while", "switch"): continue
        depth, j = 0, m.end() - 1
        while j < len(t):
            if t[j] == "{": depth += 1
            elif t[j] == "}":
                depth -= 1
                if depth == 0: break
            j += 1
        pre = t[max(0, m.start() - 600):m.start()]
        cdefs.append((n, os.path.basename(f), (pre + t[m.start():j + 1])))
mpat = re.compile(r"^\s{4,8}(public|internal)\s+([^=;{\n]*)\b(\w+)\s*(?:<[^>\n]*>)?\s*\(", re.M)
os.makedirs(f"{A2}/packets", exist_ok=True); man = []
for f in sorted(glob.glob(f"{CORE}/**/*.cs", recursive=True)):
    rel = os.path.relpath(f, CORE); t = open(f, encoding="utf-8", errors="replace").read()
    methods = [m.group(3) for m in mpat.finditer(t) if m.group(3) not in ("if", "for", "while", "switch", "return", "new", "static", "readonly")]
    if not methods: continue
    cls = os.path.splitext(os.path.basename(f))[0]
    # pieces of at most 34k chars at method boundaries
    starts = [m.start() for m in mpat.finditer(t)]; pieces = []; a = 0
    for s in starts + [len(t)]:
        if s - a > 34000 and s > a:
            cut = max([x for x in starts if a < x < s] or [s]); pieces.append(t[a:cut]); a = cut
    pieces.append(t[a:])
    for pi, body in enumerate(pieces):
        ms = list(dict.fromkeys(m.group(3) for m in mpat.finditer(body)))
        if not ms: continue
        cand, why, pri = {}, {}, {}
        for mn in ms:
            # (a) citations: C++ functions whose text mentions Class.Method or the method name (>=6 chars)
            keys = [f"{cls}.{mn}"] + ([mn] if len(mn) >= 6 else [])
            for n, cf, txt in cdefs:
                if any(k in txt for k in keys): cand.setdefault(n, cf); why.setdefault(n, set()).add(mn); pri[n] = min(pri.get(n, 9), 1)
            # (a2) normalised-name match: same letters ignoring case, or one name contains the other
            nm = mn.lower()
            for n in norm:
                nn = norm[n]
                if nn == nm or (len(nm) >= 5 and (nm in nn or (len(nn) >= 6 and nn in nm))):
                    cand.setdefault(n, byname[n][0]["file"]); why.setdefault(n, set()).add(mn); pri[n] = 0
            # (b) token similarity
            tk = toks(mn) | ({cls.lower()} if len(toks(mn)) < 2 else set())
            sc = sorted(((len(tk & dt) / len(tk | dt), n) for n, dt in dtoks.items() if tk & dt), reverse=True)[:3]
            for s2, n in sc:
                if s2 >= 0.34: cand.setdefault(n, byname[n][0]["file"] if byname[n] else "?"); why.setdefault(n, set()).add(mn); pri[n] = min(pri.get(n, 9), 2)
        lines, used = [], 0
        for n in sorted(cand, key=lambda n: (pri.get(n, 9), -len(why[n]))):
            ds = byname.get(n) or [{"sig": f"(defined in src/{cand[n]}, no header declaration)", "file": "src/" + cand[n], "comment": "", "unwired": False}]
            for d in ds[:2]:
                s3 = f"{d['file']} :: {d['sig']}" + (f"   // {d['comment'][:160]}" if d['comment'] else "")
                if used + len(s3) > 18000: break
                lines.append(s3); used += len(s3)
        pid = f"{len(man)+1:04d}"
        txt = f"=== MORDOR.CORE FILE: {rel} (part {pi+1}/{len(pieces)}) ===\n{body}\n\n=== C++ CANDIDATES (declarations from the C++ demo that may implement methods above; name-matched or citing the C# name) ===\n" + "\n".join(lines)
        open(f"{A2}/packets/{pid}.txt", "w").write(txt)
        man.append({"id": pid, "file": rel, "part": pi + 1, "methods": ms, "chars": len(txt), "candidates": len(lines)})
json.dump(man, open(f"{A2}/notes/manifest.json", "w"), indent=1)
print(len(man), "packets;", sum(len(m["methods"]) for m in man), "methods; max chars", max(m["chars"] for m in man), "; zero-candidate packets", sum(1 for m in man if m["candidates"] == 0))
