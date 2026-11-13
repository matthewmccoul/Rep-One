"""Gate for the prune: build (exit status), both suites (summary required), golden transcripts. Commit on pass, revert on fail.
usage as module: g = Gate(W); r = g.check(allow_tests_removed=set(), golden_must_match=True); g.commit(msg) / g.revert()"""
import os, re, subprocess
class Gate:
    def __init__(self, W): self.W = W; self.C = f"{W}/code"
    def sh(self, cmd, cwd=None, timeout=3000):
        p = subprocess.run(cmd, shell=True, cwd=cwd or self.C, capture_output=True, text=True, timeout=timeout); return p.returncode, p.stdout + p.stderr
    def suite(self, exe):
        rc, out = self.sh(f"./b/{exe}")
        m = re.findall(r"(\d+) tests, (\d+) checks, (\d+) failures", out)
        fails = sorted(set(re.findall(r"FAIL \[(\w+)\]", out)))
        return (tuple(map(int, m[-1])) if m else None), fails, out
    def check(self, golden_must_match=True):
        rc, out = self.sh("ninja -C b 2>&1")
        if rc != 0: return {"ok": False, "why": "build failed", "log": "\n".join(l for l in out.splitlines() if ("error" in l or "undefined reference" in l or "multiple definition" in l))[:3000]}
        res = {"ok": True}
        for k, exe in (("srd", "mordor_tests"), ("engine", "mordor_engine_tests")):
            summ, fails, out = self.suite(exe)
            if summ is None: return {"ok": False, "why": f"{exe} printed no summary (crash)", "log": out[-1500:]}
            res[k] = summ
            if summ[2] != 0 or fails: return {"ok": False, "why": f"{exe}: {summ[2]} failing", "log": "\n".join(l for l in out.splitlines() if l.startswith("FAIL"))[:3000], **res}
        diffs = []
        for s in (3, 11, 29, 41):
            rc, out = self.sh(f"./mordor_console --seed {s} --quick --bot 600 --data ../data/combat-data.json", cwd=f"{self.C}/b")
            if out != open(f"{self.C}/logs/golden/seed_{s}.txt").read():
                diffs.append(s)
                if not golden_must_match: open(f"{self.C}/logs/golden/seed_{s}.txt", "w").write(out)   # accepted move: new golden, old one stays in git history
        res["golden_diffs"] = diffs
        if golden_must_match and diffs: return {**res, "ok": False, "why": f"golden transcripts changed: seeds {diffs}"}
        return res
    def commit(self, msg):
        self.sh("git add -A"); return self.sh(f"git -c user.email=c@x -c user.name=coord commit -qm {subprocess.list2cmdline([msg])}")
    def revert(self):
        self.sh("git checkout -- . && git clean -fdq -e b/")
        self.sh("find src include tests apps -type f -newer .git/HEAD -exec touch {} +")   # fresh mtimes so ninja rebuilds what changed
