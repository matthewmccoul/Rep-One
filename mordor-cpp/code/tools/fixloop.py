"""Compile-fix loop. usage: fixloop.py <headers|build> [max_rounds]
Each round: build, group errors by file, one ant per failing file (file + errors + API index) returns the corrected file. Only the failing file is writable."""
import asyncio, json, os, re, subprocess, sys
sys.path.insert(0, os.path.dirname(__file__))
from cant import ROOT, run_all, est_tokens
import api_index
MODE = sys.argv[1]; ROUNDS = int(sys.argv[2]) if len(sys.argv) > 2 else 4
CONV = open(f"{ROOT}/spec/CONVENTIONS.md").read()
SYS = ("You fix C++ compile errors. You get one file and the compiler errors in it, plus an index of the declarations in the project headers. "
       "Fix the file so it compiles under C++17 with -fno-exceptions -fno-rtti. Rules: change as little as possible; keep every declared function and its meaning; "
       "use declarations that exist in the API INDEX and state headers (do not invent types: if a type is missing, define a small one in the file only if it is local, otherwise use an existing equivalent); "
       "do not add I/O, globals or exceptions. If the error is in a header because it uses a type that exists nowhere, replace it by the closest existing type or a plain i32 and add `// FIXED:` comment. "
       "If a function in a .cpp is not declared in any header, declare it `static` or include the right header. Output ONLY the complete corrected file in this exact form:\n=== FILE: <path> ===\n<full file>\n=== END FILE ===")
history = []
for rnd in range(1, ROUNDS + 1):
    subprocess.run(["python3", "-I", f"{ROOT}/tools/build.py"] + (["headers"] if MODE == "headers" else []), capture_output=True)
    errs = json.load(open(f"{ROOT}/logs/build_errors.json"))
    n = sum(len(v) for v in errs.values())
    print(f"round {rnd}: {len(errs)} files, {n} errors", flush=True); history.append(n)
    if not errs: break
    tasks = []
    for f, e in errs.items():
        path = f"{ROOT}/{f}"
        if not os.path.exists(path) or f.startswith("tools/"): continue
        if os.environ.get("ONLY") and not re.search(os.environ["ONLY"], f): continue
        body = open(path).read()
        # headers the file includes (directly): state headers digest + relevant declarations from other headers
        inc = "\n".join(re.findall(r'#include "mordor/[\w.]+"', body))
        idx = api_index.relevant_index(body + "\n" + "\n".join(e), cap=9000)
        sp = api_index.spine_for_header(body if f.endswith(".h") else body + "\n" + "\n".join(open(f"{ROOT}/include/mordor/{n}.h").read() for n in re.findall(r'#include "mordor/(\w+)\.h"', body) if os.path.exists(f"{ROOT}/include/mordor/{n}.h")))
        user = f"FILE: {f}\n\nCOMPILER ERRORS ({len(e)}):\n" + "\n".join(e[:60]) + f"\n\nAPI INDEX\n{idx}\n\nSTATE HEADERS (digest)\n{sp}\n\nFILE CONTENT\n{body}"
        if est_tokens(SYS, user) > 30_000:
            user = f"FILE: {f}\n\nCOMPILER ERRORS ({len(e)}):\n" + "\n".join(e[:60]) + f"\n\nAPI INDEX\n{idx}\n\nFILE CONTENT\n{body}"
        tasks.append({"id": f, "system": SYS, "user": user, "allow": [f]})
    asyncio.run(run_all(tasks, f"fix_{MODE}", 4))
print("error history:", history)
