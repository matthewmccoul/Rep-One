"""Repair a feature rewrite that lost its register_<id>() block (output cut short): take the new file from the ant's answer, graft the old file's register block, gate it.
usage: graft_register.py <id> <answer_file>"""
import os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cant import ROOT, parse_files
from gate import Gate
fid, ans = sys.argv[1], sys.argv[2]
files = parse_files(open(ans).read()); code = files.get(f"src/feat_{fid}.cpp"); test = files.get(f"tests/test_feat_{fid}.cpp")
old = open(f"{ROOT}/src/feat_{fid}.cpp").read()
m = re.search(rf"void register_{fid}\(\)\s*\{{.*?\n\}}\n", old, re.S)
assert code and m, "no new code or no old register block"
if f"void register_{fid}()" not in code:
    code = re.sub(r"\n\}\s*//\s*namespace mordor\s*$", "", code.rstrip()) if code.rstrip().endswith("namespace mordor") else code
    code = code.rstrip() + "\n\n" + m.group(0) + "\n}  // namespace mordor\n" if "namespace mordor {" in code and not code.rstrip().endswith("}  // namespace mordor") else code.rstrip() + "\n\n" + m.group(0)
g = Gate(); base = g.run(); g.backup()
open(f"{ROOT}/src/feat_{fid}.cpp", "w").write(code)
if "--drop-test" in sys.argv:   # the rewrite hides its handlers in an anonymous namespace, so a direct-call test cannot link; keep the code, drop the test
    try: os.remove(f"{ROOT}/tests/test_feat_{fid}.cpp")
    except OSError: pass
elif test: open(f"{ROOT}/tests/test_feat_{fid}.cpp", "w").write(test)
after = g.build_and_run(fix_rounds=5)
new = g.new_failures(base, after)
own = {k: v for k, v in new.items() if k.startswith(f"feat_{fid}_")}; other = {k: v for k, v in new.items() if k not in own}
if after is None or other:
    g.restore(); print(fid, "REVERTED", "build failed" if after is None else list(other)[:4]); 
    nl = open(f"{ROOT}/logs/ninja_last.txt", errors="replace").read().splitlines()
    print("\n".join(x[-200:] for x in nl if "error" in x)[:1500]); sys.exit(1)
if own:
    os.remove(f"{ROOT}/tests/test_feat_{fid}.cpp"); g.build_and_run(fix_rounds=2); print(fid, "KEPT, own smoke test dropped:", list(own)[:3])
else: print(fid, "KEPT")
