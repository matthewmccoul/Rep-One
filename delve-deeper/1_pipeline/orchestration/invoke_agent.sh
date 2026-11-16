#!/usr/bin/env bash
# Run one named agent as an independent headless Claude Code session (separate process, separate log).
# Orchestration v2 (definition-software-engineer; fixes listed in agent_design/versions/orchestration_v2/CHANGELOG.md).
# Usage: orchestration/invoke_agent.sh <agent-name> <task-brief.md> <run-label> [--staged] [--timeout <s>]
#   --staged : load agents from agent_design/development_candidates/ via --agents (session-only;
#              does NOT activate them in .claude/agents/). Definition-readiness TEST use only.
#   --timeout: wall-clock limit for THIS run. Default: DD_TIMEOUT_S if set, else 3600 for an outer run,
#              1500 for a nested run. DD_TIMEOUT_S is NOT passed to nested runs; each nested run is
#              also capped by the outer run's deadline (DD_RUNNER_DEADLINE) so it cannot outlive its parent.
# Before every run the relevant agent set is validated (validate_agents.py, with a cross-set name check);
# any error aborts the run with the validator report on stderr.
# Log: agent_design/tests/logs/<run-label>__<UTC timestamp>.json (never overwritten).
# Ledger: one row per run in agent_design/tests/run_ledger.csv, incl. UTC time, staged flag and the SHA-256 of
#         the agent file and brief computed before the run.
# Permissions: no permission-mode override; tools granted only via orchestration/allowlists.json.
# Optional env: DD_MAX_BUDGET_USD (passes --max-budget-usd; only if the user set a budget).
# Pipeline guard (added 2026-11-08 for the content pipeline): every run must come from run_pipeline.py.
#   DD_STEP_LAUNCH=1     set only by run_pipeline.py when it starts the Producer for one box of the diagram;
#                        that launch may only start development-producer.
#   DD_ALLOWED_AGENTS    the roles the Producer may dispatch in that box (pipeline.json); inherited by the
#                        Producer's commands. Roles the Producer starts get an empty list, so they dispatch nobody.
#   DD_LABEL_PREFIX      every label must start with it (task and step), so the ledger maps each run to its box.
#   DD_TASK_PREFIX, DD_TASK_BUDGET_USD  refuse to start a run once the task's ledger total reaches the budget
#                        the user approved.
#   A run without these variables is refused (exit 6).
set -euo pipefail
cd "$(dirname "$0")/.."
usage() { echo "usage: $0 <agent> <brief> <label> [--staged] [--timeout <s>]" >&2; exit 2; }
[[ $# -ge 3 ]] || usage
AGENT="$1"; BRIEF="$2"; LABEL="$3"; shift 3
STAGED=False; TMO_ARG=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --staged) STAGED=True; shift ;;
    --timeout) [[ $# -ge 2 && "$2" =~ ^[0-9]+$ ]] || usage; TMO_ARG="$2"; shift 2 ;;
    *) usage ;;
  esac
done
[[ -f "$BRIEF" ]] || { echo "brief not found: $BRIEF" >&2; exit 2; }
[[ "$LABEL" =~ ^[A-Za-z0-9._-]+$ ]] || { echo "bad label" >&2; exit 2; }

# --- pipeline guard: only run_pipeline.py's Producer, and only the roles allowed in the current box ---
guard() { echo "pipeline guard: refused: $*" >&2; exit 6; }
MODE=work   # work = a normal job; consult = a read-only consultation; flag = a high-cost-flag assessment
if [[ "${DD_STEP_LAUNCH:-}" == "1" ]]; then
  [[ "$AGENT" == "development-producer" ]] || guard "run_pipeline.py may only launch development-producer"
  CHILD_ALLOWED="${DD_ALLOWED_AGENTS:-}"; CHILD_CONSULT="${DD_CONSULT_AGENTS:-}"; CHILD_FLAG="${DD_FLAG_AGENTS:-}"   # the Producer gets the lists; its own children get none
else
  [[ -n "${DD_ALLOWED_AGENTS+x}" ]] || guard "this run was not started by run_pipeline.py"
  case " ${DD_ALLOWED_AGENTS} " in
    *" ${AGENT} "*) ;;
    *) if [[ " ${DD_CONSULT_AGENTS:-} " == *" ${AGENT} "* && "$LABEL" == *-consult-* ]]; then MODE=consult
       elif [[ " ${DD_FLAG_AGENTS:-} " == *" ${AGENT} "* && "$LABEL" == *-flag-* ]]; then MODE=flag
       else guard "${AGENT} may not work in this box of the diagram (allowed here: ${DD_ALLOWED_AGENTS:-nobody}; consultations need a label containing -consult-, flag assessments -flag-)"; fi ;;
  esac
  CHILD_ALLOWED=""; CHILD_CONSULT=""; CHILD_FLAG=""
fi
[[ -n "${DD_LABEL_PREFIX:-}" && "$LABEL" == "${DD_LABEL_PREFIX}"* ]] || guard "label must start with ${DD_LABEL_PREFIX:-<unset>}"
if [[ -n "${DD_TASK_BUDGET_USD:-}" ]]; then
  python3 -I orchestration/ledger_total.py "${DD_TASK_PREFIX:?}" --max "$DD_TASK_BUDGET_USD" >/dev/null \
    || guard "this task has reached its approved budget of ${DD_TASK_BUDGET_USD} USD"
fi

# --- timeout: outer default 3600s; nested runs never inherit the parent's DD_TIMEOUT_S ---
DEPTH="${DD_RUNNER_DEPTH:-0}"
if [[ "$DEPTH" == "0" ]]; then DEF_TMO=3600; else DEF_TMO=1500; fi
TMO="${TMO_ARG:-${DD_TIMEOUT_S:-$DEF_TMO}}"
NOW="$(date -u +%s)"
if [[ -n "${DD_RUNNER_DEADLINE:-}" ]]; then
  LEFT=$(( DD_RUNNER_DEADLINE - NOW - 60 ))
  (( LEFT > 0 )) || { echo "parent run deadline reached; refusing nested run" >&2; exit 4; }
  (( TMO > LEFT )) && TMO=$LEFT
fi
DEADLINE=$(( NOW + TMO ))

# --- validation of the agent set this run will load ---
ACTIVE=.claude/agents; CAND=agent_design/development_candidates
run_validator() {  # args: validator args...
  local out
  if ! out="$(python3 -I orchestration/validate_agents.py "$@")"; then
    echo "agent validation failed; run aborted:" >&2; echo "$out" >&2; exit 5
  fi
}
EXTRA=()
if [[ "$STAGED" == "True" ]]; then
  run_validator "$CAND" development- --against "$ACTIVE"
  AGENTS_JSON="$(python3 -I orchestration/staged_agents_json.py)"
  EXTRA+=(--agents "$AGENTS_JSON")
  if [[ -f "$CAND/${AGENT}.md" ]]; then AFILE="$CAND/${AGENT}.md"; else AFILE="$ACTIVE/${AGENT}.md"; fi
  KEY="$AGENT"; python3 -I -c "import json,sys;a=json.load(open('orchestration/allowlists.json'));sys.exit(0 if sys.argv[1] in a else 1)" "$KEY" || KEY="_staged_default"
else
  [[ -f "$ACTIVE/${AGENT}.md" ]] || { echo "agent not active: $AGENT" >&2; exit 2; }
  run_validator "$ACTIVE" definition-,development- --against "$CAND"
  AFILE="$ACTIVE/${AGENT}.md"; KEY="$AGENT"
fi
[[ -f "$AFILE" ]] || { echo "agent file not found for $AGENT" >&2; exit 2; }
ASHA="$(sha256sum "$AFILE" | cut -d' ' -f1)"; BSHA="$(sha256sum "$BRIEF" | cut -d' ' -f1)"
ALLOW="$(python3 -I -c "import json,sys;a=json.load(open('orchestration/allowlists.json'));print(a.get(sys.argv[1], a['_test_default']))" "$KEY")"
CAP="${DD_MAX_BUDGET_USD:-}"
[[ "$MODE" == "consult" ]] && CAP="${DD_CONSULT_BUDGET_USD:-0.30}"   # a consultation is a few questions, not a job
[[ -n "$CAP" ]] && EXTRA+=(--max-budget-usd "$CAP")
MODEL="$(python3 -I -c "import json,sys;m=json.load(open('orchestration/models.json'));print(m.get(sys.argv[1], m['_default']))" "$AGENT")"

TS="$(date -u +%Y%m%dT%H%M%SZ)"
LOG="agent_design/tests/logs/${LABEL}__${TS}.json"
[[ -e "$LOG" ]] && { echo "log exists, refusing overwrite: $LOG" >&2; exit 3; }
PROMPT="You are running as agent '${AGENT}'. Task label: ${LABEL}. Read and execute the task brief at ${BRIEF}. Write outputs only to the paths the brief names. You cannot ask the user anything and you cannot record user approvals. End with: STATUS (done|blocked|needs-revision), OUTPUTS (paths), ISSUES."
if [[ "$MODE" == "consult" ]]; then
  PROMPT="You are being CONSULTED as agent '${AGENT}', not given a job. Read the consultation brief at ${BRIEF}. You have read-only tools; write no files. The Producer sees no job for you in this task and asks whether you disagree. Answer briefly and end with exactly one line: CONSULT-REPLY: AGREE (no job for me now) or CONSULT-REPLY: DISAGREE (job: <what you would do and why>)."
fi
[[ "$MODE" == "consult" ]] && ALLOW="Read Glob Grep"
set +e
set -C   # noclobber: the redirect itself refuses to overwrite an existing log
env -u DD_TIMEOUT_S -u DD_STEP_LAUNCH DD_ALLOWED_AGENTS="$CHILD_ALLOWED" DD_CONSULT_AGENTS="$CHILD_CONSULT" DD_FLAG_AGENTS="$CHILD_FLAG" \
  DD_RUNNER_DEPTH=$(( DEPTH + 1 )) DD_RUNNER_DEADLINE="$DEADLINE" \
  timeout --kill-after=30 "$TMO" claude -p "${EXTRA[@]}" --model "$MODEL" --agent "$AGENT" --allowedTools "$ALLOW" \
  --output-format json "$PROMPT" > "$LOG"
RC=$?
set +C
set -e
python3 -I orchestration/log_summary.py "$LOG" "$AGENT" "$LABEL" "$BRIEF" "$RC" "$TS" "$STAGED" "$AFILE" "$ASHA" "$BSHA" "$TMO"
exit $RC
