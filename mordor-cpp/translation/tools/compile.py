"""Compile every gen/*.cpp (syntax + semantics, no link) in parallel; write errors per file to errors.json."""
import concurrent.futures as cf, glob, json, os, re, subprocess, sys
X = sys.argv[1]; G = f"{X}/cpp/gen"
def one(f):
    r = subprocess.run(["g++", "-std=c++17", "-fsyntax-only", "-fmax-errors=60", "-w", f"-I{X}/cpp", f"-I{G}", f], capture_output=True, text=True)
    return os.path.basename(f), r.returncode, r.stderr
res = {}
with cf.ThreadPoolExecutor(int(os.environ.get("JOBS", "4"))) as ex:
    for name, rc, err in ex.map(one, sorted(glob.glob(f"{G}/*.cpp"))): res[name] = {"ok": rc == 0, "err": err}
json.dump(res, open(f"{X}/errors.json", "w"), indent=0)
bad = [k for k, v in res.items() if not v["ok"]]
print(len(res), "sources;", len(res) - len(bad), "compile;", len(bad), "fail")
# where do the errors point? (file of each error line)
loc = {}
for k, v in res.items():
    for m in re.finditer(r"^(?:\S*/)?([\w.]+\.(?:h|cpp)):\d+:\d+: error", v["err"], re.M): loc[m.group(1)] = loc.get(m.group(1), 0) + 1
print("error locations (top):", sorted(loc.items(), key=lambda x: -x[1])[:15])
