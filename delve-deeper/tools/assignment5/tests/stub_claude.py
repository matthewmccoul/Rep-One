#!/usr/bin/env python3
"""A scripted stand-in (test double) for `claude -p`. It is used only by test_agent.py, never in a real agent run,
so the runner's own control flow can be tested instantly, for free and deterministically.

It reads the prompt on stdin and answers in the same JSON shape Claude Code uses (a `structured_output` field).
Behavior is scripted through $STUB_DIR/state.json:
  implemented: ids whose fake implementation file exists
  scores:      {id: [impact, urgency, unblocks]}
  mode:        ok | noop | outside | badbrace   (what the implement stage does)
  order:       ids in the order they were implemented
"""
import json
import os
import re
import sys
from pathlib import Path

args = sys.argv[1:]
prompt = sys.stdin.read()
state_path = Path(os.environ["STUB_DIR"]) / "state.json"
state = json.loads(state_path.read_text())
schema = args[args.index("--json-schema") + 1] if "--json-schema" in args else ""
SRC = "3_unreal_project/DelveDeeperTest/Source/DelveDeeperTest/"


def reply(obj):
    print(json.dumps({"is_error": False, "subtype": "success", "result": json.dumps(obj), "structured_output": obj,
                      "total_cost_usd": 0.01, "num_turns": 2, "session_id": "fake", "duration_ms": 5,
                      "modelUsage": {"claude-fake": {}}}))
    sys.exit(0)


if '"results"' in schema:  # scan stage
    ids = re.findall(r"REQUIREMENT_ID: (\S+)", prompt)
    out = []
    for rid in ids:
        done = rid in state["implemented"] and Path(SRC, f"Fake_{rid}.cpp").exists()
        i, u, b = state["scores"].get(rid, [1, 1, 1])
        ev = [{"path": SRC + f"Fake_{rid}.cpp", "symbol": f"Impl_{rid.replace('-', '_')}", "note": "fake"}] if done else \
             [{"path": "", "symbol": "", "note": "searched for nothing"}]
        if state.get("lie") and rid in state["lie"]:
            done, ev = True, [{"path": "nope/Missing.cpp", "symbol": "Nothing", "note": "invented evidence"}]
        out.append({"id": rid, "status": "implemented" if done else "missing", "summary": "fake scan", "evidence": ev,
                    "impact": i, "urgency": u, "unblocks": b, "score_reason": f"fake ratings {i}/{u}/{b}"})
    reply({"results": out})

m = re.search(r"THE REQUIREMENT TO IMPLEMENT NOW\s+REQUIREMENT_ID: (\S+)", prompt)
rid = m.group(1)
mode = state.get("mode", "ok")
if mode == "ok" or mode == "badbrace":
    body = f"// Impl_{rid.replace('-', '_')}\nvoid F() {{ }}\n" if mode == "ok" else "void F() {\n"
    Path(SRC, f"Fake_{rid}.cpp").parent.mkdir(parents=True, exist_ok=True)
    Path(SRC, f"Fake_{rid}.cpp").write_text(body)
    state["implemented"].append(rid)
elif mode == "outside":
    Path("docs").mkdir(exist_ok=True)
    Path("docs/outside.txt").write_text("not allowed")
state["order"].append(rid)
state_path.write_text(json.dumps(state))
reply({"summary": f"fake implementation of {rid}", "files_changed": [], "connection": "fake", "unverified": "everything"})
