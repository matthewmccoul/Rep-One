# Brief s003-world-designer (map v0): T001, step 3, box C1
Owner: development-world-designer | Author: development-producer | Version 1
Label: T001-s003-world | Approved basis: user reply "2" at H0 (packet work/T001/packets/s001-packet.md, routing work/T001/packets/s002-packet.md)

## Inputs (read, do not paste)
- GDD (CLAUDE.md import): Scope of This Demo; Encounter Choices (doors, traps, faction); Boss Encounter; Death/World State; Player Feedback (minimap).
- work/T001/request.md (item 1), work/T001/packets/s001-packet.md sections 1, 2, 5. work/lore.md is empty; no prior floor design exists, so start from the GDD.

## Task
Map v0 of the whole first dungeon floor, plus the entrance from town. Result is something the Software Engineer can import and the Systems and Narrative designers can attach to.

## Outputs (exact paths)
- work/T001/map/floor1_layout.md: labelled diagram (ASCII/Mermaid), room list, connections with door types (plain, secret, locked, stuck), encounter slots (three monster types, bandits, boss), three traps, puzzle location, boss room guarding the descent, town entrance. Say where hidden things sit.
- work/T001/map/floor1_map.json: nodes and edges with stable IDs; types; door type per edge; encounter slots; trap slots; coordinates for minimap. Must contain these RESERVED nodes with exactly these IDs: `town_entrance`, `storeroom` (mine storeroom; locked/stuck door proposal is yours but it must be reachable only after its clue and the three controls are reachable outside it), `puzzle_clue_site` and `puzzle_controls_site` (both outside `storeroom`, reachable without opening it; may be the same room), `ore_lift` (old ore lift with chain and drum), `sewer_grate` (Mysterious Figure's grate, near town side; the Figure's task destination must be dungeon `ore_lift`), `boss_room`, `descent`.
- work/T001/map/validate_map.py: standard-library Python validator (reachability from `town_entrance`; boss and descent reachable; clue and controls reachable with storeroom door treated as closed; every edge endpoint exists; door types legal; counts match the GDD: 3 monster types, 3 traps, 1 puzzle, 4 door types all used). Run it; log to work/T001/map/validate_map.log.
- Handoff section at the end of floor1_layout.md: what you hand the Software Engineer (layout spec, JSON by ID, validation list) and what you expect back (importer/builder, validation result, problems). You own layout; Software owns code. Mark this split PROPOSAL.

## Acceptance criteria
- Every GDD floor item present; validator exits 0 (EXECUTED, log path cited); nothing in the file reveals that the dwarves were not the makers; numbers the GDD leaves TBD stay TBD or are labelled PROPOSAL.
- Leave a short "Attachment points" table that the Systems Designer (puzzle) and Narrative Designer (quest) will use; they may ask for map v1 changes later.
- Unreal map assets/maps are NOT produced; say so. Unreal code NOT RUN.

## Allowance and stop
About 12,000 tokens of reasoning/output; keep it short. Stop limit: 2 revision rounds. Write only the paths above. End with STATUS, OUTPUTS, ISSUES.
