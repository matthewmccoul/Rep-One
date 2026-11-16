Version: 1 | Task: T001-s015-qa3 | Author: development-qa-tester | Status: done | Inputs read: brief s015-qa-pzl-eqv.md; check_spec.md Sections 0, 3, 6, 7, 8; code/puzzle/README_puzzle.md; code/eqv/README_eqv.md; tools/check_text_guard.py, tools/gen_golden.py (read in full); tests/puzzle/test_puzzle.cpp (the five named tests) | Evidence: EXECUTED (logs in work/T001/qa/out/), STATIC for review reading

# Contract decisions: puzzle P1-P5, equivalence E1-E7
QA judges only. None of these is a user approval.

## Puzzle
- **P1 (test names pzl_* plus five puzzle_* aliases): ACCEPT, with a spec correction owned by QA.** The five aliases run the same cases, and their test bodies were read and match PZL-14, 20, 23, 24 and 28. But the group regex in my spec (`-R puzzle_`, PZL-32 and PZL-33) selects only 5 aliases plus `map_puzzle_rules` (PZL-32.log), and on the mutant it still exits 0 (PZL-33s.log). The spec line was wrong, not the code. PROPOSAL for the next spec revision: PZL-32 and PZL-33 use `-R pzl_`. I ran both with `pzl_` (PZL-32b.log 37 of 37 passed; PZL-33.log mutant fails 5 tests). Not a defect.
- **P2 (extra data codes, SAVE_FIELD for unknown extra fields): ACCEPT.** Stricter than spec, rejects only broken data; the real files pass (PZL-01).
- **P3 (save consistency rules beyond the Python reference): ACCEPT.** The spec requires them (PZL-26, 27); both fail on QA fixtures; a good save loads (PZL-27g.log).
- **P4 (PZL-19 last press is the puzzle's first control): ACCEPT.** With the real data it is ctl_pick, as the spec expects (PZL-19.log).
- **P5 (bypass with an unknown route fails before any control is pressed): ACCEPT.** Spec only requires exit 1 `FAIL UNKNOWN_ROUTE` (PZL-21.log).

## Equivalence
- **E1 (eqv_check reads the real puzzle file, optional --puzzle): ACCEPT.** Needed for EQV-07; EQV-07 shows 315 mismatches against the changed-reference golden file. Note the path is compiled in, so the check depends on the real file's content.
- **E2 (golden format and vector ids): ACCEPT.** Each vector holds ops, per-op result with string key, saved snapshot and final to_dict (read in gen_golden.py; 331 vectors, 120 sequences).
- **E3 (a reference copy with no data file beside it uses the real data): ACCEPT, with a limit recorded.** My EQV-04 fixture changed the solution in code and got 315 differing vectors, so the generator does depend on the reference. Limit: the header's `solution` and the `saveload-solved` ops come from the data file, not the changed code, so those ops press the real order on the changed reference.
- **E4 (reference ValueError vs C++ UnknownControl/UnknownRoute, recorded as `error`): ACCEPT.** Six edge vectors cover it and match (EQV-05).
- **E5 (`--diff` option): ACCEPT.** Exit 1 on a different file (EQV-04d.log).
- **E6 (text guard also fails on duplicate keys, bad UTF-8, no str.* strings, non-string values; prefix-at-word-start matching): ACCEPT.** Stricter than spec, real file passes (PZL-30.log). Limit (from its own README): a paraphrase that avoids the list passes; the list is only a proxy for the lore guard.
- **E7 (ctest names for broken-input tests): ACCEPT.** Not run by QA beyond PZL-32b (`eqv_pzl_text_guard_real`); QA's own fixtures were used for EQV lines.

## Spec lines the Software Engineer dropped or changed
None dropped. Changed: PZL-32/33 group regex (see P1; QA's spec error); EQV-03 allows exit 1 or 2 and the generator gives 2 with no output file.

## Review limits
`eqv_check.cpp` was reviewed by behaviour on three QA broken inputs (tampered, changed-reference, empty), not line by line. The PZL-24 "after a day passes" clause is argued statically (the puzzle has no calendar input).

STATUS: done
OUTPUTS: work/T001/qa/contract_decisions_pzl_eqv.md
ISSUES: none new
