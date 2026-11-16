# s018-development-critic: critic check, round 3 (owner: development-critic)
Task ID T001-s018-critic. Written by the Producer (the chat session). Evidence labels STATIC (inspection) or EXECUTED (run, with log path). You report; you fix nothing.

## What to check
The content after the second revision round, against the GDD (`08_Final_GDD_Extracted.txt`) and `work/lore.md` (empty: say so, invent no lore). Read ALL of each file, then all of the GDD:
- work/T001/map/floor1_layout.md, work/T001/map/floor1_map.json, work/T001/map/validate_map.py, work/T001/map/validate_map.log
- work/T001/puzzle/storeroom_puzzle_spec.md, work/T001/puzzle/storeroom_puzzle.json, work/T001/puzzle/puzzle_state_machine.py, work/T001/puzzle/test_puzzle.py, work/T001/puzzle/test_puzzle.log
- work/T001/quest/figure_ore_lift_quest.md, work/T001/quest/figure_ore_lift_strings.csv, work/T001/quest/figure_ore_lift_quest.json, work/T001/quest/validate_quest.py, work/T001/quest/validate_quest.log
- The request they answer: pipeline_state/tasks/T001/request.md.
- For reference only: your round-2 report work/T001/critic/critic_report_r2.md and the defects in work/T001/defects/. Do not edit them. Re-check everything fresh.

## What changed since round 2 (check that nothing broke and nothing new contradicts)
- Map: no parentheses in node names (the ore lift is "Old ore lift, chain and drum"); status labels say "Version 3 (revision 2 of 2)"; the validator's logging rule.
- Puzzle: the claim tying lockpicking to a Thief was removed.
- Quest: from round 2 (a hostile Figure has lines mid-quest at standing -2; a NEW offer at -2 is refused); the quest log lines now read cleanly with the node name.
- The Producer's standing decisions are in work/T001/producer_notes.md ("Decisions the Producer made"). The user approved Use for the three controls with the words "Approve Use" (work/T001/producer_notes.md:7).

## Output
Write `work/T001/critic/critic_report_r3.md` following your role file, section 4 (Read check line first, quotes in the exact form, Mentions checked / Context checked where required, severity labels, last line `Verdict: CONSISTENT` or `Verdict: CONTRADICTIONS`). File defects for BLOCKING and MAJOR findings only in `work/T001/defects/`, using numbers from **DEF-T001-20 upward** (another run is using the lower numbers); `same as <path>` instead of duplicates.

## Citation rules (a failed check rejects the box)
- Every quote: exact wording in straight double quotes, then `(path:LINE)` with the FULL path from the project folder (for example `work/T001/map/floor1_map.json:62`), never a bare file name.
- A quote that runs across a line break cites both lines: `(path:A-B)`.
- This applies to every quote in the report, including the sections that say nothing was found.
- **Before you finish, run `python3 -I orchestration/verify_quotes.py work/T001/critic/critic_report_r3.md`.** If any line says FAIL, fix that citation and run it again until it reports all quotes verified. Do not drop a finding to make the check pass.

## Acceptance
All required checks done and shown; the verifier reports every quote verified (give its last output line in your reply); one report; no other file edited.

## Stop limit
One pass with the self-check loop. End with STATUS, OUTPUTS, ISSUES.
