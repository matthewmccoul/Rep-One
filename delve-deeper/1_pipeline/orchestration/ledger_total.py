"""Total the cost of runs in agent_design/tests/run_ledger.csv whose label starts with a prefix.

Usage: python3 -I orchestration/ledger_total.py <label-prefix> [--max <usd>]
Prints the total in USD. With --max, exits 1 when the total has reached <usd>, else 0.
Used by orchestration/invoke_agent.sh (budget guard) and run_pipeline.py. Standard library only.
A row whose cost is missing or not a number counts as 0 (the run's log was unreadable).
"""
import csv, pathlib, sys

LEDGER = pathlib.Path(__file__).resolve().parent.parent / "agent_design" / "tests" / "run_ledger.csv"


def total(prefix, ledger=LEDGER):
    t = 0.0
    if not ledger.exists():
        return t
    with ledger.open(newline="") as f:
        for row in csv.DictReader(f):
            if (row.get("label") or "").startswith(prefix):
                try:
                    t += float(row.get("cost_usd") or 0)
                except ValueError:
                    pass
    return t


def rows(prefix, ledger=LEDGER):
    if not ledger.exists():
        return []
    with ledger.open(newline="") as f:
        return [r for r in csv.DictReader(f) if (r.get("label") or "").startswith(prefix)]


if __name__ == "__main__":
    a = sys.argv[1:]
    if not a:
        print(__doc__, file=sys.stderr)
        sys.exit(2)
    t = total(a[0])
    print(f"{t:.4f}")
    if len(a) == 3 and a[1] == "--max":
        sys.exit(1 if t >= float(a[2]) else 0)
