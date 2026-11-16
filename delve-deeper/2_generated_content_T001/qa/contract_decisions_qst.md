Version: 1 | Task: T001-s015-qa4 | Author: development-qa-tester | Status: done | Evidence: EXECUTED (logs in work/T001/qa/out/) and STATIC
Inputs read: brief s015-qa-qst.md; qa/check_spec.md (Sections 0, 4, 6 QST/EQV-11..14, 7, 8); code/quest/README_quest.md (the brief names work/T001/quest/README_quest.md; the file is under code/quest/); quest JSON and CSV; code/tests/quest/test_quest.cpp (parts); code/quest/src/quest.cpp (report); code/tools/check_text_guard.py.

Decisions on the Software Engineer's contract choices (not user approvals; PROPOSAL-level test conventions only).

| # | Choice | Decision | Reason |
|---|---|---|---|
| Q1 | Failure line `FAIL <CODE> <detail>`, no colon | ACCEPT | Spec Section 0 only requires a line starting `FAIL`; QST-09 writes `FAIL MISSING_KEY fig_reward_none` without a colon. Stable across all runs (QST-03..16, 43, 48, 57-59, 67). |
| Q2 | Extra `run` events `day=<n>`, `roll=<x>` | ACCEPT | Additions only; no spec event changed. `day=` made `{day}` testable (QST-39, QST-64 show Day 7 and Day 5). `roll=` NOT exercised by QA; the reward path is covered by `quest_check reward` (QST-42) and ctest (QST-44, 65). |
| Q3 | `reward` prints keys and item, not text; default chance 0.5 and item `Test_Item` as a command-line placeholder | ACCEPT with condition | QST-42 needs only keys. Condition: the chance, item list and "rarely/usually" are TBD (Systems); the placeholder must not be read as a game value. Note `run` uses the same placeholder default (QST-34 etc. got no item only because the default roll is 0.999999). `--items ""` gives `fig_reward_none` (QST-42d). |
| Q4 | Rule rejections (`refused`, `not_observing`, `wrong_node`) printed, exit 0; only bad input exits 1 | ACCEPT | Matches QST-23, QST-29..31, QST-37, QST-66 wording ("event rejected", exit 0) and Section 0 (exit 1 = input rejected). |
| Q5 | `validate` checks more than the Python script (route ends/edges/plain, coverage, overlap, stray braces, banned words dwarf/dwarves/built/makers, CAUSE_WORD); `STAGE_COVERAGE` before `STAGE_RANGE` | ACCEPT with one query | Stricter than the reference is allowed (Section 0). Both validators agree on accept and on the five EQV-12 copies. Where two codes fire, the primary code was the spec's (QST-03/04/05/12/59a/59b show `also` lines). Query for the Producer: `CAUSE_WORD` (a gone line that names a cause) is not in my spec; it is a content rule whose approval source I could not find in the files I read. It does not affect any result here. |
| Q6 | Test names `qst_quest_<case>` | ACCEPT | `-R quest_`, `quest_roundtrip`, `quest_hostile`, `quest_fill_all`, `quest_reward_once`, `quest_load_invalid`, `quest_transition_table`, `quest_hostile_reward` each matched at least one test and ran (QST-44, 50, 52, 54, 55, 65, 70, EQV-13). `-R quest_` ran 26 of 26, 100% passed (QST-52). |

No choice is rejected, so no defect was filed for the contract choices. Defects found are DEF-T001-3 and DEF-T001-4.

Spec lines changed or substituted by QA at review time (reasons):
- QST-07: the spec fixture (route rewritten to pass e08) cannot keep the route ending at the destination `ore_lift`, so it would trip ROUTE_ENDS first. QA used a MAP copy with e10 made `locked` (`q_route_locked_map.json`) so the real quest route crosses a non-plain edge. Result `FAIL ROUTE_NOT_PLAIN e10`.
- QST-53, QST-71, EQV-14: mutation builds are in `work/T001/qa/build_mut/qst_noobs`, `qst_hostile_noline`, `qst_extra_row` (not the bare `build_mut` path). Their CMake `DD_REPO_ROOT` is set to the repo root because the copies sit deeper in the tree. EQV-14 also changes `questPath()` in the copied test to read the edited JSON copy.
- QST-59a: the spec edit (`fig_offer_refused` stage_min -1) also makes min > max, so `STAGE_RANGE` is listed as `also`; the primary code is `STAGE_COVERAGE` as required.
- QST-60 to QST-64 at one standing give the same key as at 0, so QA added discriminating runs QST-60x (offer@2, then stage=-2 gives `fig_report_present_cold`, not warm) and QST-68x (B at 2, A at 0, B's report is warm).
