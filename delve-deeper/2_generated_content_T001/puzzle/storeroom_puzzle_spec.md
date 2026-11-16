Version 3 (revision 2 of 2) | Task T001-s005-systems | Author: development-systems-designer | Status: Use on the three controls approved by the user; everything else PROPOSAL (nothing else approved) | Evidence: STATIC design + EXECUTED reference logic tests (log work/T001/puzzle/test_puzzle.log, written by test_puzzle.py itself); Unreal NOT RUN
Inputs read: GDD (CLAUDE.md import), brief s005-systems-designer.md, critic_report.md (findings 1, 2, 6), DEF-T001-1.md, DEF-T001-2.md, previous v1 puzzle files.

# Storeroom puzzle (dig -> haul -> smelt)

**Question:** one playable puzzle using only supported interactions. **GDD refs:** Encounter Choices (Search, Traps, Doors, Puzzle: "to be designed by the development team"), Player Facing Actions (Search object selection), Loot (magic key), Scope (one puzzle).

## Approved: Use on the three controls
The user replied "Approve Use". So the player action **Use on a selected object** is approved for the three controls only (`ctl_pick`, `ctl_cart`, `ctl_furnace`). The player selects the control (as with Search or targeting) and chooses Use. It approves nothing else: no other object accepts Use, and no other rule in this spec is approved.
History note (not chosen): (a) Use on controls, now approved; (b) controls as door-like objects via Search/inspect, not chosen; (c) drop the puzzle for a key-only door, not chosen.

## Layout (map IDs)
- Clue: `puzzle_clue_site` in `foreman_office`: carved panel `obj_carving`. Visible, no Search needed.
- Controls: `puzzle_controls_site` in `smelter_hall`: `ctl_pick` (pick rack), `ctl_cart` (ore cart), `ctl_furnace` (furnace).
- Opens: edge `e08` (smelter_hall > storeroom, locked). Both sites are reachable with e08 closed (tested against the map JSON).

## Carving text (key `str.carving.text`; PROPOSAL, Narrative may polish)
"Three tools keep the mine alive. First the pick bites the stone. Then the cart bears it away. Last the fire makes it fit to keep. Do the work in its order and the store will open."
It says nothing about who built the mine.

## Objects and states
| Object | States | Notes |
|---|---|---|
| obj_carving | unread, read | repeatable read |
| ctl_pick / ctl_cart / ctl_furnace | ready, set, inert | `set` = accepted step this attempt; `inert` after the door is open |

## Solution and responses (PROPOSAL)
Solution: ctl_pick, ctl_cart, ctl_furnace in that order. Puzzle state = ordered list of accepted steps.
- Correct next control: step accepted (`str.use.step_1`, `step_2`); third completes: `str.use.solved`, e08 opens, state `solved`.
- Any other control (including repeating one): `str.use.wrong`; progress resets to empty; retry at once. At most two full tries always solve it (tested for every ordering).
- **Penalty: none (PROPOSAL).** No damage, no lockout, no cost, so no softlock. Any penalty (e.g. a trap effect) is TBD; no numbers invented.
- Controls never lock out. A solved or open door never re-locks.

## Repeat / reset
After solve or any other opening, Use on a control returns `str.use.inert`; nothing changes. Partial progress persists until a wrong press or opening (no timer). A reset function exists for partial progress only (debug/design use).

## What solving opens (PROPOSAL, GDD item types only)
Door e08 opens (plain once open). Storeroom (map slots): trapped chest `trap_store` (needle_lock placeholder, effects TBD), `loot_store_1` healing potion, `loot_store_2` mana crystal; optional one authored weapon or armor piece. The storeroom is optional; not required for the descent (map: main route needs no lock).

## Routes through e08
The GDD says locked doors must be picked, unlocked with magic, or broken. The World Designer sets e08's solutions to exactly `["puzzle", "magic_key", "pick_lock", "break"]`. This spec uses the same strings; `other_routes` is `["magic_key", "pick_lock", "break"]`.
- `puzzle`: the three controls, above.
- `magic_key` (GDD: opens one lock, once) and `pick_lock` (GDD: locked doors must be picked; which class or skill may pick locks is not stated in the GDD and is not decided here, TBD): e08 opens, controls become inert, `opened_via` is the route string, state `bypassed`, no puzzle XP.
- `break` (GDD-listed option for locked doors): PROPOSAL behaviour: e08 opens, the three controls become inert, `opened_via` is `break`, state `bypassed`, no puzzle XP. Any noise, damage or cost for breaking is **TBD**; no numbers invented. Break formula or requirements (e.g. Strength) are TBD.
- Search: the puzzle needs none (clue and controls are visible). Search keeps its usual role in the room (secret door e09). The search formula is TBD (GDD) and is not touched here.
- Only the first opening counts. If e08 is already open, a later route does nothing (tested).

## Save / load
Fields: progress, solved, door_open, opened_via (puzzle | magic_key | pick_lock | break | null), wrong_count, xp_awarded. Scope PROPOSAL: per world (door state is world state; GDD says per-character relationships and shared world state, but is silent on puzzles). Autosave includes them. Round trip tested, including after break.

## XP
Amount **TBD** (XP table TBD in GDD). The GDD names traps and searches as XP sources ("Surviving encounters—including traps and searches—earns XP"), not puzzles. So whether solving the puzzle earns XP at all is a **PROPOSAL for the user**, not a rule. If XP is awarded, it is awarded at most once for the solve, and not for a bypass (magic_key, pick_lock or break). The reference code keeps XP class key `puzzle_solve` only as a placeholder for that proposal.
Counters (PROPOSAL, to AI Integration): record "puzzle solved" and "door opened by key/pick/break".

## Parameters (PROPOSAL, adjustable)
step count 3; wrong penalty none; reset on wrong yes; XP amount TBD, XP at all TBD; persistence scope world; break cost TBD; contents as above.

## Test criteria for QA
1. Reach foreman_office and smelter_hall with e08 closed. 2. Only pick, cart, furnace opens e08; the other five orderings do not. 3. After a wrong order the player can retry; no state blocks it. 4. Key, pick or break opens e08; controls then inert; no puzzle XP. 5. Save mid-sequence, reload, state kept; same after break. 6. An opened door stays open after reload and rest. 7. Carving text contains no statement about the mine's makers. 8. Use works only on the three controls.

## Open questions
Whether puzzle XP exists and its amount (user/TBD). Penalty, if any (user). Persistence scope (user). Break noise, damage, cost or requirements (user/Systems). Storeroom contents and trap effects (Systems/Narrative). Unreal component: NOT RUN (Software Engineer).

STATUS: done
OUTPUTS: work/T001/puzzle/storeroom_puzzle_spec.md, storeroom_puzzle.json, puzzle_state_machine.py, test_puzzle.py, test_puzzle.log
ISSUES: (1) User: whether puzzle XP exists, amount, wrong-attempt penalty, persistence scope, break noise/cost. (2) World Designer: map e08 `solutions` must read exactly ["puzzle","magic_key","pick_lock","break"]; I did not check their parallel revision. (3) Unreal NOT RUN: Software Engineer needs Unreal.
