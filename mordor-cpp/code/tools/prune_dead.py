"""Delete function definitions the linker proved dead (unreachable from the console and from both test binaries) that have a LIVE sibling of the same name.
Input: logs/dead_dupes.json = [[mangled, demangled, objfile], ...]. Gate-protected (suites + golden). usage: prune_dead.py [--apply]"""
import json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cant import ROOT
rows = json.load(open(f"{ROOT}/logs/dead_dupes.json"))
def split_sig(d):
    m = re.match(r"(?:mordor::)?(?:([\w:]+)::)?(\w+)\((.*)\)(?: const)?$", d.replace("mordor::", ""))
    name = re.match(r"([\w:~]+)\(", d.replace("mordor::", "")).group(1)
    params = d[d.index("(") + 1:d.rindex(")")]
    depth, n, cur = 0, 0, ""
    for ch in params:
        if ch in "<(": depth += 1
        if ch in ">)": depth -= 1
        if ch == "," and depth == 0: n += 1
    return name.split("::")[-1], (0 if not params.strip() else n + 1)
def find_def(text, name, nparams):
    out = []
    for m in re.finditer(rf"^(?!\s|//|#)[A-Za-z_][^\n;{{}}]*?\b{re.escape(name)}\s*\(([^;{{}}]*)\)\s*(?:const\s*)?\{{", text, re.M):
        ps = m.group(1); depth = n = 0
        for ch in ps:
            if ch in "<(": depth += 1
            if ch in ">)": depth -= 1
            if ch == "," and depth == 0: n += 1
        cnt = 0 if not ps.strip() else n + 1
        if cnt == nparams: out.append(m)
    return out
removed, skipped = [], []
only = sys.argv[sys.argv.index("--only") + 1] if "--only" in sys.argv else None
skipf = set(sys.argv[sys.argv.index("--skip") + 1].split(",")) if "--skip" in sys.argv else set()
skipfn = set(open(f"{ROOT}/logs/prune_skipfn.txt").read().splitlines()) if os.path.exists(f"{ROOT}/logs/prune_skipfn.txt") else set()
for mangled, dem, obj in rows:
    if dem in skipfn: skipped.append((dem, "kept: still called by surviving code")); continue
    if (only and obj != only) or obj in skipf: continue
    name, npar = split_sig(dem); path = f"{ROOT}/src/{obj}"
    if not os.path.exists(path): skipped.append((dem, "no file")); continue
    text = open(path).read(); ms = find_def(text, name, npar)
    if len(ms) != 1: skipped.append((dem, f"{len(ms)} candidate definitions")); continue
    m = ms[0]; start = m.start()
    end = text.find("\n}\n", m.end())
    if end < 0: skipped.append((dem, "no end")); continue
    end += 3
    # take the comment block directly above
    lines_before = text[:start].split("\n"); k = len(lines_before) - 1
    while k > 0 and lines_before[k - 1].lstrip().startswith("//"): k -= 1
    start = len("\n".join(lines_before[:k])) + (1 if k > 0 else 0)
    body = text[m.end() - 1:end]
    if body.count("{") != body.count("}"): skipped.append((dem, "unbalanced braces")); continue
    removed.append((dem, obj, text[start:end].count("\n")))
    if "--apply" in sys.argv: open(path, "w").write(text[:start] + text[end:].lstrip("\n"))
print("removed" if "--apply" in sys.argv else "would remove", len(removed), "definitions,", sum(r[2] for r in removed), "lines; skipped", len(skipped))
for s in skipped[:12]: print("  skip", s)
json.dump({"removed": removed, "skipped": skipped}, open(f"{ROOT}/logs/prune_dead_report.json", "w"), indent=1)
