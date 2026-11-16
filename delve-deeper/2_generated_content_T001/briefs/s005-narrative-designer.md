# s005-narrative-designer: quest revision 1 (owner: development-narrative-designer)
Task ID T001-s005-narrative. Revision 1 of at most 2 for the quest package. Written by the Producer (the chat session).

## Why this revision
The critic (work/T001/critic/critic_report.md) found a contradiction-adjacent issue in finding 4. Read finding 4 first. Do not edit the critic's report or any defect file. Do not touch map/ or puzzle/ files: the World Designer and the Systems Designer are revising those in parallel from their own briefs.

## What to change
1. **Do not assert why the chain is gone.** In `figure_ore_lift_strings.csv`, line `fig_report_gone_cold` says "Gone. Taken. Then someone was there before you." The quest data says whether and how the chain can change is still undecided (Systems/World, TBD). Rewrite that line so the Figure does not state a cause or imply anyone took it. Keep his voice (short, strange, patient, never says why), keep the stage band (-1 to 0), the `{reward_line}` blank and the stage-cold tone. Check `fig_report_gone_warm` and every other line for the same kind of assertion and fix any you find.
2. **Everything else stays**: keys, stage bands, blanks, the route words ("Down the entry hall. Past the gallery. The lift." matches map edges e02, e04, e10) and the quest JSON structure. If the quest .md or .json quotes the old line, update that quote to match.
3. The Figure still never states his reason and nothing says who built the mine.

## Logs must be written by the script
`validate_quest.py` must write `work/T001/quest/validate_quest.log` itself (open the file inside the script and write every line it prints). Do NOT use shell redirects, and do NOT type the log by hand. Then run `python3 work/T001/quest/validate_quest.py`.

## Outputs (write only these)
work/T001/quest/figure_ore_lift_strings.csv, work/T001/quest/figure_ore_lift_quest.json, work/T001/quest/figure_ore_lift_quest.md, work/T001/quest/validate_quest.py, work/T001/quest/validate_quest.log. Set the version to "Version 2 (revision 1 of 2)" where a version line exists.

## Acceptance (pass/fail)
- The validator exits 0 and its log was written by the script.
- `fig_report_gone_cold` (and any other gone-line) states no cause and implies no actor.
- Only that change (plus matching quotes) differs from version 1; keys and blanks unchanged.
- No numbers invented; reward chance stays TBD.
- No other file was changed.

## Stop limit
One pass. If a requirement cannot be met, stop and say why in ISSUES. End with STATUS, OUTPUTS, ISSUES. Label evidence STATIC or EXECUTED (with the log path). Unreal is not available: nothing here was run in Unreal.
