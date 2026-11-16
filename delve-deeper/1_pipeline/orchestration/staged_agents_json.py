"""Emit staged development candidates as a --agents JSON object (session-only, test use). Orchestration v2.

Uses the same strict parser as validate_agents.py. Any parse/validation error aborts with exit 1 and the
errors on stderr, so the runner never loads a silently mangled candidate.
"""
import json, pathlib, sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))  # python3 -I does not add the script dir to sys.path
from validate_agents import parse_agent_file, tools_of, validate_dir  # noqa: E402

CANDIDATES = HERE.parent / "agent_design" / "development_candidates"

names, errs = validate_dir(CANDIDATES, ["development-"])
if errs:
    print(json.dumps({"errors": errs}, indent=1), file=sys.stderr)
    sys.exit(1)
out = {}
for n, path in sorted(names.items()):
    fm, body, _ = parse_agent_file(path)
    agent = {"description": fm["description"], "prompt": body}
    if tools_of(fm):
        agent["tools"] = tools_of(fm)
    out[n] = agent
print(json.dumps(out))
