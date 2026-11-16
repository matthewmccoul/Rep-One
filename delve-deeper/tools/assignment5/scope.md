# Assignment 5 scope: Milestone M1, the first-floor ending

## Goal

Make the end of the Delve Deeper first-floor demo playable in the Unreal project: the player gets gold, meets the
bandit boss at the descent, pays the boss's toll, and goes down the descent, which ends the demo.

The map already defines `toll_hall`, `boss_room` and `descent` (`2_generated_content_T001/map/floor1_map.json`) and the
demo floor builds them as walkable slabs, but the demo has no boss, no gold and no descent behaviour.
The agent decides which gap to build first.

## Governing GDD sections

Source: `1_pipeline/08_Final_GDD_Extracted.txt`, the extracted text of `07_Final_GDD.pdf` ("Demo, Final Revision").
Line numbers below refer to that text file.

| Section | Lines | What it says |
|---|---|---|
| Win and Loss Conditions, "Win" | 44-45 | Reach and use the descent beyond the first floor, guarded by the faction's boss; this ends the demo. |
| Boss Encounter, "Boss and standing" | 385-387 | The boss starts at threatening and demands a toll to let the adventurer pass. |
| Boss Encounter, "Winning" | 390 | Going down the descent wins the demo, however the adventurer got past the boss. |
| Loot, Resources, and Town Services, "Currency" | 319 | Currency: Gold. |
| Player Feedback, "HUD and map" / "Message log" | 395-406 | The HUD and message log show the adventurer's state and outcomes. |

The toll amount is TBD in the GDD. The 100 gold used here is the Assignment 3 crew's `EXPERIMENTAL` proposal
(`inputs/crew_encounter/faction_stages.json`), so it is a demo value and not approved design.

## Done means

All four requirements below are `implemented` with verified evidence in the code, the project builds through its
normal Unreal build, and the behaviours below are seen when playing `DemoLevel`.

Observable acceptance, played from the start of `DemoLevel`:

1. Before the toll is paid, using the descent does not end the demo. The player is told the way is barred.
2. With less gold than the toll, asking the boss for passage shows his "cannot pay" line and takes no gold.
3. With enough gold, paying removes exactly the toll from the gold balance, shows his "toll paid" line, and the
   descent opens. The player can never be charged twice.
4. Using the open descent shows a visible win message and the demo is over.

## Not in this milestone (future work, kept visible)

Boss stages other than `threatening`; fight, flee, negotiate, gift and barter options; the daily relationship
assessments; shops and prices; combat; XP; saving. The crew's rules for them are in `inputs/crew_encounter/`.

## Requirements (machine readable)

`gdd_lines` are inclusive line ranges in the GDD text. `depends_on` lists requirement IDs that must be `implemented`
before this one is eligible to be built.

```json
{
  "milestone": "M1 first-floor ending",
  "requirements": [
    {
      "id": "DD-GOLD-01",
      "title": "The adventurer carries gold",
      "gdd_ref": "Loot, Resources, and Town Services: Currency: Gold (line 319); Player Feedback: HUD (lines 395-397)",
      "gdd_lines": [[319, 319], [395, 397]],
      "behavior": "The player has a gold balance that starts at 0 and is shown in the on-screen status text. A gold purse the player can pick up with the interact key sits in the bandit barracks and adds 100 gold (a demo fixture; the amount is the crew's EXPERIMENTAL toll value, not approved design). Picking it up twice gives nothing.",
      "depends_on": []
    },
    {
      "id": "DD-TOLL-01",
      "title": "The bandit boss waits in the boss room and demands his toll",
      "gdd_ref": "Boss Encounter: Boss and standing (lines 385-387)",
      "gdd_lines": [[385, 387]],
      "behavior": "An interactable bandit boss stands at node boss_room. Interacting with him at the starting stage (threatening) shows the string bandit_boss_stage_threatening_01 from inputs/crew_encounter/bandit_strings.csv with {adventurer_name} and {toll_amount} filled in, where the toll amount is toll_gold for the threatening stage in inputs/crew_encounter/faction_stages.json (100). The player sees how to pay.",
      "depends_on": []
    },
    {
      "id": "DD-TOLL-02",
      "title": "The player can pay the toll to open the way",
      "gdd_ref": "Boss Encounter: Boss and standing (lines 385-387); crew rule TOLL-03 in inputs/crew_encounter/player_options.csv",
      "gdd_lines": [[385, 390]],
      "behavior": "Paying with gold >= toll subtracts exactly the toll, shows bandit_boss_toll_paid_01, and records that the toll is paid for this run. Paying with less gold shows bandit_boss_cannot_pay_01, subtracts nothing and leaves the way barred. After the toll is paid the player is never charged again. Paying changes no faction favor.",
      "depends_on": ["DD-GOLD-01", "DD-TOLL-01"]
    },
    {
      "id": "DD-DESC-01",
      "title": "The descent is barred until the toll is paid, then going down wins the demo",
      "gdd_ref": "Win and Loss Conditions: Win (lines 44-45); Boss Encounter: Winning (line 390)",
      "gdd_lines": [[44, 45], [390, 390]],
      "behavior": "An interactable descent exists at node descent. Using it before the toll is paid shows a message that the boss bars the way and does not end the demo. Using it after the toll is paid shows a clear win message and ends the demo (input stops or the win banner stays up).",
      "depends_on": ["DD-TOLL-02"]
    }
  ]
}
```
