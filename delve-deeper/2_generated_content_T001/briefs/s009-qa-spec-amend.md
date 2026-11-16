# s009-qa-spec-amend: amend the check spec for quest revision 2 (owner: development-qa-tester)
Task ID T001-s009-qa. Written by the Producer (the chat session). You amend YOUR OWN spec, `work/T001/qa/check_spec.md`, and nothing else. You still write no scripts.

## What changed since your spec (read the files, do not rely on this summary)
1. Quest revision 2 (work/T001/quest/figure_ore_lift_strings.csv, figure_ore_lift_quest.json, figure_ore_lift_quest.md, validate_quest.py): a hostile Figure (-2) now has lines mid-quest. These keys have `stage_min` -2: `fig_accept_ack`, `fig_decline_reply`, `fig_report_not_yet`, `fig_report_nothing`, `fig_report_no_offer`, `fig_report_present_cold`, `fig_report_gone_cold`, `fig_reward_none`, `fig_repeat_done`, `fig_reward_item`. `fig_offer_refused` is still the only offer line at -2; a NEW offer at -2 is refused, and accept is not possible then (QST-23 stays). The JSON `stage_rule` says so.
2. Producer decisions on your ISSUES list:
   - ISSUE 2 (hostile Figure mid-quest): decided as in item 1. The quest data now covers it; QST-51 must no longer expect a missing line at -2 for those keys.
   - ISSUE 3 (validators overwrite their logs): `validate_quest.py` now writes its log only when run with no arguments or with `--log PATH`, and takes a quest FOLDER, not a file. `validate_map.py` still always writes its log (World Designer will change it in a later round); keep your "broken copies first, real run last" convention for the map validator only.
   - ISSUE 4: the review dispatch will name `work/T001/qa/fixtures/`, `work/T001/qa/out/` and `work/T001/qa/build_mut/` as writable.
   - ISSUES 5, 6: the stale "map v0" label is a cleanup item; defect status lines are yours to update after verification.
   - A hostile Figure may hand out an item (`fig_reward_item` at -2): the GDD ties rewards to nothing about standing; chance stays TBD.
3. The step-1 build (common + map) is being written now by the Software Engineer from your spec as written. Do not renumber or rename any COM or MAP line.

## What to do
- Amend only the lines that the quest changes affect: QST-51 and any other QST or EQV line that expected a missing line at -2, plus Section 0 and Section 7 text that mentions the quest validator's logging.
- Add the checks the new behaviour needs: lines at -2 exist for each key listed in item 1; a mid-quest drop to hostile (accept at -1 or 0, then the standing falls to -2, then a report) still yields a defined line and completes; an offer at -2 is still refused; each with a broken-input counterpart (a CSV copy where one of those keys has `stage_min` -1 must be rejected by the validation of coverage, if the spec has such a coverage rule; otherwise say so).
- To model a standing change mid-quest, add one event to the `quest_check run` contract: `stage=<n>` sets the current standing (-2..2) before the next event. State this as a PROPOSAL for the Software Engineer in Section 0.
- Put a short "Amendments in version 2" list at the top of the spec naming every line added or changed (ids only plus one phrase each). Set the version line to 2.
- Do not weaken any existing broken-input check.

## Acceptance
Every line still in the fixed five-field form; the amendments list matches your actual edits; no COM or MAP line changed; no other file edited.

## Stop limit
One pass. End with STATUS, OUTPUTS, ISSUES. Evidence: STATIC.
