# Brief s003-systems-designer (storeroom puzzle): T001, step 3, box C1
Owner: development-systems-designer | Author: development-producer | Version 1 | Label: T001-s003-systems
Approved basis: user reply "2" at H0. Dependencies: map v0 exists (work/T001/map/).

## Inputs
- GDD (CLAUDE.md import): Scope; Encounter Choices (Search, Traps, Doors, Puzzle); Player Facing Actions; Loot (magic key); Player Feedback.
- User request, item 2: pipeline_state/tasks/T001/request.md (correct path; read only).
- Map: work/T001/map/floor1_layout.md (Attachment points) and floor1_map.json. Reserved IDs: `puzzle_clue_site`, `puzzle_controls_site` (ctl_pick, ctl_cart, ctl_furnace), `storeroom`, edge e08 (storeroom door, locked). Do not edit map files; request map v1 changes in your ISSUES.
- work/lore.md (empty). No catalogue.

## Task
Design the one playable puzzle: mine storeroom, three controls marked pick, cart, furnace; carving clues the order dig -> haul -> smelt. Specify: actual carving text, interactable objects and their states, solution, wrong-attempt response (must not softlock; any penalty is a PROPOSAL, no invented GDD numbers), what solving opens (the storeroom door e08, and what is inside: PROPOSAL, only GDD item types), repeat/reset behaviour, save/load state, how Search or a magic key/other door route interacts, XP for solving (PROPOSAL or TBD).
Use only interactions the GDD supports. The GDD has no "pull/use control" action: model the controls as selectable objects with a "use" on a selected object (like Search's object selection) and FLAG this clearly as a rule needing the user's explicit approval (ISSUES + a "Rule needing approval" heading). Do not claim it is approved. Keep the carving free of any hint that the dwarves were not the mine's makers.

## Outputs (exact paths)
- work/T001/puzzle/storeroom_puzzle_spec.md: rules spec.
- work/T001/puzzle/storeroom_puzzle.json: data table keyed to map IDs (objects, states, solution sequence, response, opens edge, strings with stable keys).
- work/T001/puzzle/puzzle_state_machine.py: standard-library Python reference state machine (pure logic, serialisable state, no engine calls) plus work/T001/puzzle/test_puzzle.py covering all 3! orderings, partial sequences, wrong then right, repeat after solve, reset, save/load round trip, and that the JSON IDs exist in work/T001/map/floor1_map.json. Run the tests; log to work/T001/puzzle/test_puzzle.log (EXECUTED). Unreal component is not yours; say NOT RUN for anything Unreal.

## Acceptance
Clue and controls reachable with the storeroom closed; only dig->haul->smelt opens; wrong response non-softlocking; tests pass with log; TBDs stay TBD; PROPOSAL labels on values. 

## Allowance and stop
About 12,000 tokens; short. Stop limit: 2 revision rounds. Write only paths above. End with STATUS, OUTPUTS, ISSUES.
