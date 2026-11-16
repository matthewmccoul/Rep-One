# s005-systems-designer: puzzle revision 1 (owner: development-systems-designer)
Task ID T001-s005-systems. Revision 1 of at most 2 for the puzzle package. Written by the Producer (the chat session).

## Why this revision
The critic (work/T001/critic/critic_report.md, defects in work/T001/defects/) found contradictions. Read findings 1, 2 and 6 and defects DEF-T001-1 and DEF-T001-2 first. Do not edit the critic's report or any defect file. Do not touch map/ or quest/ files: the World Designer and the Narrative Designer are revising those in parallel from their own briefs.

## The user's decision (their own words)
For the "Use" action, the user replied: "Approve Use". So the new player action **Use on a selected object is approved** for the puzzle controls. Quote those two words as the user's reply. It applies only to the three controls (`ctl_pick`, `ctl_cart`, `ctl_furnace`); it approves nothing else. Remove the "Rule needing approval" wording and the "not approved" labels for Use, in the spec and in `interaction_rule` in the JSON. Everything else you propose stays a PROPOSAL.

## What to change
1. **Record the Use approval** as above. Keep alternatives (a), (b), (c) only as a short history note, marked "not chosen".
2. **Locked door e08 gets the GDD's break option.** The GDD says locked doors must be picked, unlocked with magic, or broken. The World Designer is setting e08's solutions to exactly `["puzzle", "magic_key", "pick_lock", "break"]`. Use these same strings. Remove "Breaking is not offered for e08" and its open item. Behaviour (PROPOSAL): breaking opens e08, the three controls become inert, `opened_via` is `break`, state `bypassed`, no puzzle XP. Any noise, damage or cost for breaking is TBD: invent no numbers. In the JSON, `other_routes` becomes `["magic_key", "pick_lock", "break"]`.
3. **Puzzle XP.** Keep the amount TBD. Say plainly that the GDD names traps and searches as XP sources, not puzzles, so whether solving earns XP at all is a PROPOSAL for the user. Drop "once per world" as a stated rule; keep only that if XP is awarded it is awarded at most once for the solve, and not for a bypass.
4. **Code and tests.** Update `puzzle_state_machine.py` and `test_puzzle.py` for `break` as an opening route (`opened_via` `break`, state `bypassed`, controls inert afterwards) and add tests for it. Keep every existing test (all orderings, reset, save/load, key and pick routes).

## Logs must be written by the script
`test_puzzle.py` must write `work/T001/puzzle/test_puzzle.log` itself (open the file inside the script and write every line it prints). Do NOT use shell redirects, and do NOT type the log by hand. Then run `python3 work/T001/puzzle/test_puzzle.py`.

## Outputs (write only these)
work/T001/puzzle/storeroom_puzzle_spec.md, storeroom_puzzle.json, puzzle_state_machine.py, test_puzzle.py, test_puzzle.log. Set the version to "Version 2 (revision 1 of 2)".

## Acceptance (pass/fail)
- Tests exit 0, the log was written by the script, and it covers the break route.
- The spec quotes "Approve Use" as the user's reply and no longer calls Use unapproved.
- No sentence says breaking e08 is not offered.
- `other_routes` and the spec use the exact strings above.
- XP wording matches item 3; nothing invented where the GDD is TBD.
- No other file was changed.

## Stop limit
One pass. If a requirement cannot be met, stop and say why in ISSUES. End with STATUS, OUTPUTS, ISSUES. Label evidence STATIC or EXECUTED (with the log path). Unreal is not available: nothing here was run in Unreal.
