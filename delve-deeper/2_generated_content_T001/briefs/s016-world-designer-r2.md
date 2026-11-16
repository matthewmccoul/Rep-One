# s016-world-designer-r2: map revision 2 (owner: development-world-designer)
Task ID T001-s016-world. Written by the Producer (the chat session). This is revision 2 of at most 2 for the map package: the last one. Do all items completely. Do not touch puzzle/, quest/, qa/, code/ or any defect file.

## Why
Sources: critic round-2 report (work/T001/critic/critic_report_r2.md, finding 4 and its note on logs), QA's spec issues (work/T001/qa/check_spec.md, closing ISSUES), and a Producer finding: quest log lines wrap the map node name in brackets, and the ore lift's name already has brackets, so the player would see "(Old ore lift (chain and drum))".

## What to change
1. **Status labels.** `floor1_map.json` still says "PROPOSAL map v0" in its `status`, and the layout calls the map v1. Set the version and status text in both files to "Version 3 (revision 2 of 2)" with "PROPOSAL (not approved by the user)", and make nothing say "v0".
2. **No brackets inside node names.** In `floor1_map.json` no node `name` may contain parentheses (names are inserted into text that adds its own). Rewrite the ones that do, keeping their meaning: for example "Old ore lift (chain and drum)" becomes "Old ore lift, chain and drum". Do the same in the layout tables and diagram text so both files agree. IDs, node and edge counts, coordinates, door types and every other field stay exactly as they are (other code and tests read this file; the C++ and Python validators must still accept it).
3. **The validator must not overwrite its real log by accident.** In `validate_map.py`: write `validate_map.log` only when run with no arguments (the real run) or with `--log PATH`; a run with any path argument prints its result and writes no log. Keep it writing its own log from inside the script (no shell redirects, nothing typed by hand). Add one rule: it must reject a node `name` that contains parentheses. Run `python3 work/T001/map/validate_map.py` last, with no arguments, so the real log shows the real run.
4. Nothing else changes. No numbers invented.

## Outputs (write only these)
work/T001/map/floor1_layout.md, work/T001/map/floor1_map.json, work/T001/map/validate_map.py, work/T001/map/validate_map.log.

## Acceptance (pass/fail)
- `validate_map.py` with no arguments exits 0 and its log was written by the script from that run; run on another file it writes no log.
- No node name contains parentheses; the layout agrees with the JSON; no "v0" remains.
- Node IDs, edge IDs, counts (17 nodes, 15 edges), coordinates, doors and encounter slots are unchanged.

## Stop limit
One pass. End with STATUS, OUTPUTS, ISSUES. Label evidence STATIC or EXECUTED (with the log path). Unreal is not available.
