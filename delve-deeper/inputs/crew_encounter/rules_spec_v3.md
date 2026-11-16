# Rules Spec v3: Boss Toll Encounter
Version 3 (supersedes v2; v2 unedited) | Task ID: wf-bandit-toll-01 (label wf-40-sd-v3) | Author: development-systems-designer | Status: PROPOSAL (nothing user-approved; values accepted by the TEST-SIMULATED D3 fixture or the RESOLUTION ROUND coordinator decisions are EXPERIMENTAL) | Evidence: STATIC (reasoning; arithmetic by hand, no run).
Inputs read: rules_spec_v2.md, data/player_options.csv, data/faction_stages.json, decision_register.md, qa/defects DEF-002, DEF-003, DEF-004 (DEF-009 Finding A taken from the brief, file not opened), state/approvals_TEST_SIMULATED.md, GDD in context.
Data tables: `data/faction_stages.json`, `data/player_options.csv` (rule_ids key its rows).

## Changes from v2
1. GIFT-05 new: gift score ceiling 79 (tunable 70-79). Allied only via negotiation, quests or daily assessment (DEF-002).
2. Hostile route back reworked (DEF-003): FLEE-04 `hostile_unlock` needs N=3 whole days after the flee (tunable 2-5); boss never talks while hostile (FIGHT-05 new); NEG-06 no longer unlockable; GIFT-04 now delivered only via a befriended individual bandit outside the boss room. Open Q 4 of v2 closed.
3. XP-01 new (DEF-004): stage-change XP once per character and group at highest stage reached; amount a PROPOSAL placeholder.
4. FIGHT-04 removed: merged into FIGHT-02 (id retired, not reused; option_id O04 left empty).
5. Toll lock, visit flag, assessment clamp, `hostile_unlock` marked EXPERIMENTAL (coordinator, not user).
6. `{shortfall}` and the full placeholder set stated (s9).
7. Speaker channels `system`/`ui` are the Software Engineer's job (resolution item 1); not a rule here.

## 0. Fixed GDD facts
Stage bands hostile 0-19, threatening 20-39, neutral 40-59, friendly 60-79, allied 80-100; boss belongs to faction; starts threatening; toll lower at neutral/friendly; allied passes; hostile attacks on sight; bandits present at an attack become hostile; each stage change grants XP once per group; descent wins; LLM invents no mechanics.

## 1. Conventions
- Score is an integer 0-100 clamped after every change; stage derived from bands only.
- `favor_delta` empty = no change; `0` = explicit no change. For gift rows it is a maximum, not fixed.
- `score_cap`: absolute ceiling applied immediately; only FIGHT-02 uses it (19). The gift ceiling (GIFT-05) is a different thing and is not in that column.
- `toll_gold` null (hostile) = no toll offered; 0 (allied) = free passage. Importers treat them differently.
- CSV: UTF-8, comma-separated, no embedded commas, lists use `;`. Conditions are plain-text boolean expressions (`AND`, `NOT`, `<`, `>=`, `=`).
- State names: `visit_flag`, `toll_locked`, `stage_at_dialogue_open`, `toll_paid_this_visit`, `gold`, `barter_value`, `negotiate_attempts_today`, `attempt_limit`, `gift_score_gained_today`, `daily_gift_cap`, `last_exit_reason`, `flee_day`, `day_count`, `hostile_unlock`; new in v3: `gift_score_ceiling` (79), `hostile_unlock_days` (3), `highest_stage_xp_awarded` (per character and group), `stage_xp_placeholder` (10, PROPOSAL).
- Stable `rule_id`s (FIGHT-, FLEE-, NEG-, TOLL-, GIFT-, XP-); never renumber; new rules append. v3 adds FIGHT-05, FLEE-04, GIFT-05, XP-01 (action values stay within the existing set: fight, flee, negotiate, gift).

## 2. Visit and toll lock (EXPERIMENTAL, confirmed by coordinator, not user)
`visit_flag`=1 on entering the boss room; 0 on leaving the room (incl. flee) or when a day passes. On opening toll dialogue, `stage_at_dialogue_open` and `toll_locked` are stored until the flag clears; score changes still apply but toll and non-attack behaviour follow the lock; the stage change takes effect next visit. Exception: the player's own attack (FIGHT-02) is immediate. Paying sets `toll_paid_this_visit=1`; no second charge. Allied locks toll 0.
Hostile (FIGHT-05, GDD): entering the room at stage hostile starts combat; no toll dialogue, no boss dialogue, no lock stored.
Tolls (EXPERIMENTAL): threatening 100 (tunable 60-200), neutral 50, friendly 20, allied 0, hostile null; non-increasing.

## 3. Favor deltas and daily order
Immediate deltas (EXPERIMENTAL): attack caps score at 19 (FIGHT-02); later kill -5 (FIGHT-03, 0 to -10); flee 0; toll paid 0; negotiate success +10 (5-15), failure -5 or -15; gifts per s5.
Order (EXPERIMENTAL, D3 decision 2): immediate deltas update the stored score at once; at the next daily update that stored score is the prior sent to the faction assessor. Game code applies `new = prior + clamp(returned - prior, -C, +C)`, then clamps 0-100. Assessment never sets a toll, stage rule or mechanic.
Clamp (EXPERIMENTAL scaling, confirmed; formula PROPOSAL): `C = 25 + floor(15*(min(span,30)-1)/29)`, span = days advanced (rest = 1). 25 at spans 1-2, 40 at 30+, never above 40; base tunable 15-40. Example: span 1, prior 20, returned 90 gives 45.
A negotiation failure dropping the stage to hostile mid-visit: lock holds, boss changes at next visit unless the player attacked (PROPOSAL; human to confirm).

## 4. Negotiation formula (EXPERIMENTAL, values unchanged from v2)
`skill = 0.4*CHA + 0.2*WIS + 0.2*INT + level`; `p = clamp(0.50 + slope*(skill-12) + favor_term*favor_stage + gift_bonus + god_mod + pref_mod, 0.05, 0.95)`; slope 0.03 (0.02-0.05); favor_term 0.05 (0.03-0.08); `gift_bonus` 0.01 per 10 gold gifted while `visit_flag=1`, max +0.15; god_mod 0 (TBD); `pref_mod` +0.10 with a gift offered, -0.10 if the adventurer attacked bandits in the last 3 days. Attempts: `attempt_limit` 1 per in-game day (1-3). The boss cannot be negotiated with at hostile at all (NEG-06).
Test: p in 0.05-0.95; monotone in each input; 1000 seeded trials within 5 points of p.

## 5. Cannot pay, gifts
Cannot pay (EXPERIMENTAL type; row values PROPOSAL): TOLL-04 refuse, no score change; TOLL-05 barter at 50% of shop sell value toward `{shortfall}` = toll_locked - gold; TOLL-06 inadequate offer is a failed request, no penalty. No automatic hostility or debt. Descent never permanently blocked.
Gifts (EXPERIMENTAL): GIFT-02 delta = min(15, floor(value/10)) per gift (cap 10-20); GIFT-03 at most 15 per day (10-25).
GIFT-05 (EXPERIMENTAL, resolution item 3): applied delta = max(0, min(delta, gift_score_ceiling - score)), ceiling 79 (tunable 70-79). A gift never lowers the score nor lifts it above the ceiling; a score already at or above it (e.g. 85 by assessment) gets delta 0. The portion of delta cut by the ceiling does not count toward GIFT-03. Arithmetic: from 20, gifts alone stop at 79 (friendly, toll 20); the DEF-002 four-day allied route is closed. Allied remains reachable by NEG-02 successes, quests (values TBD) and the daily assessment.

## 6. Flee, hostile route back, shared stage
Flee (EXPERIMENTAL): free attacks per GDD; fails only if unable to act; on success the adventurer ends on the door tile outside, no pursuit, `visit_flag` clears, `last_exit_reason=flee`, `flee_day=day_count` (FLEE-03).
FIGHT-04 merged into FIGHT-02: the single faction-wide cap at 19 is how "bandits present at an attack become hostile" (GDD) is done. Consequence: there is no visit-only hostility and no allied boss with hostile bandits; boss and all bandits always follow the faction stage. Hostility now lasts until the score rises.
Hostile route back (EXPERIMENTAL; condition names PROPOSAL; DEF-003): the hostile boss attacks on sight, so he is never spoken to and NEG-06 stays unavailable. FLEE-04: `hostile_unlock=1` once `day_count >= flee_day + hostile_unlock_days` (N=3 whole days, tunable 2-5) while stage is hostile and `last_exit_reason=flee`; cleared when the stage is no longer hostile. It only enables GIFT-04: gifts given to a befriended individual bandit outside the boss room, accepted at half value (delta = floor(value/20), subject to GIFT-02, GIFT-03, GIFT-05). The other route is the daily assessment (GDD: befriending individual bandits and the leader's favor feed it). Arithmetic: 19 to 20 needs a 20-gold-or-more gift after day flee_day+3, so at least 3 days; 19 to 79 needs 1200 gold at half value, at least 4 days of gifts (15/day). How an individual bandit is befriended, and where such bandits stand, is TBD (GDD gap; not invented). Exits other than flee (e.g. death) do not set the unlock; only assessment helps then. Flagged.
How bandits learn of events beyond the daily record: OPEN, not specified.

## 7. Stage-change XP (XP-01, EXPERIMENTAL rule; amount PROPOSAL placeholder)
GDD: raising a group's stage grants XP, once per stage change per group. Record `highest_stage_xp_awarded` per character and per group (faction/boss and each monster group), initial value = the group's starting stage (faction: threatening). When a stage rises above the record, for each step above it pay `stage_xp_placeholder` once and set the record to the new stage (any cause: negotiation, gift, assessment). Falls never lower the record, so attack, flee, gift back up, attack again pays nothing twice. XP amount: placeholder 10 only, not a balance value; real XP TBD in balancing (GDD). Per-character because relationships are per character (GDD). Hostile never pays (lowest stage).

## 8. Test criteria for QA
Bands: each score maps to one stage; boundaries 19/20, 39/40, 59/60, 79/80. Clamps at 0 and 100; FIGHT-02 min(score,19) at 19, 20, 100. Tolls non-increasing, allied 0, hostile null. Toll paid once per visit; constant across mid-visit stage change. Hostile entry: combat, no dialogue (FIGHT-05). Flee: door tile, visit_flag cleared, flee_day set. hostile_unlock 0 at flee_day+2, 1 at flee_day+3 (N=3); changes with N 2 and 5. Gift: from 70 a 150-gold gift gives +9 not +15; at 79 or 85 gives 0; no run of gifts alone reaches 80 (tunable ceiling 70 caps at 70). GIFT-04 floor(value/20), refused unless unlocked and via a bandit. XP-01: stage sequence threatening, hostile, threatening pays 0; threatening to neutral pays once, neutral, threatening, neutral pays 0 more; a different character pays again. Assessment delta never exceeds C(span). Gift gain per day <=15. With gold=toll-1 the player still has an option. Stat-range criterion deferred.

## 9. Allowed placeholders (Narrative strings)
Full set: `{adventurer_name}`, `{toll_amount}`, `{item}`, `{quest_target}`, `{shortfall}`. `{shortfall}` is the exact derived gold (toll_locked - gold), allowed (resolution item 2). Nothing else.

## Open questions (route via Producer)
1. How bandits learn of events beyond the daily record (human; GDD gap).
2. How an individual bandit is befriended and where they are placed (human; World Designer) - GIFT-04 depends on it.
3. Mid-visit drop to hostile by negotiation failure: confirm effect at next visit (human).
4. Unlock when hostility arises with no flee (human).
5. Item values, shop prices, stat ranges, real XP (Systems Designer, later).

STATUS: done
OUTPUTS: wf_test/design/rules_spec_v3.md; wf_test/design/data/player_options.csv; wf_test/design/data/faction_stages.json; wf_test/design/decision_register.md
ISSUES: see Open questions 1-5.
