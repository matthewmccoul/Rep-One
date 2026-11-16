# s016-systems-designer-r2: puzzle revision 2 (owner: development-systems-designer)
Task ID T001-s016-systems. Written by the Producer (the chat session). This is revision 2 of at most 2 for the puzzle package: the last one. Do not touch map/, quest/, qa/, code/ or any defect file.

## Why
Source: critic round-2 report (work/T001/critic/critic_report_r2.md, finding 1): the spec says lockpicking is for a Thief "per map", but no map file names a class for lockpicking.

## What to change
1. In `storeroom_puzzle_spec.md` (and anywhere the JSON or the Python files repeat it), remove the claim that lockpicking belongs to a Thief per the map. Say what the GDD says: locked doors must be picked, unlocked with magic, or broken (GDD, Doors), and which class or skill may pick locks is not stated and is not decided here. Search the three files for any other claim attributed to the map or the GDD that they do not support, and fix it the same way.
2. Set the version line in each file you touch to "Version 3 (revision 2 of 2)".
3. Nothing else changes: route strings, state names, the Python reference's behaviour and the 19 tests stay as they are. Re-run `python3 work/T001/puzzle/test_puzzle.py` last so `test_puzzle.log` is rewritten by the script from a real run.

## Outputs (write only these)
work/T001/puzzle/storeroom_puzzle_spec.md, storeroom_puzzle.json, puzzle_state_machine.py, test_puzzle.py, test_puzzle.log (rewrite only the ones you actually change; the log is rewritten by the test run).

## Acceptance (pass/fail)
- The tests pass (exit 0) and the log was written by the script.
- No sentence ties lockpicking to a Thief or to the map.
- Behaviour unchanged: `other_routes` is still `["magic_key", "pick_lock", "break"]`; the 19 tests are still all there.

## Stop limit
One pass. End with STATUS, OUTPUTS, ISSUES. Evidence STATIC or EXECUTED (with the log path). Unreal is not available.
