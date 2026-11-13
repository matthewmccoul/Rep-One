"""Mechanical redirect: delete the named loser definitions (and header declarations), rename their calls to the keeper,
optionally rewrite calls with a regex, include shared_helpers.h where the keeper is now called. Then gate.
usage: redirect_mech.py <S> <keeper> <msg> <loser@file>... [--sub REGEX=>REPL] [--export file:name]"""
import glob, json, os, re, subprocess, sys
S = sys.argv[1]; keeper = sys.argv[2]; msg = sys.argv[3]; rest = sys.argv[4:]; W = os.environ.get("W", f"{S}/prune"); C = f"{W}/code"
subs = []; losers = []; exports = []
it = iter(rest)
for a in it:
    if a == "--sub": x = next(it); subs.append(x.split("=>", 1))
    elif a == "--export": exports.append(next(it))
    else: losers.append(a)
sys.path.insert(0, f"{W}/scripts"); from gate import Gate
argv0 = sys.argv; sys.argv = [argv0[0], S, "src"]; exec(open(f"{W}/scripts/delete_unrun.py").read().split('if phase == "src":')[0]); C = f"{W}/code"   # the exec resets C; restore it
inv = json.load(open(f"{W}/notes/inventory.json"))
for e in exports:
    f, n = e.split(":"); d = [x for x in inv if x["name"] == n and x["file"] == f][0]
    print(subprocess.run(["python3", "-I", f"{W}/scripts/export_keeper.py", S, f, n, str(d["line"])], capture_output=True, text=True).stdout.strip())
for lo in sorted(losers, key=lambda s: s):
    n, f = lo.split("@")
    raw = open(f"{C}/{f}").read(); L = raw.split("\n"); st = strip(raw).split("\n")
    idx = [i for i, l in enumerate(st) if re.search(rf"^[A-Za-z_][\w:<>,&\*\s]*\b{re.escape(n)}\s*\(", l) and "{" in "\n".join(st[i:i + 4]) and ";" not in l.split("{")[0]]
    for L0 in sorted(idx, reverse=True):
        txt = "\n".join(st[L0:]); o = txt.find("{"); e = block_end(txt, o)
        if ";" in txt[:o]: continue
        b = L0 + txt[:e].count("\n") + 1; a = with_comments(L, L0)
        del L[a:b]; del st[a:b]
    open(f"{C}/{f}", "w").write("\n".join(L))
    for h in glob.glob(f"{C}/include/mordor/*.h"):
        t = open(h).read(); t2 = re.sub(rf"^[^\n;{{}}#]*\b{re.escape(n)}\s*\([^;{{}}]*\)\s*(const)?\s*;[^\n]*\n", "", t, flags=re.M) if n != keeper else t
        if t2 != t: open(h, "w").write(t2)
names = {l.split("@")[0] for l in losers} - {keeper}
for f in glob.glob(f"{C}/src/*.cpp"):
    t = open(f).read(); t2 = t
    for n in names:
        t2 = re.sub(rf"^[ \t]*(?:static\s+|inline\s+)*(?!return\b)[A-Za-z_][\w:<>,]*[\s&\*]+{re.escape(n)}\s*\([^;{{}}()=]*\)\s*(?:const)?\s*;[^\n]*\n", "", t2, flags=re.M)
    if t2 != t: open(f, "w").write(t2)
for f in glob.glob(f"{C}/src/*.cpp") + glob.glob(f"{C}/tests/*.cpp") + glob.glob(f"{C}/apps/*/*.cpp"):
    t = open(f).read(); t0 = t
    for n in names: t = re.sub(rf"\b{re.escape(n)}(\s*\()", keeper + r"\1", t)
    for pat, rep in subs: t = re.sub(pat, rep, t)
    if t != t0 and '#include "mordor/shared_helpers.h"' not in t:
        last = [m.end() for m in re.finditer(r"^#include[^\n]*\n", t, re.M)]; pos = last[-1] if last else 0
        t = t[:pos] + '#include "mordor/shared_helpers.h"\n' + t[pos:]
    if t != t0: open(f, "w").write(t)
g = Gate(W); r = g.check()
if r["ok"]: g.commit(msg); print("DONE", r["srd"], r["engine"])
else: g.revert(); print("FAILED", r.get("why"), r.get("log", "")[:1200])
