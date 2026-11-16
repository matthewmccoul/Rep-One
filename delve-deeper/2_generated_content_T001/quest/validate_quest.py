"""Validator for the Figure ore-lift quest. Standard library only.
Usage: python3 validate_quest.py            (real run; writes validate_quest.log)
       python3 validate_quest.py --log PATH (writes the log to PATH)
       python3 validate_quest.py <quest_dir> <map_json> (prints only; no log)"""
import csv, json, os, re, sys

BANNED = ["because", "in order to", "so that", "the reason", "my master", "my plan",
          "the dwarves were not", "not the makers", "did not build", "who built",
          "i need it for", "my purpose"]


def next_state(state, event, transitions):
    """Minimal quest state transition; unknown (state, event) keeps the state."""
    for t in transitions:
        if t["from"] == state and t["event"] == event:
            return t["to"]
    return state


def main(qdir, map_path):
    errs = []
    q = json.load(open(os.path.join(qdir, "figure_ore_lift_quest.json")))
    m = json.load(open(map_path))
    rows = list(csv.DictReader(open(os.path.join(qdir, "figure_ore_lift_strings.csv"), newline="")))
    ids = {n["id"] for n in m["nodes"]}
    edges = {e["id"] for e in m["edges"]}
    # map IDs
    for n in [q["giver"]["node"], q["destination"]["node"]] + q["route"]:
        if n not in ids:
            errs.append("unknown node " + n)
    for e in q["route_edges"]:
        if e not in edges:
            errs.append("unknown edge " + e)
    if m["quest_destinations"]["figure_task_destination"] != q["destination"]["node"]:
        errs.append("destination differs from map quest_destinations")
    # connected route
    pairs = {(e["from"], e["to"]) for e in m["edges"]} | {(e["to"], e["from"]) for e in m["edges"]}
    for a, b in zip(q["route"], q["route"][1:]):
        if (a, b) not in pairs:
            errs.append("route step not an edge: %s-%s" % (a, b))
    # keys agree
    csv_keys = [r["key"] for r in rows]
    if len(csv_keys) != len(set(csv_keys)):
        errs.append("duplicate csv keys")
    json_keys = {k for v in q["strings"].values() for k in v}
    if json_keys != set(csv_keys):
        errs.append("key mismatch json-only=%s csv-only=%s" % (sorted(json_keys - set(csv_keys)), sorted(set(csv_keys) - json_keys)))
    for k in re.findall(r"fig_\w+|lift_\w+|log_\w+", json.dumps(q["report_variants"])):
        if k not in csv_keys and not k.endswith("threshold"):
            errs.append("report variant key missing " + k)
    # blanks
    allowed = set(q["blanks_allowed"])
    for r in rows:
        found = set(re.findall(r"\{[a-z_]+\}", r["text"]))
        declared = set(x for x in re.findall(r"\{[a-z_]+\}", r["blanks"] or ""))
        if not found <= allowed:
            errs.append("undeclared blank in " + r["key"])
        if found != declared:
            errs.append("blanks column mismatch in " + r["key"])
        if not (-2 <= int(r["stage_min"]) <= int(r["stage_max"]) <= 2):
            errs.append("bad stage range " + r["key"])
        low = r["text"].lower()
        for b in BANNED:
            if b in low:
                errs.append("banned phrase '%s' in %s" % (b, r["key"]))
    # transitions self-tests
    tr = q["transitions"]
    states = set(q["states"])
    for t in tr:
        if t["from"] not in states or t["to"] not in states:
            errs.append("bad transition state " + str(t))
    tests = [
        ("not_offered", "accept", "active"),
        ("not_offered", "decline", "declined"),
        ("declined", "accept", "active"),
        ("active", "arrive_ore_lift_chain_present", "observed_present"),
        ("active", "arrive_ore_lift_chain_gone", "observed_gone"),
        ("observed_present", "report", "completed"),
        ("observed_gone", "report", "completed"),
        ("active", "report", "active"),
        ("not_offered", "report", "not_offered"),
        ("completed", "accept", "completed"),
    ]
    for s, e, want in tests:
        got = next_state(s, e, tr)
        if got != want:
            errs.append("selftest %s+%s -> %s, want %s" % (s, e, got, want))
    print("rows=%d keys_json=%d tests=%d" % (len(rows), len(json_keys), len(tests)))
    for e in errs:
        print("FAIL:", e)
    print("RESULT:", "PASS" if not errs else "FAIL")
    return 0 if not errs else 1


CAUSE_WORDS = ["taken", "stolen", "someone", "somebody", "before you", "removed", "took"]


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    args = sys.argv[1:]
    log_path = None
    if "--log" in args:
        i = args.index("--log")
        log_path = args[i + 1]
        del args[i:i + 2]
    elif not args:
        log_path = os.path.join(here, "validate_quest.log")  # real run only
    qdir = args[0] if len(args) > 0 else here
    map_path = args[1] if len(args) > 1 else os.path.join(here, "..", "map", "floor1_map.json")
    log = open(log_path, "w") if log_path else None

    class Tee:
        def write(self, s):
            sys.__stdout__.write(s)
            if log:
                log.write(s)

        def flush(self):
            sys.__stdout__.flush()
            if log:
                log.flush()

    sys.stdout = Tee()
    print("validate_quest.py Version 3 (revision 2 of 2)")
    print("log:", log_path if log_path else "none (not a no-argument run and no --log)")
    code = main(qdir, map_path)
    # gone-lines must state no cause and imply no actor
    bad = []
    for r in csv.DictReader(open(os.path.join(qdir, "figure_ore_lift_strings.csv"), newline="")):
        if "gone" in r["key"] or r["speaker"] == "mysterious_figure" and "gone" in r["text"].lower():
            for w in CAUSE_WORDS:
                if w in r["text"].lower():
                    bad.append("%s contains '%s'" % (r["key"], w))
    for b in bad:
        print("FAIL: cause/actor wording:", b)
    if bad:
        code = 1
        print("RESULT: FAIL")
    print("exit code", code)
    sys.stdout = sys.__stdout__
    if log:
        log.close()
    sys.exit(code)
