"""Delete every src/ function the console game never runs (linker --gc-sections at -O0 -fno-inline decides),
their header declarations when no definition of the name remains, and the tests/helpers that then fail to build.
usage: delete_unrun.py <S> <phase: src|headers|tests>"""
import collections, json, os, re, subprocess, sys
S, phase = sys.argv[1], sys.argv[2]; C = f"{S}/prune/code"; D = f"{S}/prune/dead"
def sh(c, cwd=C): r = subprocess.run(c, shell=True, cwd=cwd, capture_output=True, text=True); return r.returncode, r.stdout + r.stderr
def strip(t):
    t = re.sub(r"/\*.*?\*/", lambda m: re.sub(r"[^\n]", " ", m.group(0)), t, flags=re.S)
    t = re.sub(r"'(?:\\.|[^'\\\n])'", "' '", t)
    t = re.sub(r'"(?:\\.|[^"\\\n])*"', lambda m: '"' + " " * (len(m.group(0)) - 2) + '"', t)
    return re.sub(r"//[^\n]*", lambda m: " " * len(m.group(0)), t)
def block_end(t, open_pos):
    d = 0; k = open_pos
    while k < len(t):
        if t[k] == "{": d += 1
        elif t[k] == "}":
            d -= 1
            if d == 0: return k + 1
        k += 1
    return -1
def with_comments(lines, a):
    while a > 0 and lines[a - 1].strip().startswith("//"): a -= 1
    return a
if phase == "src":
    kept = collections.defaultdict(set)
    for l in open(f"{S}/feat/kept_syms.txt"):
        m = re.search(r"/prune/code/(src/[^:]+):(\d+)", l)
        if m: kept[m.group(1)].add(int(m.group(2)))
    inv = json.load(open(f"{S}/prune/notes/inventory.json"))
    dead = [d for d in inv if not any(int(d["line"]) <= x <= int(d["end"]) for x in kept[d["file"]])]
    byf = collections.defaultdict(list)
    for d in dead: byf[d["file"]].append(d)
    removed = []
    for f, ds in byf.items():
        raw = open(f"{C}/{f}", errors="replace").read(); lines = raw.split("\n"); st = strip(raw).split("\n")
        for d in sorted(ds, key=lambda d: -int(d["line"])):
            L0 = int(d["line"]) - 1
            if d["name"] not in "\n".join(lines[L0:L0 + 3]): print("SKIP (line drift)", f, d["name"]); continue
            # find the opening brace from the definition line, then its matching close, on comment/string-stripped text
            txt = "\n".join(st[L0:]); o = txt.find("{")
            e = block_end(txt, o)
            if o < 0 or e < 0: print("SKIP (no body)", f, d["name"]); continue
            b = L0 + txt[:e].count("\n") + 1
            a = L0
            if a > 0 and re.match(r"\s*template\s*<", lines[a - 1]): a -= 1
            a = with_comments(lines, a)
            del lines[a:b]; del st[a:b]
            while a < len(lines) and a > 0 and lines[a].strip() == "" and lines[a - 1].strip() == "": del lines[a]
            removed.append(f"{d['name']}@{f}:{d['line']}")
        open(f"{C}/{f}", "w").write("\n".join(lines))
    json.dump(removed, open(f"{D}/removed_functions.json", "w"), indent=0); print(len(removed), "definitions removed from", len(byf), "files")
elif phase == "headers":
    src = "\n".join(strip(open(os.path.join(C, "src", f), errors="replace").read()) for f in os.listdir(f"{C}/src") if f.endswith(".cpp"))
    hdrs = {f: open(f"{C}/include/mordor/{f}").read() for f in os.listdir(f"{C}/include/mordor") if f.endswith(".h")}
    allh = "\n".join(strip(t) for t in hdrs.values())
    gone = {r.split("@")[0] for r in json.load(open(f"{D}/removed_functions.json"))}
    defined = set(re.findall(r"\b([A-Za-z_]\w*)\s*\([^;{)]*(?:\([^)]*\)[^;{)]*)*\)\s*(?:const)?\s*(?:noexcept)?\s*\{", src + "\n" + allh))
    n = 0; dropped = []
    for f, t in hdrs.items():
        s = strip(t); cut = []
        for name in gone - defined:
            for m in re.finditer(rf"^[ \t]*(?!return\b|#)[A-Za-z_][\w:<>,&\*\s\[\]]*?\b{re.escape(name)}\s*\([^;{{}}]*\)\s*(?:const)?\s*(?:noexcept)?\s*;[^\n]*\n?", s, re.M):
                cut.append((m.start(), m.end(), name))
        if not cut: continue
        lines_start = t
        for a, b, name in sorted(cut, reverse=True):
            pre = t[:a].split("\n"); k = len(pre) - 1
            while k > 0 and pre[k - 1].strip().startswith("//"): k -= 1
            a2 = len("\n".join(pre[:k])) + (1 if k else 0)
            t = t[:a2] + t[b:]; n += 1; dropped.append(f"{name}@{f}")
        open(f"{C}/include/mordor/{f}", "w").write(t)
    json.dump(dropped, open(f"{D}/removed_declarations.json", "w"), indent=0); print(n, "declarations removed")
elif phase == "tests":
    removed = json.load(open(f"{D}/removed_tests.json")) if os.path.exists(f"{D}/removed_tests.json") else []
    for rnd in range(40):
        rc, out = sh("ninja -C b -k 0 mordor_tests mordor_engine_tests 2>&1")
        if rc == 0: print("tests build after", rnd, "rounds"); break
        hits = collections.defaultdict(set)
        for m in re.finditer(r"(tests/[\w./]+\.(?:cpp|h)):(\d+):\d+: error", out): hits[m.group(1)].add(int(m.group(2)))
        for m in re.finditer(r"/(tests/[\w./]+\.cpp)[^\n]*\n?[^\n]*?in function `([^'`(]+)", out): pass
        # linker: "...test_x.cpp:(.text+0x..): undefined reference" lines carry the function on the previous "in function" line
        for m in re.finditer(r"(tests/[\w./]+?\.cpp)\.o: in function `(?:[\w:]*::)?(\w+)", out):
            hits[m.group(1)].add(("fn", m.group(2)))
        if not hits: print("unparsed failure:\n", out[-3000:]); sys.exit(1)
        for f, hs in hits.items():
            t = open(f"{C}/{f}").read(); s = strip(t); spans = []
            _L = t.split("\n"); _ls = [0]; [_ls.append(i + 1) for i, ch in enumerate(t) if ch == "\n"]
            lines_at = [0]; [lines_at.append(i + 1) for i, ch in enumerate(t) if ch == "\n"]
            # top-level blocks: TEST(...) { } or function definitions
            for m in re.finditer(r"^(?:TEST\s*\(\s*(\w+)\s*\)|(?:static\s+|inline\s+)*[A-Za-z_][\w:<>,&\*\s]*?\b(\w+)\s*\([^;{)]*(?:\([^)]*\)[^;{)]*)*\)\s*(?:const)?)\s*\{", s, re.M):
                e = block_end(s, s.index("{", m.end() - 1)); nm = m.group(1) or m.group(2)
                spans.append((m.start(), e, nm, bool(m.group(1))))
            cut = set()
            for h in hs:
                for a, e, nm, ist in spans:
                    if (isinstance(h, int) and a <= lines_at[h - 1] < e) or (isinstance(h, tuple) and (nm == h[1] or h[1] == "test_" + nm)): cut.add((a, e, nm, ist))
            loose = [h for h in hs if isinstance(h, int) and not any(a <= lines_at[h - 1] < e for a, e, _, _ in spans)]
            if loose:
                L = t.split("\n")
                for h in sorted(loose, reverse=True):
                    if re.match(r"\s*(using\b|[^{}]*;\s*$)", L[h - 1]): removed.append(f"line {L[h-1].strip()}@{f}"); del L[h - 1]
                    else: print("loose error line not removable", f, h, L[h - 1]); sys.exit(1)
                t = "\n".join(L); s = strip(t)
                open(f"{C}/{f}", "w").write(t); continue   # blocks are handled next round
            if not cut: print("no block found for", f, sorted(map(str, hs))[:5]); sys.exit(1)
            for a, e, nm, ist in sorted(cut, reverse=True):
                pre = t[:a].split("\n"); k = len(pre) - 1
                while k > 0 and pre[k - 1].strip().startswith("//"): k -= 1
                a2 = len("\n".join(pre[:k])) + (1 if k else 0)
                t = t[:a2] + t[e:].lstrip("\n"); removed.append(f"{'TEST ' if ist else 'helper '}{nm}@{f}")
            open(f"{C}/{f}", "w").write(t)
        print("round", rnd, "removed so far", len(removed), flush=True)
    json.dump(removed, open(f"{D}/removed_tests.json", "w"), indent=0)
