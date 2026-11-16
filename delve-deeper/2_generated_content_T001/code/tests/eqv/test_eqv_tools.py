#!/usr/bin/env python3
"""Tests of gen_golden.py, eqv_check and check_text_guard.py, including their broken-input behaviour.
  test_eqv_tools.py <case> <code_dir> <repo_root> <eqv_check exe> <scratch dir>
Cases: gen_and_match, gen_missing_reference, gen_depends_on_reference, gen_changed_code_variant,
       eqv_tampered, eqv_empty, eqv_changed_reference, eqv_bad_files, guard_real, guard_leaks.
Each case prints its checks and exits 0 only if all hold."""
import json
import os
import re
import shutil
import subprocess
import sys

case, CODE, ROOT, EXE, SCR = sys.argv[1:6]
GEN = os.path.join(CODE, "tools", "gen_golden.py")
GUARD = os.path.join(CODE, "tools", "check_text_guard.py")
REF = os.path.join(ROOT, "work", "T001", "puzzle", "puzzle_state_machine.py")
PUZ = os.path.join(ROOT, "work", "T001", "puzzle", "storeroom_puzzle.json")
os.makedirs(SCR, exist_ok=True)
bad = []


def run(*cmd):
    p = subprocess.run(list(cmd), capture_output=True, text=True)
    return p.returncode, p.stdout + p.stderr


def py(script, *a):
    return run(sys.executable, "-I", script, *a)


def check(name, cond, info=""):
    print(("ok   " if cond else "FAIL ") + name + ((" :: " + info.strip()[:300]) if info else ""))
    if not cond:
        bad.append(name)


def golden(name="golden.json"):
    out = os.path.join(SCR, name)
    rc, txt = py(GEN, out)
    check("generator exit 0 for " + name, rc == 0, txt)
    return out, txt


def changed_ref_dir(kind):
    """A copy of the reference whose solution order is ctl_furnace, ctl_cart, ctl_pick."""
    d = os.path.join(SCR, "ref_" + kind)
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    src = open(REF, encoding="utf-8").read()
    if kind == "data":  # data file beside the copy carries the changed order
        shutil.copy(REF, os.path.join(d, "puzzle_state_machine.py"))
        data = json.load(open(PUZ, encoding="utf-8"))
        data["solution"] = ["ctl_furnace", "ctl_cart", "ctl_pick"]
        json.dump(data, open(os.path.join(d, "storeroom_puzzle.json"), "w", encoding="utf-8"))
    else:  # code carries it; no data file beside the copy
        pat = 'self.solution = list(self.data["solution"])'
        assert pat in src
        src = src.replace(pat, 'self.solution = ["ctl_furnace", "ctl_cart", "ctl_pick"]')
        open(os.path.join(d, "puzzle_state_machine.py"), "w", encoding="utf-8").write(src)
    return os.path.join(d, "puzzle_state_machine.py")


if case == "gen_and_match":
    out, txt = golden()
    doc = json.load(open(out))
    c = doc["counts"]
    check("at least 120 sequence vectors, exactly 120", c["sequences"] == 120, str(c))
    check("bypass vectors from every reachable state and route", c["bypass"] >= 3 * 3, str(c))
    check("save/load vectors at each prefix", c["save_load"] >= 120, str(c))
    ids = [v["id"] for v in doc["vectors"]]
    check("vector ids unique", len(ids) == len(set(ids)))
    check("every vector has inputs, results, expected to_dict",
          all({"ops", "results", "expected"} <= set(v) and
              set(v["expected"]) == {"progress", "solved", "door_open", "opened_via", "wrong_count", "xp_awarded"}
              for v in doc["vectors"]))
    check("generator printed the count", re.search(r"OK %d vectors" % c["total"], txt) is not None, txt)
    rc, o = run(EXE, out)
    check("eqv_check exit 0", rc == 0, o)
    check("eqv_check says N vectors, 0 mismatches with N = generator count",
          o.strip().splitlines()[-1] == "%d vectors, 0 mismatches" % c["total"], o)
    out2 = os.path.join(SCR, "golden_again.json")
    py(GEN, out2)
    check("generator is deterministic", open(out, "rb").read() == open(out2, "rb").read())

elif case == "gen_missing_reference":
    out = os.path.join(SCR, "golden_bad.json")
    if os.path.exists(out):
        os.remove(out)
    rc, o = py(GEN, "--reference", os.path.join(SCR, "no_such_reference.py"), out)
    check("missing reference: exit 1 or 2", rc in (1, 2), o)
    check("missing reference: no output file", not os.path.exists(out))
    # unreadable/unusable references
    empty = os.path.join(SCR, "empty_ref.py")
    open(empty, "w").close()
    rc, o = py(GEN, "--reference", empty, out)
    check("empty reference: exit 1, no file", rc == 1 and not os.path.exists(out), o)
    broken = os.path.join(SCR, "broken_ref.py")
    open(broken, "w").write("def (:\n")
    rc, o = py(GEN, "--reference", broken, out)
    check("syntax-error reference: exit 1, no file", rc == 1 and not os.path.exists(out), o)
    nocls = os.path.join(SCR, "nocls_ref.py")
    open(nocls, "w").write("X = 1\n")
    rc, o = py(GEN, "--reference", nocls, out)
    check("reference without the class: exit 1, no file", rc == 1 and not os.path.exists(out), o)
    rc, o = py(GEN)
    check("no arguments: exit 2", rc == 2, o)

elif case == "gen_depends_on_reference":
    a, _ = golden("golden.json")
    for kind in ("data", "code"):
        ref = changed_ref_dir(kind)
        b = os.path.join(SCR, "golden_changed_%s.json" % kind)
        rc, o = py(GEN, "--reference", ref, b)
        check("changed reference (%s) generates, exit 0" % kind, rc == 0, o)
        rc, o = py(GEN, "--diff", a, b)
        check("changed reference (%s): golden differs (--diff exit 1)" % kind, rc == 1, o)
    rc, o = py(GEN, "--diff", a, a)
    check("--diff of a file with itself: exit 0", rc == 0, o)
    rc, o = py(GEN, "--diff", a, os.path.join(SCR, "nope.json"))
    check("--diff with a missing file: exit 2", rc == 2, o)

elif case == "eqv_tampered":
    a, _ = golden()
    doc = json.load(open(a))
    v = next(x for x in doc["vectors"] if x["id"] == "seq-004")
    v["expected"]["door_open"] = not v["expected"]["door_open"]
    t = os.path.join(SCR, "tampered.json")
    json.dump(doc, open(t, "w"))
    rc, o = run(EXE, t)
    check("tampered door_open: exit 1", rc == 1, o)
    check("names the vector id", "MISMATCH seq-004" in o, o)
    check("exactly one mismatch", o.strip().splitlines()[-1].endswith(", 1 mismatches"), o)
    # tamper a per-step result and a save_load snapshot as well
    doc = json.load(open(a))
    v = next(x for x in doc["vectors"] if x["id"] == "seq-040")
    v["results"][0]["string_key"] = "str.use.wrong"
    json.dump(doc, open(t, "w"))
    rc, o = run(EXE, t)
    check("tampered per-step string key: exit 1 naming seq-040", rc == 1 and "MISMATCH seq-040" in o, o)
    doc = json.load(open(a))
    v = next(x for x in doc["vectors"] if x["id"].startswith("saveload-0") and len(x["results"]) > 1 and x["results"][1].get("saved"))
    v["results"][1]["saved"]["wrong_count"] += 5
    json.dump(doc, open(t, "w"))
    rc, o = run(EXE, t)
    check("tampered save snapshot: exit 1 naming the vector", rc == 1 and "MISMATCH " + v["id"] in o, o)

elif case == "eqv_empty":
    for name, text, want in (("empty_vectors.json", '{"vectors":[]}', "FAIL NO_VECTORS"),
                             ("no_vectors_key.json", '{"format":1}', "FAIL GOLDEN_SHAPE"),
                             ("not_object.json", '[1,2]', "FAIL GOLDEN_SHAPE"),
                             ("vectors_not_array.json", '{"vectors":{}}', "FAIL GOLDEN_SHAPE"),
                             ("empty_file.json", "", "FAIL JSON_EMPTY"),
                             ("truncated.json", '{"vectors":[', "FAIL JSON_")):
        p = os.path.join(SCR, name)
        open(p, "w").write(text)
        rc, o = run(EXE, p)
        check("%s: exit 1, '%s'" % (name, want), rc == 1 and o.startswith(want), o)

elif case == "eqv_changed_reference":
    for kind in ("data", "code"):
        ref = changed_ref_dir(kind)
        b = os.path.join(SCR, "golden_changed_%s.json" % kind)
        rc, o = py(GEN, "--reference", ref, b)
        check("generate from changed reference (%s)" % kind, rc == 0, o)
        rc, o = run(EXE, b)
        m = re.search(r"(\d+) vectors, (\d+) mismatches", o)
        check("C++ keeps the real order: exit 1 with many mismatches (%s)" % kind,
              rc == 1 and m is not None and int(m.group(2)) > 50, o)

elif case == "eqv_bad_files":
    rc, o = run(EXE)
    check("no arguments: exit 2", rc == 2, o)
    rc, o = run(EXE, os.path.join(SCR, "does_not_exist.json"))
    check("missing golden file: exit 2", rc == 2, o)
    a, _ = golden()
    rc, o = run(EXE, a, "--puzzle", os.path.join(SCR, "no_puzzle.json"))
    check("missing puzzle file: exit 2", rc == 2, o)
    # golden vector built from a malformed vector (no ops): counted as a mismatch, not a crash
    p = os.path.join(SCR, "malformed_vector.json")
    open(p, "w").write('{"vectors":[{"id":"x1"}]}')
    rc, o = run(EXE, p)
    check("vector without ops: exit 1 naming x1", rc == 1 and "MISMATCH x1" in o, o)
    p = os.path.join(SCR, "no_id.json")
    open(p, "w").write('{"vectors":[{"ops":[]}]}')
    rc, o = run(EXE, p)
    check("vector without id: exit 1", rc == 1, o)
    # an explicit puzzle with a different solution order must fail the real golden file
    d = json.load(open(PUZ, encoding="utf-8"))
    d["solution"] = ["ctl_furnace", "ctl_cart", "ctl_pick"]
    alt = os.path.join(SCR, "alt_puzzle.json")
    json.dump(d, open(alt, "w"))
    rc, o = run(EXE, a, "--puzzle", alt)
    check("--puzzle with another order: exit 1", rc == 1, o)

elif case == "guard_real":
    rc, o = py(GUARD, PUZ)
    check("real puzzle file: exit 0", rc == 0, o)

elif case == "guard_leaks":
    base = json.load(open(PUZ, encoding="utf-8"))

    def variant(name, mutate, want_rc, want_text):
        d = json.loads(json.dumps(base))
        mutate(d)
        p = os.path.join(SCR, name)
        json.dump(d, open(p, "w"))
        rc, o = py(GUARD, p)
        check("%s: exit %d, '%s'" % (name, want_rc, want_text), rc == want_rc and want_text in o, o)

    variant("pz_text_leak.json", lambda d: d["strings"].__setitem__("str.carving.text", "The dwarves built this mine."),
            1, "str.carving.text")
    for w in ("dwarf", "builder", "built", "maker", "makers", "ancient", "colonist", "newcomer", "Dwarves", "ANCIENT"):
        variant("leak_%s.json" % w, lambda d, w=w: d["strings"].__setitem__("str.use.wrong", "Work of the %s." % w),
                1, "str.use.wrong")
    variant("leak_other_key.json", lambda d: d["strings"].__setitem__("str.log.door_other", "Newcomers left it open."),
            1, "str.log.door_other")
    variant("leak_plural.json", lambda d: d["strings"].__setitem__("str.label.pick", "Dwarfs' pick rack"), 1, "str.label.pick")
    variant("leak_invisible.json", lambda d: d["strings"].__setitem__("str.label.cart", "Anc­ient cart"), 1, "str.label.cart")
    variant("leak_fullwidth.json", lambda d: d["strings"].__setitem__("str.label.cart", "Ｂuilt cart"), 1, "str.label.cart")
    variant("leak_nested.json", lambda d: d.__setitem__("extra", {"str.note": ["fine", "the makers"]}), 1, "str.note")
    variant("word_inside_other_word_ok.json", lambda d: d["strings"].__setitem__("str.label.cart", "Rebuilt? No: Ore cart"), 0, "OK")
    variant("non_str_key_not_scanned.json", lambda d: d.__setitem__("note", "ancient"), 0, "OK")
    variant("non_string_value.json", lambda d: d["strings"].__setitem__("str.label.cart", 5), 1, "BAD_STRING")
    variant("no_strings.json", lambda d: d.pop("strings"), 1, "NO_STRINGS")
    variant("empty_strings.json", lambda d: d.__setitem__("strings", {}), 1, "NO_STRINGS")
    # raw text cases
    p = os.path.join(SCR, "dup_key.json")
    open(p, "w").write('{"strings":{"str.a":"The dwarves built this.","str.a":"fine"}}')
    rc, o = py(GUARD, p)
    check("duplicate key: exit 1 JSON_DUP_KEY", rc == 1 and "JSON_DUP_KEY" in o, o)
    p = os.path.join(SCR, "syntax.json")
    open(p, "w").write('{"strings":')
    rc, o = py(GUARD, p)
    check("broken JSON: exit 1 JSON_SYNTAX", rc == 1 and "JSON_SYNTAX" in o, o)
    p = os.path.join(SCR, "bad_utf8.json")
    open(p, "wb").write(b'{"strings":{"str.a":"\xff"}}')
    rc, o = py(GUARD, p)
    check("bad UTF-8: exit 1", rc == 1, o)
    rc, o = py(GUARD, os.path.join(SCR, "missing.json"))
    check("missing file: exit 2", rc == 2, o)
    rc, o = py(GUARD)
    check("no arguments: exit 2", rc == 2, o)

elif case == "guard_csv_real":
    QCSV = os.path.join(ROOT, "work", "T001", "quest", "figure_ore_lift_strings.csv")
    rc, o = py(GUARD, QCSV)
    check("real quest CSV: exit 0 (QST-17)", rc == 0, o)

elif case == "guard_csv_leaks":
    QCSV = os.path.join(ROOT, "work", "T001", "quest", "figure_ore_lift_strings.csv")
    real = open(QCSV, encoding="utf-8").read()
    old = "Come back and say.\","
    check("fixture anchor found in the real CSV", real.count(old) >= 1)

    def csv_variant(name, text, want_rc, want_text, also=()):
        p = os.path.join(SCR, name)
        open(p, "w", encoding="utf-8", newline="").write(text)
        rc, o = py(GUARD, p)
        ok = rc == want_rc and want_text in o and all(a in o for a in also)
        check("%s: exit %d, '%s'" % (name, want_rc, want_text), ok, o)

    # QST-18: one edit in fig_offer_cold (the first match is the fig_offer_cold row)
    csv_variant("q_reason.csv", real.replace(old, "Come back and say. I need it because the mine is mine.\",", 1),
                1, "TEXT_LEAK", ("fig_offer_cold", "because"))
    for w in ("dwarves", "built", "makers", "ancient", "newcomers", "the reason", "my master"):
        csv_variant("q_leak_%s.csv" % w.replace(" ", "_"),
                    real.replace(old, "Come back and say. Its %s.\"," % w, 1),
                    1, "TEXT_LEAK", ("fig_offer_cold", {"makers": "maker", "newcomers": "newcomer"}.get(w, w.split()[0])))  # names the list word
    csv_variant("q_leak_later_row.csv", real.rstrip("\n") + "\nfig_zz,mysterious_figure,-2,2,\"The dwarves built it.\",\n",
                1, "TEXT_LEAK", ("fig_zz",))
    csv_variant("q_key_column_not_scanned.csv", real.replace("fig_offer_cold,", "dwarves_key,", 1), 0, "OK")  # only `text` is scanned
    # malformed files: a different failure from a leak
    csv_variant("q_open_quote.csv", real + "fig_zz,mysterious_figure,0,0,\"never closed,\n", 1, "CSV_SYNTAX")
    csv_variant("q_ragged.csv", real.replace("fig_offer_cold,mysterious_figure,-1,0,", "fig_offer_cold,mysterious_figure,-1,", 1), 1, "CSV_RAGGED")
    csv_variant("q_no_text_col.csv", real.replace("key,speaker,stage_min,stage_max,text,blanks", "key,speaker,stage_min,stage_max,words,blanks", 1),
                1, "CSV_NO_COLUMN")
    csv_variant("q_dup_key.csv", real.rstrip("\n") + "\nfig_offer_cold,mysterious_figure,-1,0,\"fine\",\n", 1, "CSV_DUP_KEY")
    csv_variant("q_empty.csv", "", 1, "CSV_EMPTY")
    csv_variant("q_header_only.csv", "key,speaker,stage_min,stage_max,text,blanks\n", 1, "CSV_NO_ROWS")
    p = os.path.join(SCR, "q_open_quote.csv")
    rc, o = py(GUARD, p)
    check("malformed is not reported as a leak", "TEXT_LEAK" not in o, o)
    p = os.path.join(SCR, "q_bad_utf8.csv")
    open(p, "wb").write(b"key,text\nk,\xff\n")
    rc, o = py(GUARD, p)
    check("CSV not UTF-8: exit 1 CSV_SYNTAX", rc == 1 and "CSV_SYNTAX" in o, o)
    rc, o = py(GUARD, os.path.join(SCR, "missing.csv"))
    check("missing CSV: exit 2", rc == 2, o)
    # --words replaces the list in CSV mode
    rc, o = py(GUARD, QCSV, "--words", "chain")
    check("--words chain on the real CSV: exit 1 naming a key", rc == 1 and "TEXT_LEAK" in o and "fig_" in o, o)

else:
    print("unknown case " + case)
    sys.exit(2)

print("RESULT %s" % ("FAIL: " + ", ".join(bad) if bad else "all checks held"))
sys.exit(1 if bad else 0)
