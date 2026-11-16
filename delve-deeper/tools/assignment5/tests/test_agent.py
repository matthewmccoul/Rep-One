#!/usr/bin/env python3
"""Tests for the Assignment 5 runner. They use tests/stub_claude.py, so they cost nothing and need no account.

    python3 delve-deeper/tools/assignment5/tests/test_agent.py

Each integration test targets a way the agent could go wrong: building a lower-ranked gap first, building a gap
whose dependency is missing, reporting success after a failed build, rewriting a finished feature, or accepting
evidence that is not in the files.
"""
import contextlib
import io
import json
import os
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import run_agent as ra  # noqa: E402

STUB = f"{sys.executable} {HERE / 'stub_claude.py'}"
GDD = ra.DEFAULT_ROOT / "1_pipeline" / "08_Final_GDD_Extracted.txt"
# DD-TOLL-02 has the highest score but depends on GOLD-01 and TOLL-01, so it must wait for them.
SCORES = {"DD-GOLD-01": [2, 2, 2], "DD-TOLL-01": [3, 2, 3], "DD-TOLL-02": [3, 3, 2], "DD-DESC-01": [3, 3, 0]}


class PriorityScore(unittest.TestCase):
    def test_formula_and_examples_from_the_handoff(self):
        self.assertEqual(ra.priority_score(3, 3, 2), 17)
        self.assertEqual(ra.priority_score(2, 1, 0), 8)
        self.assertEqual(ra.priority_score(0, 0, 0), 0)
        self.assertEqual(ra.priority_score(3, 3, 3), 18)

    def test_rejects_out_of_range_and_non_integers(self):
        for bad in ((4, 0, 0), (-1, 0, 0), (0, 0, 9), (1.5, 0, 0), ("2", 0, 0), (True, 0, 0)):
            with self.assertRaises(ValueError):
                ra.priority_score(*bad)


def res(status, i=1, u=1, b=1):
    return {"status": status, "impact": i, "urgency": u, "unblocks": b, "summary": "", "score_reason": ""}


class SelectNext(unittest.TestCase):
    reqs = [{"id": "A", "depends_on": []}, {"id": "B", "depends_on": []}, {"id": "C", "depends_on": ["A"]}]

    def test_highest_score_wins(self):
        sel = ra.select_next(self.reqs, {"A": res("missing", 1, 1, 0), "B": res("missing", 3, 3, 0), "C": res("implemented")})
        self.assertEqual(sel["selected"]["id"], "B")

    def test_tie_goes_to_lower_id(self):
        sel = ra.select_next(self.reqs, {"A": res("missing", 2, 2, 2), "B": res("missing", 2, 2, 2), "C": res("implemented")})
        self.assertEqual(sel["selected"]["id"], "A")

    def test_blocked_candidate_is_never_chosen_even_with_the_top_score(self):
        results = {"A": res("missing", 1, 1, 1), "B": res("implemented"), "C": res("missing", 3, 3, 3)}
        sel = ra.select_next(self.reqs, results)
        self.assertEqual(sel["selected"]["id"], "A")
        self.assertEqual(sel["waiting"], [{"id": "C", "waiting_on": ["A"], "why": "dependencies not implemented yet"}])

    def test_partial_is_a_gap_and_implemented_is_not(self):
        sel = ra.select_next(self.reqs, {"A": res("implemented"), "B": res("partial"), "C": res("implemented")})
        self.assertEqual([c["id"] for c in sel["candidates"]], ["B"])

    def test_nothing_eligible_when_everything_waits(self):
        sel = ra.select_next(self.reqs, {"A": res("blocked"), "B": res("implemented"), "C": res("missing")})
        self.assertIsNone(sel["selected"])

    def test_exhausted_requirement_is_skipped(self):
        sel = ra.select_next(self.reqs, {"A": res("missing"), "B": res("implemented"), "C": res("implemented")}, exhausted={"A"})
        self.assertIsNone(sel["selected"])
        self.assertEqual(sel["skipped"][0]["id"], "A")


class StaticChecks(unittest.TestCase):
    def check(self, name, text):
        d = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, d, True)
        (d / name).write_text(text)
        return ra.static_check(d, [{"path": name, "kind": "added"}])

    def test_balanced_cpp_passes_and_unbalanced_fails(self):
        self.assertEqual(self.check("a.cpp", 'void F() { auto s = "}"; // {\n}\n'), [])
        self.assertTrue(self.check("a.cpp", "void F() {\n"))

    def test_generated_header_must_be_last_include(self):
        bad = '#include "X.generated.h"\n#include "Y.h"\nUCLASS()\nclass A {};\n'
        good = '#include "Y.h"\n#include "X.generated.h"\nUCLASS()\nclass A {};\n'
        self.assertTrue(self.check("X.h", bad))
        self.assertEqual(self.check("X.h", good), [])

    def test_json_and_csv(self):
        self.assertTrue(self.check("a.json", "{nope"))
        self.assertEqual(self.check("a.json", '{"a": 1}'), [])
        self.assertTrue(self.check("a.csv", "a,b\n1,2,3\n"))


class AgentLoop(unittest.TestCase):
    """Runs the real runner end to end against a scripted stand-in for Claude Code."""

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp, True)
        self.ws = self.tmp / "ws"
        (self.ws / "3_unreal_project/DelveDeeperTest/Source/DelveDeeperTest").mkdir(parents=True)
        (self.ws / "docs").mkdir()
        self.fake = self.tmp / "fake"
        self.fake.mkdir()
        os.environ["STUB_DIR"] = str(self.fake)
        self.set_state()

    def set_state(self, **kw):
        state = {"implemented": [], "scores": SCORES, "mode": "ok", "order": []}
        state.update(kw)
        (self.fake / "state.json").write_text(json.dumps(state))

    def state(self):
        return json.loads((self.fake / "state.json").read_text())

    def go(self, *extra):
        argv = ["--project-root", str(self.ws), "--gdd", str(GDD), "--runs-dir", str(self.tmp / "runs"),
                "--claude-bin", STUB, *extra]
        with contextlib.redirect_stdout(io.StringIO()):
            code = ra.run(ra.parse_args(argv))
        run_dir = sorted((self.tmp / "runs").iterdir())[-1]
        return code, json.loads((run_dir / "summary.json").read_text()), run_dir

    def test_builds_in_dependency_order_not_in_score_order(self):
        code, summary, run_dir = self.go("--build-cmd", "true")
        self.assertEqual(self.state()["order"], ["DD-TOLL-01", "DD-GOLD-01", "DD-TOLL-02", "DD-DESC-01"])
        self.assertEqual(code, ra.EXIT_OK)
        self.assertEqual(summary["result"], "SUCCESS")
        self.assertTrue((run_dir / "RUN_LOG.md").read_text().count("Selected:") == 4)

    def test_success_is_not_reported_when_the_build_fails(self):
        code, summary, _ = self.go("--build-cmd", "echo 'error C2065: undeclared identifier'; exit 1")
        self.assertEqual(code, ra.EXIT_BLOCKED)
        self.assertNotEqual(summary["result"], "SUCCESS")
        self.assertEqual(self.state()["order"], ["DD-TOLL-01", "DD-TOLL-01"])  # two attempts at one gap, then it stops
        self.assertIn("still fails its check", summary["why"])

    def test_without_a_build_the_result_is_unverified_not_success(self):
        code, summary, _ = self.go()
        self.assertEqual(code, ra.EXIT_UNVERIFIED)
        self.assertEqual(summary["result"], "IMPLEMENTED, BUILD UNVERIFIED")

    def test_completed_work_is_not_rewritten(self):
        self.go("--build-cmd", "true")
        before = self.state()["order"][:]
        code, summary, _ = self.go("--build-cmd", "true")
        self.assertEqual(code, ra.EXIT_OK)
        self.assertEqual(self.state()["order"], before)  # no implement call on the second run
        self.assertEqual(summary["iterations"], [])

    def test_a_stalled_agent_stops_with_a_blocker(self):
        self.set_state(mode="noop")
        code, summary, _ = self.go("--build-cmd", "true")
        self.assertEqual(code, ra.EXIT_BLOCKED)
        self.assertEqual(summary["result"], "STALLED")

    def test_edits_outside_the_allowed_paths_are_reverted(self):
        self.set_state(mode="outside")
        code, _, _ = self.go("--build-cmd", "true")
        self.assertEqual(code, ra.EXIT_BLOCKED)
        self.assertFalse((self.ws / "docs/outside.txt").exists())

    def test_malformed_cpp_counts_as_a_failed_check(self):
        self.set_state(mode="badbrace")
        code, summary, _ = self.go("--build-cmd", "true")
        self.assertEqual(code, ra.EXIT_BLOCKED)
        self.assertEqual(summary["iterations"][0]["check"], "failed")
        self.assertEqual(self.state()["order"], ["DD-TOLL-01", "DD-TOLL-01"])  # did not move on to another gap

    def test_invented_evidence_does_not_count_as_implemented(self):
        self.set_state(lie=["DD-GOLD-01"])
        code, summary, run_dir = self.go("--scan-only")
        scan = json.loads((run_dir / "scan_01.json").read_text())
        self.assertEqual(scan["model_output"]["results"][0]["status"], "implemented")
        self.assertEqual(scan["verified"]["DD-GOLD-01"]["status"], "partial")

    def test_scan_only_writes_nothing(self):
        code, _, _ = self.go("--scan-only")
        self.assertEqual(code, ra.EXIT_OK)
        self.assertEqual(self.state()["order"], [])

    def test_a_missing_claude_is_a_stage_failure(self):
        argv = ["--project-root", str(self.ws), "--gdd", str(GDD), "--runs-dir", str(self.tmp / "runs"),
                "--claude-bin", "definitely-not-claude"]
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(ra.run(ra.parse_args(argv)), ra.EXIT_FAILED)


if __name__ == "__main__":
    unittest.main(verbosity=2)
