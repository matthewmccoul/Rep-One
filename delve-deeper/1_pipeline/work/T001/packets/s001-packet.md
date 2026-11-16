Version: 1 | Task: T001 | Author: Producer | Status: PROPOSAL awaiting user decision H0 | Evidence: STATIC (inspection only)
Inputs read: GDD (required parts), request.md, decisions.md, history.md, 03_DEVELOPMENT_WORKFLOW.mmd, 02_GAME_DEVELOPMENT_PROCESS.md (startup section), pipeline_state/tasks/T001/step_briefs/s001-P0.md. work/catalogue.md does not exist (no earlier packages).

# Review packet: T001 starting brief (PROPOSAL, nothing here is approved)

## 1. What you asked
A reusable combat system for Unreal, for a dungeon-crawler RPG. It is built from the "mordor combat research" data. The system is borrowed from PLATO's Avatar, which you have confirmed is not proprietary. You want the system, not the content (creatures, spells, names).

## 2. Blocking finding
I could not find the "mordor combat research" in this project. A search of the folder for mordor, avatar, plato and combat file names found only your request text. Without it the Systems Designer would be guessing at the rules. **Decision needed: tell us where the research data is (or paste/add it to the project), before or together with approving.** I would not dispatch design work until it is present.

## 3. Recommended first task (PROPOSAL)
**T001-A: Combat system rules specification and data schema (design only, no Unreal code yet).**
- Owner: Systems Designer. Reviewers: Software Engineer (buildability), QA Tester (testability), UI/UX (only for combat feedback needs).
- Scope: extract from the research the combat rules in generic form: action timers, hit/miss, damage, armor, initiative/order, flee/negotiate hooks, spell-casting and resource use, status effects, death/recovery hooks. Each rule is cited back to the research. Content stays out: creatures, spells and items appear only as placeholder example rows. Unresolved or ambiguous items stay TBD.
- Fit with the GDD: real-time timer-driven combat, fixed positions in combat, one active adventurer, Fight/Flee/Negotiate, flee gives each enemy one free attack, daily AI assessment is not part of combat (GDD: combat stays in ordinary code). Where the research is turn-based or differs from the GDD, the spec lists it as an open decision for you. I do not resolve it.
- Outputs (proposed paths): `work/T001/combat_rules_spec.md`; `work/T001/data/combat_params.json` (and CSV tables) with counters, thresholds and formulas as data; `work/T001/open_questions.md`.
- Acceptance: every rule traces to a research source or is marked TBD; the data tables validate as JSON/CSV; a worked example of one combat round and one flee are included; QA lists test cases from it.
- Stop limit: 2 revision rounds, then a blocker report to you.

## 4. Proposed milestones (PROPOSAL, for you to change)
| ID | Milestone | Roles | Gate |
|---|---|---|---|
| M1 | Rules spec and data tables (T001-A) | Systems Designer; reviewers above | You approve design (D3) |
| M2 | Combat core as Unreal gameplay component(s): timers, actions, damage, targeting, data-driven loading. Needs: how Unreal work enters (C++, Blueprints or data tables) and how I reach Unreal. | Software Engineer | Your review |
| M3 | Headless/rules simulation tests, then in-engine test cases | QA Tester, Software Engineer | Evidence review |
| M4 | Combat feedback hooks (HUD text, log lines), if you want them | UI/UX, Software Engineer | Your review |
Each milestone returns to you before the next starts.

## 5. Budget (PROPOSAL)
Task total: **USD 8.00**, with a 25% reserve inside it. Rough split: M1 about 2.5 USD (spec, 2 review passes); M2 about 3 USD; M3 about 1.5 USD. M4 only if you ask for it (not funded here). Each dispatch gets its own `DD_MAX_BUDGET_USD` cap, at most 2.00. GDD token guide: about 40,000 tokens per engineering task and 12,000 per design task. This split is a Producer estimate, not a measured cost.

## 6. Map jobs
Not part of this task (no World Designer / map work).

## 7. Test evidence and known limits
- Evidence: STATIC only. Nothing was run or tested.
- NOT RUN: Unreal Engine (needs your machine and licence); Unreal access method is undecided, so I assume none. M2 and the in-engine part of M3 cannot start until you decide this.
- The GDD leaves combat timings, XP, and several formulas TBD; the spec will keep them as tunable data, not final values.
- Name/IP: you stated the system is not proprietary. Non-generic creature or spell names are excluded from this task, so I do not treat them as an open risk.

## 8. Decisions needed from you
1. Where is the mordor combat research? (blocking)
2. Approve T001-A as the first task, or revise it.
3. Approve or change the 8.00 USD task budget and the 2-round revision bound.
4. Unreal: C++, Blueprints or data tables, and how agents will reach the project (needed before M2).
5. Should M4 (combat feedback and UI) be in scope?
Choices at H0: "Revise recommendation" or "Approved brief".
