"""Generate src/stubs_generated.cpp: default-returning definitions for functions that are declared in headers but defined nowhere (so the demo always links).
usage: make_stubs.py  (reads logs/undefined_refs.json from the last build; demangles with c++filt)"""
import json, re, subprocess, sys, os
ROOT = "/tmp/claude-0/-home-user-Rep-One/17a752bb-9e88-5884-a5bf-3b7fc4ab8c87/scratchpad/mordor/code"
names = json.load(open(f"{ROOT}/logs/undefined_refs.json"))
print(len(names), "undefined symbols")
dem = subprocess.run(["c++filt"], input="\n".join(names), capture_output=True, text=True).stdout.splitlines()
json.dump(dem, open(f"{ROOT}/logs/undefined_demangled.json", "w"), indent=1)
print("\n".join(dem[:40]))
