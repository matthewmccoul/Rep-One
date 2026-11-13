"""Inventory every function definition in src/: name, file, lines, params, body; name-level call graph; references from tests/ and apps/; linker-dead flag."""
import glob, json, os, re, sys, collections
W = sys.argv[1]; C = f"{W}/code"
defre = re.compile(r"^(?:static\s+|inline\s+|constexpr\s+)*[A-Za-z_][\w:<>,&\*\s]*?\b([A-Za-z_]\w*)\s*\(([^;{)]*(?:\([^)]*\)[^;{)]*)*)\)\s*(?:const)?\s*\{", re.M)
KW = {"if", "for", "while", "switch", "return", "sizeof", "catch"}
def strip(t):
    t = re.sub(r"/\*.*?\*/", lambda m: " " * len(m.group(0)), t, flags=re.S)
    return re.sub(r"//[^\n]*", lambda m: " " * len(m.group(0)), t)
funcs = []
for f in sorted(glob.glob(f"{C}/src/*.cpp")):
    raw = open(f, errors="replace").read(); t = strip(raw); rel = os.path.relpath(f, C)
    for m in defre.finditer(t):
        n = m.group(1)
        if n in KW: continue
        d, j = 0, t.index("{", m.end() - 1)
        k = j
        while k < len(t):
            if t[k] == "{": d += 1
            elif t[k] == "}":
                d -= 1
                if d == 0: break
            k += 1
        params = m.group(2).strip(); np = 0 if not params or params == "void" else params.count(",") + 1
        line = raw[:m.start()].count("\n") + 1; end = raw[:k].count("\n") + 1
        funcs.append({"name": n, "file": rel, "line": line, "end": end, "nparams": np, "sig": " ".join(raw[m.start():j].split())[:300],
                      "body": raw[m.start():k + 1], "cbody": t[m.start():k + 1]})
names = collections.Counter(fn["name"] for fn in funcs)
ident = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
for fn in funcs:
    inner = fn["cbody"][fn["cbody"].index("{"):]
    fn["calls"] = sorted({x for x in ident.findall(inner) if x in names and x != fn["name"]})
callers = collections.defaultdict(set)
for fn in funcs:
    for c in fn["calls"]: callers[c].add(f"{fn['name']}@{fn['file']}")
def refs(dirpat):
    r = collections.Counter()
    for f in glob.glob(dirpat):
        t = strip(open(f, errors="replace").read())
        for x in ident.findall(t):
            if x in names: r[x] += 1
    return r
tests = refs(f"{C}/tests/*.cpp") + refs(f"{C}/tests/*.h"); apps = refs(f"{C}/apps/*/*.cpp")
unw = open(f"{C}/UNWIRED.md").read()
dead = set()
for l in unw.splitlines():
    mm = re.match(r"- `([^`]+)` \(\d+\): (.*)", l)
    if mm:
        for x in re.split(r", (?=[A-Za-z_~ ])", mm.group(2)): dead.add((mm.group(1), x.strip()))
for fn in funcs:
    fn["callers"] = sorted(callers[fn["name"]]); fn["test_refs"] = tests[fn["name"]]; fn["app_refs"] = apps[fn["name"]]
    fn["dead"] = (os.path.basename(fn["file"]), fn["name"]) in dead
    fn["game_layer"] = os.path.basename(fn["file"]).startswith(("game_", "feat_", "features", "feature_registry"))
    fn["lines"] = fn["end"] - fn["line"] + 1
    del fn["cbody"]
json.dump(funcs, open(f"{W}/notes/inventory.json", "w"), indent=0)
multi = [n for n, c in names.items() if c > 1]
print(len(funcs), "definitions;", len(names), "names;", len(multi), "names defined more than once;", sum(1 for f in funcs if f["dead"]), "dead (linker list)")
print("files:", len({f['file'] for f in funcs}), "| total body lines:", sum(f["lines"] for f in funcs))
