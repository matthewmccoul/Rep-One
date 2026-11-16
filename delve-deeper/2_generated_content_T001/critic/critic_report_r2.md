Read check: work/T001/map/floor1_layout.md (98 lines); work/T001/map/floor1_map.json (63); work/T001/map/validate_map.py (109); work/T001/map/validate_map.log (1); work/T001/puzzle/storeroom_puzzle_spec.md (66); work/T001/puzzle/storeroom_puzzle.json (48); work/T001/puzzle/puzzle_state_machine.py (96); work/T001/puzzle/test_puzzle.py (233); work/T001/puzzle/test_puzzle.log (26); work/T001/quest/figure_ore_lift_quest.md (26); work/T001/quest/figure_ore_lift_strings.csv (26); work/T001/quest/figure_ore_lift_quest.json (62); work/T001/quest/validate_quest.py (129); work/T001/quest/validate_quest.log (4); pipeline_state/tasks/T001/request.md (18); work/T001/producer_notes.md (36); work/lore.md (3); work/T001/defects/DEF-T001-1.md (12); work/T001/defects/DEF-T001-2.md (11); 08_Final_GDD_Extracted.txt (read in full via CLAUDE.md import). Round 1 report work/T001/critic/critic_report.md was listed for reference only; I did not rely on it.

# Critic report (round 2, task T001-s006-critic)

Evidence: STATIC inspection of all files. I did not run the validators or tests. The three logs were read as written (validate_map.log, test_puzzle.log, validate_quest.log); I did not re-run them. Unreal: nothing was run inside Unreal.

Lore: work/lore.md holds only "Accepted content only. Empty until the user accepts a package. The GDD is separate." (work/lore.md:3). There is no accepted lore, so no lore contradiction is possible; I invented none.

## Findings

### 1. MINOR: the puzzle spec attributes a lockpicking rule to the map that the map does not contain
- Content: "`pick_lock` (lockpicking; Thief per map)" (work/T001/puzzle/storeroom_puzzle_spec.md:43)
- The map's e08 wording gives no class: "pick the lock" (work/T001/map/floor1_layout.md:68). The GDD sets class lockpicking requirements nowhere; it says only that "characters who cannot pick locks" can use a key.
- Why: "Thief per map" is a claim about another file that I cannot find there. It could be read as settling an unknown (who can pick locks). The GDD leaves class requirements TBD: "specific requirements are TBD in testing".
- Mentions checked: the Thief/lockpick search over the map, puzzle and quest files found "Thief" only at work/T001/puzzle/storeroom_puzzle_spec.md:43. The other lockpicking mentions are work/T001/map/floor1_layout.md:63, work/T001/map/floor1_layout.md:75 and work/T001/map/validate_map.py:72, and none names a class.
- Context checked: line 43 sits under "Routes through e08", which is headed PROPOSAL. Severity stays MINOR.

### 2. MINOR: the quest file misstates what the GDD says about the drain
- Content: "the GDD says a drain in town" (work/T001/quest/figure_ore_lift_quest.md:26)
- GDD: "A pair of glowing eyes behind the bars of a sewer drain" (08_Final_GDD_Extracted.txt:355). The GDD does not say "in town".
- The map file states it correctly: "The GDD lists the Figure among town characters and does not say where the drain is" (work/T001/map/floor1_layout.md:98).
- Why: the two files disagree about what the GDD says. The Producer's decision to keep the grate beside the entrance is listed in work/T001/producer_notes.md and is not re-flagged. Only the quest file's description of the GDD is wrong.
- Context checked: work/T001/map/floor1_layout.md:43 and work/T001/map/floor1_layout.md:98 both mark the placement a PROPOSAL.

### 3. MINOR: stale or conflicting statements about how the logs were produced
- Content: "Validator log was written by hand from printed output" (work/T001/quest/figure_ore_lift_quest.md:26)
- The script writes its own log: `log = open(os.path.join(here, "validate_quest.log"), "w")` (work/T001/quest/validate_quest.py:100)
- Producer note: "Known: the puzzle and quest test logs were typed in by hand (redirects were blocked), not captured by a tool." (work/T001/producer_notes.md:17)
- Why: the evidence-label claims disagree across files, and I cannot tell from inspection which is current. Not a GDD matter. QA or the Producer should re-run the checks and capture the logs.

### 4. MINOR: stale status labels in the map files
- Content: "status": "PROPOSAL map v0 (not approved by user)" (work/T001/map/floor1_map.json:4)
- The layout names this revision map v1: "# Floor 1 layout, map v1 (revision 1)" (work/T001/map/floor1_layout.md:4)
- Why: a label mismatch only. Both say PROPOSAL and nothing is claimed as approved.

## Required checks and what they found

- **GDD unknowns settled as decided.** None found.
  - Puzzle XP is stated as TBD: "Amount **TBD** (XP table TBD in GDD)." (work/T001/puzzle/storeroom_puzzle_spec.md:52)
  - The penalty is marked PROPOSAL: "**Penalty: none (PROPOSAL).**" (work/T001/puzzle/storeroom_puzzle_spec.md:31)
  - The GDD leaves the puzzle open: "Puzzle: One puzzle, to be designed by the development team." (08_Final_GDD_Extracted.txt:295)
  - The toll amounts, trap effects and encounter rates are left TBD in work/T001/map/floor1_layout.md:58.
- **Forbidden reveals (the Figure).** None found.
  - The GDD says the Figure "never reveals what these tasks are for" (08_Final_GDD_Extracted.txt:357-358).
  - The Figure's offer asks only: "Is the chain still on the drum?" (work/T001/quest/figure_ore_lift_strings.csv:2).
  - The reward line matches the GDD ("Usually he gives no reward, saying there is still much work to be done", 08_Final_GDD_Extracted.txt:358): "There is still much work to be done." (work/T001/quest/figure_ore_lift_strings.csv:16).
  - The round-1 line that asserted the chain was "taken" is gone. The gone lines now give no cause. The validator also scans for this wording, but I did not re-run it.
- **Names, numbers, stages, speakers.**
  - The map matches the counts: "Three monster types; three traps; four door types (plain, secret, locked, and stuck); one puzzle" (08_Final_GDD_Extracted.txt:56). The validator log reads "PASS: 17 nodes, 15 edges, 3 monster types, 3 traps, 1 puzzle, 4 door types, plain-only route to descent exists" (work/T001/map/validate_map.log:1).
  - I counted three traps in the JSON myself: trap_dart, trap_pit, trap_store.
  - Stages are the GDD scale of -2 to 2 and the speakers are the Figure, the player, narration and the message log.
  - The boss demands the toll at boss_room, and the descent is reachable only through boss_room (work/T001/map/floor1_layout.md:76). This agrees with the GDD boss section.
- **Tone and voice.** The Figure's lines are terse and use stage directions with eyes, arms and fingers. This fits the GDD description "lanky green arms and pointed fingers".
- **"Approved" or "final" claims on proposals.** None found. Every file marks its content PROPOSAL; the only approval claimed is Use on the three controls.
- **Request fit** (pipeline_state/tasks/T001/request.md):
  - Clue and controls are reachable with e08 closed: the validator checks this, and I traced it in the JSON. The clue is in foreman_office, reached via mess_hall e05. The controls are in smelter_hall, reached via e06 and e07.
  - The Figure asks from the grate about the ore lift chain, and the destination is ore_lift in the map's `quest_destinations`.
  - Use: the user's approval is recorded as "the new player action "Use" on a selected object is approved for the puzzle controls" (work/T001/producer_notes.md:7). The spec quotes it too: The user replied "Approve Use" (work/T001/puzzle/storeroom_puzzle_spec.md:9). I cannot verify the user's words independently of those notes.
  - Gap, not filed: the request wants "functional code that can be run inside Unreal Engine". The delivered code is Python reference logic and validators, with Unreal not run. The Producer's notes list the Software Engineer's C++ as not yet done, so this is a pending step and not a contradiction.

## Round 1 defects

- DEF-T001-1 (Use not in the GDD): its question is answered by the user's recorded "Approve Use", scoped to the three controls. The spec says Use is approved for the three controls only: "no other object accepts Use" (work/T001/puzzle/storeroom_puzzle_spec.md:9). The GDD action list itself still does not list Use. By the Producer's decision this is no longer open; QA updates the status line.
- DEF-T001-2 (e08 had no break): fixed. e08 now lists `"solutions": ["puzzle", "magic_key", "pick_lock", "break"]` (work/T001/map/floor1_map.json:51), against "Locked doors must be picked, unlocked with magic, or broken." (08_Final_GDD_Extracted.txt:292). All three GDD options are present. "puzzle" is an added route marked PROPOSAL and does not remove any GDD option, so I treat it as fixed.
- The round 1 minor items are addressed in these files: the chain "taken" wording, the toll slot (now at boss_room), and puzzle XP (TBD). The grate placement is a Producer decision.

## Defects filed
None. There are no BLOCKING or MAJOR findings, and I filed no files in work/T001/defects/.

Verdict: CONSISTENT
