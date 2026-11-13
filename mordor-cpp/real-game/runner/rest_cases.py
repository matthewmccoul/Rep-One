"""Rest in the city: edit hits and age with the character unloaded, load into the city, unload, read hits and age back.
Mordor.Core: days = round_half_even((100 - cur/max*100) / 3.25); Age += days; CurHits = MaxHits; spell points refilled."""
import json, sys
sys.path.insert(0, "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/real/runner"); from realgame import Game, S, start, crashed
def core_days(cur, mx):
    if mx <= 0 or cur >= mx: return 0
    x = (100 - cur / mx * 100) / 3.25
    r = round(x)   # Python round is half-even, like MidpointRounding.ToEven
    return r
cases = [(10, 77), (1, 77), (38, 77), (76, 77), (77, 77), (5, 15), (13, 15), (30, 60)]
g = Game(); start(g); out = open(f"{S}/real/runner/rest_run.out", "w")
for cur, mx in cases:
    g.set({"Hits": cur, "MaxHits": mx, "Age": 7300, "SpellPoints": 1, "Pack(1).Id": 0, "Pack(1).Ref": 0, "Pack(1).Equipped": 0, **{f"M586D({i})": 0 for i in range(36)}, "M5700": 0})
    g.load_and_run()
    if crashed(g): r = {"cur": cur, "max": mx, "real": "CRASH"}; start(g)
    else:
        g.city_to_menu(); g.unload(); s = g.read()
        r = {"cur": cur, "max": mx, "real_hits": int(s["Hits"]), "real_age_added": int(float(s["Age"])) - 7300, "real_sp": s.get("SpellPoints"),
             "core_hits": mx, "core_age_added": core_days(cur, mx)}
        r["match"] = r["real_hits"] == r["core_hits"] and r["real_age_added"] == r["core_age_added"]
    print(json.dumps(r), file=out, flush=True); print(json.dumps(r), flush=True)
