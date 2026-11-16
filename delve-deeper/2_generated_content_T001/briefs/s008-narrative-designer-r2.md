# s008-narrative-designer-r2: quest revision 2 (owner: development-narrative-designer)
Task ID T001-s008-narrative. Written by the Producer (the chat session). This is revision 2 of at most 2 for the quest package: the last one. Anything further would go to the user as a blocker, so do these items completely.

## Why
Sources: the critic's round-2 report (work/T001/critic/critic_report_r2.md, finding 2 and finding 3) and QA's spec issues (work/T001/qa/check_spec.md, ISSUES 2 and 3 at its end and check QST-51). Do not edit those files, nor the defects, nor any map/ or puzzle/ file.

## What to change
1. **A hostile Figure mid-quest still talks (Producer decision).** The player can accept the quest at stage -1 or higher, and the Figure's standing can fall to hostile (-2) at a daily update before they report. Today no accept, decline or report line covers -2. In `figure_ore_lift_strings.csv`, set `stage_min` to -2 for the lines that can occur after the quest is offered: `fig_accept_ack`, `fig_decline_reply`, `fig_report_not_yet`, `fig_report_nothing`, `fig_report_no_offer`, `fig_report_present_cold`, `fig_report_gone_cold`, `fig_reward_none`, `fig_repeat_done`, and `fig_reward_item` if it can be given at that stage. Keep `fig_offer_refused` as the only offer line at -2. The cold wording may carry hostile tone at -2; if a line reads wrongly for a hostile Figure, adjust its wording but keep his voice (short, strange, patient, never says why). Update `stage_rule` in `figure_ore_lift_quest.json` and the matching text in `figure_ore_lift_quest.md`: cold variants cover -2..0 once the quest has been offered; the refusal applies only to making a new offer.
2. **Fix a wrong claim.** `figure_ore_lift_quest.md` (and any ISSUES text it carries) says the GDD puts the sewer drain "in town". The GDD lists the Figure under "Watchers and Town Characters" and says only "sewer drain". Replace it with: the GDD does not place the drain; the map (World Designer, a PROPOSAL) puts it beside the town entrance.
3. **Stale log wording.** The quest .md still says the validation log was hand-written. It is now written by the script. Make the file say so.
4. **The validator must not overwrite the real log by accident.** In `validate_quest.py`: write `validate_quest.log` only when run with no arguments (the real run) or with `--log PATH`; a run on any other file prints its result and writes no log. Keep it writing its own log from inside the script (no shell redirects, nothing typed by hand). Run `python3 work/T001/quest/validate_quest.py` last, with no arguments, so the real log shows the real run.
5. Nothing else changes: no key, blank or ID is renamed; no numbers invented; reward chance stays TBD.

## Outputs (write only these)
work/T001/quest/figure_ore_lift_strings.csv, work/T001/quest/figure_ore_lift_quest.json, work/T001/quest/figure_ore_lift_quest.md, work/T001/quest/validate_quest.py, work/T001/quest/validate_quest.log. Version line: "Version 3 (revision 2 of 2)".

## Acceptance (pass/fail)
- `validate_quest.py` exits 0; the log was written by the script from a no-argument run.
- Every line listed in item 1 has `stage_min` -2; `fig_offer_refused` is the only offer line at -2; the JSON `stage_rule` and the .md agree with the CSV.
- No sentence says the GDD puts the drain in town.
- Only these changes differ from version 2.

## Stop limit
One pass. End with STATUS, OUTPUTS, ISSUES. Label evidence STATIC or EXECUTED (with the log path). Unreal is not available.
