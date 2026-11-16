#!/usr/bin/env python3
"""run_pipeline.py: the start file of the Delve Deeper content pipeline (standard library only).

Usage (from any folder):  python3 run_pipeline.py
                          python3 run_pipeline.py --check    (checks the setup and the diagram, runs nothing)

Each run does as much work as it can, then stops at the next question for the user. The question is
written to pipeline_state/message_for_user.md (and printed). The user's reply, word for word, goes into
pipeline_state/reply.txt; running this file again reads the reply and continues.

It walks 03_DEVELOPMENT_WORKFLOW.mmd one place at a time:
- a box: one development-producer session does that box. In it the Producer may dispatch only the roles
  pipeline.json lists for that box; orchestration/invoke_agent.sh refuses every other role.
- a box with several arrows out: the Producer names one arrow, and it must be one of that box's arrows.
- a diamond (a "You ..." question): the user picks one of its arrows by writing the choice's exact words (any capitals, anywhere in the reply). Nothing else can pick it.
Before the first box (and after a finished task) it asks the user pipeline.json's opening question.
Things that are not arrows in the diagram, and so are offered only to the user: ending a task, running a
box again after it failed, and raising a task's budget once it is spent.
"""
import datetime, hashlib, json, os, pathlib, re, shutil, signal, subprocess, sys, time, urllib.request

ROOT = pathlib.Path(__file__).resolve().parent
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "orchestration"))
import ledger_total  # noqa: E402
import validate_agents  # noqa: E402

CONFIG_FILE = ROOT / "pipeline.json"
STATE_DIR = ROOT / "pipeline_state"
STATE_FILE = STATE_DIR / "state.json"
MESSAGE_FILE = STATE_DIR / "message_for_user.md"
REPLY_FILE = STATE_DIR / "reply.txt"
LOCK_FILE = STATE_DIR / "run.lock"
INVOKE = ROOT / "orchestration" / "invoke_agent.sh"
ROLES_DIR = ROOT / "agent_design" / "development_candidates"
END_TASK = "End this task here"
PACKET_SHOWN_CHARS = 20000


class Stop(Exception):
    """A problem with the pipeline itself (setup, diagram, configuration). Shown to the user; nothing runs."""


def now():
    return datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")


def rel(p):
    return str(pathlib.Path(p).resolve().relative_to(ROOT))


def write_atomic(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name + ".tmp")
    tmp.write_text(text, encoding="utf-8")
    tmp.replace(path)


# ---------------------------------------------------------------- the diagram
_NODE = r'([A-Za-z][A-Za-z0-9_]*)\s*(?:\["([^"]*)"\]|\{"([^"]*)"\})?'
_ARROW = r'(?:-->\|([^|]*)\||(-->)|-\.\s+(.+?)\s+\.->|(-\.->))'
EDGE_RE = re.compile(r'^' + _NODE + r'\s*' + _ARROW + r'\s*' + _NODE + r'$')
NODE_RE = re.compile(r'^' + _NODE + r'$')


def parse_diagram(path):
    """Return (nodes, arrows). nodes: id -> {shape: box|diamond, label}. Arrows keep the file's order.
    Fails loudly on any line it does not understand, so it never guesses at the diagram."""
    nodes, arrows = {}, []

    def note(nid, box, diamond, where):
        if box is None and diamond is None:
            nodes.setdefault(nid, None)
            return
        shape, label = ("diamond", diamond) if diamond is not None else ("box", box)
        old = nodes.get(nid)
        if old and (old["shape"], old["label"]) != (shape, label):
            raise Stop(f"{path.name} {where}: {nid} is defined twice, differently")
        nodes[nid] = {"shape": shape, "label": label}

    for n, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line, where = raw.strip(), f"line {n}"
        if (not line or line.startswith("%%") or line == "end" or re.match(r"^flowchart\s+\w+$", line)
                or re.match(r"^subgraph\s", line)):
            continue
        m = EDGE_RE.match(line)
        if m:
            g = m.groups()
            note(g[0], g[1], g[2], where)
            note(g[7], g[8], g[9], where)
            dotted = g[5] is not None or g[6] is not None
            label = g[3] if g[3] is not None else (g[5] if g[5] is not None else "")
            arrows.append({"src": g[0], "dst": g[7], "label": label.strip(), "dotted": dotted})
            continue
        m = NODE_RE.match(line)
        if m and (m.group(2) is not None or m.group(3) is not None):
            note(m.group(1), m.group(2), m.group(3), where)
            continue
        raise Stop(f"{path.name} {where}: line not understood: {raw.strip()!r}")
    missing = sorted(k for k, v in nodes.items() if v is None)
    if missing:
        raise Stop(f"{path.name}: used but never given a label: {missing}")
    return nodes, arrows


class Flow:
    def __init__(self):
        try:
            self.cfg = json.loads(CONFIG_FILE.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as e:
            raise Stop(f"pipeline.json cannot be read: {e}")
        self.nodes, self.arrows = parse_diagram(ROOT / self.cfg["diagram"])
        self.check()

    def out(self, nid):
        return [a for a in self.arrows if a["src"] == nid]

    def arrow_name(self, a):
        return a["label"] or f"to {a['dst']}"

    def check(self):
        c, nodes = self.cfg, self.nodes
        boxes = c["boxes"]
        problems = []
        for nid, n in nodes.items():
            if nid == c["input_box"]:
                continue
            if n["shape"] == "box" and nid not in boxes:
                problems.append(f"box {nid} ({n['label']!r}) has no entry in pipeline.json")
            if n["shape"] == "diamond" and nid in boxes:
                problems.append(f"{nid} is a decision for the user, not a box; remove it from pipeline.json")
            if not self.out(nid):
                problems.append(f"{nid} has no arrow out of it")
            names = [self.arrow_name(a).lower() for a in self.out(nid)]
            if len(names) != len(set(names)):
                problems.append(f"two arrows out of {nid} have the same label")
        for nid, b in boxes.items():
            if nid not in nodes:
                problems.append(f"pipeline.json names {nid}, which is not in the diagram")
            for r in b["roles"]:
                if not (ROLES_DIR / f"{r}.md").is_file():
                    problems.append(f"pipeline.json role {r} has no role file in {rel(ROLES_DIR)}")
        if [a["dst"] for a in self.out(c["input_box"])] != [c["start_box"]]:
            problems.append(f"the diagram's input {c['input_box']} must have exactly one arrow, to {c['start_box']}")
        ba = c["budget_approved_by"]
        if ba["diamond"] not in nodes or ba["arrow"].lower() not in [self.arrow_name(a).lower() for a in self.out(ba["diamond"])]:
            problems.append("pipeline.json budget_approved_by does not match an arrow in the diagram")
        if not (ROLES_DIR / f"{c['producer_agent']}.md").is_file():
            problems.append(f"no role file for {c['producer_agent']}")
        try:
            mm = json.loads((ROOT / "orchestration" / "models.json").read_text(encoding="utf-8"))
            for r in {c["producer_agent"]} | {r for b in boxes.values() for r in b["roles"]}:
                if r not in mm:
                    problems.append(f"orchestration/models.json has no model for {r}")
        except (OSError, ValueError) as e:
            problems.append(f"orchestration/models.json cannot be read: {e}")
        if problems:
            raise Stop("the diagram and pipeline.json do not match:\n- " + "\n- ".join(problems))


# ---------------------------------------------------------------- setup and state
def unpack_roles():
    """Ten role files ship as one bundle (agent_design/agents_ten_development_roles_bundle.md); the eleventh, the
    critic, ships as its own file. Unpack any role file that is missing; never overwrite one that exists."""
    bundle = ROOT / "agent_design" / "agents_ten_development_roles_bundle.md"
    splitter = ROOT / "orchestration" / "split_bundle.py"
    made = []
    if bundle.is_file() and splitter.is_file():
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            r = subprocess.run([sys.executable, "-I", str(splitter), str(bundle), tmp], capture_output=True, text=True)
            if r.returncode != 0:
                raise Stop("unpacking the role files failed:\n" + (r.stderr or r.stdout))
            for f in sorted((pathlib.Path(tmp) / "agent_design" / "development_candidates").glob("*.md")):
                dest = ROLES_DIR / f.name
                if not dest.exists():
                    ROLES_DIR.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(f, dest)
                    made.append(f.name)
    elif not (ROLES_DIR.is_dir() and any(ROLES_DIR.glob("*.md"))):
        raise Stop("the role files are missing, and so is the bundle they unpack from "
                   "(agent_design/agents_ten_development_roles_bundle.md) or orchestration/split_bundle.py")
    if made:
        print(f"Unpacked {len(made)} role files into {rel(ROLES_DIR)}/ (only the ones that were missing).")


def setup():
    if sys.version_info < (3, 8):
        raise Stop("Python 3.8 or newer is needed")
    try:
        import yaml  # noqa: F401  (orchestration/validate_agents.py needs PyYAML)
    except ImportError:
        raise Stop("PyYAML is missing. Install it with: python3 -m pip install pyyaml")
    for prog in ("claude", "timeout", "sha256sum", "bash"):
        if not shutil.which(prog):
            raise Stop(f"the program '{prog}' is not installed or not on PATH"
                       + (" (claude is Claude Code)" if prog == "claude" else ""))
    INVOKE.chmod(INVOKE.stat().st_mode | 0o111)
    CHECKER = ROOT / "orchestration" / "run_check.sh"
    if CHECKER.is_file():
        CHECKER.chmod(CHECKER.stat().st_mode | 0o111)   # downloads and unzips often drop the executable bit
    for d in (ROOT / "agent_design" / "tests" / "logs", ROOT / "work", STATE_DIR):
        d.mkdir(parents=True, exist_ok=True)
    lore = ROOT / "work" / "lore.md"
    if not lore.exists():   # the lore starts empty: content becomes lore only when the user accepts a package (box K)
        lore.write_text("# Lore\n\nAccepted content only. Empty until the user accepts a package. The GDD is separate.\n",
                        encoding="utf-8")


def load_state():
    if STATE_FILE.exists():
        return json.loads(STATE_FILE.read_text(encoding="utf-8"))
    return {"version": 1, "task_count": 0, "task": None, "pending": None, "past_tasks": []}


def save_state(st):
    write_atomic(STATE_FILE, json.dumps(st, indent=1))


class Lock:
    def __enter__(self):
        STATE_DIR.mkdir(parents=True, exist_ok=True)
        if LOCK_FILE.exists():
            try:
                pid = int(LOCK_FILE.read_text().strip())
                os.kill(pid, 0)
                raise Stop(f"run_pipeline.py is already running (process {pid}). Wait for it to finish.")
            except (ValueError, ProcessLookupError, PermissionError):
                pass
        LOCK_FILE.write_text(str(os.getpid()))
        return self

    def __exit__(self, *exc):
        try:
            LOCK_FILE.unlink()
        except FileNotFoundError:
            pass


# ---------------------------------------------------------------- task records the Producer reads
def task_dir(t):
    return STATE_DIR / "tasks" / t["id"]


def write_task_records(t):
    d = task_dir(t)
    write_atomic(d / "request.md", f"# Task {t['id']}: what the user asked (their exact words)\n\n{t['request']}\n")
    lines = [f"# Task {t['id']}: the user's decisions (their exact words)",
             "Written only by run_pipeline.py, from the user's own replies. Each entry is the user's own message.", ""]
    for x in t["decisions"]:
        lines += [f"## {x['time']}, after step {x['after_step']}: {x['question']}",
                  f"- Choices offered: {'; '.join(x['options'])}" if x["options"] else "- (open question)",
                  f"- Choice: {x['choice']}" if x["choice"] else "- Choice: (free text)",
                  "- The user's reply, word for word:", "", "> " + x["reply"].replace("\n", "\n> "), ""]
    write_atomic(d / "decisions.md", "\n".join(lines) + "\n")
    lines = [f"# Task {t['id']}: boxes run so far", ""]
    for h in t["history"]:
        lines += [f"## Step {h['step']}: {h['box']} \"{h['label']}\" ({h['outcome']})",
                  f"- Summary: {h.get('summary') or '(none)'}",
                  f"- Outputs: {', '.join(h.get('outputs') or []) or '(none)'}",
                  f"- Arrow taken: {h.get('arrow') or '(none)'}", ""]
    write_atomic(d / "history.md", "\n".join(lines) + "\n")


def hash_tree(d):
    """Fingerprint every file in pipeline_state/ except the lock and the box logs (those change during a box)."""
    out = {}
    if d.exists():
        for p in sorted(d.rglob("*")):
            if p.is_file() and p != LOCK_FILE and p.parent.name != "step_logs":
                out[rel(p)] = hashlib.sha256(p.read_bytes()).hexdigest()
    return out


# ---------------------------------------------------------------- one box
MODEL_NAMES = {"haiku": "Claude Haiku 5.5", "sonnet": "Claude Sonnet 5.5"}

ANTHROPIC_PRICE_URL = "https://platform.claude.com/docs/en/about-claude/pricing.md"
VERCEL_PRICE_URL = "https://ai-gateway.vercel.sh/v1/models"


def money(cell):
    m = re.search(r"\$\s*([\d,]*\.?\d+)", cell)
    return float(m.group(1).replace(",", "")) if m else None


def prices_from_anthropic(text, names):
    """names: alias -> the model's name in Anthropic's pricing table. Returns alias -> USD per million tokens."""
    out = {}
    for alias, name in names.items():
        rows = []
        for line in text.splitlines():
            if not line.startswith("|"):
                continue
            c = [x.strip() for x in line.strip().strip("|").split("|")]
            if len(c) >= 6 and (c[0] == name or c[0].startswith(name + " (")):
                nums = [money(x) for x in c[1:6]]
                if None not in nums:
                    rows.append((c[0], nums))
        if not rows:
            raise ValueError(f"no price row for {name!r} in the table")
        def rates(n):
            return {"input": n[0], "cache_write": n[1], "cache_read": n[3], "output": n[4]}
        entry = rates(rows[0][1])
        if len(rows) > 1:
            m = re.search(r"over ([\d,]+) tokens", rows[1][0])
            entry["above_tokens"] = int(m.group(1).replace(",", "")) if m else None
            entry["above"] = rates(rows[1][1])
        out[alias] = entry
    return out


def prices_from_vercel(text, ids):
    """ids: alias -> the model's id in the Vercel AI Gateway list. Returns alias -> USD per million tokens."""
    data = {m["id"]: m for m in json.loads(text)["data"]}
    out = {}
    for alias, mid in ids.items():
        if mid not in data or not data[mid].get("pricing"):
            raise ValueError(f"no pricing for {mid!r} in the list")
        pr = data[mid]["pricing"]
        def per_m(x):
            return round(float(x) * 1e6, 6)
        entry = {"input": per_m(pr["input"]), "cache_write": per_m(pr["input_cache_write"]),
                 "cache_read": per_m(pr["input_cache_read"]), "output": per_m(pr["output"])}
        tiers = {k: pr.get(k + "_tiers") for k in ("input", "input_cache_write", "input_cache_read", "output")}
        if all(v and len(v) > 1 for v in tiers.values()):
            entry["above_tokens"] = int(tiers["input"][1]["min"]) - 1
            entry["above"] = {"input": per_m(tiers["input"][1]["cost"]), "cache_write": per_m(tiers["input_cache_write"][1]["cost"]),
                              "cache_read": per_m(tiers["input_cache_read"][1]["cost"]), "output": per_m(tiers["output"][1]["cost"])}
        out[alias] = entry
    return out


def fetch_prices(mm):
    """Anthropic first; if that fails, Vercel; if that fails, say FAILED (never a silent guess)."""
    lookup = mm["_price_lookup"]
    aliases = sorted({v for k, v in mm.items() if not k.startswith("_")})
    attempts = []
    for source, env_name, default_url, key, parse in (
            ("Anthropic pricing page", "PIPELINE_PRICE_URL_ANTHROPIC", ANTHROPIC_PRICE_URL, "anthropic", prices_from_anthropic),
            ("Vercel AI Gateway model list", "PIPELINE_PRICE_URL_VERCEL", VERCEL_PRICE_URL, "vercel", prices_from_vercel)):
        url = os.environ.get(env_name, default_url)
        try:
            req = urllib.request.Request(url, headers={"User-Agent": "pipeline-price-check"})
            with urllib.request.urlopen(req, timeout=30) as r:
                text = r.read().decode("utf-8", errors="replace")
            prices = parse(text, {a: lookup[a][key] for a in aliases})
            for a, e in prices.items():
                if not (0 < e["input"] <= e["output"] and e["cache_read"] > 0):
                    raise ValueError(f"implausible prices for {a}: {e}")
            return {"ok": True, "source": source, "url": url, "fetched": now(), "prices": prices, "attempts": attempts}
        except Exception as e:
            attempts.append(f"{source} ({url}): {type(e).__name__}: {str(e)[:150]}")
    return {"ok": False, "fetched": now(), "attempts": attempts}


def fmt_price(x):
    s = f"{x:.4f}".rstrip("0")
    whole, _, frac = s.partition(".")
    return f"{whole}.{frac.ljust(2, '0')}"


def price_lines(c):
    mm = json.loads((ROOT / "orchestration" / "models.json").read_text(encoding="utf-8"))
    res = fetch_prices(mm)
    roster = ", ".join(f"{r} ({v})" for r, v in mm.items() if not r.startswith("_"))
    if not res["ok"]:
        return ["## Model prices: FAILED",
                f"The program tried to fetch current prices at {res['fetched']} and could not:"] + \
               [f"- {a}" for a in res["attempts"]] + [
                "You have NO current price data. Do this, in this order, in the budget section of your packet:",
                "- First, say plainly that the price check FAILED and that you have no current price data.",
                "- Then, and only then, give the price you remember for each model the team runs, and label every one exactly "
                "\"PAST REMEMBERED PRICE (not verified)\", so the user can see it is not a current price.",
                "- Do not present the budget as built on current prices. Say it rests on remembered prices.",
                f"The models in the team: {roster}.", ""]
    L = ["## Model prices (fetched by the program just before this step)",
         f"Source: {res['source']}, {res['url']}, fetched {res['fetched']}."]
    for a in res["attempts"]:
        L.append(f"A source tried earlier failed, so the next one was used: {a}")
    L.append("USD per million tokens:")
    for alias, e in sorted(res["prices"].items()):
        line = (f"- {alias} ({MODEL_NAMES.get(alias, alias)}): input {fmt_price(e['input'])}; output {fmt_price(e['output'])}; "
                f"cache write {fmt_price(e['cache_write'])}; cache read {fmt_price(e['cache_read'])}")
        if e.get("above"):
            ab = e["above"]
            line += (f". For prompts over {e['above_tokens']:,} tokens: input {fmt_price(ab['input'])}; output {fmt_price(ab['output'])}; "
                     f"cache write {fmt_price(ab['cache_write'])}; cache read {fmt_price(ab['cache_read'])}")
        L.append(line)
    L += ["Cost is price times tokens. In every budget table: one row for each role that will run, with QA on its own row (not lumped with "
          "other roles); for Haiku, show the cost at both price tiers (prompts up to, and over, 100,000 tokens). State the price you used for "
          "each model, the tokens you assumed per dispatch, and where each number came from.", ""]
    return L


def team_lines(c):
    """The Producer's view of its team: which model each agent runs on (orchestration/models.json, the same file
    invoke_agent.sh uses to start every agent), so planning and budgets can account for what each agent is."""
    m = json.loads((ROOT / "orchestration" / "models.json").read_text(encoding="utf-8"))
    L = ["## The team you coordinate, and the model each agent runs on",
         "Set by the program (orchestration/models.json), not by the user. Use it when you size work, choose who does "
         "what, and estimate budgets."]
    al = json.loads((ROOT / "orchestration" / "allowlists.json").read_text(encoding="utf-8"))
    tools_seen = ""
    for role, model in m.items():
        if not role.startswith("_"):
            tools = al.get(role, al["_staged_default"])   # the same choice invoke_agent.sh makes
            tools_seen += " " + tools
            L.append(f"- {role}: {model} ({MODEL_NAMES.get(model, model)}); tools: {tools}")
            rf = ROLES_DIR / f"{role}.md"
            if rf.is_file():   # what the agent does, in its own role file's words
                fm, _, _ = validate_agents.parse_agent_file(rf)
                if fm.get("description"):
                    L.append(f"  Does: {fm['description']}")
    for what, art in (("Google Drive", "a"), ("Unreal", "an")):
        if what.split()[-1].lower() not in tools_seen.lower():
            L.append(f"- No agent is granted {art} {what} tool. Plan work that needs {what} without assuming an agent can reach it.")
    if "g++" in tools_seen or "clang++" in tools_seen:
        L.append("- Build tools: the Software Engineer and QA may run g++, clang++, cmake, make, ninja and ctest here. Design for that: an "
                 "engine-neutral C++ core (rules, timers, data loading) compiled and tested here, with a thin Unreal layer on top that only the "
                 "user's machine can build and play. Label core results EXECUTED here and the Unreal layer NOT RUN until the user's machine runs it.")
    if "run_check.sh" in tools_seen:
        L.append("- `orchestration/run_check.sh <program> [args]` runs one check and prints EXIT=<code>. Agents may not chain commands with ; && | or redirects.")
    if not shutil.which("dotnet"):
        L.append("- .NET is not installed here, so a runnable reference that needs .NET (the mordor research's headless reference needs .NET 8) cannot be run unless it is installed.")
    L += [f"- Each agent's role file, with its responsibilities, is `{rel(ROLES_DIR)}/<role name>.md`. Read the role file of "
          "every agent you assign before you write its brief.",
          f"- Effort setting for all agents: {c['effort']} (not confirmed to apply to every model).",
          f"- Spending cap per agent run: {c['per_run_cap_usd']:.2f} USD.", ""]
    return L


ROLE_DISPLAY = {"development-ui-ux-designer": "UI/UX Designer", "development-qa-tester": "QA Tester",
                "development-ai-integration-engineer": "AI Integration Engineer"}


def load_models():
    return json.loads((ROOT / "orchestration" / "models.json").read_text(encoding="utf-8"))


def model_alias(agent):
    mm = load_models()
    return mm.get(agent, mm["_default"])


def role_display(role):
    if role in ROLE_DISPLAY:
        return ROLE_DISPLAY[role]
    s = role[len("development-"):] if role.startswith("development-") else role
    return s.replace("-", " ").title()


def role_short(role):
    return role[len("development-"):] if role.startswith("development-") else role


def other_roles(c):
    return [r for r in load_models() if not r.startswith("_") and r != c["producer_agent"]]


def inputs_lines():
    d = ROOT / "work" / "inputs"
    L = ["## Data the user has supplied (`work/inputs/`)"]
    files = sorted(p for p in d.rglob("*") if p.is_file()) if d.is_dir() else []
    if not files:
        L.append("- Nothing yet. If the task needs the user's data (for example the mordor combat research in their Google Drive), no agent can "
                 "fetch it: say so in your packet. The chat session copies named data into `work/inputs/` once the user approves the copy.")
    else:
        L += [f"- `{rel(p)}` ({p.stat().st_size:,} bytes)" for p in files[:60]]
        if len(files) > 60:
            L.append(f"- ... and {len(files) - 60} more files")
    g = ROOT / "orchestration" / "known_gaps.md"
    if g.is_file():
        L += ["", g.read_text(encoding="utf-8").rstrip("\n")]
    return L + [""]


def history_lines(t):
    p = ROOT / "orchestration" / "cost_history.md"
    L = [p.read_text(encoding="utf-8").rstrip("\n"), ""] if p.is_file() else []
    by = {}
    for r in ledger_total.rows(t["id"] + "-"):
        n, cost = by.get(r["agent"], (0, 0.0))
        try:
            cost += float(r.get("cost_usd") or 0)
        except ValueError:
            pass
        by[r["agent"]] = (n + 1, cost)
    if by:
        L += ["**This task so far (this pipeline's own ledger):** " + "; ".join(
            f"{a}: {n} run(s), {cost:.2f} USD" for a, (n, cost) in sorted(by.items())), ""]
    return L


def detect_flags(c, t, prefix, step, box):
    """Flag every run of this step that cost more than its model's limit. The per-run cap is unchanged."""
    limits = c.get("cost_flags", {})
    have = {f["label"] for f in t.setdefault("flags", [])}
    for row in ledger_total.rows(prefix):
        alias = model_alias(row["agent"])
        try:
            cost = float(row.get("cost_usd") or 0)
        except ValueError:
            cost = 0.0
        if alias in limits and cost > limits[alias] and row["label"] not in have:
            t["flags"].append({"label": row["label"], "agent": row["agent"], "model": alias, "cost_usd": round(cost, 4),
                               "limit_usd": limits[alias], "turns": row.get("turns"), "log": row.get("log"), "brief": row.get("brief"),
                               "step": step, "box": box, "status": "open", "verdict": None})


def resolve_flags(c, t, prefix, reasons):
    """An open flag is assessed once the Analyst's file work/<task>/flags/<flag label>.md exists and ends in a Verdict line."""
    analyst = c.get("flag_analyst")
    dispatched = [r for r in ledger_total.rows(prefix) if "-flag-" in r["label"] and r["agent"] == analyst]
    wrote = 0
    for f in t.get("flags", []):
        if f["status"] != "open":
            continue
        ap = ROOT / "work" / t["id"] / "flags" / (f["label"] + ".md")
        if not ap.is_file():
            continue
        verdicts = re.findall(r"^\s*Verdict:\s*(.+)$", ap.read_text(encoding="utf-8"), re.M | re.I)
        if not verdicts:
            reasons.append(f"the assessment {rel(ap)} has no line starting 'Verdict:'")
            continue
        f.update({"status": "assessed", "verdict": verdicts[-1].strip()[:200], "assessment": rel(ap)})
        wrote += 1
    if dispatched and not wrote and not reasons:
        reasons.append("the Analyst was dispatched on a high-cost flag (label with -flag-) but no assessment "
                       "work/<task>/flags/<flag label>.md with a 'Verdict:' line was written")


def flag_lines(t, step_no):
    fl = t.get("flags") or []
    if not fl:
        return []
    open_ = [f for f in fl if f["status"] == "open"]
    L = ["## High-cost flags (a consultation: it adds to the normal work and never replaces or reroutes it)"]
    for f in open_:
        L.append(f"- OPEN `{f['label']}`: {f['agent']} on {f['model']} cost {f['cost_usd']:.2f} USD (flag limit {f['limit_usd']:.2f}), "
                 f"{f['turns']} turns, flagged at step {f['step']}. Raw data: log `{f['log']}`, brief `{f['brief']}`, and its row in "
                 "`agent_design/tests/run_ledger.csv`." + (" OVERDUE: open for 2 or more steps." if step_no - f["step"] >= 2 else ""))
    for f in fl:
        if f["status"] == "assessed":
            L.append(f"- assessed `{f['label']}`: {f['verdict']} (`{f['assessment']}`)")
    if open_:
        tid = t["id"]
        L += ["- Dispatch the Development Systems Analyst on each open flag, in this box or a later one, in addition to the work already planned. "
              "Whoever was due to receive information (QA included) still receives it; the Analyst gets the flag as well, never instead.",
              f"- Dispatch form: `orchestration/invoke_agent.sh development-development-systems-analyst work/{tid}/briefs/<step>-flag-<n>.md "
              f"<label prefix>flag-<n> --staged`. The label must contain -flag-. Point the brief at the raw files above; do not paste their contents.",
              f"- The brief must ask the Analyst to write `work/{tid}/flags/<flag label>.md` assessing: what the run was; why it cost so much; "
              "whether the cost is justified; whether there is an obvious error (for example the agent was fed huge or garbled input); whether "
              "there is a clearly better way to do it. Depth should match the case: short for a clear, justified cost or a simple error with a "
              "clear fix; more critical thinking for a serious or unclear problem. The file ends with one line: `Verdict: JUSTIFIED`, "
              "`Verdict: FIXABLE (what to change)`, `Verdict: SERIOUS (why)` or `Verdict: UNCLEAR (what is missing)`.",
              "- When the assessment is back, read it, decide what to do about it, and say what you decided in your packet."]
    return L + [""]


def section_text(text, keyword):
    """The text under the first heading whose title contains keyword, up to the next heading of the same or higher level."""
    out, level = None, 0
    for ln in text.splitlines():
        m = re.match(r"^(#+)\s*(.*)$", ln)
        if out is None:
            if m and keyword in m.group(2).lower():
                out, level = [], len(m.group(1))
        elif m and len(m.group(1)) <= level:
            break
        else:
            out.append(ln)
    return None if out is None else "\n".join(out)


def check_roster(flow, t, text):
    sec = section_text(text, "roster")
    if sec is None:
        return ["the starting-brief packet has no 'Roster' section (one line for each of the other ten agents)"]
    consulted = {r["agent"] for r in ledger_total.rows(t["id"] + "-") if "-consult-" in r["label"]}
    R = []
    for role in other_roles(flow.cfg):
        disp = norm(role_display(role))
        line = next((ln for ln in sec.splitlines() if norm(ln.strip("|-* \t")).startswith(disp)), None)
        if line is None:
            R.append(f"the roster does not list {role_display(role)} (one line per agent, starting with its name): no agent may be dropped")
            continue
        low = line.lower()
        if "bench" in low and role not in consulted:
            R.append(f"the roster benches {role_display(role)} but it was never consulted (dispatch it with a label containing -consult-, then decide)")
        elif "bench" not in low and "consult" in low and role not in consulted:
            R.append(f"the roster says {role_display(role)} was CONSULTED but no consultation run is recorded")
        elif not any(w in low for w in ("task", "consult", "bench")):
            R.append(f"the roster line for {role_display(role)} has no disposition (TASKED, CONSULTED or BENCHED)")
    return R


def write_roster(flow, t, text):
    sec = section_text(text, "roster") or ""
    rows = [r for r in ledger_total.rows(t["id"] + "-") if "-consult-" in r["label"]]
    consulted = {r["agent"]: r for r in rows}
    tasked, then_tasked, then_benched, pending = [], [], [], []
    for role in other_roles(flow.cfg):
        disp = norm(role_display(role))
        line = next((ln.strip() for ln in sec.splitlines() if norm(ln.strip("|-* \t")).startswith(disp)), "")
        low = line.lower()
        item = f"{role_display(role)}: {line.strip('|- ').strip()}"
        if role in consulted:
            (then_benched if "bench" in low else then_tasked if "task" in low else pending).append(item)
        else:
            tasked.append(item)
    L = [f"# Roster record for task {t['id']}",
         "Written by run_pipeline.py from the Producer's packet roster and the run ledger, for the Development Systems Analyst.", "",
         "## Tasked without a consultation"] + [f"- {x}" for x in tasked] + ["", "## In question (consulted), then tasked"] + \
        [f"- {x}" for x in then_tasked] + ["", "## In question (consulted), then benched"] + [f"- {x}" for x in then_benched] + \
        ["", "## Consulted, no decision recorded"] + [f"- {x}" for x in pending] + ["", "## Consultation runs"] + \
        [f"- {r['label']}: {r['agent']}, log `{r['log']}`" for r in rows]
    write_atomic(ROOT / "work" / t["id"] / "roster.md", "\n".join(L) + "\n")


def check_packet(flow, t, box, text):
    c, R = flow.cfg, []
    heads = " | ".join(m.group(1).lower() for m in re.finditer(r"^#+\s*(.*)$", text, re.M))
    if "evidence" not in heads and "limits" not in heads:
        R.append("the packet has no 'Test evidence and known limits' section")
    if "decision" not in heads:
        R.append("the packet has no 'Decisions needed' section")
    if re.search(r"see issues", text, re.I) and "issues" not in heads:
        R.append("the packet says 'see ISSUES' but has no ISSUES section")
    if box == c["budget_proposed_in_box"]:
        b = section_text(text, "budget")
        if b is None:
            R.append("the starting-brief packet has no 'Budget' section")
        else:
            low = b.lower()
            if "price" not in low or "token" not in low:
                R.append("the budget section does not state the prices and the token counts it assumed")
            if not any(re.search(r"\bqa\b", ln, re.I) and not re.search(r"critic|analyst", ln, re.I) for ln in b.splitlines()):
                R.append("the budget section has no row of its own for QA")
            if "haiku" in low and not re.search(r"100,?000|100k", low):
                R.append("the budget section uses Haiku but shows neither of its price tiers (prompts up to and over 100,000 tokens)")
        R += check_roster(flow, t, text)
    return R


def check_models(c, prefix, reasons):
    """The model each run was actually served must be the one orchestration/models.json says."""
    for r in ledger_total.rows(prefix):
        if not r.get("log"):
            continue
        try:
            mu = json.loads((ROOT / r["log"]).read_text(encoding="utf-8")).get("modelUsage")
        except (OSError, ValueError):
            continue
        if not isinstance(mu, dict) or not mu:
            continue
        want = model_alias(r["agent"])
        if not any(want in k.lower() for k in mu):
            reasons.append(f"run {r['label']} ({r['agent']}) is set to {want} in orchestration/models.json but ran on {', '.join(mu)}")


def newest_activity():
    best = 0.0
    for base in (ROOT / "work", ROOT / "agent_design" / "tests"):
        if base.is_dir():
            for q in base.rglob("*"):
                try:
                    if q.is_file():
                        best = max(best, q.stat().st_mtime)
                except OSError:
                    pass
    proj = pathlib.Path.home() / ".claude" / "projects" / re.sub(r"[^A-Za-z0-9]", "-", str(ROOT))
    if proj.is_dir():
        for q in proj.glob("*.jsonl"):   # transcripts of the agent sessions started in this folder
            try:
                best = max(best, q.stat().st_mtime)
            except OSError:
                pass
    return best


def run_watched(cmd, env, out, c):
    """Run the box; stop it if nothing has moved for stuck_minutes (no new or changed file, no agent transcript update)."""
    limit = float(os.environ.get("PIPELINE_STUCK_SECONDS", c.get("stuck_minutes", 15) * 60))
    poll = float(os.environ.get("PIPELINE_POLL_SECONDS", 20))
    proc = subprocess.Popen(cmd, cwd=ROOT, env=env, stdout=out, stderr=subprocess.STDOUT, start_new_session=True)
    last = max(time.time(), newest_activity())
    while True:
        try:
            return proc.wait(timeout=poll), None
        except subprocess.TimeoutExpired:
            last = max(last, newest_activity())
            if time.time() - last > limit:
                try:
                    os.killpg(proc.pid, signal.SIGKILL)
                except OSError:
                    proc.kill()
                proc.wait()
                return proc.returncode, (f"the box was stopped: no activity (no new or changed file under work/ or agent_design/tests/, "
                                         f"no agent transcript update) for {limit / 60:.1f} minutes")


def step_brief(flow, t, box, sid, prefix, result_path, packet_path):
    c = flow.cfg
    node, bcfg = flow.nodes[box], c["boxes"][box]
    roles = bcfg["roles"]
    outs = flow.out(box)
    tid = t["id"]
    if t.get("budget_usd") is not None:
        budget = (f"{t['budget_usd']:.2f} USD approved by the user for this task; "
                  f"{ledger_total.total(tid + '-'):.2f} USD spent so far. invoke_agent.sh refuses new runs once it is reached.")
    else:
        budget = "no task budget is approved yet (each run is capped at %.2f USD)." % c["per_run_cap_usd"]
    write_where = f"`work/{tid}/`" + (" and `work/catalogue.md`" if box == "K" else "")
    L = [f"# Step {t['step']} of task {tid}: box {box} \"{node['label']}\"",
         f"Written by run_pipeline.py (a program, not an agent) at {now()}. You are development-producer.",
         "",
         "You are doing ONE box of the workflow in `03_DEVELOPMENT_WORKFLOW.mmd`. run_pipeline.py decides which box runs "
         "next and asks the user every \"You ...\" question itself. Do only this box, then stop.",
         "",
         "## Read first",
         f"- `{rel(task_dir(t) / 'request.md')}`: what the user asked, in their own words.",
         f"- `{rel(task_dir(t) / 'decisions.md')}`: every decision the user has made in this task, in their own words. "
         "run_pipeline.py writes this file only from the user's own replies; each entry counts as the user's own message.",
         f"- `{rel(task_dir(t) / 'history.md')}`: the boxes already run in this task, with summaries and output files.",
         f"- `03_DEVELOPMENT_WORKFLOW.mmd`; {bcfg['guide']}",
         "- `work/catalogue.md`, if it exists (accepted packages from earlier tasks).",
         *([f"- `work/{tid}/roster.md`: the roster record (tasked / consulted then tasked / consulted then benched)."]
           if (ROOT / "work" / tid / "roster.md").is_file() else []),
         "",
         "## This box",
         f"- Box {box}: \"{node['label']}\".",
         ("- Roles you may dispatch in this box: " + ", ".join(roles) + ". orchestration/invoke_agent.sh refuses every other role.")
         if roles else "- Roles you may dispatch in this box: none. This box is your own work; orchestration/invoke_agent.sh refuses every dispatch.",
         f"- Budget: {budget}",
         f"- Write only under {write_where}. Never write in `pipeline_state/`.",
         ]
    if box in c.get("consult_boxes", []):
        L += [f"- Consultations: you may dispatch any of the other ten agents read-only to ask whether it sees a job (label `{prefix}consult-<short name>`, "
              f"for example `{prefix}consult-world-designer`). Form: `orchestration/invoke_agent.sh <agent-name> work/{tid}/briefs/{sid}-consult-<short name>.md "
              f"{prefix}consult-<short name> --staged`. It has read-only tools and a cost cap of {c.get('consult_cap_usd', 0.3):.2f} USD. Its reply ends with "
              "CONSULT-REPLY: AGREE or DISAGREE and is printed in the command output."]
    if box == c.get("analyst_box", "A"):
        L += [f"- This is the Analyst's box. Give the Analyst: the open high-cost flags below (the raw files, not pasted text), the roster record "
              f"`work/{tid}/roster.md` if it exists, this task's ledger (`agent_design/tests/run_ledger.csv`, rows whose label starts `{tid}-`), and the "
              "measured cost history and current prices in this brief."]
    if box == c["budget_proposed_in_box"]:
        L += ["- Roster rule (the program checks your packet): never drop an agent. The packet needs a `## Roster` section with one line for each of the "
              "other ten agents: its name, then TASKED, CONSULTED or BENCHED, and a reason. If you see no job for an agent, consult it first (see "
              "Consultations), then decide to task it or bench it. BENCHED is allowed only after a consultation. The program records tasked / "
              "consulted-then-tasked / consulted-then-benched in the roster record for the Analyst.",
              "- Packet sections the program checks: the result or plan, `Test evidence and known limits`, `Decisions needed`, `Budget` (prices, tokens, QA on "
              "its own row, both Haiku tiers) and `Roster`. If you write 'see ISSUES', include an ISSUES section."]
    L += [""]
    L += team_lines(c)
    L += inputs_lines()
    L += price_lines(c)
    L += history_lines(t)
    L += flag_lines(t, t["step"])
    if roles:
        L += ["## Dispatching",
              f"- Write one brief per dispatch to `work/{tid}/briefs/{sid}-<role>.md` (task ID, source/version refs, inputs, "
              f"exact output paths under `work/{tid}/`, acceptance criteria, token allowance, stop limit). Keep briefs focused: links, not pasted text.",
              "- Dispatch with exactly this form, one simple command at a time, with no cd, ;, &&, pipes or environment-variable "
              "prefix (model, effort and spending cap are already set):",
              f"  `orchestration/invoke_agent.sh <agent-name> work/{tid}/briefs/{sid}-<role>.md {prefix}<short-role> --staged`",
              "- Check each result's STATUS. Verify the named output files exist (ls) before treating a dispatch as done.",
              ""]
    L += ["## When you finish",
          f"Write `{rel(result_path)}` as JSON with these keys:",
          "- \"status\": \"done\", \"blocked\" or \"needs-revision\"",
          "- \"summary\": what happened in this box, 120 words or fewer",
          "- \"outputs\": list of every file this box produced or changed (paths from the project folder)",
          "- \"questions_for_user\": list of open questions only the user can answer (may be empty)"]
    if bcfg.get("verify_quotes"):
        L.append(f"- \"critic_report\": the path of the critic's report, `work/{tid}/critic/critic_report.md`. "
                 "run_pipeline.py runs orchestration/verify_quotes.py on it; any quote that does not match its file and line "
                 "fails this box. The report's last line decides your \"next\": 'Verdict: CONSISTENT' or 'Verdict: CONTRADICTIONS'.")
    if len(outs) == 1:
        a = outs[0]
        dst = flow.nodes[a["dst"]]
        if dst["shape"] == "diamond":
            L += [f"- \"packet\": \"{rel(packet_path)}\"",
                  "",
                  f"The next step is the user's decision {a['dst']}: \"{dst['label']}\". Write the review packet for it at "
                  f"`{rel(packet_path)}`: the working result, test evidence, known limits and the decisions needed (your role "
                  "file, 'Review'). The user will be offered these choices, exactly: "
                  + "; ".join(f"\"{flow.arrow_name(x)}\"" for x in flow.out(a["dst"])) + "."]
        else:
            L += ["", f"The next box is {a['dst']} \"{dst['label']}\" (the only arrow out of this box). Do not add a \"next\" key."]
    else:
        L += ["- \"next\": the label of the arrow you choose, copied exactly from the list below",
              "",
              "Choose ONE arrow out of this box. The arrows, from `03_DEVELOPMENT_WORKFLOW.mmd`:"]
        for a in outs:
            L.append(f"  - \"{flow.arrow_name(a)}\" leads to {a['dst']} \"{flow.nodes[a['dst']]['label']}\"")
        if any(flow.nodes[a["dst"]]["shape"] == "diamond" for a in outs):
            L.append(f"If the arrow you choose leads to a \"You ...\" decision, write a review packet at `{rel(packet_path)}` "
                     "and add \"packet\" with that path.")
    if box == c["budget_proposed_in_box"]:
        L += ["", "Also add \"budget_usd\": your proposed budget for the whole task in USD (a number). If the user approves "
              "your recommendation, run_pipeline.py enforces that budget for every later run in this task."]
    L += ["", "Then end with STATUS, OUTPUTS, ISSUES.", "",
          "## Step data (also read by run_pipeline.py's checks)",
          "```json",
          json.dumps({"task": tid, "step": t["step"], "box": box, "roles": roles, "label_prefix": prefix,
                      "result": rel(result_path), "packet": rel(packet_path),
                      "arrows": [{"label": flow.arrow_name(a), "to": a["dst"],
                                  "to_is_decision": flow.nodes[a["dst"]]["shape"] == "diamond"} for a in outs],
                      "budget_needed": box == c["budget_proposed_in_box"]}, indent=1),
          "```", ""]
    return "\n".join(L)


def run_box(flow, st, box):
    """Run one box. Returns (True, result, arrow) or (False, reasons, None)."""
    c, t = flow.cfg, st["task"]
    t["step"] += 1
    sid = f"s{t['step']:03d}"
    prefix = f"{t['id']}-{sid}-"
    roles = c["boxes"][box]["roles"]
    wd = ROOT / "work" / t["id"]
    result_path = wd / "step_results" / f"{sid}-{box}.json"
    packet_path = wd / "packets" / f"{sid}-packet.md"
    brief_path = task_dir(t) / "step_briefs" / f"{sid}-{box}.md"
    log_path = task_dir(t) / "step_logs" / f"{sid}-{box}.txt"
    for p in (result_path, packet_path, log_path):
        p.parent.mkdir(parents=True, exist_ok=True)
    write_task_records(t)
    write_atomic(brief_path, step_brief(flow, t, box, sid, prefix, result_path, packet_path))
    save_state(st)
    before = hash_tree(STATE_DIR)

    env = {k: v for k, v in os.environ.items() if not k.startswith("DD_")}
    open_flags = [f for f in t.get("flags", []) if f["status"] == "open"]
    env.update({"DD_STEP_LAUNCH": "1", "DD_ALLOWED_AGENTS": " ".join(roles), "DD_LABEL_PREFIX": prefix,
                "DD_TASK_PREFIX": t["id"] + "-", "DD_MAX_BUDGET_USD": f"{c['per_run_cap_usd']:.2f}",
                "DD_CONSULT_AGENTS": " ".join(other_roles(c)) if box in c.get("consult_boxes", []) else "",
                "DD_CONSULT_BUDGET_USD": f"{c.get('consult_cap_usd', 0.3):.2f}",
                "DD_FLAG_AGENTS": c.get("flag_analyst", "") if open_flags else "",
                "ANTHROPIC_MODEL": c["model"], "CLAUDE_CODE_EFFORT_LEVEL": c["effort"]})
    if t.get("budget_usd") is not None:
        env["DD_TASK_BUDGET_USD"] = f"{t['budget_usd']:.2f}"
    cmd = [str(INVOKE), c["producer_agent"], rel(brief_path), prefix + "producer", "--staged",
           "--timeout", str(c["producer_box_timeout_s"])]
    print(f"[{now()}] task {t['id']} step {t['step']}: box {box} \"{flow.nodes[box]['label']}\" is running "
          f"(log: {rel(log_path)})", flush=True)
    with log_path.open("w", encoding="utf-8") as f:
        rc, stuck = run_watched(cmd, env, f, c)
    after = hash_tree(STATE_DIR)

    reasons, result, arrow, packet_text = [], None, None, None
    if stuck:
        reasons.append(stuck)
    changed = sorted(k for k in set(before) | set(after) if before.get(k) != after.get(k))
    if changed:
        reasons.append("files in pipeline_state/ were changed during the box (only run_pipeline.py may change them): "
                       + ", ".join(changed))
    for row in ledger_total.rows(prefix):
        ok = ((row["agent"] == c["producer_agent"] and row["label"] == prefix + "producer") or row["agent"] in roles
              or ("-consult-" in row["label"] and box in c.get("consult_boxes", []))
              or ("-flag-" in row["label"] and row["agent"] == c.get("flag_analyst")))
        if not ok:
            reasons.append(f"run {row['label']} used {row['agent']}, which may not work in box {box}")
    if roles and not [r for r in ledger_total.rows(prefix) if r["agent"] in roles and "-consult-" not in r["label"]]:
        reasons.append("no specialist run was recorded for this box (roles allowed here: " + ", ".join(roles)
                       + "); the Producer must dispatch them and must not do their work itself")
    if rc != 0:
        reasons.append(f"the Producer run ended with exit code {rc} (log: {rel(log_path)})")
    if not result_path.exists():
        reasons.append(f"the Producer did not write {rel(result_path)}")
    else:
        try:
            result = json.loads(result_path.read_text(encoding="utf-8"))
            if not isinstance(result, dict):
                raise ValueError("not a JSON object")
        except (ValueError, json.JSONDecodeError) as e:
            reasons.append(f"{rel(result_path)} is not valid JSON: {e}")
            result = None
    if result is not None:
        status = result.get("status")
        if status != "done":
            reasons.append(f"the Producer reported status {status!r}: {result.get('summary', '')}")
        outs = flow.out(box)
        if len(outs) == 1:
            arrow = outs[0]
        else:
            want = " ".join(str(result.get("next", "")).split()).lower()
            hits = [a for a in outs if flow.arrow_name(a).lower() == want]
            if len(hits) != 1:
                reasons.append(f"the Producer's \"next\" ({result.get('next')!r}) is not one of the arrows out of {box}: "
                               + "; ".join(flow.arrow_name(a) for a in outs))
            else:
                arrow = hits[0]
        if arrow and flow.nodes[arrow["dst"]]["shape"] == "diamond":
            pk = (ROOT / str(result.get("packet") or rel(packet_path))).resolve()
            if ROOT not in pk.parents or not pk.is_file() or not pk.read_text(encoding="utf-8").strip():
                reasons.append(f"no review packet for the user's decision {arrow['dst']} "
                               f"(expected at {rel(packet_path)}, inside this folder)")
            else:
                result["packet"] = rel(pk)
                packet_text = pk.read_text(encoding="utf-8")
                reasons += check_packet(flow, t, box, packet_text)
        if c["boxes"][box].get("verify_quotes") and not reasons:
            rp = (ROOT / str(result.get("critic_report") or "")).resolve()
            if ROOT not in rp.parents or not rp.is_file():
                reasons.append("the Producer did not give an existing \"critic_report\" path inside this folder")
            else:
                vr = subprocess.run([sys.executable, "-I", str(ROOT / "orchestration" / "verify_quotes.py"), str(rp)],
                                    cwd=ROOT, capture_output=True, text=True)
                if vr.returncode != 0:
                    reasons.append("the critic's quotes did not verify: " + " | ".join(
                        x for x in vr.stdout.splitlines() if x.startswith("FAIL") or "quotes verified" in x)[:600])
                verdicts = re.findall(r"Verdict:\s*(CONSISTENT|CONTRADICTIONS)", rp.read_text(encoding="utf-8"))
                want = c["boxes"][box].get("verdict_arrows", {}).get(verdicts[-1] if verdicts else "", None)
                if not verdicts:
                    reasons.append("the critic's report has no 'Verdict: CONSISTENT' or 'Verdict: CONTRADICTIONS' line")
                elif arrow and want and flow.arrow_name(arrow).lower() != want.lower():
                    reasons.append(f"the report's verdict is {verdicts[-1]} but the Producer chose \"{flow.arrow_name(arrow)}\"")
        if box == c["budget_proposed_in_box"]:
            b = result.get("budget_usd")
            if not isinstance(b, (int, float)) or isinstance(b, bool) or b <= 0:
                reasons.append(f"the Producer did not give a budget (\"budget_usd\" must be a number above 0; got {b!r})")
        outputs = result.get("outputs") or []
        if not isinstance(outputs, list):
            reasons.append("\"outputs\" must be a list of paths")
            outputs = []
        missing = [o for o in outputs if not (ROOT / str(o)).exists()]
        if missing:
            reasons.append("listed outputs that do not exist: " + ", ".join(map(str, missing)))
    check_models(c, prefix, reasons)
    detect_flags(c, t, prefix, t["step"], box)
    resolve_flags(c, t, prefix, reasons)
    if not reasons and packet_text is not None and box == c["budget_proposed_in_box"]:
        write_roster(flow, t, packet_text)
    t["history"].append({"step": t["step"], "box": box, "label": flow.nodes[box]["label"],
                         "outcome": "done" if not reasons else "did not finish",
                         "summary": (result or {}).get("summary") if not reasons else "; ".join(reasons),
                         "outputs": (result or {}).get("outputs") if isinstance((result or {}).get("outputs"), list) else [],
                         "arrow": f"\"{flow.arrow_name(arrow)}\" to {arrow['dst']}" if (arrow and not reasons) else None,
                         "cost_usd": round(ledger_total.total(prefix), 4)})
    if reasons:
        return False, reasons, None
    return True, result, arrow


# ---------------------------------------------------------------- questions for the user
def spent(t):
    return ledger_total.total(t["id"] + "-")


def norm(s):
    return " ".join(re.sub(r"[^a-z0-9]+", " ", s.lower()).split())


def opening(flow, st, note=""):
    q = flow.cfg["opening_question"]
    return {"kind": "opening", "question": q, "options": [], "message": (note + "\n\n" if note else "") + q}


def gate(flow, st, diamond, last_result):
    t = st["task"]
    node = flow.nodes[diamond]
    options = [{"text": flow.arrow_name(a), "dst": a["dst"]} for a in flow.out(diamond)]
    options.append({"text": END_TASK, "dst": None})
    ba = flow.cfg["budget_approved_by"]
    L = [f"Producer: Decision needed (task {t['id']}, after step {t['step']}): \"{node['label']}\"", ""]
    if last_result:
        if last_result.get("summary"):
            L += [f"What the last box did: {last_result['summary']}", ""]
        qs = last_result.get("questions_for_user") or []
        if qs:
            L += ["Open questions the Producer has for you:"] + [f"- {q}" for q in qs] + [""]
    if diamond == ba["diamond"] and t.get("proposed_budget_usd") is not None:
        L.append(f"Proposed budget for this task: {t['proposed_budget_usd']:.2f} USD. "
                 f"Choosing \"{ba['arrow']}\" approves it. Spent so far: {spent(t):.2f} USD.")
    elif t.get("budget_usd") is not None:
        L.append(f"Spent so far: {spent(t):.2f} USD of the approved {t['budget_usd']:.2f} USD.")
    fl = t.get("flags") or []
    if fl:
        n_open = sum(1 for f in fl if f["status"] == "open")
        L.append(f"High-cost flags in this task: {n_open} open, {len(fl) - n_open} assessed" +
                 ("".join(f"; open: {f['label']} ({f['cost_usd']:.2f} USD)" for f in fl if f["status"] == "open")) + ".")
    pk = (last_result or {}).get("packet")
    if pk and (ROOT / pk).is_file():
        text = (ROOT / pk).read_text(encoding="utf-8")
        L += [f"Review packet: {pk}", "", "----- review packet -----", text[:PACKET_SHOWN_CHARS]]
        if len(text) > PACKET_SHOWN_CHARS:
            L.append(f"[... the packet continues; open {pk} for the rest]")
        L += ["----- end of review packet -----"]
    L += ["", "Reply with the exact words of your choice (any capitals). You can add notes.", ""]
    for o in options:
        extra = " (not an arrow in the diagram: ends this task)" if o["dst"] is None else ""
        L.append(f"- {o['text']}{extra}")
    return {"kind": "gate", "diamond": diamond, "question": node["label"], "options": options,
            "message": "\n".join(L), "last_result": last_result}


def blocked(flow, st, box, reasons):
    t = st["task"]
    L = [f"Producer: Step {t['step']} of task {t['id']} (box {box}, \"{flow.nodes[box]['label']}\") did not finish.", "",
         "Why:"] + [f"- {r}" for r in reasons] + ["",
         "Reply with the exact words of your choice (neither choice is an arrow in the diagram):", "",
         "- Run this box again", f"- {END_TASK}"]
    return {"kind": "blocked", "box": box, "question": f"Box {box} did not finish",
            "options": [{"text": "Run this box again", "dst": box}, {"text": END_TASK, "dst": None}],
            "message": "\n".join(L)}


def budget_reached(flow, st, box):
    t = st["task"]
    L = [f"Producer: Task {t['id']} has spent {spent(t):.2f} USD of its approved {t['budget_usd']:.2f} USD budget, "
         f"so nothing more will run. The next box would be {box} \"{flow.nodes[box]['label']}\".", "",
         "Reply with the exact words of your choice (neither choice is an arrow in the diagram):", "",
         "- Raise the budget (write the new total in USD too, for example \"Raise the budget to 25\")", f"- {END_TASK}"]
    return {"kind": "budget", "box": box, "question": "The task's budget is spent",
            "options": [{"text": "Raise the budget", "dst": box}, {"text": END_TASK, "dst": None}],
            "message": "\n".join(L)}


def cap_question(flow, st, box, cap, res):
    t = st["task"]
    options = [{"text": flow.arrow_name(a), "dst": a["dst"]} for a in flow.out(box)] + [{"text": END_TASK, "dst": None}]
    L = [f"Producer: The loop \"{cap['arrow']}\" out of box {box} (\"{flow.nodes[box]['label']}\") has already run "
         f"{cap['max']} times in task {t['id']}, which is its limit, and the Producer wants to send it round again.", "",
         f"What the last box did: {res.get('summary', '(no summary)')}"]
    if res.get("critic_report"):
        L.append(f"Critic report: {res['critic_report']}")
    L += ["", "Reply with the exact words of your choice (both arrows are from the diagram):", ""]
    for o in options:
        L.append(f"- {o['text']}" + (" (not an arrow in the diagram: ends this task)" if o["dst"] is None else ""))
    return {"kind": "cap", "box": box, "question": f"Loop limit reached at {box}", "options": options, "message": "\n".join(L)}


def record(t, pending, reply, choice):
    t["decisions"].append({"time": now(), "after_step": t["step"], "question": pending["question"],
                           "options": [o["text"] for o in pending["options"]], "choice": choice, "reply": reply})


def end_task(st, how):
    t = st["task"]
    t["status"] = how
    t["ended"] = now()
    write_task_records(t)
    st["past_tasks"].append({"id": t["id"], "status": how, "ended": t["ended"]})


def handle_reply(flow, st, reply):
    p = st["pending"]
    if p["kind"] == "opening":
        if not reply.strip():
            st["pending"] = opening(flow, st, "Producer: I did not get an answer.")
            return
        st["task_count"] += 1
        t = {"id": f"T{st['task_count']:03d}", "request": reply.rstrip("\n"), "started": now(), "status": "open",
             "position": flow.cfg["start_box"], "step": 0, "budget_usd": None, "proposed_budget_usd": None,
             "decisions": [], "history": []}
        st["task"] = t
        record(t, p, reply.rstrip("\n"), None)
        st["pending"] = None
        return
    t = st["task"]
    p.setdefault("base", p["message"])
    low = f" {norm(reply)} "
    found = [o for o in p["options"] if f" {norm(o['text'])} " in low]
    found = [o for o in found if not any(o is not q and f" {norm(o['text'])} " in f" {norm(q['text'])} " for q in found)]
    if len(found) != 1:   # no choice named, or two different choices named: nothing is picked
        p["message"] = "Producer: I need a reply that contains the exact words of exactly one of the choices below.\n\n" + p["base"]
        t["decisions"].append({"time": now(), "after_step": t["step"], "question": p["question"] + " (reply not understood)",
                               "options": [o["text"] for o in p["options"]], "choice": None, "reply": reply.rstrip("\n")})
        return
    opt = found[0]
    if p["kind"] == "budget" and opt["dst"] is not None:
        nums = re.findall(r"\d+(?:\.\d+)?", reply)
        if not nums or float(nums[0]) <= spent(t):
            p["message"] = ("Producer: To raise the budget, write the new total in USD too, above the "
                            f"{spent(t):.2f} USD already spent (for example \"Raise the budget to 25\").\n\n" + p["base"])
            return
        record(t, p, reply.rstrip("\n"), f"{opt['text']} to {float(nums[0]):.2f} USD")
        t["budget_usd"] = float(nums[0])
        st["pending"] = None
        return
    record(t, p, reply.rstrip("\n"), opt["text"])
    st["pending"] = None
    if opt["dst"] is None:
        end_task(st, "ended by the user")
        return
    if p["kind"] in ("gate", "cap"):
        frm = p.get("diamond") or p["box"]
        ba = flow.cfg["budget_approved_by"]
        if p["kind"] == "gate" and p["diamond"] == ba["diamond"] and opt["text"].lower() == ba["arrow"].lower():
            t["budget_usd"] = t.get("proposed_budget_usd")
        if p["kind"] == "cap":   # the user chose: the loop cap starts counting again from here
            t.setdefault("loop_counts", {})[f"{frm}|{opt['text']}"] = 0
        move(flow, st, frm, opt["dst"])
    # "blocked": run the box again; "budget" handled above. Position is unchanged.


def move(flow, st, frm, dst):
    t = st["task"]
    if dst == flow.cfg["start_box"] and frm in flow.cfg["ask_opening_question_when_arriving_from"]:
        end_task(st, f"completed (arrow from {frm} to {dst}: a new goal starts a new task)")
        return
    t["position"] = dst


def advance(flow, st):
    """Run boxes until a question for the user is pending."""
    while st["pending"] is None:
        t = st["task"]
        if t is None or t["status"] != "open":
            note = f"(Task {t['id']} is {t['status']}.)" if t else ""
            st["pending"] = opening(flow, st, note)
            break
        pos = t["position"]
        if flow.nodes[pos]["shape"] == "diamond":
            st["pending"] = gate(flow, st, pos, t.get("last_result"))
            break
        if t.get("budget_usd") is not None and spent(t) >= t["budget_usd"]:
            st["pending"] = budget_reached(flow, st, pos)
            break
        ok, res, arrow = run_box(flow, st, pos)
        if not ok:
            st["pending"] = blocked(flow, st, pos, res)
            break
        cap = next((x for x in flow.cfg.get("loop_caps", [])
                    if x["box"] == pos and x["arrow"].lower() == flow.arrow_name(arrow).lower()), None)
        if cap:
            counts = t.setdefault("loop_counts", {})
            key = f"{pos}|{flow.arrow_name(arrow)}"
            if counts.get(key, 0) >= cap["max"]:
                t["last_result"] = res
                st["pending"] = cap_question(flow, st, pos, cap, res)
                break
            counts[key] = counts.get(key, 0) + 1
        t["last_result"] = res
        if pos == flow.cfg["budget_proposed_in_box"]:
            t["proposed_budget_usd"] = float(res["budget_usd"])
        move(flow, st, pos, arrow["dst"])
        save_state(st)
    if st["task"]:
        write_task_records(st["task"])
    save_state(st)


def show(text, code=0):
    write_atomic(MESSAGE_FILE, text.rstrip("\n") + "\n")
    print("=" * 20 + " message for the user (pipeline_state/message_for_user.md) " + "=" * 20)
    print(text)
    return code


def main(argv):
    os.chdir(ROOT)
    try:
        unpack_roles()
        flow = Flow()
        setup()
        if argv[1:2] == ["--check"]:
            print(f"OK: {len(flow.nodes)} places and {len(flow.arrows)} arrows read from {flow.cfg['diagram']}; "
                  "pipeline.json matches; Python, PyYAML, claude and timeout found.")
            return 0
        with Lock():
            st = load_state()
            if REPLY_FILE.exists():
                reply = REPLY_FILE.read_text(encoding="utf-8")
                arch = STATE_DIR / "replies" / f"{datetime.datetime.now(datetime.timezone.utc):%Y%m%dT%H%M%S%fZ}.txt"
                arch.parent.mkdir(parents=True, exist_ok=True)
                REPLY_FILE.replace(arch)
                if st["pending"] is None:
                    st["pending"] = opening(flow, st)
                    save_state(st)
                    return show("Producer: There was no question waiting for that reply, so nothing was done with it.\n\n"
                                + st["pending"]["message"])
                handle_reply(flow, st, reply)
                save_state(st)
            if st["pending"] is None:
                advance(flow, st)
            return show(st["pending"]["message"])
    except Stop as e:
        return show(f"Producer: The pipeline stopped before doing anything more, because of a problem with its setup:\n\n{e}", 2)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
