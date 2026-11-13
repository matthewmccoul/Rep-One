"""Remove duplicate declarations that make the public headers fail to compile together (later header loses). usage: dedupe_headers.py"""
import re, subprocess, sys, os
sys.path.insert(0, os.path.dirname(__file__))
ROOT = "/tmp/claude-0/-home-user-Rep-One/17a752bb-9e88-5884-a5bf-3b7fc4ab8c87/scratchpad/mordor/code"
PAT = re.compile(r"(include/mordor/[\w.]+\.h):(\d+):\d+: error: (?:redefinition of|multiple definition of|conflicting declaration|‘[^’]+’ redeclared as|redeclaration of)")
def remove_decl(path, line_no):
    lines = open(path).read().split("\n"); i = line_no - 1
    # include preceding comment lines
    j = i
    while j > 0 and lines[j - 1].strip().startswith("//"): j -= 1
    depth = 0; k = i; seen_brace = False
    while k < len(lines):
        depth += lines[k].count("{") - lines[k].count("}"); seen_brace |= "{" in lines[k]
        if (seen_brace and depth <= 0 and lines[k].rstrip().endswith(";")) or (not seen_brace and lines[k].rstrip().endswith(";")): break
        k += 1
    del lines[j:k + 1]
    open(path, "w").write("\n".join(lines))
removed = []
for it in range(40):
    subprocess.run(["python3", "-I", f"{ROOT}/tools/build.py", "headers"], capture_output=True)
    out = open(f"{ROOT}/logs/last_build.txt").read()
    hits = []
    for m in PAT.finditer(out): hits.append((m.group(1), int(m.group(2))))
    if not hits: break
    # remove one declaration per file per pass (line numbers shift), the last one first
    done = set()
    for f, ln in sorted(hits, key=lambda x: -x[1]):
        if f in done: continue
        remove_decl(f"{ROOT}/{f}", ln); removed.append((f, ln)); done.add(f)
print("removed", len(removed), "duplicate declarations;", "headers now compile" if not hits else "errors remain (see logs/last_build.txt)")
