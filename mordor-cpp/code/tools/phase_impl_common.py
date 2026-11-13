import os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
from cant import ROOT
from partsplit import SLUG
FOUND = {"types", "rng", "events", "ctx", "math_util", "json_lite", "game_data"}
def modules():
    """module name -> (part id, half) for every non-foundation, non-state header that exists."""
    out = {}
    for f in sorted(os.listdir(f"{ROOT}/include/mordor")):
        m = f[:-2]
        if not f.endswith(".h") or m in FOUND or m.startswith("state_"): continue
        mm = re.match(r"^([a-z]+?)(\d*)([ab]?)$", m)
        if not mm: continue
        slug, k, half = mm.groups()
        ch = [c for c, v in SLUG.items() if v == slug]
        if not ch: continue
        pid = f"c{ch[0]}p{k or 1}"
        out[m] = (pid, half)
    return out
