"""Regression gate for patch rounds. Snapshot src/include/tests, build, run BOTH suites, and report tests that newly fail.
usage (module): g = Gate(); base = g.run(); g.backup(); ...apply patch...; after = g.build_and_run(); new = g.new_failures(base, after); if new: g.restore()
CLI: gate.py run   -> build + run both suites, print the failing counts."""
import collections, json, os, re, shutil, subprocess, sys
ROOT = "/tmp/claude-0/-home-user-Rep-One/17a752bb-9e88-5884-a5bf-3b7fc4ab8c87/scratchpad/mordor/code"
BAK = ROOT + "/../gate_backup"
DIRS = ["src", "include", "tests", "apps"]
def sh(cmd, cwd=ROOT, timeout=3000):
    p = subprocess.run(cmd, shell=True, cwd=cwd, capture_output=True, text=True, timeout=timeout); return p.returncode, p.stdout + p.stderr
def parse(out):
    d = collections.defaultdict(list)
    for ln in out.splitlines():
        m = re.match(r"FAIL \[(\w+)\] \S*/(tests/[\w.]+):(\d+): (.*)", ln)
        if m: d[m.group(1)].append(f"{m.group(2)}:{m.group(3)}: {m.group(4)}")
    return d
GOLD = ROOT + "/logs/golden"
SEEDS = [3, 11, 29, 41]
def golden_run(seed):
    rc, out = sh(f"./mordor_console --seed {seed} --quick --bot 600 --data ../data/combat-data.json", cwd=ROOT + "/build")
    return out
class Gate:
    def golden_record(self):
        os.makedirs(GOLD, exist_ok=True)
        for s in SEEDS: open(f"{GOLD}/seed_{s}.txt", "w").write(golden_run(s))
    def golden(self):
        """Behaviour check: fixed-seed autopilot transcripts must stay byte-identical (any difference is a changed mechanic)."""
        bad = {}
        for s in SEEDS:
            p = f"{GOLD}/seed_{s}.txt"
            if os.path.exists(p) and golden_run(s) != open(p).read(): bad[f"golden_seed{s}"] = [f"transcript differs from logs/golden/seed_{s}.txt"]
        return bad
    def run(self):
        """Run both suites on the current build (no rebuild). Returns {'engine': {test: [lines]}, 'srd': {...}, 'counts': (...)}."""
        r = {}
        for key, exe in (("engine", "mordor_engine_tests"), ("srd", "mordor_tests")):
            rc, out = sh(f"./{exe}", cwd=ROOT + "/build")
            r[key] = dict(parse(out)); r[key + "_summary"] = (re.findall(r"(\d+) tests, (\d+) checks, (\d+) failures", out) or [("?", "?", "?")])[-1]
            if r[key + "_summary"][0] == "?": r[key]["CRASH_" + exe] = [f"{exe} printed no summary (crashed, exit {rc})"]   # a crash must never read as zero failures
        r["srd"].update(self.golden())
        return r
    def build_and_run(self, fix_rounds=3):
        rc, out = sh(f"python3 -I tools/fixloop.py build {fix_rounds}")
        rc, out2 = sh("ninja -k 0 > ../logs/ninja_last.txt 2>&1", cwd=ROOT + "/build")
        if rc != 0: return None        # the exit status, not the text: a failed build must never be judged by stale binaries
        return self.run()
    def backup(self):
        shutil.rmtree(BAK, ignore_errors=True); os.makedirs(BAK)
        for d in DIRS: shutil.copytree(f"{ROOT}/{d}", f"{BAK}/{d}")
    def restore(self):
        for d in DIRS:
            shutil.rmtree(f"{ROOT}/{d}"); shutil.copytree(f"{BAK}/{d}", f"{ROOT}/{d}", copy_function=shutil.copyfile)   # new mtimes, so ninja rebuilds
        sh("ninja 2>&1 | tail -1", cwd=ROOT + "/build")
    @staticmethod
    def new_failures(base, after):
        if after is None: return {"BUILD": ["the build failed"]}
        new = {}
        for k in ("engine", "srd"):
            for t, lines in after[k].items():
                if t not in base[k]: new[t] = lines
        return new
if __name__ == "__main__":
    g = Gate()
    if len(sys.argv) > 1 and sys.argv[1] == "golden-record": g.golden_record(); print("recorded", SEEDS); sys.exit(0)
    r = g.build_and_run() if len(sys.argv) > 2 and sys.argv[2] == "build" else g.run()
    print("engine", r["engine_summary"], "failing tests", len(r["engine"]), "| srd", r["srd_summary"], "failing tests", len(r["srd"]))
