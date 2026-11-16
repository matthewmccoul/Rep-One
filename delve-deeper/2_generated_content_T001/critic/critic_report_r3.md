Read check: work/T001/map/floor1_layout.md (99 lines); work/T001/map/floor1_map.json (63); work/T001/map/validate_map.py (128); work/T001/map/validate_map.log (1); work/T001/puzzle/storeroom_puzzle_spec.md (66); work/T001/puzzle/storeroom_puzzle.json (48); work/T001/puzzle/puzzle_state_machine.py (96); work/T001/puzzle/test_puzzle.py (233); work/T001/puzzle/test_puzzle.log (26); work/T001/quest/figure_ore_lift_quest.md (30); work/T001/quest/figure_ore_lift_strings.csv (26); work/T001/quest/figure_ore_lift_quest.json (62); work/T001/quest/validate_quest.py (143); work/T001/quest/validate_quest.log (5); pipeline_state/tasks/T001/request.md (18); work/T001/producer_notes.md (57); work/lore.md (3); work/T001/critic/critic_report_r2.md (70, reference only); work/T001/defects/ listed (DEF-T001-1 to DEF-T001-4, reference only, not edited); 08_Final_GDD_Extracted.txt (read in full via the CLAUDE.md import).

# Critic report (round 3, task T001-s018-critic)

Evidence: STATIC inspection. I did not run the validators or tests; the three logs were read as written. Nothing was run inside Unreal.

Lore: work/lore.md holds only "Accepted content only. Empty until the user accepts a package. The GDD is separate." (work/lore.md:3). There is no accepted lore, so no lore contradiction is possible and I invented none.

## Findings

### 1. MINOR: a declined quest is answered with a line that says the Figure never asked
- Content: "I asked nothing of you. Yet." (work/T001/quest/figure_ore_lift_strings.csv:11); the JSON maps the declined state to it: "\"declined\": \"fig_report_no_offer\"," (work/T001/quest/figure_ore_lift_quest.json:32).
- Conflicts with the content's own offer flow: "Player picks accept or decline" (work/T001/quest/figure_ore_lift_quest.md:13). A player who declined was asked, so the line is false for that state.
- Not a GDD or lore contradiction; the GDD sets no such rule. Severity MINOR.
- Context checked: work/T001/quest/figure_ore_lift_quest.md:15 groups "no offer (never asked or declined)" together on purpose, so this is a wording issue, not a state-machine error. The GDD (08_Final_GDD_Extracted.txt:355-358) places no constraint on this.

### 2. MINOR: the "magic" option for locked doors is carried only as a consumable key
- Content: "\"solutions\": [\"puzzle\", \"magic_key\", \"pick_lock\", \"break\"]" (work/T001/map/floor1_map.json:51), while the layout cites the rule as "GDD: locked doors can be picked, magicked open, or broken" (work/T001/map/floor1_layout.md:69).
- GDD: "Locked doors must be picked, unlocked with magic, or broken." (08_Final_GDD_Extracted.txt:292). The GDD treats the key as separate from unlocking with magic: "characters who cannot pick locks or unlock them with magic can still make their way through the game." (08_Final_GDD_Extracted.txt:329).
- Why: e08 has no spell-based unlock entry, while the secret door e09 lists "magic" (work/T001/map/floor1_map.json:52). The same slot may simply not need it. I am flagging it only because the layout wording says "magicked open". The round-1 defect is otherwise fixed.
- Mentions checked: GDD lines 292, 293, 328 and 329 (all touch magic and locks); the map and puzzle files use "magic_key" throughout. None of them settles whether a spell unlock is meant for e08. Spells are only counted in the GDD ("at least three spells") and are not listed.
- Context checked: work/T001/map/floor1_layout.md:63-69 marks the puzzle route PROPOSAL; the GDD gives no spell list.

### 3. MINOR: stale or mismatched labels inside the map and quest files
- Map layout column header "Ways through without lockpicking" (work/T001/map/floor1_layout.md:64) while its e08 row lists "pick_lock" (work/T001/map/floor1_layout.md:69).
- Task id mismatch: the map JSON says "\"task\": \"T001-s005-world\"," (work/T001/map/floor1_map.json:3) and the layout says "Task: T001-s016-world" (work/T001/map/floor1_layout.md:1). The quest JSON says "\"task\": \"T001-s003-narrative\"," (work/T001/quest/figure_ore_lift_quest.json:3) and the quest note says "Task: T001-s008-narrative" (work/T001/quest/figure_ore_lift_quest.md:1).
- Stale issue: "work/T001/request.md missing; brief cites it" (work/T001/map/floor1_layout.md:99). The request is at pipeline_state/tasks/T001/request.md (it exists, 18 lines).
- Not a GDD matter. The earlier round-2 label items (parentheses, "map v0", "Thief per map", drain "in town", log origin) are gone from the current files, which I re-read.

## Required checks and what they found

- **GDD unknowns settled as decided.** None found.
  - Puzzle XP: "Amount **TBD** (XP table TBD in GDD)." (work/T001/puzzle/storeroom_puzzle_spec.md:52).
  - Penalty: "**Penalty: none (PROPOSAL).**" (work/T001/puzzle/storeroom_puzzle_spec.md:31).
  - Lockpicking class: "which class or skill may pick locks is not stated in the GDD and is not decided here, TBD" (work/T001/puzzle/storeroom_puzzle_spec.md:43). The Thief claim from round 2 is gone.
  - Toll, spawn rates and trap effects: "Spawn rates, toll and trap effects are TBD." (work/T001/map/floor1_layout.md:59).
  - Puzzle design: "Puzzle: One puzzle, to be designed by the development team." (08_Final_GDD_Extracted.txt:295 area; the line is in the Encounter Choices list).
  - Rare reward chance: "chance TBD (Systems)" (work/T001/quest/figure_ore_lift_quest.md:10).
  - The hostile-Figure rule is marked PROPOSAL: "Hostile refusal and cold/warm banding are my PROPOSAL." (work/T001/quest/figure_ore_lift_quest.md:30).
- **Forbidden reveals (the Figure).** None found.
  - GDD: "never reveals what these tasks are for" (08_Final_GDD_Extracted.txt:357-358).
  - Offer lines ask only: "Is the chain still on the drum?" (work/T001/quest/figure_ore_lift_strings.csv:2); the gone line gives no cause: "The drum is bare. Gone. Hm." (work/T001/quest/figure_ore_lift_strings.csv:14).
  - The carving says nothing about makers: "It says nothing about who built the mine." (work/T001/puzzle/storeroom_puzzle_spec.md:19).
  - Reward matches the GDD: "There is still much work to be done." (work/T001/quest/figure_ore_lift_strings.csv:16).
  - The hostile mid-quest change holds: "Not you. Not today." (work/T001/quest/figure_ore_lift_strings.csv:4) is the only line at -2 for a new offer, and the other lines work at -2 with stage_min of -2. Wording stays terse and in character; nothing invented about his knowledge.
- **Names, numbers, stages, speakers.**
  - Counts match the GDD: "Three monster types; three traps; four door types (plain, secret, locked, and stuck); one puzzle" (08_Final_GDD_Extracted.txt:56) against "PASS: 17 nodes, 15 edges, 3 monster types, 3 traps, 1 puzzle, 4 door types, plain-only route to descent exists" (work/T001/map/validate_map.log:1). I counted trap_dart, trap_pit and trap_store in the JSON myself.
  - Stages: the scale in the quest JSON is "-2 hostile, -1 threatening, 0 neutral, 1 friendly, 2 allied (GDD stages)" (work/T001/quest/figure_ore_lift_quest.json:41), matching the GDD's five stages.
  - Boss: toll demanded at boss_room, descent only through boss_room (work/T001/map/floor1_layout.md:77), matching "The bandit boss guards the descent and starts at threatening" (08_Final_GDD_Extracted.txt:385).
  - Node names have no parentheses; "Old ore lift, chain and drum" (work/T001/map/floor1_map.json:33) and the validator enforces it (work/T001/map/validate_map.py:44).
  - Quest log lines now read cleanly with the node name: "You told the figure what you saw at the {node_name}." (work/T001/quest/figure_ore_lift_strings.csv:24).
- **Tone and voice.** The Figure is terse and uses eyes, arms and fingers, matching "lanky green arms and pointed fingers" (08_Final_GDD_Extracted.txt:355-356).
- **"Approved" or "final" claims on proposals.** None found. Every file labels content PROPOSAL; the only approval claimed is Use on the three controls: "Use on the three controls approved by the user; everything else PROPOSAL (nothing else approved)" (work/T001/puzzle/storeroom_puzzle_spec.md:1). It is backed by the Producer's note: "Use on a selected object for the three puzzle controls: approved (your words: \"Approve Use\")" (work/T001/producer_notes.md:36). I cannot verify the user's words independently of the notes.
- **Request fit** (pipeline_state/tasks/T001/request.md). Clue and controls are reachable with e08 closed (validator at work/T001/map/validate_map.py:87-90; I traced foreman_office via e05 and smelter_hall via e06 and e07). The Figure asks from the grate about the chain, and the destination is `ore_lift` in the map's `quest_destinations`. The code gap (Unreal not run) is stated in each file and by the Producer; it is not a contradiction.

## Round 2 and earlier defects
- DEF-T001-1 and DEF-T001-2 remain fixed, as in round 2. DEF-T001-3 and DEF-T001-4 are QA/Software Engineer defects on code (not content) and are outside this check.

## Defects filed
None. There are no BLOCKING or MAJOR findings.

Verdict: CONSISTENT
