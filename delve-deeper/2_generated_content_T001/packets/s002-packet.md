Version: 1 | Task: T001 | Author: development-producer | Status: done | Evidence: STATIC (nothing run)
Inputs read: GDD (required parts), request.md, decisions.md, history.md, 03_DEVELOPMENT_WORKFLOW.mmd, work/T001/packets/s001-packet.md, work/lore.md (empty). No catalogue exists.

# Routing: T001 after approval of the starting brief

## Basis
The user's own reply "2" chose "Approved brief" at H0. I treat the s001 packet as approved as written: three packages, 12.00 USD task budget, 2 revision rounds per package, Unreal delivery as data-driven JSON/CSV plus thin C++ loaders with Python validation. I record no approval beyond that reply.

## Arrow chosen: "Required systems accepted" -> C1 (World, Systems, Narrative and Assets: create)
Why: the task is content creation. No new engine capability or rule needs building first. The GDD and the approved brief already fix what the puzzle and quest may use. Interface work and process analysis are not needed yet.

## Open points (carried, not decided by me)
- Decision 3 (puzzle controls as selectable "use" objects) was in the approved packet. The Systems Designer must still flag it as a rule needing the user's explicit approval. If the user rejects it, the route becomes D1 or U.
- Interaction not named in the GDD: the user's "uses only interactions the project supports" binds the Systems Designer.

## C1 dispatch plan (PROPOSAL, sequenced; one level deep)
Wave 1 (this step's successor): World Designer, map v0.
- Inputs: GDD, request.md, this plan. Outputs: layout spec with labelled diagram, JSON node/edge list with stable IDs, validator, reserved nodes (storeroom, ore lift, sewer grate near town entrance, boss room, descent).
- Hands to Software Engineer later: layout spec, JSON keyed by IDs, validation list. Software Engineer hands back importer, validation result, problems. World owns layout; Software owns code and checks.
- Allowance about 12k tokens; stop limit 2 revisions.

Wave 2 (after map v0, in parallel):
- Systems Designer: puzzle spec, JSON data table, state-machine design. Clue and controls reachable before the storeroom opens; wrong-attempt response must not softlock; penalty values labelled PROPOSAL.
- Narrative Designer: Figure quest dialogue (receive and report-back, CSV with game-filled blanks), quest JSON, destination and discovery tied to map IDs. His reason stays hidden. Reward follows the GDD (usually none, rarely a rare item). Lore guard: no hint that the dwarves were not the makers.
- World Designer: map v1 fitting the puzzle and quest destination.

Wave 3: Software Engineer (importer, puzzle state machine, quest state, validators, Python tests EXECUTED with log paths; Unreal code NOT RUN). Then Critic once (brief names every new content file, GDD and work/lore.md), then QA, then user review.

## Budget status
12.00 USD approved; 0.22 USD spent at step start. Estimate about 10 to 12 runs, unmeasured. Per-run cap 2.00 USD.

## Known limits
Unreal is unavailable here. All of the above is a plan; no content exists yet.

STATUS: done
OUTPUTS: work/T001/packets/s002-packet.md, work/T001/step_results/s002-P.json
ISSUES: Decision 3 rule wording needs the user's explicit approval via Systems Designer (user). No Unreal access (user). Approvals recorded only by the pipeline (undecided by user).
