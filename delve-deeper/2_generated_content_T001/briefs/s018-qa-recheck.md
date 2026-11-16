# s018-qa-recheck: QA re-check after rewrite round 1 (owner: development-qa-tester)
Task ID T001-s018-qa. Written by the Producer (the chat session). You re-check ONLY what changed. Same rules as your review dispatch (`work/T001/qa/check_spec.md` Section 7): review before you run, always run through the logging runner `python3 -I work/T001/qa/run_check.py <id> -- <command ...>` (tool-written logs in `work/T001/qa/out/`), fixtures under `work/T001/qa/fixtures/`, no shell redirects, one simple command at a time, you fix nothing and edit no code or content.

## What changed since your review
1. Software Engineer rewrite round 1 (read `work/T001/defects/DEF-T001-3.md` and `DEF-T001-4.md`, and the Engineer's notes in `work/T001/code/quest/README_quest.md` and `work/T001/code/eqv/README_eqv.md`): `tools/check_text_guard.py` now reads the quest CSV; the test `qst_quest_transition_table` now drives the real Engine and compares with the JSON table, and can be pointed at another quest JSON with the environment variable `DD_QUEST_JSON`; the hostile tests no longer crash after a failed CHECK.
2. Wave 1 content cleanups: the map has no parentheses in node names (the ore lift is now "Old ore lift, chain and drum"), status labels say "Version 3 (revision 2 of 2)", `validate_map.py` writes its log only when run with no argument or with `--log`; the puzzle spec no longer ties lockpicking to a Thief.
3. The canonical build `work/T001/build/` has been rebuilt with all of this (123 ctest tests). Use it; do not rebuild it.

## What to do
- **DEF-T001-3:** re-run QST-17 and QST-18 and the text-guard lines PZL-30 and PZL-31 (regression). Judge the Engineer's wording list against the spec lines: the Engineer asked you to confirm its reading of QST-16 and QST-18 (a leak such as `q_reason.csv`'s "because").
- **DEF-T001-4:** re-run EQV-13 and EQV-14 using `DD_QUEST_JSON` (for example `python3 -I work/T001/qa/run_check.py EQV-14 -- env DD_QUEST_JSON=<your fixture> ctest --test-dir work/T001/build --output-on-failure -R quest_transition_table`). Run two mutations: your original extra row (`not_offered --report--> completed`) and one changed target on an existing row. Both must FAIL the transition test. Also re-run your QST-71 mutation and report whether the hostile tests still crash (SEGFAULT) or now fail cleanly.
- **Regression for the content changes:** re-run MAP-01, MAP-02, EQV-09, EQV-10, QST-01, QST-02, and every check whose expected output mentions a node name (use Grep on the spec for "Old ore lift" and "chain and drum"; update only the expectation text in your results notes, not the spec). Then run the whole suite once: `python3 -I work/T001/qa/run_check.py REGRESSION-ctest -- ctest --test-dir work/T001/build --output-on-failure --output-log work/T001/qa/out/REGRESSION-ctest.ctest.log`.
- **Defect statuses:** update the status line of DEF-T001-3 and DEF-T001-4 (you have Edit): verified fixed with the log paths, or still open with why. If one is still open after this, say it is rewrite round 2 of 2 for the Software Engineer; do not file duplicates.
- **Results:** write `work/T001/qa/results_recheck.csv` (columns `check_id,result,label,command,exit_code,log_path,defect_id,note`) for what you re-ran. Do not overwrite your earlier results files.
- Writable paths: `work/T001/qa/fixtures/`, `work/T001/qa/out/`, `work/T001/qa/mut/`, `work/T001/qa/build_mut/`, `work/T001/qa/results_recheck.csv`, `work/T001/defects/` (status lines only, plus new defects if you find any, next free number from DEF-T001-10 upward).

## Acceptance
A row in `results_recheck.csv` for every re-run item, with real exit codes and log paths; DEF-T001-3 and DEF-T001-4 have a verified status or a stated reason; your reply gives the counts and any new defects.

## Stop limit
One pass over the list above. End with STATUS, OUTPUTS, ISSUES. Evidence: EXECUTED (with log paths).
