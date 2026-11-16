Producer: Decision needed (task T001, after step 2): "You approve the starting brief?"

What the last box did: Revised the starting brief after the user's rejection. Scope now covers the full workflow to finished, testable Unreal combat code: research digest, rules spec and data, minimal feedback spec, implementation, QA, acceptance, with a gate before each milestone. The team, not the user, chooses the code method. Budget raised to 20 USD with prices and token assumptions shown. Research on Google Drive cannot be read by any agent (no Drive tool); it was not found in the project and nothing was guessed. Unreal is NOT RUN. Evidence is STATIC.

Proposed budget for this task: 20.00 USD. Choosing "Approved brief" approves it. Spent so far: 0.35 USD.
Review packet: work/T001/packets/s002-packet.md

----- review packet -----
Version: 2 (revision of s001) | Task: T001 | Author: Producer | Status: PROPOSAL awaiting user decision H0 | Evidence: STATIC (inspection only)
Inputs read: GDD (required parts), request.md, decisions.md (both of the user's revision replies), history.md, s001 packet, 03_DEVELOPMENT_WORKFLOW.mmd, step brief s002-P0. work/catalogue.md does not exist.

# Review packet: T001 starting brief, revised (PROPOSAL; nothing here is approved)

## What changed since the first version, in response to your reply
- The first version stopped at a design document. This version carries the whole workflow: research, design, approval, implementation, QA, acceptance. The goal is functional combat code for Unreal, delivered by the agent team.
- The budget is raised from 8 to 20 USD.
- I no longer hand you technical choices. The team decides the code method (the GDD says the development team decides how work enters Unreal). You see only results and approval gates.
- Minimal combat feedback (a plain text log and a few readouts) is in scope, as you said.
- I'm sorry for the question about C++ versus Blueprints. That was the team's job, and it was wrong to put it on you.

## Goal (PROPOSAL)
Functional, content-free combat code for a first-person dungeon crawler, built for Unreal. The rules come from the mordor combat research, in the PLATO Avatar style you described. It follows the GDD: real-time, timer-driven actions; fixed positions while combat lasts; one active adventurer; Fight, Flee and Negotiate hooks; combat in ordinary game code with no LLM. Creatures, spells and items exist only as placeholder data rows.

## The research data (a fact the pipeline must handle)
You said the research is in your Google Drive. The brief says no agent is granted a Drive tool, so I cannot read it and neither can any role. I did not find it anywhere in the project folder. I did not guess its contents. Nothing in this plan assumes I know the rules. The Systems Designer's first job is to work only from the research once it is in the project folder. How the file gets there is a pipeline matter (see ISSUES). It is not a design question for you.

## Plan: milestones (PROPOSAL; each returns to you before the next starts)
| ID | Milestone | Owner, reviewers | Hands on | Your gate |
|---|---|---|---|---|
| M1 | Research digest: every rule in the research, listed generically with source citations; conflicts with the GDD listed | Systems Designer; Critic checks against GDD | to M2 | none (routine) |
| M2 | Combat rules spec and data tables (JSON/CSV): timers, hit/miss, damage, armor, order, flee, negotiate hooks, spells and resources, status, death hooks. Unresolved items stay TBD. | Systems Designer; reviewers Software Engineer (buildable), QA (testable) | spec and tables to M3, M4, M5 | Design approval (D3) |
| M3 | Minimal combat feedback spec: text log, target and condition readouts, debug keys | UI/UX Designer | to M4 | review with M2 |
| M4 | Implementation: data-driven combat components and tests, in the form the Software Engineer judges best (the Engineer records the choice and reason). Delivered as source plus a rules simulation that runs outside Unreal. | Software Engineer | build to M5 | none until M5 |
| M5 | QA: test cases from the spec; executed against the simulation; in-Unreal cases written as a checklist for your machine | QA Tester; defects routed by cause | results to you | Feature accepted? |
| M6 | Accepted package recorded in the catalogue; process review | Producer; Development Systems Analyst | next task | your acceptance |
AI Integration, World, Narrative and Asset roles are not used (no content, no assessors, no maps). Map job split: not applicable.

## Budget (PROPOSAL): 20.00 USD for the whole task
Prices used (from the step brief, Anthropic pricing fetched 2026-11-11): Haiku 5.5 input 0.10, output 0.50 per million tokens; Sonnet 5.5 input 2.00, output 10.00 per million.
Assumptions are my estimates, not measured: a Haiku dispatch uses about 300k input tokens (tool context and re-reads) and 40k output, which is about 0.05 USD; I round up to 0.15 USD for retries. A Sonnet dispatch uses about 200k input and 20k output, which is about 0.60 USD. Cache discounts are ignored (conservative). The GDD guideline of 12,000 to 40,000 tokens per task is far below real session use, so I did not rely on it.
| Item | Runs | Unit USD | Total |
|---|---|---|---|
| Haiku roles (Systems, UI/UX, Software; including revisions) | about 40 | 0.15 | 6.00 |
| Sonnet roles (QA, Critic, Analyst) | about 14 | 0.60 | 8.40 |
| Producer coordination and packets (Sonnet) | about 4 | 0.60 | 2.40 |
| Subtotal | | | 16.80 |
| Reserve for failures and rework | | | 3.20 |
| Total | | | 20.00 |
Every dispatch carries `DD_MAX_BUDGET_USD` (at most 2.00) and a task-ID label. This estimate is revised after M2 using actual cost records.

## Revision bound (PROPOSAL)
Each milestone gets 3 revision rounds. At the bound the team stops, preserves its work and reports a concrete blocker to you. The Systems Designer and the Analyst may propose a different bound from test evidence.

## Acceptance for the first deliverable, M2 (PROPOSAL)
Every rule cites the research or is marked TBD. The data tables parse. The spec has a worked combat round and a worked flee. QA has derived test cases from it. Departures from the GDD are listed, not resolved by the team.

## Test evidence and known limits
- Evidence: STATIC only. Nothing was run.
- NOT RUN: Unreal Engine (needs your machine; no agent has an Unreal tool). Code is therefore verified by the outside-Unreal simulation (SIMULATED or EXECUTED, labelled as such by QA), and in-engine compile and play checks are done on your machine from a checklist. Until then "runs in Unreal" cannot be claimed.
- NOT RUN: Google Drive (no agent has the tool). The research is unread.
- The research may be turn-based while the GDD is real-time; the spec lists any conflict for your decision and does not settle it.

## Decision needed
Choose "Approved brief" or "Revise recommendation". If you revise, say in your own words what to change.

STATUS: done

----- end of review packet -----

Reply with the exact words of your choice (any capitals). You can add notes.

- Revise recommendation
- Approved brief
- End this task here (not an arrow in the diagram: ends this task)
