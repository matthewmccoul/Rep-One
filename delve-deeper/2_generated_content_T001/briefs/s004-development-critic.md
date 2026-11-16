# Brief: T001-s004-critic (Producer to development-critic)
Task ID: T001, step 4, box CR. Owner: development-critic. Evidence labels: STATIC/SIMULATED/EXECUTED, never upgraded.

## Inputs
- GDD: 08_Final_GDD_Extracted.txt (loaded in your session)
- Lore: work/lore.md
- New content files (from step 3, C1):
  - work/T001/map/floor1_layout.md
  - work/T001/map/floor1_map.json
  - work/T001/map/validate_map.py
  - work/T001/map/validate_map.log
  - work/T001/puzzle/storeroom_puzzle_spec.md
  - work/T001/puzzle/storeroom_puzzle.json
  - work/T001/puzzle/puzzle_state_machine.py
  - work/T001/puzzle/test_puzzle.py
  - work/T001/puzzle/test_puzzle.log
  - work/T001/quest/figure_ore_lift_quest.md
  - work/T001/quest/figure_ore_lift_strings.csv
  - work/T001/quest/figure_ore_lift_quest.json
  - work/T001/quest/validate_quest.py
  - work/T001/quest/validate_quest.log
- User request and decisions: pipeline_state/tasks/T001/request.md, pipeline_state/tasks/T001/decisions.md (read only)
- Shared defect directory: work/T001/defects/ (file BLOCKING and MAJOR findings there per 'Defects and checks')

## Output
- work/T001/critic/critic_report.md, in the format of your role file section 4. The last line must be `Verdict: CONSISTENT` or `Verdict: CONTRADICTIONS`.

## Acceptance criteria
Every new content file is checked against GDD and lore. Check that the map, puzzle and quest tie together by shared IDs. Check that the puzzle uses only supported interactions. Check that the Figure's reason stays hidden. Every quote matches its file and line. Report only; do not fix.

## Token allowance / stop limit
About 15,000 tokens. One report, no revision rounds. If you cannot finish, report what you checked and return blocked.
