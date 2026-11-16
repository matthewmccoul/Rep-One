# s015-qa-qst: QA review, part 4 of 4: QST (owner: development-qa-tester)
Task ID T001-s015-qa4. Written by the Producer (the chat session). You are doing the REVIEW dispatch your spec describes (`work/T001/qa/check_spec.md`, Section 7 "Review rules for the later dispatch", and Section 8). Read Sections 0, 7 and 8 first. Your part covers: **Section 4 (QST-01 onward, every line including the version-2 additions) and EQV-11 to EQV-14 from Section 6 (quest data agreement with `validate_quest.py`, the transition table and its mutation)**. Other parts run after or before yours and cover the rest.

## What exists
The Software Engineer built `work/T001/code/` from your spec (core in `common/ map/ puzzle/ quest/`, tools in `tools/`, the Unreal plugin in `unreal/`, READMEs in each module). The canonical build is ALREADY built in `work/T001/build/`: do not rebuild or clean it; use it read-only. The spec's commands use `work/T001/build/...`: run them as written. Wherever a spec command needs something that does not exist (for example a fixture), you create it (below).

## What to do for every check line in your scope
1. **Review before you run.** Read the test, script or tool the line exercises. Does it test what the spec line says? Does it fail on broken input? Note anything lenient.
2. **Run it with the logging runner, always:** `python3 -I work/T001/qa/run_check.py <check-id> -- <command words ...>`. It writes the tool-made log `work/T001/qa/out/<check-id>.log` and passes the command's exit code through. Compare the exit code and output with the spec's expected result. Do not run a check any other way (shell redirects are not available, and logs typed by hand do not count as evidence).
3. **Broken-input lines (B):** create the fixture the line names under `work/T001/qa/fixtures/`, from a copy of the real file with ONE edit (use small `python3 -I -c` commands; keep each fixture's single edit in the check's note). A (B) check passes when the checker REJECTS the broken input. If a checker accepts it, or cannot fail, that is a defect in the checker.
4. **Mutation builds** (the lines that name `qa/build_mut`): copy only what is needed (`CMakeLists.txt`, `common`, `map`, `puzzle`, `quest`, `eqv`, `cli`, `tests`, `tools`) into `work/T001/qa/mut/<name>/` with python3, apply the single mutation the line names, configure and build in `work/T001/qa/build_mut/<name>`, and run ctest there with `--output-log`. The mutated tests must FAIL.
5. **Not runnable here** (anything needing Unreal): `NOT RUN`, never pass.
6. **Results:** write `work/T001/qa/results_qst.csv` with the Write tool, one row per check line: `check_id,result,label,command,exit_code,log_path,defect_id,note` (result is pass, fail, blocked or not-run; label is EXECUTED or NOT RUN). Quote only exit codes and log paths from the runner; do not retype outputs as evidence.
7. **Defects:** file each failure and each checker that cannot fail in `work/T001/defects/` as `DEF-T001-<n>.md` (list the directory, take the next free number; begin `Filed by: development-qa-tester`; class, repro command, expected with spec ref, actual, log path, routed owner, status open). `same as <path>` instead of a duplicate. When a check proves an earlier defect fixed or answered, update its status line (you have Edit).
8. **Contract choices:** decide Q1 to Q6 in `work/T001/quest/README_quest.md`. Note: `validate_quest.py` takes a quest FOLDER; run it on broken copies first and the real folder last, and use `--log` into `work/T001/qa/out/` if you need its log. Record each decision (accept or reject, with a reason) in `work/T001/qa/contract_decisions_qst.md`. A rejected choice is a defect routed to the Software Engineer.

## Rules
- You do not fix anything, edit code or content, or edit other roles' files. You judge only against the spec, the GDD and approved content. TBD means not testable.
- Writable paths: `work/T001/qa/fixtures/`, `work/T001/qa/out/`, `work/T001/qa/mut/`, `work/T001/qa/build_mut/`, `work/T001/qa/results_qst.csv`, `work/T001/qa/contract_decisions_qst.md`, `work/T001/defects/`. (The CMake directories `work/T001/qa/build_clang` and `work/T001/qa/build_mut/*` are yours for the extra builds.)
- Run order for the map validator `validate_map.py`: broken copies first, the real file last (it always overwrites its log).
- One simple command at a time; no `cd`, `;`, `&&`, pipes, or redirects.
- Be efficient: the budget is tight. Batch reading, do not re-read big files, do not narrate. If you cannot finish every line, mark the rest `not-run` with the reason and say so; never pass a line you did not run.

## Acceptance
Every check line in your scope has a row in your results file with a real exit code and log path (or not-run with a reason); every failure has a defect; the contract decisions file exists; your reply gives the counts (pass, fail, blocked, not-run) and the defect ids.

## Stop limit
One pass over your scope. End with STATUS, OUTPUTS, ISSUES. Evidence label: EXECUTED (with log paths) or NOT RUN.
