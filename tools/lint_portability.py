"""Unreal-portability lint for the rules core (include/ and src/). Exit 1 on violations. usage: lint_portability.py"""
import glob, os, re, sys
ROOT = "/tmp/claude-0/-home-user-Rep-One/17a752bb-9e88-5884-a5bf-3b7fc4ab8c87/scratchpad/mordor/code"
RULES = [
 (r"#\s*include\s*<(iostream|fstream|sstream|cstdio|stdio\.h|chrono|thread|mutex|atomic|future|filesystem|random|ctime|time\.h|regex)>", "platform header"),
 (r"\bstd::(cout|cerr|cin|ifstream|ofstream|stringstream|thread|mutex|chrono|random_device|mt19937|async|filesystem)\b", "platform facility"),
 (r"\b(printf|fprintf|puts|fopen|fread|fwrite|getenv|system|srand|rand|time|clock|exit|abort)\s*\(", "C runtime call"),
 (r"\b(throw|try|catch)\b\s*[({]?", "exception keyword"),
 (r"\b(dynamic_cast|typeid)\b", "RTTI"),
 (r"\bthread_local\b", "thread_local"),
 (r"^static\s+(?!constexpr|const\b|inline|void|[\w:<>,\s\*&]+\()[^;(]*=[^=]", "file-scope mutable static"),
 (r"\bvirtual\b", "virtual (allowed only in rng.h/events.h interfaces)"),
]
ALLOW_VIRTUAL = {"rng.h", "events.h"}
viol = []
for f in sorted(glob.glob(f"{ROOT}/include/mordor/*.h") + glob.glob(f"{ROOT}/src/*.cpp")):
    b = os.path.basename(f); txt = open(f).read()
    txt = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), txt, flags=re.S)
    for i, line in enumerate(txt.split("\n"), 1):
        code = re.sub(r"//.*", "", line); code = re.sub(r'"(\\.|[^"\\])*"', '""', code)
        for pat, what in RULES:
            if what.startswith("virtual") and b in ALLOW_VIRTUAL: continue
            if b == "json_lite.cpp" and what == "C runtime call" and "strtod" in code: continue
            if re.search(pat, code): viol.append((b, i, what, line.strip()[:100]))
by = {}
for b, i, w, l in viol: by.setdefault(w, []).append((b, i, l))
print(f"{len(viol)} violations in {len({v[0] for v in viol})} files")
for w, items in by.items():
    print(f"  {w}: {len(items)}")
    for b, i, l in items[:6]: print(f"     {b}:{i}: {l}")
open(f"{ROOT}/logs/lint.txt", "w").write("\n".join(f"{b}:{i}: {w}: {l}" for b, i, w, l in viol))
sys.exit(1 if viol else 0)
