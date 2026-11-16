#!/usr/bin/env python3
"""Free test of run_pipeline.py and the guard in orchestration/invoke_agent.sh. Standard library only.

Usage: python3 tests/test_pipeline.py

It copies this folder to a temporary folder and puts tests/fake_claude/claude (a stand-in for Claude Code that
spends nothing) first on PATH. The stand-in plays the Producer from a plan, so the test checks what the PROGRAM
enforces: the diagram's arrows, the user's questions, the roles allowed in each box, the budget, and tampering.
It does not test what real agents write. Exit 0 only if every check passes.
"""
import json, os, pathlib, shutil, subprocess, sys, tempfile

HERE = pathlib.Path(__file__).resolve().parent
SRC = HERE.parent
fails, checks = [], 0


def check(name, cond, detail=""):
    global checks
    checks += 1
    print(("PASS  " if cond else "FAIL  ") + name + ("" if cond else f"   <- {detail}"))
    if not cond:
        fails.append(name)


tmp = pathlib.Path(tempfile.mkdtemp(prefix="pipeline_test_"))
T = tmp / "copy"
shutil.copytree(SRC, T, ignore=shutil.ignore_patterns("pipeline_state", "work", "tests", "__pycache__", "tests_logs"))
(T / "agent_design" / "tests").mkdir(parents=True, exist_ok=True)
plan_f, log_f = tmp / "plan.json", tmp / "dispatch_log.txt"
(tmp / "fixtures").mkdir(exist_ok=True)
ENV = dict(os.environ, PATH=str(HERE / "fake_claude") + os.pathsep + os.environ["PATH"],
           FAKE_PLAN=str(plan_f), FAKE_LOG=str(log_f),
           PIPELINE_PRICE_URL_ANTHROPIC=(tmp / "fixtures" / "anthropic.md").as_uri(),
           PIPELINE_PRICE_URL_VERCEL=(tmp / "fixtures" / "vercel.json").as_uri())
ENV = {k: v for k, v in ENV.items() if not k.startswith("DD_")}


FIX = tmp / "fixtures"
(FIX / "anthropic.md").write_text("""| Model | Base input tokens | 5m cache writes | 1h cache writes | Cache hits and refreshes | Output tokens |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Claude Sonnet 5.5 | $2 / MTok | $2.50 / MTok | $4 / MTok | $0.10 / MTok<sup>2</sup> | $10 / MTok |
| Claude Haiku 5.5 (for prompts up to 100,000 tokens) | $0.10 / MTok | $0.125 / MTok | $0.20 / MTok | $0.01 / MTok | $0.50 / MTok |
| Claude Haiku 5.5 (for prompts over 100,000 tokens) | $0.50 / MTok | $0.625 / MTok | $1 / MTok | $0.05 / MTok | $2.50 / MTok |
""")
(FIX / "vercel.json").write_text(json.dumps({"data": [
    {"id": "anthropic/claude-sonnet-5.5", "pricing": {"input": "0.000002", "output": "0.00001", "input_cache_read": "0.0000001", "input_cache_write": "0.0000025"}},
    # the haiku input price here is deliberately different (0.20 instead of 0.10) so a test can tell which source was used
    {"id": "anthropic/claude-haiku-5.5", "pricing": {"input": "0.0000002", "output": "0.0000005", "input_cache_read": "0.00000001", "input_cache_write": "0.000000125"}}]}))
GOOD_A, GOOD_V = (FIX / "anthropic.md").as_uri(), (FIX / "vercel.json").as_uri()
BAD = (FIX / "does_not_exist").as_uri()


def prices(a, v):
    code = "import sys,json; sys.argv=['x']; import run_pipeline as rp; print('\\n'.join(rp.price_lines(json.load(open('pipeline.json')))))"
    r = subprocess.run([sys.executable, "-B", "-c", code], cwd=T, capture_output=True, text=True,
                       env={**ENV, "PIPELINE_PRICE_URL_ANTHROPIC": a, "PIPELINE_PRICE_URL_VERCEL": v})
    return r.stdout + r.stderr


def plan(d):
    plan_f.write_text(json.dumps(d))


def run():
    r = subprocess.run([sys.executable, "run_pipeline.py"], cwd=T, env=ENV, capture_output=True, text=True)
    return r.stdout + r.stderr, (T / "pipeline_state" / "message_for_user.md").read_text()


def reply(text):
    (T / "pipeline_state").mkdir(exist_ok=True)
    (T / "pipeline_state" / "reply.txt").write_text(text)
    return run()


def state():
    return json.loads((T / "pipeline_state" / "state.json").read_text())


def dispatch_log():
    return log_f.read_text() if log_f.exists() else ""


plan({})
setup_ok = subprocess.run([sys.executable, "run_pipeline.py", "--check"], cwd=T, env=ENV, capture_output=True, text=True)
check("the diagram and pipeline.json match (run_pipeline.py --check)", setup_ok.returncode == 0, setup_ok.stdout + setup_ok.stderr)
roles_made = sorted(f.name for f in (T / "agent_design" / "development_candidates").glob("*.md"))
check("all eleven role files exist after unpacking, including the critic shipped as its own file",
      len(roles_made) == 11 and "development-critic.md" in roles_made and "development-producer.md" in roles_made, roles_made)

_, msg = run()
nothing_ran = (not (T / "agent_design" / "tests" / "run_ledger.csv").exists()
               and not any(f for f in (T / "work").rglob("*") if f.name != "lore.md"))
check("the first thing it does is ask the opening question, and nothing runs",
      msg.strip() == json.loads((SRC / "pipeline.json").read_text())["opening_question"] and nothing_ran, msg)

_, msg = reply("I want to generate 3 monsters")
check("after the request it stops at the starting-brief decision, offering exactly the diagram's arrows plus 'end'",
      'You approve the starting brief?' in msg and "- Revise recommendation" in msg and "- Approved brief" in msg and "- End this task here" in msg, msg)
brief1 = (T / "pipeline_state/tasks/T001/step_briefs/s001-P0.md").read_text()
check("the Producer's brief tells it which model every agent runs on",
      "development-software-engineer: haiku" in brief1 and "development-qa-tester: sonnet" in brief1 and "development-producer: sonnet" in brief1, brief1[-700:])
check("the Producer's brief carries prices fetched from the Anthropic source, with both of Haiku's price tiers",
      "Source: Anthropic pricing page" in brief1 and "haiku (Claude Haiku 5.5): input 0.10" in brief1 and "For prompts over 100,000 tokens: input 0.50" in brief1
      and "sonnet (Claude Sonnet 5.5): input 2.00; output 10.00" in brief1, brief1[-1500:])
out = prices(BAD, GOOD_V)
check("if Anthropic fails the prices come from Vercel, and the brief says Anthropic failed",
      "Source: Vercel AI Gateway" in out and "Anthropic pricing page (" in out and "haiku (Claude Haiku 5.5): input 0.20" in out and "FAILED" not in out, out)
out = prices(BAD, BAD)
i_say, i_label = out.find("First, say plainly that the price check FAILED"), out.find('label every one exactly "PAST REMEMBERED PRICE (not verified)"')
check("if both sources fail the brief says FAILED, names both failures, and has the Producer say it has no prices BEFORE giving labelled remembered ones",
      "## Model prices: FAILED" in out and out.count("URLError") == 2 and "NO current price data" in out and 0 <= i_say < i_label
      and "Do not present the budget as built on current prices" in out, out)
check("the request is stored word for word",
      "I want to generate 3 monsters" in (T / "pipeline_state/tasks/T001/request.md").read_text())

_, msg = reply("sure, go ahead")
check("a reply that names no choice is not taken as approval", "I need a reply that contains the exact words" in msg and state()["task"]["position"] == "H0", msg)
_, msg = reply("Approved brief, or maybe Revise recommendation")
check("a reply naming two different choices picks neither", "I need a reply that contains the exact words" in msg and state()["task"]["position"] == "H0", msg)

plan({"P": {"next": "Rules unresolved"},
      "D1": {"dispatch": ["development-systems-designer", "development-narrative-designer"]},
      "D2": {"dispatch": ["development-narrative-designer", "development-qa-tester"]}})
_, msg = reply("I think the answer is: APPROVED BRIEF, looks good")
log = dispatch_log()
check("approving runs P, D1 and D2 in the diagram's order, then stops at 'You approve design or experiment?'",
      "You approve design or experiment?" in msg and [h["box"] for h in state()["task"]["history"]] == ["P0", "P", "D1", "D2"], msg)
check("a role that belongs in D2 but not D1 was refused in D1",
      "D1 dispatch development-narrative-designer -> exit 6" in log, log)
check("the roles listed for a box were allowed in it",
      "D1 dispatch development-systems-designer -> exit 0" in log and "D2 dispatch development-narrative-designer -> exit 0" in log, log)
check("each agent is started on its own model: haiku for the Narrative Designer, sonnet for QA",
      "ran development-narrative-designer with model haiku" in log and "ran development-qa-tester with model sonnet" in log, log)
check("the budget the user approved is the one in force", state()["task"]["budget_usd"] == 3.0)

(T / "x.md").write_text("brief")
r = subprocess.run(["orchestration/invoke_agent.sh", "development-qa-tester", "x.md", "T001-s999-x", "--staged"],
                   cwd=T, env=ENV, capture_output=True, text=True)
check("starting an agent by hand, outside run_pipeline.py, is refused", r.returncode == 6 and "not started by run_pipeline.py" in r.stderr, r.stderr)

plan({"I1": {"tamper": True, "dispatch": ["development-software-engineer"]}})
_, msg = reply("Approved specification")
check("a box that edits pipeline_state/ is stopped and the user is told",
      "did not finish" in msg and "pipeline_state/state.json" in msg, msg)

plan({"I1": {}})
_, msg = reply("Run this box again")
check("a box with specialists that dispatches none of them is stopped (the Producer may not do their work itself)",
      "did not finish" in msg and "no specialist run was recorded" in msg, msg)

GOOD = '"FINAL GDD" (08_Final_GDD_Extracted.txt:1)'
BAD = '"this sentence is not in the GDD" (08_Final_GDD_Extracted.txt:1)'
CR_BAD = {"dispatch": ["development-critic"], "next": "Consistent with GDD and lore", "report": f"Read check: x\n{BAD}\nVerdict: CONSISTENT\n"}
CR_OK = {"dispatch": ["development-critic"], "next": "Consistent with GDD and lore", "report": f"Read check: x\n{GOOD}\nVerdict: CONSISTENT\n"}
CR_NO = {"dispatch": ["development-critic"], "next": "Contradictions found", "report": f"Read check: x\n{GOOD}\nVerdict: CONTRADICTIONS\n"}
base = {"I1": {"dispatch": ["development-software-engineer"]}, "I2": {"dispatch": ["development-qa-tester"]},
        "C1": {"dispatch": ["development-world-designer"]}, "C2": {"dispatch": ["development-qa-tester"]},
        "K": {"next": "Not an arrow"}}
plan({**base, "CR": CR_BAD})
_, msg = reply("Run this box again")
check("the diagram's feature question 'Feature accepted?' reaches the user", "Feature accepted?" in msg, msg)
_, msg = reply("Accepted capabilities and formats")
check("new content goes through the critic box before assembly, and a critic quote that does not match its file stops the box",
      "did not finish" in msg and "quotes did not verify" in msg and state()["task"]["position"] == "CR", msg)
plan({**base, "CR": CR_OK})
_, msg = reply("Run this box again")
check("with verified quotes and a CONSISTENT verdict the content goes on to assembly and then to the user",
      "You accept content?" in msg and "CR" in [h["box"] for h in state()["task"]["history"]], msg)
plan({**base, "CR": CR_NO})
_, msg = reply("Content revision")
check("when the critic finds contradictions the content goes back to C1, and after the loop limit the user is asked",
      "has already run 2 times" in msg and "Critic report:" in msg, msg)
_, msg = reply("Consistent with GDD and lore")
check("the user can send it on to assembly despite the contradictions, and it reaches 'You accept content?'",
      "You accept content?" in msg, msg)
_, msg = reply("Accepted package")
check("an arrow the Producer invents is rejected", "did not finish" in msg and "Not an arrow" in msg, msg)

plan({"K": {"next": "New goal or changed boundaries"}})
_, msg = reply("Run this box again")
check("'New goal or changed boundaries' ends the task and asks the opening question again",
      msg.strip().endswith("Producer: How can I help with development?") and state()["task"]["status"].startswith("completed"), msg)

plan({"P0": {"budget_usd": 0.15, "cost": 0.1}, "P": {"next": "Rules unresolved", "cost": 0.1}})
reply("research existing RPG systems")
_, msg = reply("Approved brief")
check("a task whose spending reaches its approved budget stops before the next box",
      "approved 0.15 USD budget" in msg and "nothing more will run" in msg, msg)
_, msg = reply("Raise the budget")
check("raising the budget needs a new total above what is spent", "above the" in msg, msg)
_, msg = reply("Raise the budget to 20 dollars")
check("raising the budget takes the new total from the user's own words", state()["task"]["budget_usd"] == 20.0, msg)


# ======================================================================= sandbox scenarios (each runs in its own copy of the folder)
import time as _time

class Box:
    def __init__(self, name, extra_env=None):
        self.T = tmp / name
        shutil.copytree(SRC, self.T, ignore=shutil.ignore_patterns("pipeline_state", "work", "tests", "__pycache__", "tests_logs"))
        (self.T / "agent_design" / "tests").mkdir(parents=True, exist_ok=True)
        self.plan_f, self.log_f = tmp / f"{name}_plan.json", tmp / f"{name}_log.txt"
        env = dict(os.environ, PATH=str(HERE / "fake_claude") + os.pathsep + os.environ["PATH"], FAKE_PLAN=str(self.plan_f), FAKE_LOG=str(self.log_f),
                   PIPELINE_PRICE_URL_ANTHROPIC=GOOD_A, PIPELINE_PRICE_URL_VERCEL=GOOD_V)
        self.env = {k: v for k, v in env.items() if not k.startswith("DD_")}
        self.env.update(extra_env or {})
        self.plan({})
    def plan(self, d): self.plan_f.write_text(json.dumps(d))
    def run(self):
        r = subprocess.run([sys.executable, "run_pipeline.py"], cwd=self.T, env=self.env, capture_output=True, text=True)
        return (r.stdout + r.stderr) if not (self.T / "pipeline_state" / "message_for_user.md").exists() else \
            (self.T / "pipeline_state" / "message_for_user.md").read_text()
    def reply(self, text):
        (self.T / "pipeline_state").mkdir(exist_ok=True); (self.T / "pipeline_state" / "reply.txt").write_text(text); return self.run()
    def state(self): return json.loads((self.T / "pipeline_state" / "state.json").read_text())
    def log(self): return self.log_f.read_text() if self.log_f.exists() else ""
    def read(self, rel): return (self.T / rel).read_text()

def at_d3(name, p_plan, d1, d2):
    """Request -> approved brief -> P -> D1 -> D2 -> stops at the user's D3 decision."""
    b = Box(name); b.plan({"P0": p_plan} if p_plan else {}); b.run(); b.reply("a test request"); b.reply("Approved brief")
    return b

# ---- packet checks
b = Box("packets"); b.plan({"P0": {"packet_text": "# Packet\nonly this\n"}}); b.run(); m = b.reply("a test request")
check("a packet with no evidence, decision, budget or roster section stops the box and says what is missing",
      "did not finish" in m and "no 'Test evidence and known limits' section" in m and "no 'Decisions needed' section" in m
      and "no 'Budget' section" in m and "no 'Roster' section" in m, m)
good_roster = "## Roster\n" + "".join(f"- {r}: TASKED\n" for r in ["Systems Designer", "World Designer", "Narrative Designer", "UI/UX Designer", "Software Engineer", "AI Integration Engineer", "Asset Curator", "QA Tester", "Critic", "Development Systems Analyst"])
b.plan({"P0": {"packet_text": "# P\n## Test evidence and known limits\nx\n## Decisions needed\nsee ISSUES\n## Budget\nQA, Critic and Analyst: 3 runs, 1.00 USD\nHaiku roles: 4 runs\n" + good_roster}})
m = b.reply("Run this box again")
check("a budget with QA lumped in with other roles, no prices or tokens, no Haiku price tiers, and 'see ISSUES' with no ISSUES section is refused",
      "did not finish" in m and "does not state the prices and the token counts" in m and "no row of its own for QA" in m
      and "neither of its price tiers" in m and "'see ISSUES' but has no ISSUES section" in m, m)

# ---- roster rule and consultations
b = Box("roster"); b.plan({"P0": {"roster_lines": {"World Designer": "BENCHED (no job seen)"}}}); b.run(); m = b.reply("a test request")
check("benching an agent that was never consulted is refused", "did not finish" in m and "benches World Designer but it was never consulted" in m, m)
b.plan({"P0": {"consult": ["development-world-designer"], "roster_lines": {"World Designer": "BENCHED (consulted, agrees)"}}})
m = b.reply("Run this box again")
roster_md = b.read("work/T001/roster.md") if (b.T / "work/T001/roster.md").exists() else ""
check("an agent consulted first and then benched is allowed, runs read-only with a small cap, and the roster record sorts it into 'consulted, then benched'",
      "You approve the starting brief?" in m and "ran development-world-designer with model haiku as a consultation with tools [Read Glob Grep] cap 0.30" in b.log()
      and "## In question (consulted), then benched\n- World Designer" in roster_md and "- Systems Designer" in roster_md.split("## In question (consulted), then tasked")[0], roster_md + b.log())
b = Box("roster2"); b.plan({"P0": {"consult": ["development-qa-tester"], "roster_lines": {"QA Tester": "CONSULTED, then TASKED (it saw a job)"}}}); b.run(); m = b.reply("a test request")
rm = b.read("work/T001/roster.md") if (b.T / "work/T001/roster.md").exists() else ""
check("an agent consulted and then tasked is recorded as 'consulted, then tasked'", "## In question (consulted), then tasked\n- QA Tester" in rm, rm)
b.plan({"P0": {"roster_lines": {"Critic": "CONSULTED (replied)"}}}); b.reply("Revise recommendation")
m = (b.T / "pipeline_state" / "message_for_user.md").read_text()
check("saying an agent was CONSULTED when no consultation run exists is refused", "did not finish" in m and "says Critic was CONSULTED but no consultation run is recorded" in m, m)
b.plan({"P0": {"packet_text": "# P\n## Test evidence and known limits\nx\n## Decisions needed\nx\n## Budget\nprices per million tokens, tokens assumed\n| QA Tester | 1 | 1 |\n"}})
m = b.reply("Run this box again")
check("a packet that leaves out the roster is refused: no agent may simply be dropped", "no 'Roster' section" in m, m)

# ---- guard: consultation and flag modes
def guard(env_extra, agent, label):
    bp = tmp / "guard_brief.md"; bp.write_text("brief")
    gp = tmp / "guard_plan.json"; gp.write_text("{}")
    env = {k: v for k, v in dict(os.environ, PATH=str(HERE / "fake_claude") + os.pathsep + os.environ["PATH"], FAKE_PLAN=str(gp), FAKE_LOG=str(tmp / "guard_log.txt")).items() if not k.startswith("DD_")}
    env.update({"DD_ALLOWED_AGENTS": "", "DD_LABEL_PREFIX": "T9-", **env_extra})
    r = subprocess.run(["orchestration/invoke_agent.sh", agent, str(bp), label, "--staged"], cwd=T, env=env, capture_output=True, text=True)
    return r.returncode
WD, AN = "development-world-designer", "development-development-systems-analyst"
check("consultation mode: allowed only for an agent on the consult list and a label with -consult-",
      guard({"DD_CONSULT_AGENTS": WD}, WD, "T9-consult-wd") == 0 and guard({"DD_CONSULT_AGENTS": WD}, WD, "T9-wd") == 6
      and guard({"DD_CONSULT_AGENTS": WD}, "development-qa-tester", "T9-consult-qa") == 6 and guard({}, WD, "T9-consult-wd") == 6)
check("flag mode: allowed only for the Analyst with -flag- in the label, and only while a flag is open",
      guard({"DD_FLAG_AGENTS": AN}, AN, "T9-flag-1") == 0 and guard({"DD_FLAG_AGENTS": AN}, AN, "T9-analysis") == 6
      and guard({"DD_FLAG_AGENTS": AN}, WD, "T9-flag-1") == 6 and guard({}, AN, "T9-flag-1") == 6)

# ---- cost flags
FLAG_PLAN = {"P": {"next": "Rules unresolved"}, "D1": {"dispatch": ["development-systems-designer"]},
             "D2": {"dispatch": ["development-qa-tester"], "flag_assess": [{"label": "T001-s003-systems-designer", "text": "Why: long input.\nVerdict: JUSTIFIED"}]},
             "costs": {"development-systems-designer": 0.30, "development-qa-tester": 1.20}}
b = Box("flags"); b.run(); b.reply("a test request"); b.plan(FLAG_PLAN); m = b.reply("Approved brief")
st = b.state()["task"]; fl = {f["label"]: f for f in st.get("flags", [])}
d2 = b.read("pipeline_state/tasks/T001/step_briefs/s004-D2.md")
check("a Haiku run over 0.25 USD and a Sonnet run over 1.00 USD are flagged; the per-run cap is untouched",
      set(fl) == {"T001-s003-systems-designer", "T001-s004-qa-tester"} and fl["T001-s003-systems-designer"]["model"] == "haiku"
      and fl["T001-s004-qa-tester"]["model"] == "sonnet" and "DD_MAX_BUDGET_USD" not in d2, str(fl))
check("the next brief tells the Producer about the open flag, points to the raw files, and says it is a consultation that never replaces QA",
      "OPEN `T001-s003-systems-designer`" in d2 and "agent_design/tests/logs/" in d2 and "never instead" in d2 and "do not paste their contents" in d2
      and "Verdict: JUSTIFIED" in d2 and "-flag-" in d2, d2[-2500:])
check("the Analyst's assessment with a Verdict line closes its flag, the other stays open, QA still ran, and the user's question reports both",
      fl["T001-s003-systems-designer"]["status"] == "assessed" and fl["T001-s003-systems-designer"]["verdict"] == "JUSTIFIED"
      and fl["T001-s004-qa-tester"]["status"] == "open" and "High-cost flags in this task: 1 open, 1 assessed" in m
      and "D2 dispatch development-qa-tester -> exit 0" in b.log() and "D2 dispatch development-development-systems-analyst -> exit 0 (label flag-1)" in b.log(), m + b.log())
b = Box("flags_missing"); b.run(); b.reply("a test request")
bad = json.loads(json.dumps(FLAG_PLAN)); bad["D2"]["flag_assess"] = [{"label": "T001-s003-systems-designer", "text": None}]
b.plan(bad); m = b.reply("Approved brief")
check("dispatching the Analyst on a flag without getting an assessment file stops the box",
      "did not finish" in m and "no assessment" in m and "Verdict:" in m, m)
b = Box("noflag"); b.run(); b.reply("a test request")
ok = json.loads(json.dumps(FLAG_PLAN)); ok["costs"] = {"development-systems-designer": 0.20, "development-qa-tester": 0.90}; ok["D2"].pop("flag_assess")
b.plan(ok); m = b.reply("Approved brief")
check("a Haiku run at 0.20 USD and a Sonnet run at 0.90 USD are not flagged", not b.state()["task"].get("flags"), str(b.state()["task"].get("flags")))

# ---- the model each run was served must match orchestration/models.json
b = Box("models"); b.run(); b.reply("a test request")
b.plan({"P": {"next": "Rules unresolved"}, "D1": {"dispatch": ["development-systems-designer"], "outputs": ["x.md"]}, "serve_model": {"development-systems-designer": "claude-opus-5-5"}})
m = b.reply("Approved brief")
check("a run served a different model from the one models.json names stops the box and says which",
      "did not finish" in m and "is set to haiku in orchestration/models.json but ran on claude-opus-5-5" in m, m)

# ---- stuck-run detector
b = Box("stuck", {"PIPELINE_STUCK_SECONDS": "3", "PIPELINE_POLL_SECONDS": "1"}); b.plan({"P0": {"hang": 25}}); b.run()
t0 = _time.time(); m = b.reply("a test request"); took = _time.time() - t0
check("a box that shows no activity for the stuck limit is stopped and the user is told why (here after 3 seconds, not 25)",
      "did not finish" in m and "was stopped: no activity" in m and took < 15, f"{took:.1f}s\n{m}")

# ---- run_check.sh wrapper
def rc(*args):
    r = subprocess.run(["orchestration/run_check.sh", *args], cwd=T, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True); return r.stdout
(T / "work").mkdir(exist_ok=True); prog = T / "work" / "prog.sh"; prog.write_text("#!/bin/sh\necho hello from work\n"); prog.chmod(0o755)
check("run_check.sh prints the program's output and its exit code, and refuses programs that are not allowed",
      rc("python3", "-c", "import sys; sys.exit(3)").strip().endswith("EXIT=3") and "hello from work" in rc("work/prog.sh") and rc("work/prog.sh").strip().endswith("EXIT=0")
      and "refused" in rc("rm", "-rf", "x") and rc("rm", "-rf", "x").strip().endswith("EXIT=126") and "refused" in rc("bash", "-c", "id")
      and "refused" in rc("/bin/ls") and "refused" in rc("../outside.sh"), rc("rm", "-rf", "x"))

# ---- allowlists
al = json.loads((SRC / "orchestration" / "allowlists.json").read_text())
comp = ["g++", "clang++", "cmake", "make", "ninja"]
check("compilers are granted to the Software Engineer and QA only; WebFetch to the Systems Designer only; the Critic has its own entry without WebSearch",
      all(f"Bash({c}:*)" in al["development-software-engineer"] and f"Bash({c}:*)" in al["development-qa-tester"] for c in comp)
      and not any("Bash(g++:*)" in v for k, v in al.items() if k not in ("development-software-engineer", "development-qa-tester") and isinstance(v, str))
      and "WebFetch" in al["development-systems-designer"] and not any("WebFetch" in v for k, v in al.items() if k != "development-systems-designer" and k != "_comment" and isinstance(v, str))
      and "development-critic" in al and "WebSearch" not in al["development-critic"], str(al.get("development-critic")))
check("every role that can run python also has run_check.sh",
      all("run_check.sh" in v for k, v in al.items() if not k.startswith("_comment") and "Bash(python3:*)" in v and isinstance(v, str)))

# ---- what the Producer is told
b = Box("brief"); b.run(); b.reply("a test request"); br = b.read("pipeline_state/tasks/T001/step_briefs/s001-P0.md")
check("the Producer's brief carries the team, tools, build-tool advice, inputs, prices, measured cost history, consultation and roster rules",
      "## The team you coordinate" in br and "Build tools: the Software Engineer and QA may run g++" in br and "engine-neutral C++ core" in br
      and "run_check.sh <program>" in br and "## Data the user has supplied" in br and "Source: Anthropic pricing page" in br
      and "## Measured cost history" in br and "Consultations: you may dispatch any of the other ten agents read-only" in br and "Roster rule" in br
      and "QA on its own row" in br and "Mordor-Original-Manual.pdf" in br and "Mordor-Recovered-Community-Archive.zip" in br, br[-3000:])

shutil.rmtree(tmp, ignore_errors=True)
print(f"\n{checks - len(fails)} of {checks} checks passed")
sys.exit(1 if fails else 0)
