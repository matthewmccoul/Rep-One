# Brief s003-narrative-designer (Mysterious Figure quest): T001, step 3, box C1
Owner: development-narrative-designer | Author: development-producer | Version 1 | Label: T001-s003-narrative
Approved basis: user reply "2" at H0. Dependencies: map v0 exists (work/T001/map/).

## Inputs
- GDD (CLAUDE.md import): Watchers and Town Characters (Mysterious Figure), Assessment Consequences, Encounter Choices (Search, Doors), Score bands, Runtime AI.
- User request, item 3: pipeline_state/tasks/T001/request.md (correct path; read only).
- Map: work/T001/map/floor1_layout.md (Attachment points) and floor1_map.json. IDs: `sewer_grate` (giver), `ore_lift` (destination, `quest_destinations.figure_task_destination`). Do not edit map files; request changes in ISSUES.
- work/lore.md (empty). No catalogue.

## Task
One Mysterious Figure quest. From behind the sewer grate he asks whether the old ore lift's chain is still on the drum. His reason is never revealed. Write: the actual task-receiving dialogue, the report-back dialogue (variants for: chain present, chain gone, adventurer did not go / not yet / came back with nothing), where the adventurer goes (map IDs and a short route), what they find at `ore_lift` (the discovery; how it is made visible: by arrival, or by Search per GDD rules, your choice, state it), what completes the quest (checkable condition), reward per GDD (usually none, "there is still much work to be done"; rarely a rare item: PROPOSAL, mark the chance TBD). The Figure's voice: glowing eyes, lanky green arms, pointed fingers, loosely Evil, terse. Dialogue may vary by relationship stage (five stages, GDD bands) in a small way; blanks such as {item}, {node_name}, {day} are filled from game state. The quest must work without UI/AI changes (uses ordinary dialogue box and message log). Lore guard: nothing hints that the dwarves were not the mine's makers; do not state who built the lift beyond "the old miners/dwarves" surface premise.

## Outputs (exact paths)
- work/T001/quest/figure_ore_lift_quest.md: quest design and voice notes.
- work/T001/quest/figure_ore_lift_strings.csv: string table (key, speaker, stage_min, stage_max, text, blanks).
- work/T001/quest/figure_ore_lift_quest.json: quest data (giver, destination node ID, discovery, states, completion condition, outcomes, reward, string keys). Match the ID style of the map JSON.
- work/T001/quest/validate_quest.py: standard-library validator (JSON and CSV keys agree, every blank is in a declared set, map IDs exist in work/T001/map/floor1_map.json, reason-never-revealed check as a simple banned-phrase list); a minimal quest state-transition function plus self-tests; run it, log to work/T001/quest/validate_quest.log (EXECUTED). Unreal quest component is not yours; NOT RUN.

## Acceptance
Both dialogues complete; destination and discovery tie to real map IDs; completion condition checkable; reason hidden; GDD reward rule followed; validator passes with log; TBDs stay TBD, values labelled PROPOSAL.

## Allowance and stop
About 12,000 tokens; short. Stop limit: 2 revision rounds. Write only paths above. End with STATUS, OUTPUTS, ISSUES.
