"""Gate-driven dead-duplicate pruning: apply, build; any pruned function that surviving code still calls (compile or LINK error) is kept; repeat; then suites + golden."""
import re, subprocess, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gate import Gate, sh, ROOT
SK = f"{ROOT}/logs/prune_skipfn.txt"
if os.path.exists(SK): os.remove(SK)
g = Gate(); base = g.run(); g.backup(); skipfn, skipfile = set(), set()
for attempt in range(12):
    g.restore(); open(SK, "w").write("\n".join(sorted(skipfn)))
    out = subprocess.run(["python3", "-I", "tools/prune_dead.py", "--apply", "--skip", ",".join(sorted(skipfile)) or "none"], capture_output=True, text=True, cwd=ROOT).stdout.splitlines()[0]
    rc, txt = sh("ninja -k 0 2>&1", cwd=ROOT + "/build")
    badfiles = set(re.findall(r"src/(\w+\.cpp):\d+:\d+: error", txt)); undefined = set(re.findall(r"undefined reference to `([^']+)'", txt))
    print(f"attempt {attempt}: {out}; compile errors in {sorted(badfiles)}; undefined refs {len(undefined)}", flush=True)
    if not badfiles and not undefined: break
    skipfile |= badfiles; skipfn |= {u for u in undefined}
else:
    print("did not converge"); g.restore(); sys.exit(1)
after = g.run(); new = g.new_failures(base, after)
if new: print("REVERT, new failures:", list(new)[:5]); g.restore(); sys.exit(1)
print("KEPT. engine", after["engine_summary"], "srd", after["srd_summary"], "kept-because-called:", len(skipfn), "skipped files:", sorted(skipfile))
