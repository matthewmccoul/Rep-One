#!/usr/bin/env python3
"""Assignment 5: goal-oriented coding agent for Delve Deeper.

The runner owns the control flow and the numbers. The model, called through `claude -p`, helps with the three jobs
that need judgement: reading the GDD against the code (scan), rating the evidence behind each score input, and
writing the code (implement).

    Read the scoped GDD requirements
    Scan the real project
    Repeat while required gaps remain
        Score the eligible gaps and choose one          (priority_score, below)
        Apply the agent's implementation
        Run the applicable build or behavior checks
        Rescan the updated code and record the result
    Report success once all scoped requirements pass

Standard library only. Run from the repository root:

    python3 delve-deeper/tools/assignment5/run_agent.py

Exit codes: 0 success (all requirements implemented and the build check passed); 1 a stage failed (model call,
invalid output); 2 blocked, stalled or out of retries (requirements left open); 3 all requirements scan as
implemented but no build check was configured, so the build is unverified.
"""
from __future__ import annotations

import argparse
import csv
import datetime
import difflib
import hashlib
import io
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
DEFAULT_ROOT = HERE.parents[1]  # the capstone workspace: delve-deeper/

EXIT_OK, EXIT_FAILED, EXIT_BLOCKED, EXIT_UNVERIFIED = 0, 1, 2, 3
STATUSES = ("implemented", "partial", "missing", "blocked")
OPEN_STATUSES = ("missing", "partial")

# Where the implement stage may write (relative to the workspace root). Anything else is reverted.
UE = "3_unreal_project/DelveDeeperTest/"
ALLOWED_EDIT_PREFIXES = (
    UE + "Source/",
    UE + "Plugins/DelveDeeperContent/Source/",
    UE + "Plugins/DelveDeeperContent/Content/Data/",
    UE + "Config/",
    UE + "Tests/",
)
EXCLUDED_DIRS = {"Binaries", "Intermediate", "Saved", "DerivedDataCache", ".git", "assignment5_runs", "__pycache__"}


# --------------------------------------------------------------------------------------------------------------
# Priority scoring
# --------------------------------------------------------------------------------------------------------------

W_IMPACT, W_URGENCY, W_UNBLOCKS = 3, 2, 1


def priority_score(impact: int, urgency: int, unblocks: int) -> int:
    """Rank a gap. Each input is a whole number 0..3. Returns 3*impact + 2*urgency + unblocks (range 0..18).

    impact:   how much the gap matters to the milestone's playable behavior
    urgency:  how soon the milestone cannot be tested or finished without it
    unblocks: how many other scoped requirements are waiting on it
    """
    for name, value in (("impact", impact), ("urgency", urgency), ("unblocks", unblocks)):
        if isinstance(value, bool) or not isinstance(value, int) or not 0 <= value <= 3:
            raise ValueError(f"{name} must be a whole number from 0 to 3, got {value!r}")
    return W_IMPACT * impact + W_URGENCY * urgency + W_UNBLOCKS * unblocks


def select_next(requirements: list[dict], results: dict[str, dict], exhausted=()) -> dict:
    """Pick the next gap to build.

    A gap is a requirement whose status is missing or partial. It is eligible only if every requirement it
    depends on is implemented; otherwise it is waiting, and we say what it waits for. Eligible gaps are sorted by
    score, highest first, with the requirement ID as a stable tiebreaker (lower ID wins).
    """
    candidates, waiting, skipped = [], [], []
    for req in requirements:
        rid = req["id"]
        res = results[rid]
        if res["status"] == "blocked":
            waiting.append({"id": rid, "waiting_on": [], "why": f"blocked: {res.get('summary', '')}"})
            continue
        if res["status"] not in OPEN_STATUSES:
            continue
        unmet = [d for d in req.get("depends_on", []) if results[d]["status"] != "implemented"]
        if unmet:
            waiting.append({"id": rid, "waiting_on": unmet, "why": "dependencies not implemented yet"})
            continue
        if rid in exhausted:
            skipped.append({"id": rid, "why": "retry limit reached"})
            continue
        candidates.append({
            "id": rid,
            "score": priority_score(res["impact"], res["urgency"], res["unblocks"]),
            "impact": res["impact"], "urgency": res["urgency"], "unblocks": res["unblocks"],
            "score_reason": res.get("score_reason", ""),
        })
    candidates.sort(key=lambda c: (-c["score"], c["id"]))
    return {"selected": candidates[0] if candidates else None, "candidates": candidates,
            "waiting": waiting, "skipped": skipped}


# --------------------------------------------------------------------------------------------------------------
# Scope, prompts, GDD
# --------------------------------------------------------------------------------------------------------------

class StageError(Exception):
    """A model call or its output was unusable. The stage failed; this is not the same as a build failure."""


def load_scope(path: Path) -> tuple[dict, str]:
    text = path.read_text(encoding="utf-8")
    m = re.search(r"```json\s*\n(.*?)\n```", text, re.S)
    if not m:
        raise StageError(f"{path}: no ```json requirements block found")
    scope = json.loads(m.group(1))
    ids = [r["id"] for r in scope["requirements"]]
    if len(ids) != len(set(ids)):
        raise StageError("scope has duplicate requirement IDs")
    for r in scope["requirements"]:
        for key in ("id", "title", "gdd_ref", "gdd_lines", "behavior", "depends_on"):
            if key not in r:
                raise StageError(f"requirement {r.get('id')} lacks '{key}'")
        for d in r["depends_on"]:
            if d not in ids:
                raise StageError(f"{r['id']} depends on unknown requirement {d}")
    return scope, text


def load_prompts(path: Path) -> dict[str, str]:
    text = path.read_text(encoding="utf-8")
    parts = re.split(r"^## ", text, flags=re.M)
    sections = {}
    for part in parts[1:]:
        title, _, body = part.partition("\n")
        sections[title.strip()] = body.strip()
    for need in ("SHARED RULES", "STAGE: SCAN", "STAGE: IMPLEMENT"):
        if need not in sections:
            raise StageError(f"{path}: missing section '{need}'")
    return sections


def gdd_excerpt(gdd_lines: list[str], ranges: list[list[int]]) -> str:
    out = []
    for lo, hi in ranges:
        out.append(f"[GDD lines {lo}-{hi}]")
        out.extend(gdd_lines[lo - 1:hi])
    return "\n".join(out)


def requirement_block(req: dict, gdd_lines: list[str]) -> str:
    return (f"REQUIREMENT_ID: {req['id']}\nTitle: {req['title']}\nGDD source: {req['gdd_ref']}\n"
            f"Depends on: {', '.join(req['depends_on']) or 'nothing'}\n"
            f"Required behavior: {req['behavior']}\n{gdd_excerpt(gdd_lines, req['gdd_lines'])}\n")


# --------------------------------------------------------------------------------------------------------------
# Calling Claude Code
# --------------------------------------------------------------------------------------------------------------

SCAN_SCHEMA = {
    "type": "object",
    "properties": {"results": {"type": "array", "items": {
        "type": "object",
        "properties": {
            "id": {"type": "string"},
            "status": {"type": "string", "enum": list(STATUSES)},
            "summary": {"type": "string"},
            "evidence": {"type": "array", "items": {
                "type": "object",
                "properties": {"path": {"type": "string"}, "symbol": {"type": "string"}, "note": {"type": "string"}},
                "required": ["path", "symbol", "note"]}},
            "impact": {"type": "integer", "minimum": 0, "maximum": 3},
            "urgency": {"type": "integer", "minimum": 0, "maximum": 3},
            "unblocks": {"type": "integer", "minimum": 0, "maximum": 3},
            "score_reason": {"type": "string"},
        },
        "required": ["id", "status", "summary", "evidence", "impact", "urgency", "unblocks", "score_reason"]}}},
    "required": ["results"],
}

IMPLEMENT_SCHEMA = {
    "type": "object",
    "properties": {
        "summary": {"type": "string"},
        "files_changed": {"type": "array", "items": {"type": "string"}},
        "connection": {"type": "string"},
        "unverified": {"type": "string"},
    },
    "required": ["summary", "files_changed", "connection", "unverified"],
}


def call_claude(claude_bin: str, prompt: str, cwd: Path, *, schema: dict, model: str | None, allowed_tools: str,
                permission_mode: str, max_turns: int, max_budget: float, timeout: int) -> dict:
    """Run one non-interactive Claude Code call and return the parsed, validated response."""
    cmd = shlex.split(claude_bin) + ["-p", "--output-format", "json", "--json-schema", json.dumps(schema),
                                     f"--allowedTools={allowed_tools}", "--permission-mode", permission_mode,
                                     "--max-turns", str(max_turns), "--max-budget-usd", str(max_budget)]
    if model:
        cmd += ["--model", model]
    try:
        proc = subprocess.run(cmd, input=prompt, capture_output=True, text=True, cwd=str(cwd), timeout=timeout)
    except FileNotFoundError as exc:
        raise StageError(f"cannot run '{claude_bin}': {exc}") from exc
    except subprocess.TimeoutExpired as exc:
        raise StageError(f"claude call timed out after {timeout}s") from exc
    if proc.returncode != 0:
        raise StageError(f"claude exited {proc.returncode}: {(proc.stderr or proc.stdout).strip()[-600:]}")
    try:
        data = json.loads(proc.stdout)
    except json.JSONDecodeError as exc:
        raise StageError(f"claude output is not JSON: {proc.stdout[:300]!r}") from exc
    if data.get("is_error"):
        raise StageError(f"claude reported an error: {str(data.get('result'))[:500]}")
    structured = data.get("structured_output")
    if not isinstance(structured, dict):
        raise StageError("claude response has no structured_output (missing result): "
                         f"subtype={data.get('subtype')!r} result={str(data.get('result'))[:300]!r}")
    return {"output": structured, "cost": float(data.get("total_cost_usd") or 0), "turns": data.get("num_turns"),
            "session_id": data.get("session_id"), "duration_ms": data.get("duration_ms"),
            "models": sorted((data.get("modelUsage") or {}).keys())}


# --------------------------------------------------------------------------------------------------------------
# Scan validation
# --------------------------------------------------------------------------------------------------------------

def _is_excluded(rel: str) -> bool:
    return any(part in EXCLUDED_DIRS for part in Path(rel).parts)


def verify_evidence(root: Path, item: dict) -> tuple[bool, str]:
    """An evidence item counts only if the file exists in the workspace and really contains the symbol."""
    rel, symbol = (item.get("path") or "").strip(), (item.get("symbol") or "").strip()
    if not rel:
        return False, "no path"
    target = (root / rel).resolve()
    try:
        target.relative_to(root.resolve())
    except ValueError:
        return False, "path outside workspace"
    if _is_excluded(rel):
        return False, "build output or cache directory"
    if not target.is_file():
        return False, "file does not exist"
    if not symbol:
        return False, "no symbol given"
    try:
        text = target.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        return False, f"cannot read: {exc}"
    return (symbol in text), ("symbol found" if symbol in text else "symbol not found in file")


def validate_scan(raw: dict, requirements: list[dict], root: Path) -> dict[str, dict]:
    """Turn the model's scan output into trusted per-requirement results, checking everything we rely on."""
    items = raw.get("results")
    if not isinstance(items, list):
        raise StageError("scan output has no 'results' list")
    by_id: dict[str, dict] = {}
    for it in items:
        rid = it.get("id")
        if rid in by_id:
            raise StageError(f"scan reports {rid} more than once")
        by_id[rid] = it
    want = [r["id"] for r in requirements]
    if set(by_id) != set(want):
        raise StageError(f"scan IDs {sorted(by_id)} do not match scope IDs {sorted(want)}")
    results = {}
    for rid in want:
        it = by_id[rid]
        if it.get("status") not in STATUSES:
            raise StageError(f"{rid}: invalid status {it.get('status')!r}")
        try:
            priority_score(it.get("impact"), it.get("urgency"), it.get("unblocks"))  # range check on the inputs
        except ValueError as exc:
            raise StageError(f"{rid}: invalid score input ({exc})") from exc
        evidence = []
        for ev in it.get("evidence", []):
            ok, why = verify_evidence(root, ev)
            evidence.append({**ev, "verified": ok, "check": why})
        status, note = it["status"], ""
        if status == "implemented" and not any(e["verified"] for e in evidence):
            status, note = "partial", "claimed implemented, but no evidence item was verified in the files; treated as partial"
        results[rid] = {"status": status, "model_status": it["status"], "summary": (it.get("summary") or "") + (f" [{note}]" if note else ""),
                        "evidence": evidence, "impact": it["impact"], "urgency": it["urgency"],
                        "unblocks": it["unblocks"], "score_reason": it.get("score_reason", "")}
    return results


# --------------------------------------------------------------------------------------------------------------
# File snapshots, diffs, checks
# --------------------------------------------------------------------------------------------------------------

def snapshot(root: Path, dest: Path) -> None:
    def ignore(_dir, names):
        return [n for n in names if n in EXCLUDED_DIRS]
    shutil.copytree(root, dest, ignore=ignore, dirs_exist_ok=True)


def _files(base: Path) -> dict[str, Path]:
    out = {}
    for p in base.rglob("*"):
        if p.is_file():
            rel = p.relative_to(base).as_posix()
            if not _is_excluded(rel):
                out[rel] = p
    return out


def _digest(p: Path) -> str:
    return hashlib.sha1(p.read_bytes()).hexdigest()


def diff_trees(before: Path, root: Path) -> tuple[list[dict], str]:
    """Return (changes, unified patch text) from the pre-change snapshot to the current workspace."""
    old, new = _files(before), _files(root)
    changes, patch = [], []
    for rel in sorted(set(old) | set(new)):
        if rel in old and rel in new:
            if _digest(old[rel]) == _digest(new[rel]):
                continue
            kind = "modified"
        else:
            kind = "added" if rel in new else "deleted"
        changes.append({"path": rel, "kind": kind})
        try:
            a = old[rel].read_text(encoding="utf-8").splitlines(keepends=True) if rel in old else []
            b = new[rel].read_text(encoding="utf-8").splitlines(keepends=True) if rel in new else []
            patch.extend(difflib.unified_diff(a, b, fromfile=f"a/{rel}" if rel in old else "/dev/null",
                                              tofile=f"b/{rel}" if rel in new else "/dev/null"))
        except UnicodeDecodeError:
            patch.append(f"Binary file {rel} {kind}\n")
    return changes, "".join(patch)


def revert_paths(before: Path, root: Path, paths: list[str]) -> None:
    for rel in paths:
        target, saved = root / rel, before / rel
        if saved.exists():
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(saved, target)
        elif target.exists():
            target.unlink()


def allowed(rel: str) -> bool:
    return rel.startswith(ALLOWED_EDIT_PREFIXES)


def _strip_cpp(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    text = re.sub(r'R"\((.*?)\)"', '""', text, flags=re.S)
    text = re.sub(r'"(\\.|[^"\\\n])*"', '""', text)
    return re.sub(r"'(\\.|[^'\\\n])*'", "' '", text)


def static_check(root: Path, changes: list[dict]) -> list[str]:
    """Cheap checks that run anywhere. They catch truncated or malformed output, not compile errors."""
    problems = []
    for ch in changes:
        if ch["kind"] == "deleted":
            continue
        rel, path = ch["path"], root / ch["path"]
        text = path.read_text(encoding="utf-8", errors="replace")
        if rel.endswith(".json"):
            try:
                json.loads(text)
            except json.JSONDecodeError as exc:
                problems.append(f"{rel}: invalid JSON ({exc})")
        elif rel.endswith(".csv"):
            try:
                rows = list(csv.reader(io.StringIO(text)))
                width = len(rows[0]) if rows else 0
                bad = [i + 1 for i, r in enumerate(rows) if r and len(r) != width]
                if bad:
                    problems.append(f"{rel}: rows with a different column count than the header: {bad[:5]}")
            except csv.Error as exc:
                problems.append(f"{rel}: invalid CSV ({exc})")
        elif rel.endswith((".cpp", ".h")):
            stripped = _strip_cpp(text)
            for open_c, close_c in ("{}", "()", "[]"):
                if stripped.count(open_c) != stripped.count(close_c):
                    problems.append(f"{rel}: unbalanced '{open_c}{close_c}' "
                                    f"({stripped.count(open_c)} vs {stripped.count(close_c)})")
            if rel.endswith(".h") and re.search(r"\b(UCLASS|USTRUCT|UENUM)\s*\(|GENERATED_BODY", text):
                includes = re.findall(r'^\s*#include\s+"([^"]+)"', text, flags=re.M)
                if not includes or not includes[-1].endswith(".generated.h"):
                    problems.append(f"{rel}: the .generated.h include must be the last #include")
    return problems


def run_build(build_cmd: str | None, root: Path, timeout: int) -> dict:
    """Run the configured build command. Returns state 'passed', 'failed' or 'not_run'."""
    if not build_cmd:
        return {"state": "not_run", "log": "No build command configured (--build-cmd or DD_BUILD_CMD); "
                                           "Unreal builds need UE 5.8 on Windows.", "errors": ""}
    try:
        proc = subprocess.run(build_cmd, shell=True, cwd=str(root), capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return {"state": "failed", "log": f"build timed out after {timeout}s", "errors": f"build timed out after {timeout}s"}
    log = (proc.stdout or "") + (proc.stderr or "")
    if proc.returncode == 0:
        return {"state": "passed", "log": log, "errors": ""}
    lines = log.splitlines()
    errs = [ln for ln in lines if re.search(r"\berror\b\s*(C\d+|:|LNK)|fatal error|Error:|error:", ln)]
    return {"state": "failed", "log": log, "errors": "\n".join((errs or lines)[-40:]) or f"exit code {proc.returncode}"}


# --------------------------------------------------------------------------------------------------------------
# Run log (a plain Markdown blackboard)
# --------------------------------------------------------------------------------------------------------------

class RunLog:
    def __init__(self, run_dir: Path):
        self.dir = run_dir
        run_dir.mkdir(parents=True, exist_ok=True)
        self.path = run_dir / "RUN_LOG.md"
        self.path.write_text("", encoding="utf-8")

    def write(self, text: str) -> None:
        with self.path.open("a", encoding="utf-8") as f:
            f.write(text.rstrip() + "\n\n")
        print(text.rstrip(), flush=True)

    def save(self, name: str, content) -> None:
        text = content if isinstance(content, str) else json.dumps(content, indent=2)
        (self.dir / name).write_text(text, encoding="utf-8")


def now() -> str:
    return datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def score_table(sel: dict) -> str:
    rows = ["| Requirement | Impact | Urgency | Unblocks | Score |", "|---|---|---|---|---|"]
    for c in sel["candidates"]:
        rows.append(f"| {c['id']} | {c['impact']} | {c['urgency']} | {c['unblocks']} | **{c['score']}** |")
    return "\n".join(rows)


# --------------------------------------------------------------------------------------------------------------
# The agent loop
# --------------------------------------------------------------------------------------------------------------

def run(args: argparse.Namespace) -> int:
    root = Path(args.project_root).resolve()
    scope, scope_text = load_scope(Path(args.scope))
    prompts = load_prompts(Path(args.prompts))
    gdd_lines = Path(args.gdd).read_text(encoding="utf-8").splitlines()
    reqs = scope["requirements"]
    run_id = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    log = RunLog(Path(args.runs_dir).resolve() / run_id)
    build_cmd = args.build_cmd or os.environ.get("DD_BUILD_CMD")
    total_cost = 0.0
    summary = {"run_id": run_id, "started": now(), "milestone": scope.get("milestone"), "iterations": [],
               "scans": [], "claude_calls": []}

    def call(stage: str, prompt: str, schema: dict, tools: str, mode: str, turns: int, timeout: int) -> dict:
        nonlocal total_cost
        res = call_claude(args.claude_bin, prompt, root, schema=schema, model=args.model, allowed_tools=tools,
                          permission_mode=mode, max_turns=turns, max_budget=args.budget_per_call, timeout=timeout)
        total_cost += res["cost"]
        summary["claude_calls"].append({"stage": stage, "cost_usd": res["cost"], "turns": res["turns"],
                                        "session_id": res["session_id"], "models": res["models"],
                                        "duration_ms": res["duration_ms"]})
        return res

    def finish(code: int, result: str, why: str) -> int:
        summary.update({"ended": now(), "result": result, "exit_code": code, "why": why,
                        "total_cost_usd": round(total_cost, 4)})
        log.save("summary.json", summary)
        log.write(f"## Final result: {result}\n\n{why}\n\nExit code {code}. Model cost this run: ${total_cost:.2f}. "
                  f"Artifacts: `{log.dir}`")
        return code

    log.write(f"# Assignment 5 agent run {run_id}\n\n- Started: {now()}\n- Milestone: {scope.get('milestone')}\n"
              f"- Workspace: `{root}`\n- GDD: `{args.gdd}`\n- Model: `{args.model}`\n"
              f"- Build command: `{build_cmd or 'none configured'}`\n- Limits: {args.max_iterations} iterations, "
              f"{args.max_attempts} attempts per requirement")
    log.write("## Scoped requirements read from the GDD\n\n" + "\n".join(
        f"- **{r['id']}** {r['title']} (GDD: {r['gdd_ref']}); depends on: {', '.join(r['depends_on']) or 'none'}" for r in reqs))

    attempts: dict[str, int] = {}
    last_failure: dict[str, str] = {}
    exhausted: set[str] = set()
    failing_check: set[str] = set()  # requirements whose last change left the build or static check failing
    stalls = 0
    build_state = "not_run"
    wrote_anything = False

    def scan(n: int) -> dict[str, dict]:
        ctx = [prompts["SHARED RULES"], prompts["STAGE: SCAN"], "---\nSCOPE FILE\n" + scope_text,
               "---\nREQUIREMENTS AND THEIR GDD TEXT\n" + "\n".join(requirement_block(r, gdd_lines) for r in reqs)]
        res = call(f"scan {n}", "\n\n".join(ctx), SCAN_SCHEMA, "Read,Grep,Glob", "default", 60, args.scan_timeout)
        results = validate_scan(res["output"], reqs, root)
        for rid, why in last_failure.items():
            if results[rid]["status"] == "implemented":
                results[rid].update(status="partial", summary=results[rid]["summary"] +
                                    f" [overridden: the last check for this change failed: {why[:200]}]")
        log.save(f"scan_{n:02d}.json", {"model_output": res["output"], "verified": results,
                                        "cost_usd": res["cost"], "models": res["models"]})
        summary["scans"].append({"n": n, "statuses": {k: v["status"] for k, v in results.items()}})
        lines = [f"## Scan {n} ({'starting scan' if n == 1 else 'rescan'})\n", "| Requirement | Status | Evidence verified | Summary |", "|---|---|---|---|"]
        for r in reqs:
            v = results[r["id"]]
            ok = sum(e["verified"] for e in v["evidence"])
            lines.append(f"| {r['id']} | **{v['status']}** | {ok}/{len(v['evidence'])} | {v['summary'][:220]} |")
        log.write("\n".join(lines))
        return results

    try:
        results = scan(1)
        iteration = 0
        while True:
            open_ids = [r["id"] for r in reqs if results[r["id"]]["status"] != "implemented"]
            if not open_ids:
                break
            sel = select_next(reqs, results, exhausted)
            log.save(f"scores_{iteration + 1:02d}.json", sel)
            lines = [f"## Scoring before step {iteration + 1}\n", "Score = 3 x impact + 2 x urgency + unblocks. "
                     "Ties go to the lower requirement ID.\n"]
            lines.append(score_table(sel) if sel["candidates"] else "No eligible candidates.")
            for w in sel["waiting"]:
                lines.append(f"- **{w['id']}** is waiting: {w['why']}"
                             + (f"; it needs {', '.join(w['waiting_on'])} to be implemented first" if w["waiting_on"] else ""))
            for s in sel["skipped"]:
                lines.append(f"- **{s['id']}** skipped: {s['why']}")
            if sel["selected"]:
                c = sel["selected"]
                lines.append(f"\n**Selected: {c['id']}** (score {c['score']}). Reason from the scan: {c['score_reason']}")
            log.write("\n".join(lines))
            if args.scan_only:
                return finish(EXIT_OK, "SCAN ONLY", f"Scan and scoring done; nothing was written. Next would be {sel['selected']['id'] if sel['selected'] else 'none'}.")
            if not sel["selected"]:
                blockers = "; ".join(f"{w['id']} waits on {w['waiting_on'] or w['why']}" for w in sel["waiting"]) or "retry limits reached"
                return finish(EXIT_BLOCKED, "BLOCKED", f"Requirements remain open ({', '.join(open_ids)}) but none is eligible: {blockers}.")
            if iteration >= args.max_iterations:
                return finish(EXIT_BLOCKED, "ITERATION LIMIT", f"Stopped after {iteration} steps with requirements still open: {', '.join(open_ids)}.")
            iteration += 1
            chosen = sel["selected"]["id"]
            req = next(r for r in reqs if r["id"] == chosen)
            res_before = results[chosen]

            ctx = [prompts["SHARED RULES"], prompts["STAGE: IMPLEMENT"], "---\nSCOPE FILE\n" + scope_text,
                   "---\nTHE REQUIREMENT TO IMPLEMENT NOW\n" + requirement_block(req, gdd_lines),
                   f"Scan status: {res_before['status']}\nScan summary: {res_before['summary']}\n"
                   "Scan evidence:\n" + "\n".join(f"- {e['path']} :: {e['symbol']} :: {e['note']}" for e in res_before["evidence"]),
                   "Other requirements still open (do NOT implement these now): " + ", ".join(i for i in open_ids if i != chosen)]
            if chosen in last_failure:
                ctx.append("---\nTHE PREVIOUS ATTEMPT AT THIS REQUIREMENT FAILED. Exact error:\n" + last_failure[chosen])
            before = Path(tempfile.mkdtemp(prefix="dd_a5_before_"))
            try:
                snapshot(root, before)
                log.write(f"## Step {iteration}: implement {chosen}\n\nAttempt {attempts.get(chosen, 0) + 1} of {args.max_attempts}. Calling Claude Code with write access.")
                impl = call(f"implement {chosen}", "\n\n".join(ctx), IMPLEMENT_SCHEMA, "Read,Grep,Glob,Edit,Write",
                            "acceptEdits", 80, args.implement_timeout)
                changes, patch = diff_trees(before, root)
                bad = [c["path"] for c in changes if not allowed(c["path"])]
                if bad:
                    revert_paths(before, root, bad)
                    changes, patch = diff_trees(before, root)
                    last_failure[chosen] = f"You edited files outside the allowed paths and they were reverted: {bad}"
                    log.write(f"**Rejected:** the change touched files outside the allowed paths ({bad}); those edits were reverted.")
            finally:
                shutil.rmtree(before, ignore_errors=True)
            attempts[chosen] = attempts.get(chosen, 0) + 1
            log.save(f"implement_{iteration:02d}.json", {"requirement": chosen, "model_output": impl["output"],
                                                         "changes": changes, "cost_usd": impl["cost"], "models": impl["models"]})
            log.save(f"diff_{iteration:02d}.patch", patch)
            step = {"step": iteration, "requirement": chosen, "score": sel["selected"]["score"],
                    "changed_files": changes, "check": None}
            summary["iterations"].append(step)
            if not changes:
                stalls += 1
                last_failure.setdefault(chosen, "the previous attempt changed no files")
                log.write(f"**No files changed** by the implementation of {chosen}. Its summary: {impl['output'].get('summary', '')[:300]}")
                if attempts[chosen] >= args.max_attempts:
                    exhausted.add(chosen)
                    if chosen in failing_check:
                        return finish(EXIT_BLOCKED, "BLOCKED", f"{chosen} still fails its check and the last attempt changed nothing. "
                                      "Stopping with the tree as the agent left it instead of stacking more changes on a failing build.")
                if stalls >= 2:
                    return finish(EXIT_BLOCKED, "STALLED", f"Two implementation attempts in a row changed nothing. {chosen} stays open.")
                continue
            stalls = 0
            wrote_anything = True
            log.write("**Files changed:**\n" + "\n".join(f"- `{c['path']}` ({c['kind']})" for c in changes) +
                      f"\n\nHow the model says the game reaches it: {impl['output'].get('connection', '')[:500]}\n\n"
                      f"Model says it could not verify: {impl['output'].get('unverified', '')[:400]}")

            problems = static_check(root, changes)
            if problems:
                chk = {"state": "failed", "errors": "static check: " + "; ".join(problems), "log": "\n".join(problems)}
            else:
                chk = run_build(build_cmd, root, args.build_timeout)
            build_state = chk["state"]
            log.save(f"check_{iteration:02d}.txt", chk["log"])
            step["check"] = chk["state"]
            label = {"passed": "build passed", "failed": "check FAILED", "not_run": "build not run (static checks passed)"}[chk["state"]]
            log.write(f"### Check after step {iteration}: {label}" + (f"\n\n```\n{chk['errors'][:1500]}\n```" if chk["errors"] else ""))
            if chk["state"] == "failed":
                last_failure[chosen] = chk["errors"][:3000]
                failing_check.add(chosen)
                if attempts[chosen] >= args.max_attempts:
                    exhausted.add(chosen)
                    return finish(EXIT_BLOCKED, "BLOCKED", f"{chosen} still fails its check after {attempts[chosen]} attempts, so it stays open. "
                                  "Stopping with the tree as the agent left it instead of stacking more changes on a failing build.\n\n"
                                  + chk["errors"][:1200])
            else:
                last_failure.pop(chosen, None)
                failing_check.discard(chosen)

            results = scan(iteration + 1)

        # No open gaps remain by the scan.
        if args.scan_only:
            return finish(EXIT_OK, "SCAN ONLY", "Scan done; every scoped requirement is already implemented. Nothing was written or built.")
        if not wrote_anything:
            log.write("## No gaps found\n\nEvery scoped requirement is already implemented, so nothing was written. Verifying the build once.")
            chk = run_build(build_cmd, root, args.build_timeout)
            build_state = chk["state"]
            log.save("check_final.txt", chk["log"])
            if chk["state"] == "failed":
                return finish(EXIT_BLOCKED, "BLOCKED", "All requirements scan as implemented but the build fails:\n" + chk["errors"][:1500])
        if build_state == "passed":
            return finish(EXIT_OK, "SUCCESS", "All scoped requirements are implemented with verified evidence and the build check passed."
                          + ("" if wrote_anything else " No files were written in this run."))
        return finish(EXIT_UNVERIFIED, "IMPLEMENTED, BUILD UNVERIFIED",
                      "All scoped requirements scan as implemented with verified evidence, but no build was run here, so the Unreal build "
                      "and in-game behavior are NOT verified. Run with --build-cmd on a machine with UE 5.8, then play DemoLevel.")
    except StageError as exc:
        return finish(EXIT_FAILED, "STAGE FAILED", f"{exc}")


def parse_args(argv=None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Assignment 5 goal-oriented coding agent for Delve Deeper.")
    p.add_argument("--project-root", default=str(DEFAULT_ROOT), help="capstone workspace (default: delve-deeper/)")
    p.add_argument("--gdd", default=str(DEFAULT_ROOT / "1_pipeline" / "08_Final_GDD_Extracted.txt"))
    p.add_argument("--scope", default=str(HERE / "scope.md"))
    p.add_argument("--prompts", default=str(HERE / "prompts.md"))
    p.add_argument("--runs-dir", default=str(DEFAULT_ROOT / "assignment5_runs"))
    p.add_argument("--model", default="sonnet", help="model alias or ID passed to claude --model")
    p.add_argument("--claude-bin", default="claude", help="the Claude Code executable (may include arguments)")
    p.add_argument("--build-cmd", default=None, help="command that builds the Unreal project (or set DD_BUILD_CMD)")
    p.add_argument("--max-iterations", type=int, default=8)
    p.add_argument("--max-attempts", type=int, default=2, help="attempts per requirement before giving up on it")
    p.add_argument("--budget-per-call", type=float, default=5.0, help="USD cap for each claude call")
    p.add_argument("--scan-timeout", type=int, default=1200)
    p.add_argument("--implement-timeout", type=int, default=2400)
    p.add_argument("--build-timeout", type=int, default=3600)
    p.add_argument("--scan-only", action="store_true", help="scan and score, write nothing")
    return p.parse_args(argv)


if __name__ == "__main__":
    sys.exit(run(parse_args()))
