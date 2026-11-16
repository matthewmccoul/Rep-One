# s017-software-engineer-fixes: fix the two defects QA found (owner: development-software-engineer)
Task ID T001-s017-software. Written by the Producer (the chat session). Rewrite round 1 of at most 2, as QA's review rules say. QA found two real defects in your work; the Producer confirmed both. Read `work/T001/defects/DEF-T001-3.md` and `work/T001/defects/DEF-T001-4.md` first (do not edit them: QA updates their status when it re-checks).

## Fixes
1. **DEF-T001-3: `tools/check_text_guard.py` cannot read the quest CSV.** QA spec lines QST-17 and QST-18 (in `work/T001/qa/check_spec.md`) run it on `work/T001/quest/figure_ore_lift_strings.csv`. Make it read both JSON files (as now) and the quest CSV (the `text` column, keyed by `key`). On the real CSV it exits 0. On a CSV where a line states who built the mine or what happened to its makers (see the spec lines for the exact word list and examples) it exits 1 and names the key and the offending word. A malformed CSV is a different failure from a leak: say which. Keep its existing behaviour and tests for JSON, and add tests for the CSV cases (clean, leak in `fig_offer_cold`, malformed).
2. **DEF-T001-4: `quest_transition_table` compares the table with itself.** It must be able to fail when the table and the engine disagree. Rewrite the test (`work/T001/code/tests/quest/test_quest.cpp`) so that it drives the real `Engine` through every (state, event) pair (offer, accept, decline, arrive with chain present, gone and other, report, unknown event, from every state) using only the public engine operations, observes the resulting state, and compares it with the `transitions` table in the quest JSON: listed pairs must give the listed state, unlisted pairs must leave the state unchanged. If the engine hard-codes parts of this (QA noted a switch in `Engine::report`), either make the engine take the table's transitions or make the test show every difference; the acceptance below decides.
   Add a way to point this one test at another quest JSON without editing source: the environment variable `DD_QUEST_JSON` (path), used by `quest_transition_table` only, documented in `quest/README_quest.md`. QA will use it to run its mutation (an extra row `not_offered --report--> completed`).
3. **Hostile tests crash after a failed check** (QA note under DEF-T001-4: in the QST-71 mutation the hostile tests ended in SEGFAULT after their CHECK failures). Make those tests stop cleanly on a failed CHECK instead of dereferencing a null result. The tests must still fail when the behaviour is wrong.

## Prove it yourself, then QA re-checks
- Create your own fixture under `work/T001/code/tests/quest/fixtures/` with the extra row, run the transition test against it through `DD_QUEST_JSON`, and show that it FAILS; show that it passes on the real file. Do the equivalent for the text guard: a leaking CSV fixture fails, the real CSV passes.
- Report the real ctest counts.

## Files you may edit
`work/T001/code/tools/check_text_guard.py`, `work/T001/code/tests/quest/`, `work/T001/code/tests/eqv/test_eqv_tools.py` (only for the text-guard tests), `work/T001/code/quest/` (only if the engine must change for item 2), `work/T001/code/quest/CMakeLists.txt` and `work/T001/code/eqv/CMakeLists.txt` (only to register tests), `work/T001/code/quest/README_quest.md`, `work/T001/code/eqv/README_eqv.md`, `work/T001/code/logs/`. If you touch `quest/`, keep it core-clean (no exceptions, no RTTI, no Unreal includes) and note it. Do NOT edit anything else (`common/`, `map/`, `puzzle/`, `cli/`, top-level `CMakeLists.txt`, `unreal/`, the spec, defects, content files).

## How to build and run
- Configure: `cmake -S work/T001/code -B work/T001/build_fix`. Build: `cmake --build work/T001/build_fix`.
- Test: `ctest --test-dir work/T001/build_fix --output-on-failure --output-log work/T001/code/logs/ctest_fix1.log` (all tests; ctest writes the log; no shell redirects; nothing typed by hand). To run an executable by hand use only the canonical `work/T001/build/` ones (allowed); your own build directory's executables only run through ctest.
- One simple command at a time.

## Acceptance
- Builds clean; all tests pass, including the new ones; the transition test FAILS on your extra-row fixture and passes on the real file; the text guard passes the real CSV and fails your leaking CSV naming the key.
- The unreal plugin's generated core copies are not changed by you; if `quest/` changes, say so, so the Producer re-runs `tools/sync_core_into_plugin.py`.

## Stop limit
One pass, with a self-check loop on your own failures. End with STATUS, OUTPUTS, ISSUES. Evidence: EXECUTED (with log paths).
