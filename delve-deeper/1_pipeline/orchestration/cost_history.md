## Measured cost history (earlier test runs of this crew; indications, not limits)
Sources: the run ledgers and handoffs from the Delve Deeper tests of 2026-11-04 to 2026-11-09. Different harness, one run each, so read these as rough guides. Costs are Claude Code's own estimates.

**Ten-agent crew, all Sonnet 5.5 (the Assignment 3 run): 37 agent runs, 11.90 USD, 743 turns.** Role, runs, cost, share of cost, average cost per run:
- QA Tester: 7 runs, 2.56 USD, 21.5%, 0.37 per run (24.2% of all turns)
- Producer: 4 runs, 2.41 USD, 20.3%, 0.60 per run
- Development Systems Analyst: 2 runs, 2.18 USD, 18.4%, 1.09 per run (55 and 68 turns)
- Software Engineer: 6 runs, 1.19 USD, 10.0%, 0.20 per run
- Systems Designer: 3 runs, 0.95 USD, 8.0%, 0.32 per run
- Narrative Designer: 5 runs, 0.81 USD, 6.8%, 0.16 per run
- UI/UX Designer: 3 runs, 0.64 USD, 5.4%, 0.21 per run
- World Designer: 4 runs, 0.62 USD, 5.2%, 0.15 per run
- AI Integration Engineer: 2 runs, 0.35 USD, 3.0%, 0.18 per run
- Asset Curator: 1 run, 0.19 USD, 1.6%
Tokens, 31 of those 37 runs (input = fresh + cache write + cache read): 8,900,666 input and 223,264 output in all. QA: 6 runs, 3,554,120 input (39.9%) and 76,282 output (34.2%), about 592,000 input and 12,700 output per QA run.

**Ten-agent crew, all Haiku 5.5: 28 runs, 1.92 USD, about 0.07 per run.** By role: Development Systems Analyst 0.93 USD (48%), Producer 0.34, Software Engineer 0.20, QA Tester 0.11 (5.7%).

**QA's share of total cost, ten-agent runs** (even share would be 10%): Sonnet 21.5%, GLM Flash 38.7%, DeepSeek 22.4%, Ling 16.6%, Haiku 5.7%.

**Four-agent crew (Producer, Systems Designer, Narrative Designer, QA)**, QA's share of cost (even share 25%): all Opus 5.5 38.3% (QA 35.1% of input tokens, 43.3% of output tokens); all Haiku 36.1%; Sonnet with an Opus QA 58.9%; Haiku with an Opus QA 94.5%. Assignment 4 pipeline (8 runs, 1.68 USD): QA 27.8% of cost, 24.2% of input tokens, 30.5% of output tokens.

**Patterns.** QA was the single largest-cost agent in 8 of the 11 runs where per-agent cost is known. In four-agent crews QA took 27% to 47% of cost, and 59% to 94% when it ran on Opus. In ten-agent crews it averaged about 21%. An average run cost about 0.07 USD on Haiku and about 0.32 USD on Sonnet. Haiku 5.5 charges 5 times more per token once a prompt passes 100,000 tokens (see the prices above). Analyst runs are long and expensive on Sonnet.

Live probes, 2026-11-11 (trivial briefs, so floors rather than typical costs): Haiku specialist 0.006 USD; Critic (Sonnet) 0.10; Analyst (Sonnet) 0.09; Haiku consultation 0.006; Producer (Sonnet) 0.09 to 0.13 for a four-turn brief.
