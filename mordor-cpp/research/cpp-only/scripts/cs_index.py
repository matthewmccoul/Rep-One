"""Index every method/property body in OpenDejenol's C# sources (Core and App) for retrieval. usage: cs_index.py <S>"""
import glob, json, os, re, sys
S = sys.argv[1]; R = f"{S}/dl/src/OpenDejenol-v0.1.3/src"
sig = re.compile(r"^[ \t]*(?:\[[^\]]*\]\s*)*(?:(?:public|private|internal|protected|static|override|virtual|async|readonly|sealed|unsafe|new|partial|extern)\s+)+[\w<>\[\]\?,\.\(\) ]+?\b(\w+)\s*(?:<[^>]*>)?\s*\(", re.M)
out = []
for f in glob.glob(f"{R}/**/*.cs", recursive=True):
    if "/obj/" in f or "/bin/" in f or ".Tests" in f: continue
    t = open(f, errors="replace").read(); rel = os.path.relpath(f, R)
    cls = None
    for m in sig.finditer(t):
        name = m.group(1)
        if name in ("if", "for", "while", "switch", "using", "return", "catch", "foreach", "lock"): continue
        j = m.end(); depth = 1
        while j < len(t) and depth: depth += {"(": 1, ")": -1}.get(t[j], 0); j += 1
        k = j
        while k < len(t) and t[k] in " \t\r\n": k += 1
        if t.startswith("=>", k): e = t.find(";", k) + 1
        elif k < len(t) and t[k] == "{":
            d = 0; e = k
            while e < len(t):
                if t[e] == "{": d += 1
                elif t[e] == "}":
                    d -= 1
                    if d == 0: e += 1; break
                e += 1
        else: continue
        # leading doc comment
        s = m.start(); pre = t[:s].rstrip().splitlines()[-15:]
        doc = []
        for l in reversed(pre):
            if l.strip().startswith("///") or l.strip().startswith("//"): doc.insert(0, l.strip())
            else: break
        c = re.findall(r"\b(?:class|struct|record|enum)\s+(\w+)", t[:s]); cls = c[-1] if c else ""
        out.append({"file": rel, "cls": cls, "name": name, "text": "\n".join(doc) + "\n" + t[m.start():e]})
json.dump(out, open(f"{S}/feat/cs_index.json", "w"))
import collections; print(len(out), "methods;", collections.Counter(o["file"].split("/")[0] for o in out))
