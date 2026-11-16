"""Split a FILE-header bundle back into files (standard library only).

Usage: python3 -I split_bundle.py <bundle.md> <project_root>

Bundles in this submission mark each file with a line:
    =================== FILE: <path> ===================
Paths beginning with "development_candidates/" are written under <project_root>/agent_design/;
every other path is written under <project_root> as given.
Example: python3 -I split_bundle.py agents_ten_development_roles_bundle.md .
"""
import re, sys, pathlib

bundle, root = sys.argv[1], sys.argv[2]
t = pathlib.Path(bundle).read_text(encoding="utf-8")
parts = re.split(r"^=================== FILE: (.+?) ===================\n", t, flags=re.M)
for name, body in zip(parts[1::2], parts[2::2]):
    rel = "agent_design/" + name if name.startswith("development_candidates/") else name
    p = pathlib.Path(root) / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(body.strip("\n") + "\n", encoding="utf-8")
    print("wrote", p)
