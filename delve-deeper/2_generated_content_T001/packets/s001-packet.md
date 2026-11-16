Version: 1 | Task: T001 | Author: development-producer | Status: done (awaiting user decision H0) | Evidence: STATIC (inspection only; nothing run)
Inputs read: GDD (all required parts), request.md, decisions.md, history.md (empty), 03_DEVELOPMENT_WORKFLOW.mmd, 02_GAME_DEVELOPMENT_PROCESS.md (Startup, roles), work/lore.md (empty), World Designer role file, project file list.

# Review packet H0: proposed starting brief for T001 (PROPOSAL until your own message approves it)

## 1. Project state found
- No catalogue (`work/catalogue.md` absent), no accepted lore (`work/lore.md` empty), no existing floor design, no Unreal project or specs in this folder. So there is no prior floor design to start from; the map starts from GDD facts only.
- GDD facts that bind the three pieces (source of truth over the storeroom/ore-lift proposals only where they conflict):
  - One town, one floor, boss-guarded descent; boss is the bandit leader (faction). Boss starts threatening, toll by standing.
  - Three monster types, three traps, four door types (plain, secret, locked, stuck), one puzzle, 3 spells/weapons/armor/consumables (magic keys exist).
  - Mysterious Figure: eyes behind a sewer drain, asks for tasks (slay, fetch, go to coordinates and report); never reveals why; usually no reward, rarely a rare item.
  - Puzzle: "one puzzle, to be designed by the development team". Supported interactions: search, doors, traps, inventory/equip, fight/flee/negotiate. GDD lists no generic "pull lever" interaction, so the controls must be framed as selecting an object (as in Search "Select an object") — see Decision 3.
  - Nothing in the GDD conflicts with a storeroom or an ore lift; both are consistent with the dwarven-mine premise. The GDD does not define them, so they are proposals, adopted unless the Critic finds a contradiction.
  - Lore guard: full-game secret is that dwarves were not the makers. The demo shows only the surface premise; the Figure's hidden reason and carving text must not reveal it.

## 2. Proposed scope (T001)
Three linked content packages, each with runnable code and data, tested outside Unreal.

| # | Package | Lead | Supporting | Key outputs |
|---|---|---|---|---|
| 1 | First-floor map | World Designer | Software Engineer (code side) | Markdown plan with labelled diagram; JSON node/edge list with stable IDs (entrance, rooms, connections and door types, encounter slots, 3 traps, puzzle room, ore lift site, boss room, descent); Python validator (reachability, boss/descent reachable, puzzle clue and controls reachable before storeroom opens); Unreal-side importer/builder code (C++ or Python editor script) |
| 2 | Storeroom puzzle | Systems Designer | World Designer (placement), Software Engineer | Rules spec: carving clue, three controls (pick, cart, furnace), solution dig→haul→smelt, wrong-attempt response, what opens; JSON data table; state-machine code with Python tests (all sequences, reset, save/load state); Unreal component |
| 3 | Mysterious Figure quest | Narrative Designer | Software Engineer (quest-state code); World Designer (destination ID) | Receive and report-back dialogue (CSV string table with game-filled blanks), quest JSON (giver, destination node ID, discovery, completion condition, outcomes), validator that every ID exists in the map JSON; Unreal quest-state code |

**Map job split (PROPOSAL, per role files):** World Designer hands the Software Engineer the layout spec and JSON placement data keyed by stable IDs plus what to validate; Software Engineer hands back importer, built/validated result and problems found. World Designer owns creative layout; Software Engineer owns code and checks, no design changes.

**Roles NOT planned:** UI/UX (only if you want puzzle controls or quest log presentation specified; flagged in Decision 3), AI Integration (no runtime assessment change), Asset Curator (no assets requested; placeholder IDs only).

## 3. Sequence and dependencies (PROPOSAL)
1. Map v0 (World Designer): rooms, graph, slots, reserving a storeroom node and an ore-lift node. Unblocks 2 and 3.
2. In parallel after 1: puzzle design (Systems Designer) and quest dialogue (Narrative Designer); World Designer fits the puzzle layout and quest destination into the map (map v1).
3. Software Engineer: importer, puzzle state machine, quest state, validators, Python tests.
4. Critic: one dispatch with every new content file, the GDD and `work/lore.md`; reports only. CONTRADICTIONS routes back to creators.
5. QA Tester: test cases and results (CSV), defect reports; run validators.
6. You review the package; accepted work goes to the catalogue and lore.
Each completed piece returns to you for review before it becomes a basis for further work.

## 4. Budget and bounds (PROPOSAL)
- Task budget: **12.00 USD** total (per-run cap stays 2.00). Estimate ~10-12 runs: World ×3 (map, fit, fix), Systems ×2, Narrative ×2, Software ×3, Critic ×1, QA ×1, reserve ≈ 25%. This is an estimate, not measured.
- Token guide (GDD Token Budget is provisional): design/narrative/world ~12k tokens per bounded task; engineering ~40k; QA/producer ~15k. Larger runs must be justified in the result.
- Revision bound: at most **2 revision rounds per package** (including Critic-driven ones). At the bound, stop, preserve work, report a blocker to you.
- Deadline: course curriculum date (not given here); no calendar date invented.

## 5. Acceptance criteria (PROPOSAL)
- Map: every GDD floor item present (entrance from town, rooms and connections with door types, encounter slots for 3 monster types and the bandits, 3 traps, puzzle room, boss room guarding descent); validator passes (EXECUTED when run); JSON IDs stable.
- Puzzle: clue and all three controls reachable without opening the storeroom; only the correct order opens it; wrong order has a defined, non-softlocking response; test run covers all 3! orderings plus repeat/reset; uses only GDD-supported interactions.
- Quest: both dialogues complete with blanks filled from state; Figure never states his reason; destination and discovery map to real map IDs; completion condition is checkable in code; reward follows GDD ("usually none, rarely rare item").
- All code: Python tests EXECUTED with log paths; Unreal code labelled **NOT RUN - Unreal Engine not available here**, STATIC review only.
- Critic verdict CONSISTENT before assembly; QA results in CSV.

## 6. Known limits
- Nothing was run or tested in this box (STATIC only). Unreal Engine, project and access method are not available/undecided; Unreal code cannot be verified here.
- Numbers (toll, XP, timings, spawn rates, puzzle wrong-attempt penalty size) remain TBD where the GDD leaves them; designers may propose values labelled PROPOSAL.
- No prior floor design exists, so the map has no starting point beyond the GDD.

## 7. Decisions needed from you
1. Approve or revise scope, role split and sequence above.
2. Approve the 12.00 USD task budget and 2-revision-round bound, or give others.
3. Interaction wording: the GDD names no "pull/turn control" action. Proposal: treat the three controls as selectable objects, "use" on a selected object, like Search's object selection; Systems Designer would flag it as a rule needing your approval. Alternatively add UI/UX to specify it. Which?
4. Unreal delivery form (C++, Blueprints, data tables, or editor Python): GDD leaves it to the dev team. Proposal: data-driven JSON/CSV plus thin C++ loaders, Python for validation. Approve, or choose.
5. Who records approvals remains undecided; this pipeline records your replies.

Choices offered: "Revise recommendation" or "Approved brief".

STATUS: done
OUTPUTS: work/T001/packets/s001-packet.md, work/T001/step_results/s001-P0.json
ISSUES: Decisions 1-5 (user). No Unreal access (user). No existing floor design (info).
