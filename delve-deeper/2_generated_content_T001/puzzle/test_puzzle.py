import itertools
import json
import os
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from puzzle_state_machine import StoreroomPuzzle, load_data, OTHER_ROUTES  # noqa: E402

LOG = os.path.join(HERE, "test_puzzle.log")


class _Tee:
    """Writes every line to the console and to the log file (the script writes the log itself)."""

    def __init__(self, f):
        self.f = f

    def write(self, s):
        sys.stdout.write(s)
        self.f.write(s)

    def flush(self):
        sys.stdout.flush()
        self.f.flush()

MAP = os.path.join(HERE, "..", "map", "floor1_map.json")
SOL = ["ctl_pick", "ctl_cart", "ctl_furnace"]


class T(unittest.TestCase):
    def test_all_orderings(self):
        for perm in itertools.permutations(SOL):
            p = StoreroomPuzzle()
            for c in perm:
                p.use(c)
            if list(perm) == SOL:
                self.assertTrue(p.door_open and p.solved)
            else:
                self.assertFalse(p.door_open, perm)

    def test_all_orderings_then_correct_still_works(self):
        # no softlock: after any 3 uses in any order, the right sequence still opens the door
        for perm in itertools.permutations(SOL):
            p = StoreroomPuzzle()
            for c in perm:
                p.use(c)
            if p.solved:
                continue
            # a leftover partial step may need one extra attempt; two full tries always suffice
            for _ in range(2):
                for c in SOL:
                    p.use(c)
            self.assertTrue(p.door_open, perm)

    def test_partial(self):
        p = StoreroomPuzzle()
        self.assertEqual(p.use("ctl_pick")[0], "step_ok")
        self.assertEqual(p.state, "partial")
        self.assertFalse(p.door_open)
        p.use("ctl_cart")
        self.assertFalse(p.door_open)

    def test_wrong_then_right(self):
        p = StoreroomPuzzle()
        p.use("ctl_pick")
        self.assertEqual(p.use("ctl_furnace")[0], "wrong_reset")
        self.assertEqual(p.progress, [])
        self.assertEqual(p.wrong_count, 1)
        for c in SOL:
            r = p.use(c)
        self.assertEqual(r[0], "solved")

    def test_repeat_same_control_is_wrong(self):
        p = StoreroomPuzzle()
        p.use("ctl_pick")
        self.assertEqual(p.use("ctl_pick")[0], "wrong_reset")

    def test_repeat_after_solve(self):
        p = StoreroomPuzzle()
        for c in SOL:
            p.use(c)
        for c in SOL + SOL[::-1]:
            self.assertEqual(p.use(c)[0], "inert")
        self.assertTrue(p.door_open)
        self.assertEqual(p.claim_xp(), "puzzle_solve")
        self.assertIsNone(p.claim_xp())

    def test_reset(self):
        p = StoreroomPuzzle()
        p.use("ctl_pick")
        p.reset()
        self.assertEqual(p.state, "idle")
        for c in SOL:
            p.use(c)
        p.reset()
        self.assertTrue(p.door_open)  # never re-locks

    def test_break_route(self):
        p = StoreroomPuzzle()
        self.assertTrue(p.open_other("break"))
        self.assertTrue(p.door_open)
        self.assertEqual(p.opened_via, "break")
        self.assertEqual(p.state, "bypassed")
        self.assertFalse(p.solved)
        for c in SOL:
            self.assertEqual(p.use(c), ("inert", "str.use.inert"))
        self.assertEqual(p.state, "bypassed")
        self.assertIsNone(p.claim_xp())

    def test_break_after_partial_progress(self):
        p = StoreroomPuzzle()
        p.use("ctl_pick")
        self.assertTrue(p.open_other("break"))
        self.assertEqual(p.progress, [])
        self.assertEqual(p.use("ctl_cart")[0], "inert")
        self.assertEqual(p.state, "bypassed")

    def test_break_after_solve_is_noop(self):
        p = StoreroomPuzzle()
        for c in SOL:
            p.use(c)
        self.assertFalse(p.open_other("break"))
        self.assertEqual(p.opened_via, "puzzle")
        self.assertEqual(p.state, "solved")

    def test_solve_after_break_is_noop(self):
        p = StoreroomPuzzle()
        p.open_other("break")
        for c in SOL:
            p.use(c)
        self.assertFalse(p.solved)
        self.assertEqual(p.opened_via, "break")

    def test_break_save_load(self):
        p = StoreroomPuzzle()
        p.open_other("break")
        q = StoreroomPuzzle.from_dict(json.loads(json.dumps(p.to_dict())))
        self.assertEqual(q.to_dict(), p.to_dict())
        self.assertEqual(q.state, "bypassed")
        self.assertEqual(q.opened_via, "break")
        self.assertEqual(q.use("ctl_pick")[0], "inert")

    def test_other_routes_strings_match_json(self):
        self.assertEqual(load_data()["other_routes"], ["magic_key", "pick_lock", "break"])
        self.assertEqual(list(OTHER_ROUTES), ["magic_key", "pick_lock", "break"])
        with self.assertRaises(ValueError):
            StoreroomPuzzle().open_other("puzzle")

    def test_other_routes(self):
        for how in ("magic_key", "pick_lock", "break"):
            p = StoreroomPuzzle()
            p.use("ctl_pick")
            self.assertTrue(p.open_other(how))
            self.assertEqual(p.state, "bypassed")
            self.assertEqual(p.use("ctl_cart")[0], "inert")
            self.assertIsNone(p.claim_xp())
        p = StoreroomPuzzle()
        for c in SOL:
            p.use(c)
        self.assertFalse(p.open_other("magic_key"))
        self.assertEqual(p.opened_via, "puzzle")

    def test_save_load(self):
        p = StoreroomPuzzle()
        p.use("ctl_pick")
        p.use("ctl_furnace")
        p.use("ctl_pick")
        q = StoreroomPuzzle.from_dict(json.loads(json.dumps(p.to_dict())))
        self.assertEqual(q.to_dict(), p.to_dict())
        q.use("ctl_cart")
        q.use("ctl_furnace")
        self.assertTrue(q.door_open)
        r = StoreroomPuzzle.from_dict(json.loads(json.dumps(q.to_dict())))
        self.assertEqual(r.state, "solved")

    def test_unknown_control(self):
        with self.assertRaises(ValueError):
            StoreroomPuzzle().use("ctl_x")

    def test_json_ids_in_map(self):
        d = load_data()
        m = json.load(open(MAP, encoding="utf-8"))
        nodes = {n["id"] for n in m["nodes"]}
        edges = {e["id"]: e for e in m["edges"]}
        for k in ("clue_site", "controls_site", "clue_room", "controls_room", "opens_room"):
            self.assertIn(d[k], nodes, k)
        self.assertIn(d["opens_edge"], edges)
        self.assertEqual(edges[d["opens_edge"]]["door"], "locked")
        self.assertEqual(m["puzzle"]["opens_edge"], d["opens_edge"])
        site = next(n for n in m["nodes"] if n["id"] == d["controls_site"])
        self.assertEqual(site["controls"], d["controls"])
        self.assertEqual(d["solution"], SOL)
        for o in d["objects"].values():
            self.assertIn(o["site"], nodes)
        for k in d["strings"]:
            self.assertTrue(k.startswith("str."))

    def test_clue_and_controls_reachable_with_e08_closed(self):
        m = json.load(open(MAP, encoding="utf-8"))
        d = load_data()
        adj = {}
        for e in m["edges"]:
            if e["id"] == d["opens_edge"]:
                continue
            adj.setdefault(e["from"], set()).add(e["to"])
            adj.setdefault(e["to"], set()).add(e["from"])
        seen, st = {"town_entrance"}, ["town_entrance"]
        while st:
            for n in adj.get(st.pop(), ()):
                if n not in seen:
                    seen.add(n)
                    st.append(n)
        self.assertIn(d["clue_room"], seen)
        self.assertIn(d["controls_room"], seen)
        self.assertNotIn("storeroom", seen)

    def test_string_keys_used_exist(self):
        d = load_data()
        p = StoreroomPuzzle()
        for c in SOL + ["ctl_pick", "ctl_pick"]:
            self.assertIn(p.use(c)[1], d["strings"])


if __name__ == "__main__":
    with open(LOG, "w", encoding="utf-8") as logf:
        out = _Tee(logf)
        out.write("test_puzzle.py run by python3; log written by this script (Version 3, revision 2 of 2)\n")
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(T)
        res = unittest.TextTestRunner(stream=out, verbosity=2).run(suite)
        out.write("EXIT CODE: %d\n" % (0 if res.wasSuccessful() else 1))
    sys.exit(0 if res.wasSuccessful() else 1)
