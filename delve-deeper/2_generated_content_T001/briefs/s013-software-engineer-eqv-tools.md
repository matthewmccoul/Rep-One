# s013-software-engineer-eqv-tools: equivalence tooling and text guard (owner: development-software-engineer)
Task ID T001-s013-software. Written by the Producer (the chat session). Build step 4a of 4 for the C++ package. Step 4b (the Unreal wrapper source) is built IN PARALLEL by another run: keep to your own files (rules below).

## Goal
Tools that show the C++ core gives the same results as the Python reference, and a guard on the carving text. Read `work/T001/code/README_code.md`, `puzzle/README_puzzle.md` and `quest/README_quest.md` first (the core rules are binding: no exceptions, no RTTI, no Unreal includes in core; tools and test programs outside the core may use normal C++).

## What QA specified (implement as written)
`work/T001/qa/check_spec.md` (version 2): Section 0 (conventions, contract), Section 6 (EQV-01 to EQV-14), and the PZL lines PZL-30 and PZL-31 (the carving-text guard script). If a line cannot be implemented as written, say which and why: never drop one silently. You do not edit the spec. You do not mark checks as passing: QA reviews them on broken inputs later. Specifically:
- `work/T001/code/tools/gen_golden.py <out.json> [--reference <path to a puzzle_state_machine.py>]`: runs the Python reference (default `work/T001/puzzle/puzzle_state_machine.py`) and writes golden vectors: every control sequence of length 1 to 4 over the three controls (120), each bypass route from each reachable state, save and load at each prefix; each vector holds its inputs and the expected `to_dict()`. It fails (exit 1 or 2, and writes no file or an error) if the reference is missing or unreadable, and its output must change when the reference's solution order changes.
- `eqv/` module: executable `eqv_check <golden.json>` that replays every vector through the C++ puzzle core and compares. Prints `N vectors, 0 mismatches`; exits 1 and names the vector id on a mismatch; an empty or zero-vector file fails with `FAIL NO_VECTORS`. Define it in `eqv/CMakeLists.txt` (not core, so ordinary flags are fine; link `dd_cli`, `dd_puzzle`, `dd_common`). Put the source in `eqv/src/`. Do NOT edit the top-level `CMakeLists.txt` or `cli/CMakeLists.txt`.
- `quest_transition_table` ctest (EQV-13): if the quest step did not already provide it, add it in `eqv/CMakeLists.txt` with its test source under `work/T001/code/tests/eqv/`; if it exists, do not duplicate it and say where it is.
- `work/T001/code/tools/check_text_guard.py` (PZL-30, PZL-31): checks the carving text for statements about who built the mine or what happened to its makers, exactly as the spec says, with its own broken-input behaviour.
- `work/T001/code/eqv/README_eqv.md`: how to run all of it, the contract as built, contract changes with reasons, what is NOT RUN. Do NOT edit `README_code.md`.

## Files you must NOT edit (shared)
Top-level `CMakeLists.txt`, `cli/CMakeLists.txt`, `common/`, `map/`, `puzzle/`, `quest/`, `unreal/` (the parallel run), `tests/testing.h`, `README_code.md`. If you need a change in any of them, list it in ISSUES.

## How to build and run (use your OWN build directory)
- Configure: `cmake -S work/T001/code -B work/T001/build_eqv -DDD_MODULES="common;map;puzzle;quest;eqv;cli"`.
- Build: `cmake --build work/T001/build_eqv`. Test: `ctest --test-dir work/T001/build_eqv --output-on-failure --output-log work/T001/code/logs/ctest_step4a.log` (ctest writes the log; no shell redirects; nothing typed by hand).
- You cannot run executables from your build directory by hand: add ctest tests that run them. Python tools you may run directly with `python3 -I`; give `gen_golden.py` an output path under `work/T001/code/logs/` for your own self-test.
- One simple command at a time: no `cd`, `;`, `&&`, pipes.

## Acceptance
- Builds clean, no warnings; every test you add passes; `gen_golden.py` produces at least 120 sequence vectors and `eqv_check` reports 0 mismatches on them; each broken-input behaviour in the spec lines you implemented is shown by a test of your own.
- No change to any shared file.

## Stop limit
One pass, with a self-check loop on your own failures. Write only under `work/T001/code/` (the paths named above) and your build directory. End with STATUS, OUTPUTS, ISSUES. Evidence: EXECUTED (with log path).
