"""Build and summarise errors per file. usage: build.py [headers]  (headers = syntax-check all public headers only)
Writes logs/build_errors.json = {file: [error lines]}; prints a summary."""
import collections, json, os, re, subprocess, sys
ROOT = "/tmp/claude-0/-home-user-Rep-One/17a752bb-9e88-5884-a5bf-3b7fc4ab8c87/scratchpad/mordor/code"
def run(cmd, cwd=ROOT):
    p = subprocess.run(cmd, shell=True, cwd=cwd, capture_output=True, text=True); return p.returncode, p.stdout + p.stderr
def parse(out):
    errs = collections.defaultdict(list)
    for ln in out.splitlines():
        m = re.match(r"(?:\.\./)?([^\s:]+\.(?:cpp|h)):(\d+):(\d+): (error|fatal error): (.*)", ln)
        if m: errs[m.group(1).replace(ROOT + "/", "")].append(f"{m.group(1)}:{m.group(2)}: {m.group(5)}")
        m2 = re.match(r".*?/?([^\s/]+\.cpp):\d+: undefined reference to `(.*)'", ln)
    return errs
if len(sys.argv) > 1 and sys.argv[1] == "headers":
    hs = sorted(f for f in os.listdir(f"{ROOT}/include/mordor") if f.endswith(".h"))
    open(f"{ROOT}/tools/all_headers.cpp", "w").write("\n".join(f'#include "mordor/{h}"' for h in hs) + "\nint main() { return 0; }\n")
    rc, out = run("g++ -std=c++17 -fsyntax-only -Wall -Wextra -Iinclude tools/all_headers.cpp 2>&1")
else:
    os.makedirs(f"{ROOT}/build", exist_ok=True)
    run("cmake -G Ninja .. > /dev/null", cwd=f"{ROOT}/build")
    rc, out = run("ninja -k 0 2>&1", cwd=f"{ROOT}/build")
errs = parse(out)
json.dump(errs, open(f"{ROOT}/logs/build_errors.json", "w"), indent=1)
undefined = sorted(set(re.findall(r"undefined reference to `([^']+)'", out)))
json.dump(undefined, open(f"{ROOT}/logs/undefined_refs.json", "w"), indent=1)
print(f"rc={rc} files with errors: {len(errs)} total errors: {sum(len(v) for v in errs.values())} undefined refs: {len(undefined)}")
for f, e in sorted(errs.items(), key=lambda kv: -len(kv[1]))[:25]: print(f"  {len(e):4} {f}")
open(f"{ROOT}/logs/last_build.txt", "w").write(out)
