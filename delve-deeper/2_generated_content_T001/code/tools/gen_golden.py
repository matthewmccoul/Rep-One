#!/usr/bin/env python3
"""Golden-vector generator for the C++ equivalence check (EQV-02..04).

  gen_golden.py <out.json> [--reference <puzzle_state_machine.py>]
  gen_golden.py --diff <a.json> <b.json>      (exit 0 identical, 1 different, 2 unreadable)

Runs the Python reference and writes vectors. Each vector holds its inputs (`ops`) and what the
reference produced: a per-op result and the final `to_dict()`.
Exit 0 written; 1 reference cannot be used or generation failed (no file written);
2 usage or I/O error (no file written). The output file is written only after everything succeeded.
"""
import hashlib
import importlib.util
import itertools
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_REF = os.path.normpath(os.path.join(HERE, "..", "..", "puzzle", "puzzle_state_machine.py"))
REAL_DATA = os.path.normpath(os.path.join(HERE, "..", "..", "puzzle", "storeroom_puzzle.json"))
FORMAT = 1


class GenError(Exception):
    def __init__(self, code, msg):
        Exception.__init__(self, msg)
        self.code = code


def load_reference(path):
    if not os.path.isfile(path):
        raise GenError(2, "reference not found: %s" % path)
    try:
        with open(path, "rb") as f:
            raw = f.read()
    except OSError as e:
        raise GenError(2, "reference unreadable: %s (%s)" % (path, e))
    if not raw.strip():
        raise GenError(1, "reference is empty: %s" % path)
    sys.dont_write_bytecode = True
    try:
        spec = importlib.util.spec_from_file_location("golden_reference", path)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
    except Exception as e:  # any failure in the reference file is a failure of the generator
        raise GenError(1, "reference cannot be loaded: %s (%s: %s)" % (path, type(e).__name__, e))
    for name in ("StoreroomPuzzle", "OTHER_ROUTES", "load_data"):
        if not hasattr(mod, name):
            raise GenError(1, "reference lacks %s: %s" % (name, path))
    return mod, raw


def make_factory(mod):
    """Returns (factory, data_source). The reference reads storeroom_puzzle.json next to itself; when a
    copy of the reference sits where that file is absent, the real data file is passed in instead."""
    try:
        data = mod.load_data()
        src = "next to reference"
    except (OSError, ValueError):
        try:
            with open(REAL_DATA, encoding="utf-8") as f:
                data = json.load(f)
        except (OSError, ValueError) as e:
            raise GenError(2, "puzzle data unreadable: %s (%s)" % (REAL_DATA, e))
        src = "real data file (reference copy has none beside it)"
    return (lambda: mod.StoreroomPuzzle(json.loads(json.dumps(data)))), data, src


def run_ops(mod, factory, ops):
    p = factory()
    results = []
    for op in ops:
        kind = op["op"]
        try:
            if kind == "use":
                rk, sk = p.use(op["control"])
                results.append({"result": rk, "string_key": sk})
            elif kind == "open":
                results.append({"result": "applied" if p.open_other(op["route"]) else "not_applied"})
            elif kind == "claim_xp":
                results.append({"result": p.claim_xp()})
            elif kind == "reset":
                p.reset()
                results.append({"result": "ok"})
            elif kind == "save_load":
                saved = json.loads(json.dumps(p.to_dict()))
                p = mod.StoreroomPuzzle.from_dict(saved, p.data)
                results.append({"result": "ok", "saved": saved})
            else:
                raise GenError(1, "internal: unknown op %s" % kind)
        except ValueError:
            results.append({"result": "error"})
    return results, p.to_dict()


def build(mod, raw):
    factory, data, data_src = make_factory(mod)
    controls = list(data["controls"])
    routes = list(mod.OTHER_ROUTES)
    if not controls or not data.get("solution"):
        raise GenError(1, "reference data has no controls or solution")
    vectors = []

    def add(vid, kind, ops):
        results, final = run_ops(mod, factory, ops)
        vectors.append({"id": vid, "kind": kind, "ops": ops, "results": results, "expected": final})

    uses = lambda seq: [{"op": "use", "control": c} for c in seq]

    # 1. every control sequence of length 1..4 (3+9+27+81 = 120), then claim_xp twice.
    seqs = []
    for n in range(1, 5):
        seqs.extend(itertools.product(controls, repeat=n))
    for i, s in enumerate(seqs):
        add("seq-%03d" % (i + 1), "sequence", uses(s) + [{"op": "claim_xp"}, {"op": "claim_xp"}])
    n_seq = len(seqs)

    # 2. reachable states (prefixes of length 0..4, one representative per distinct to_dict).
    reps = {}
    for n in range(0, 5):
        for s in itertools.product(controls, repeat=n):
            _, d = run_ops(mod, factory, uses(s))
            reps.setdefault(json.dumps(d, sort_keys=True), s)
    states = sorted(reps.values(), key=lambda s: (len(s), s))

    # 3. each bypass route from each reachable state, then a second route, one more press, claim_xp.
    n_byp = 0
    for si, s in enumerate(states):
        for r in routes:
            ops = uses(s) + [{"op": "open", "route": r}]
            ops += [{"op": "open", "route": r2} for r2 in routes] + uses(controls[:1]) + [{"op": "claim_xp"}]
            add("bypass-%03d-%s" % (si + 1, r), "bypass", ops)
            n_byp += 1

    # 4. save and load at each prefix of every sequence (length 1..4): the state after each prefix goes through
    #    to_dict/from_dict, then the rest of the sequence continues on the loaded puzzle.
    n_sl = 0
    for i, s in enumerate(seqs):
        ops = []
        for k, c in enumerate(s):
            ops.append({"op": "use", "control": c})
            ops.append({"op": "save_load"})
        add("saveload-%03d" % (i + 1), "save_load", ops)
        n_sl += 1
    # save/load of bypassed and solved states, then a press.
    for si, s in enumerate(states):
        for r in routes:
            add("saveload-bypass-%03d-%s" % (si + 1, r), "save_load",
                uses(s) + [{"op": "open", "route": r}, {"op": "save_load"}] + uses(controls[:1]) + [{"op": "claim_xp"}])
            n_sl += 1
    add("saveload-solved", "save_load", uses(data["solution"]) + [{"op": "save_load"}, {"op": "claim_xp"}, {"op": "save_load"},
                                                                  {"op": "claim_xp"}])
    n_sl += 1

    # 5. edge cases: unknown control and route (the reference raises; the C++ returns a code and changes nothing).
    add("edge-unknown-control", "edge", uses(controls[:1]) + [{"op": "use", "control": "ctl_unknown"}] + uses(controls[1:2]))
    add("edge-unknown-route", "edge", uses(controls[:1]) + [{"op": "open", "route": "kick"}] + uses(controls[1:2]))
    add("edge-puzzle-as-route", "edge", [{"op": "open", "route": "puzzle"}])
    add("edge-reset", "edge", uses(controls[:2]) + [{"op": "reset"}] + uses(controls[:1]))
    add("edge-reset-after-solve", "edge", uses(data["solution"]) + [{"op": "reset"}, {"op": "claim_xp"}])
    add("edge-reset-after-bypass", "edge", [{"op": "open", "route": routes[0]}, {"op": "reset"}] + uses(controls[:1]))
    n_edge = 6

    return {
        "format": FORMAT,
        "reference_sha256": hashlib.sha256(raw).hexdigest(),
        "data_source": data_src,
        "controls": controls,
        "solution": list(data["solution"]),
        "routes": routes,
        "counts": {"sequences": n_seq, "bypass": n_byp, "save_load": n_sl, "edge": n_edge, "total": len(vectors)},
        "vectors": vectors,
    }


def diff_files(a, b):
    try:
        with open(a, "rb") as f:
            da = f.read()
        with open(b, "rb") as f:
            db = f.read()
    except OSError as e:
        print("ERROR cannot read: %s" % e)
        return 2
    if da == db:
        print("IDENTICAL")
        return 0
    try:
        ja, jb = json.loads(da), json.loads(db)
        va = {v["id"]: v for v in ja.get("vectors", [])}
        vb = {v["id"]: v for v in jb.get("vectors", [])}
        n = sum(1 for k in va if k in vb and va[k] != vb[k]) + len(set(va) ^ set(vb))
        print("DIFFERENT: %d vectors differ" % n)
    except (ValueError, AttributeError, KeyError, TypeError):
        print("DIFFERENT")
    return 1


def main(argv):
    if len(argv) >= 2 and argv[0] == "--diff":
        if len(argv) != 3:
            print("usage: gen_golden.py --diff <a.json> <b.json>")
            return 2
        return diff_files(argv[1], argv[2])
    ref = DEFAULT_REF
    rest = []
    i = 0
    while i < len(argv):
        if argv[i] == "--reference":
            if i + 1 >= len(argv):
                print("usage: gen_golden.py <out.json> [--reference <path>]")
                return 2
            ref = argv[i + 1]
            i += 2
        else:
            rest.append(argv[i])
            i += 1
    if len(rest) != 1:
        print("usage: gen_golden.py <out.json> [--reference <path>]")
        return 2
    out = rest[0]
    try:
        mod, raw = load_reference(ref)
        doc = build(mod, raw)
    except GenError as e:
        print("FAIL GENERATOR: %s" % e)
        return e.code
    except Exception as e:
        print("FAIL GENERATOR: %s: %s" % (type(e).__name__, e))
        return 1
    if doc["counts"]["sequences"] < 1 or not doc["vectors"]:
        print("FAIL NO_VECTORS: nothing generated")
        return 1
    try:
        os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
        tmp = out + ".part"
        with open(tmp, "w", encoding="utf-8", newline="\n") as f:
            json.dump(doc, f, indent=None, sort_keys=True, separators=(",", ":"))
            f.write("\n")
        os.replace(tmp, out)
    except OSError as e:
        print("ERROR cannot write %s: %s" % (out, e))
        return 2
    c = doc["counts"]
    print("OK %d vectors (sequences=%d bypass=%d save_load=%d edge=%d) written to %s" %
          (c["total"], c["sequences"], c["bypass"], c["save_load"], c["edge"], out))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
