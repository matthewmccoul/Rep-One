"""Summarize one run log and append a row to agent_design/tests/run_ledger.csv (orchestration v2).

Usage: log_summary.py <log> <agent> <label> <brief> <exit> <utc_ts> <staged> <agent_file> <agent_sha256>
                      <brief_sha256> <timeout_s>
Hashes are computed by the runner BEFORE the run so the row names the exact files that were executed.
A ledger with the v1 header is migrated in place (new columns appended; pre-v2 rows marked n/a).
"""
import csv, json, pathlib, re, sys

HEADER_V1 = ["label", "agent", "brief", "log", "exit", "is_error", "turns", "cost_usd",
             "input_tokens_total", "output_tokens", "session_id"]
NEW_COLS = ["utc_ts", "staged", "agent_file", "agent_sha256", "brief_sha256", "timeout_s"]
HEADER = HEADER_V1 + NEW_COLS
LEDGER = pathlib.Path("agent_design/tests/run_ledger.csv")


def migrate():
    if not LEDGER.exists():
        return
    rows = list(csv.reader(LEDGER.open(newline="")))
    if not rows or rows[0] == HEADER:
        return
    if rows[0] != HEADER_V1:
        sys.exit(f"run_ledger.csv has an unknown header {rows[0]}; refusing to append")
    out = [HEADER]
    for r in rows[1:]:
        m = re.search(r"__(\d{8}T\d{6}Z)\.json$", r[3])
        out.append(r + [m.group(1) if m else "n/a", "False", "n/a (pre-v2 row)",
                        "n/a (pre-v2 row)", "n/a (pre-v2 row)", "1500 (v1 default)"])
    tmp = LEDGER.with_suffix(".csv.tmp")
    with tmp.open("w", newline="") as f:
        csv.writer(f).writerows(out)
    tmp.replace(LEDGER)


def main(a):
    log, agent, label, brief, rc, ts, staged, afile, asha, bsha, tmo = a[:11]
    try:
        d = json.loads(pathlib.Path(log).read_text() or "{}")
    except (OSError, json.JSONDecodeError):
        d = {"is_error": True, "result": "unparseable or missing log"}
    u = d.get("usage") or {}
    row = [label, agent, brief, log, rc, d.get("is_error"), d.get("num_turns"), d.get("total_cost_usd"),
           u.get("input_tokens", 0) + u.get("cache_creation_input_tokens", 0) + u.get("cache_read_input_tokens", 0),
           u.get("output_tokens"), d.get("session_id"), ts, staged, afile, asha, bsha, tmo]
    migrate()
    new = not LEDGER.exists()
    with LEDGER.open("a", newline="") as f:
        w = csv.writer(f)
        if new:
            w.writerow(HEADER)
        w.writerow(row)
    denials = d.get("permission_denials") or []
    print(f"[{label}] agent={agent} exit={rc} error={d.get('is_error')} turns={d.get('num_turns')} "
          f"cost_usd={d.get('total_cost_usd')} staged={staged} permission_denials={len(denials)}")
    print((d.get("result") or "")[-1500:])


if __name__ == "__main__":
    if sys.argv[1:2] == ["--migrate"]:
        migrate()
    else:
        main(sys.argv[1:])
