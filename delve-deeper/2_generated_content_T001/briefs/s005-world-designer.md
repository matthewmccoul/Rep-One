# s005-world-designer: map revision 1 (owner: development-world-designer)
Task ID T001-s005-world. Revision 1 of at most 2 for the map package. Everything you write is a PROPOSAL. Written by the Producer (the chat session).

## Why this revision
The critic (work/T001/critic/critic_report.md, defects in work/T001/defects/) found contradictions. Read findings 1, 3 and 5 and defect DEF-T001-2 first. Do not edit the critic's report or any defect file. Do not touch puzzle/ or quest/ files: the Systems Designer and the Narrative Designer are revising those in parallel from their own briefs.

## What to change (map side only)
1. **Locked door e08 gets the GDD's break option.** The GDD says a locked door must be picked, unlocked with magic, or broken. In `floor1_map.json`, edge e08's `solutions` must be exactly `["puzzle", "magic_key", "pick_lock", "break"]` (these four strings, this order; the Systems Designer uses the same strings). Update the layout table row for e08 ("Ways through") to match, and remove any text saying break is not offered.
2. **Validator rule for locked and stuck doors.** Add to `validate_map.py`: every `locked` edge lists `magic_key`, `pick_lock` and `break`; every `stuck` edge's solutions are exactly `["break"]`. Existing checks stay.
3. **Sewer grate: one description.** Keep node `sewer_grate` and edge e01 (town_entrance to sewer_grate) exactly as they are in the JSON. Make `floor1_layout.md` describe the grate once and consistently with the JSON (the JSON puts it in area_upper beside the entrance). State that the GDD lists the Figure among the town characters and does not say where the drain is, so this placement is a PROPOSAL, and list it in ISSUES for the user. Do not move it.
4. **Toll and boss.** The GDD has the bandit boss demand the toll to let the adventurer pass. In the layout text and in a `note` on slot `enc_bandit_toll`, say plainly that the boss demands the toll at `boss_room`, and that the bandits in `toll_hall` are crew (fight, negotiate or befriend; befriending individuals can influence the faction's daily assessment). Do not rename or move any ID.

## Logs must be written by the script
`validate_map.py` must write `work/T001/map/validate_map.log` itself (open the file inside the script and write every line it prints). Do NOT use shell redirects. Then run `python3 work/T001/map/validate_map.py`. The log must be the script's own output, not text you typed.

## Outputs (write only these)
work/T001/map/floor1_layout.md, work/T001/map/floor1_map.json, work/T001/map/validate_map.py, work/T001/map/validate_map.log. Set the version line in each to "Version 2 (revision 1 of 2)".

## Acceptance (pass/fail)
- The validator exits 0 and its log was written by the script.
- e08's solutions are exactly the four strings above; layout and JSON agree.
- No node or edge ID was added, removed or renamed.
- Layout and JSON describe the grate the same way; the toll text says the boss demands it.
- No other file was changed.
- No numbers invented; anything the GDD leaves TBD stays TBD.

## Stop limit
One pass. If a requirement cannot be met, stop and say why in ISSUES. End with STATUS, OUTPUTS, ISSUES. Label evidence STATIC or EXECUTED (with the log path). Unreal is not available: nothing here was run in Unreal.
