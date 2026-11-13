"""Merge clusters of identical helpers into one copy each in src/shared_helpers.cpp (declared in include/mordor/shared_helpers.h).
Per cluster: copy the keeper there, delete every member's definition and header declaration, rename call sites, add the include, gate.
usage: merge_clusters.py <S>"""
import glob, json, os, re, subprocess, sys
S = sys.argv[1]; W = f"{S}/prune"; C = f"{W}/code"; sys.path.insert(0, f"{W}/scripts"); from gate import Gate
sys.argv = [sys.argv[0], S, "src"]; exec(open(f"{W}/scripts/delete_unrun.py").read().split('if phase == "src":')[0])  # strip, block_end, with_comments
plan = json.load(open(f"{W}/notes/plan_clusters.json")); state_f = f"{W}/notes/cluster_state.json"
state = json.load(open(state_f)) if os.path.exists(state_f) else {}
SH, SC = f"{C}/include/mordor/shared_helpers.h", f"{C}/src/shared_helpers.cpp"
if False:
    open(SH, "w").write('#pragma once\n// One shared copy of helpers that several modules had each written for themselves.\n#include "mordor/types.h"\n#include "mordor/game_data.h"\n#include "mordor/state_party.h"\n\nnamespace mordor {\n}  // namespace mordor\n')
    open(SC, "w").write('#include "mordor/shared_helpers.h"\n\nnamespace mordor {\n}  // namespace mordor\n')
    cm = open(f"{C}/CMakeLists.txt").read()
def span(path, name, line):
    raw = open(path).read(); L = raw.split("\n"); st = strip(raw).split("\n"); L0 = line - 1
    for d in (0, -1, 1, -2, 2, -3, 3):
        if 0 <= L0 + d < len(L) and re.search(rf"\b{re.escape(name)}\s*\(", L[L0 + d]) and not L[L0 + d].strip().startswith("//"): L0 += d; break
    else: return None
    txt = "\n".join(st[L0:]); o = txt.find("{"); e = block_end(txt, o)
    if o < 0 or e < 0 or ";" in txt[:o]: return None
    b = L0 + txt[:e].count("\n") + 1; a = L0
    if a > 0 and re.match(r"\s*template\s*<", L[a - 1]): a -= 1
    return with_comments(L, a), b, L
g = Gate(W)
for i, p in enumerate(plan):
    if state.get(str(i), {}).get("status") == "DONE": continue
    k = p["keeper"]; members = [k] + p["losers"]; canon = k["name"]
    ks = span(f"{C}/{k['file']}", k["name"], k["line"])
    if not ks: state[str(i)] = {"status": "FAILED", "why": "keeper not found"}; continue
    kbody = "\n".join(ks[2][ks[0]:ks[1]])
    kbody = re.sub(r"^(\s*)(?:static\s+|inline\s+)+", r"\1", kbody, flags=re.M)
    sig = re.search(rf"^([^\n{{]*\b{re.escape(canon)}\s*\([^{{]*\))\s*(const)?\s*\{{", strip(kbody), re.M)
    decl = re.sub(r"\s+", " ", kbody[sig.start(1):sig.end(1)]).strip() + ";"
    # includes the keeper's file uses, so the moved body still compiles
    incs = re.findall(r'^#include\s+"mordor/[^"]+"', open(f"{C}/{k['file']}").read(), re.M)
    # 1) delete every member definition (bottom-up per file) and its header declarations
    for m in sorted(members, key=lambda m: (m["file"], -m["line"])):
        sp = span(f"{C}/{m['file']}", m["name"], m["line"])
        if not sp: continue
        a, b, L = sp; del L[a:b]; open(f"{C}/{m['file']}", "w").write("\n".join(L))
        for h in glob.glob(f"{C}/include/mordor/*.h"):
            if h == SH: continue
            t = open(h).read(); t2 = re.sub(rf"^[^\n;{{}}#]*\b{re.escape(m['name'])}\s*\([^;{{}}]*\)\s*(const)?\s*;[^\n]*\n", "", t, flags=re.M) if not re.search(rf"\b{re.escape(m['name'])}\s*\([^;]*\)\s*(const)?\s*\{{", t) else t
            if t2 != t: open(h, "w").write(t2)
    # 2) add the canonical copy to the shared module
    h = open(SH).read(); c = open(SC).read()
    for inc in incs:
        if inc not in c: c = c.replace('#include "mordor/shared_helpers.h"\n', '#include "mordor/shared_helpers.h"\n' + inc + "\n", 1)
        if inc not in h and "shared_helpers" not in inc: h = h.replace('#include "mordor/game_data.h"\n', '#include "mordor/game_data.h"\n' + inc + "\n", 1)
    h = h.replace("}  // namespace mordor", decl + "\n}  // namespace mordor"); c = c.replace("}  // namespace mordor", kbody + "\n\n}  // namespace mordor")
    open(SH, "w").write(h); open(SC, "w").write(c)
    # 3) rename call sites of every differently named member, and include the shared header where calls exist
    names = {m["name"] for m in members}
    for f in glob.glob(f"{C}/src/*.cpp") + glob.glob(f"{C}/tests/*.cpp") + glob.glob(f"{C}/apps/*/*.cpp") + glob.glob(f"{C}/include/mordor/*.h"):
        if f in (SH, SC): continue
        t = open(f).read(); t0 = t
        for n in names - {canon}: t = re.sub(rf"\b{re.escape(n)}(\s*\()", canon + r"\1", t)
        if re.search(rf"\b{re.escape(canon)}\s*\(", strip(t)) and '#include "mordor/shared_helpers.h"' not in t and f.endswith(".cpp"):
            last = [mm.end() for mm in re.finditer(r"^#include[^\n]*\n", t, re.M)]; pos = last[-1] if last else 0
            t = t[:pos] + '#include "mordor/shared_helpers.h"\n' + t[pos:]
        if t != t0: open(f, "w").write(t)
    r = g.check(golden_must_match=True)
    if r["ok"]:
        g.commit(f"merge identical helpers into shared_helpers: keep {canon}; remove " + ", ".join(f"{m['name']}@{os.path.basename(m['file'])}" for m in members))
        state[str(i)] = {"status": "DONE", "suites": [r["srd"], r["engine"]]}; print(i, "DONE", canon, r["srd"], r["engine"], flush=True)
    else:
        g.revert(); state[str(i)] = {"status": "FAILED", "why": r.get("why"), "log": r.get("log", "")[:2500]}; print(i, "FAILED", canon, r.get("why"), flush=True)
    json.dump(state, open(state_f, "w"), indent=1)
