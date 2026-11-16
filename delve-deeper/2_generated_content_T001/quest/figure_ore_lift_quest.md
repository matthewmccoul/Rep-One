Version 3 (revision 2 of 2) | Task: T001-s008-narrative (v1 was s003, v2 s005) | Author: development-narrative-designer | Status: done, PROPOSAL (nothing approved by the user) | Evidence: STATIC design; validator EXECUTED (validate_quest.log, written by the script from a no-argument run)
Inputs read: GDD (CLAUDE.md import), brief s008-narrative-designer-r2.md, and the v2 files in work/T001/quest/. (Earlier inputs: s003 brief, request.md item 3, work/T001/map/floor1_layout.md and floor1_map.json.)

# Quest: "The Chain" (quest_figure_ore_lift)

**Giver:** Mysterious Figure at `sewer_grate`. **Ask:** is the old ore lift's chain still on the drum? His reason is never given.
**Route:** sewer_grate > town_entrance > entry_hall > collapsed_gallery (trap_pit, mon_type_2) > `ore_lift` (mon_type_3). Edges e01, e02, e04, e10, all plain doors. Needs no pick, search or breaking.
**Discovery (my choice): by arrival.** On first arrival at `ore_lift` while the quest is active, the game shows the lift text (`lift_arrival_present` / `lift_arrival_gone`) and writes a log line. No Search roll, so the quest cannot be blocked by a failed check. The chain state is an authored world flag; PROPOSAL default "present". Whether it can change in play is TBD (Systems/World). No Figure line states or implies why the chain is gone or who removed it; `fig_report_gone_cold` reads "The drum is bare. Gone. Hm."
**Completion (checkable):** state is observed_* (character arrived at `ore_lift` while active) AND then reports at `sewer_grate`. Either chain state completes it.
**Reward (GDD):** usually none ("There is still much work to be done."). Rarely a rare item; chance TBD (Systems), item picked by the game from authored rare items.

## Flow
1. Talk to Figure: offer (cold wording at stages -1..0, warm at 1..2). At hostile (-2) he makes no new offer: `fig_offer_refused` is the only offer line at -2. Player picks accept or decline (choices at -1..2, since the offer is only made there).
2. Go to `ore_lift`; arrival text and log line fix the observation.
3. Return and talk: variant by state: present / gone, not yet, nothing (long absence; threshold TBD, Systems), no offer (never asked or declined), already done. **Cold wording covers -2..0 once the quest has been offered; warm covers 1..2.** The refusal applies only to making a new offer, so a Figure who falls to hostile mid-quest still takes the report.
4. Reward roll by the game: `fig_reward_none` or `fig_reward_item`; filled into `{reward_line}`.

## Stage rule (CSV, JSON `stage_rule` and this file agree)
`stage_min` is -2 for: fig_accept_ack, fig_decline_reply, fig_report_not_yet, fig_report_nothing, fig_report_no_offer, fig_report_present_cold, fig_report_gone_cold, fig_reward_none, fig_reward_item, fig_repeat_done. Warm report lines stay 1..2; offer lines unchanged (cold -1..0, warm 1..2, refused -2).
Wording change: `fig_decline_reply` lost "I will ask again", which contradicts the refusal at -2 (now "*The eyes dim.* Then go."). No other wording changed; the other shared lines read acceptably for a hostile Figure.

## Voice notes
Terse; short sentences; stage directions in asterisks for eyes, green arms and pointing fingers. Never says why. Warm stages are only slightly less cold. Lore guard: lift is "the old miners'"; no word on who built anything beyond the surface premise. The banned-phrase list in the validator checks reason-giving and lore leaks (simple check only).

## Works without UI/AI changes
Uses the ordinary dialogue box and message log. Wording changes by the stage already computed from the daily score; no new assessor call. Hook (PROPOSAL): accept, observe and report may be listed in the Figure's daily action record; weights are not set here.

STATUS: done
OUTPUTS: work/T001/quest/figure_ore_lift_quest.md, figure_ore_lift_strings.csv, figure_ore_lift_quest.json, validate_quest.py, validate_quest.log
ISSUES: (1) Unreal quest component NOT RUN / not mine (Software Engineer, Unreal missing). (2) Rare-item chance, item list, "nothing" long-absence threshold TBD (Systems). Also Systems: `fig_reward_item` is now allowed at -2 because nothing restricts the reward roll by stage; whether a hostile Figure may give an item is a Systems call. (3) Whether chain state can change in play, e.g. gods placing things: unspecified (Systems/World). (4) Drain location: the GDD does not place the drain; it lists the Figure under "Watchers and Town Characters" and says only "sewer drain". The map (World Designer, a PROPOSAL) puts it beside the town entrance (Producer/World). (5) How the Figure learns of events is unspecified in the GDD; here the game simply records the quest flag, nothing invented about his knowledge. (6) Hostile refusal and cold/warm banding are my PROPOSAL. (7) The validator log is now written by the script itself (no-argument run, or `--log PATH`); a run on other arguments prints only.
