# s006-development-critic: critic check, round 2 (owner: development-critic)
Task ID T001-s006-critic. Written by the Producer (the chat session). Evidence labels STATIC (inspection) or EXECUTED (run, with log path). You are the critic: you report, you fix nothing.

## What to check
New content after revision 1, against the GDD (`08_Final_GDD_Extracted.txt`) and `work/lore.md` (empty: say so, invent no lore). Read ALL of each file, then all of the GDD:
- work/T001/map/floor1_layout.md, work/T001/map/floor1_map.json, work/T001/map/validate_map.py, work/T001/map/validate_map.log
- work/T001/puzzle/storeroom_puzzle_spec.md, work/T001/puzzle/storeroom_puzzle.json, work/T001/puzzle/puzzle_state_machine.py, work/T001/puzzle/test_puzzle.py, work/T001/puzzle/test_puzzle.log
- work/T001/quest/figure_ore_lift_quest.md, work/T001/quest/figure_ore_lift_strings.csv, work/T001/quest/figure_ore_lift_quest.json, work/T001/quest/validate_quest.py, work/T001/quest/validate_quest.log
- Request they answer: pipeline_state/tasks/T001/request.md (read it; the content must meet it where it speaks to the GDD).
- Round 1 for reference only: work/T001/critic/critic_report.md and the defects in work/T001/defects/. Do not edit them. Do not trust them either: re-check everything fresh.

## Context you may rely on (decisions, not sources of truth over the GDD)
- The user approved the new player action Use for the three puzzle controls with the words "Approve Use" (work/T001/producer_notes.md:7).
- The Producer's standing decisions are in work/T001/producer_notes.md under "Decisions the Producer made". Do not re-flag a listed decision unless the content contradicts it or the GDD.
- Defects from round 1: say in your report which of them (DEF-T001-1, DEF-T001-2) the new content fixes and which remain. You have no Edit tool and must not rewrite them.

## Output
Write `work/T001/critic/critic_report_r2.md` following your role file, section 4 (`Read check:` line first, quotes in the exact form, `Mentions checked:` / `Context checked:` where required, severity labels, last line `Verdict: CONSISTENT` or `Verdict: CONTRADICTIONS`). File new defects for BLOCKING and MAJOR findings only in `work/T001/defects/` (next free number after listing the directory; `same as <path>` instead of duplicates).

## Citation rules (a failed check rejects the box)
- Every quote: exact wording in straight double quotes, then `(path:LINE)` with the FULL path from the project folder, for example `work/T001/map/floor1_map.json:62`. Never a bare file name or a word like `csv`.
- A quote that runs across a line break cites both lines: `(path:A-B)`.
- This applies to every quote in the report, including the sections that say nothing was found.
- **Before you finish, run `python3 -I orchestration/verify_quotes.py work/T001/critic/critic_report_r2.md`.** If any line says FAIL, fix that citation (path, line numbers, or the exact wording) and run it again until it reports all quotes verified. Do not drop a finding to make the check pass; correct its citation.

## Acceptance
All required checks done and shown; the verifier reports every quote verified (give its last output line in your reply); one report; no other file edited.

## Stop limit
One pass, with the self-check loop above. End with STATUS, OUTPUTS, ISSUES.
