"""Re-apply saved investigator outputs. usage: reapply.py <round> <cluster> [cluster...]   (GATE=1: gated, reverting on newly failing tests)"""
import json, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.argv, args = sys.argv[:1], sys.argv[1:]
import phase_investigate as pi
rnd, clusters = args[0], args[1:]
gate = gbase = None
if os.environ.get("GATE"):
    from gate import Gate
    gate = Gate(); gate.backup(); os.system(f"cd {pi.ROOT}/build && find {pi.ROOT}/src {pi.ROOT}/include {pi.ROOT}/tests -type f -exec touch {{}} + && ninja 2>&1 | tail -1"); gbase = gate.run()
    print("baseline: engine", len(gbase["engine"]), "srd", len(gbase["srd"]), flush=True)
for c in clusters:
    out = open(f"{pi.ROOT}/spec/investigate/r{rnd}_{c}.out").read()
    if gate is None: print(c, "patches applied:", pi.apply_patches(out)); continue
    gate.backup(); n = pi.apply_patches(out)
    if not n: print(c, "no patches"); continue
    after = gate.build_and_run(); new = gate.new_failures(gbase, after)
    if new:
        gate.restore(); print(c, "REVERTED, newly failing:", {k: v[:1] for k, v in list(new.items())[:6]}, flush=True)
        CF = f"{pi.ROOT}/logs/gate_conflicts.json"; cf = json.load(open(CF)) if os.path.exists(CF) else {}
        cf[c] = [f"{k}: " + "; ".join(v[:2]) for k, v in list(new.items())[:12]]; json.dump(cf, open(CF, "w"), indent=1)
    else: gbase = after; print(c, f"kept: engine failing {len(after['engine'])}, srd failing {len(after['srd'])}", flush=True)
