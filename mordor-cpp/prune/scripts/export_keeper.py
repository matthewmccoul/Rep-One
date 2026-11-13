"""Make a file-local function callable from other files, in place: drop `static`, or close the anonymous namespace around it,
and declare it in include/mordor/shared_helpers.h (plus the includes its file uses). usage: export_keeper.py <S> <file> <name> <line>"""
import os, re, sys
S, rel, name, line = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4]); C = os.environ.get("W", f"{S}/prune") + "/code"
sys.argv = [sys.argv[0], S, "src"]; exec(open(f"{S}/prune/scripts/delete_unrun.py").read().split('if phase == "src":')[0]); C = os.environ.get("W", f"{S}/prune") + "/code"   # the exec resets C; restore it
raw = open(f"{C}/{rel}").read(); L = raw.split("\n"); st = strip(raw)
L0 = line - 1
for d in (0, -1, 1, -2, 2):
    if re.search(rf"\b{re.escape(name)}\s*\(", L[L0 + d]): L0 += d; break
txt = "\n".join(st.split("\n")[L0:]); o = txt.find("{"); e = block_end(txt, o); b = L0 + txt[:e].count("\n") + 1
a = with_comments(L, L0)
sig = re.sub(r"\s+", " ", re.sub(r"^(?:static\s+|inline\s+)+", "", strip(txt[:o]).strip())) + ";"
L[L0] = re.sub(r"^(\s*)(?:static\s+)", r"\1", L[L0])
# inside an anonymous namespace? count opens/closes of `namespace {` before L0
pre = "\n".join(st.split("\n")[:a]); depth = 0
for m in re.finditer(r"namespace\s*\{|\}\s*//\s*namespace\s*$", pre, re.M): depth += 1 if m.group(0).startswith("namespace") else -1
if depth > 0 and re.search(r"namespace\s*\{", pre):
    L.insert(b, "namespace {"); L.insert(a, "}  // namespace")
open(f"{C}/{rel}", "w").write("\n".join(L))
H = f"{C}/include/mordor/shared_helpers.h"; h = open(H).read()
for inc in re.findall(r'^#include\s+"mordor/[^"]+"', raw, re.M):
    if inc not in h and "shared_helpers" not in inc and os.path.exists(f"{C}/include/{inc.split(chr(34))[1]}"): h = h.replace('#include "mordor/game_data.h"\n', '#include "mordor/game_data.h"\n' + inc + "\n", 1)
if sig not in h: h = h.replace("}  // namespace mordor", sig + "\n}  // namespace mordor")
open(H, "w").write(h)
t = open(f"{C}/{rel}").read()
if '#include "mordor/shared_helpers.h"' not in t:
    last = [m.end() for m in re.finditer(r"^#include[^\n]*\n", t, re.M)]; pos = last[-1] if last else 0
    open(f"{C}/{rel}", "w").write(t[:pos] + '#include "mordor/shared_helpers.h"\n' + t[pos:])
print("exported", name, "from", rel, "as:", sig)
