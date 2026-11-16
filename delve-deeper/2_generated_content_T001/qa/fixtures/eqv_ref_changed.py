"""Reference state machine for the storeroom puzzle. Pure logic, stdlib only.
Version 2 (revision 1 of 2). Use-on-controls is approved by the user ("Approve Use"); everything else is PROPOSAL."""
import json
import os

_DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "storeroom_puzzle.json")

# Same strings as the World Designer's e08 solutions (minus "puzzle", which is the controls themselves).
OTHER_ROUTES = ("magic_key", "pick_lock", "break")


def load_data(path=_DATA):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


class StoreroomPuzzle:
    """States: idle/partial (progress list), solved, bypassed. Door e08 open iff door_open."""

    def __init__(self, data=None):
        self.data = data or load_data()
        self.solution = ["ctl_furnace", "ctl_cart", "ctl_pick"]
        self.controls = list(self.data["controls"])
        self.progress = []
        self.solved = False
        self.door_open = False
        self.opened_via = None
        self.wrong_count = 0
        self.xp_awarded = False

    @property
    def state(self):
        if self.solved:
            return "solved"
        if self.opened_via:
            return "bypassed"
        return "partial" if self.progress else "idle"

    def use(self, control):
        """'use' on a selected control. Returns (result_key, string_key)."""
        if control not in self.controls:
            raise ValueError("unknown control: %s" % control)
        if self.solved or self.opened_via:
            return "inert", "str.use.inert"
        if control == self.solution[len(self.progress)]:
            self.progress.append(control)
            if len(self.progress) == len(self.solution):
                self.solved = True
                self.door_open = True
                self.opened_via = "puzzle"
                return "solved", "str.use.solved"
            return "step_ok", "str.use.step_%d" % len(self.progress)
        # wrong: controls reset, nothing else lost; never blocks a retry
        self.progress = []
        self.wrong_count += 1
        return "wrong_reset", "str.use.wrong"

    def open_other(self, how):
        """Door e08 opened by 'magic_key', 'pick_lock' or 'break'; puzzle becomes inert.
        Any noise, damage or cost for 'break' is TBD and not modelled here."""
        if how not in OTHER_ROUTES:
            raise ValueError(how)
        if self.door_open:
            return False
        self.door_open = True
        self.opened_via = how
        self.progress = []
        return True

    def claim_xp(self):
        """If XP is awarded at all (PROPOSAL, GDD silent), at most once and only for a puzzle solve,
        never for a bypass. Amount TBD; returns an XP class key or None."""
        if self.solved and not self.xp_awarded:
            self.xp_awarded = True
            return self.data["xp_class"]
        return None

    def reset(self):
        """Designer/debug reset of partial progress only; a solved/open door never re-locks."""
        if not (self.solved or self.opened_via):
            self.progress = []

    def to_dict(self):
        return {"progress": list(self.progress), "solved": self.solved, "door_open": self.door_open,
                "opened_via": self.opened_via, "wrong_count": self.wrong_count, "xp_awarded": self.xp_awarded}

    @classmethod
    def from_dict(cls, d, data=None):
        p = cls(data)
        p.progress = list(d["progress"])
        p.solved = bool(d["solved"])
        p.door_open = bool(d["door_open"])
        p.opened_via = d["opened_via"]
        p.wrong_count = int(d["wrong_count"])
        p.xp_awarded = bool(d["xp_awarded"])
        return p
