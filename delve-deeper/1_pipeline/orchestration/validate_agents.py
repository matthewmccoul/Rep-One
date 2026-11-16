"""Strict validator and shared parser for Claude Code agent files (orchestration v2). Fails loudly.

Usage:
  validate_agents.py <dir> [prefixes] [--against <dir> ...]
    prefixes  : comma-separated allowed name prefixes, e.g. "definition-" or "definition-,development-"
    --against : other agent directories for the cross-set duplicate-name check. A name present in both
                sets is an error unless the two files are byte-identical (an activated copy of a staged file).
Prints a JSON report to stdout; exit 0 only if there are no errors.

Library use (staged_agents_json.py): parse_agent_file(path) -> (frontmatter: dict, body: str, errors: list)
"""
import hashlib, json, pathlib, re, sys

try:
    import yaml  # PyYAML; Claude Code parses frontmatter as YAML, so strict YAML is the closer check
except ImportError:  # pragma: no cover
    yaml = None

ALLOWED_KEYS = {"name", "description", "tools"}           # model/permissionMode intentionally omitted
ALLOWED_TOOLS = {"Read", "Write", "Edit", "Glob", "Grep", "WebSearch", "WebFetch", "Bash", "Agent"}
FORBIDDEN_FOR_DEFINITION = {"Agent"}                        # R1: dispatch is via the runner, not Agent
MIN_BODY_WORDS = 300
ALLOWLISTS = pathlib.Path(__file__).resolve().parent / "allowlists.json"


def parse_agent_file(path):
    """Parse one agent file strictly. Returns (fm, body, errors). Never guesses."""
    p = pathlib.Path(path)
    errs = []
    t = p.read_text(encoding="utf-8")
    m = re.match(r"^---\n(.*?)\n---\n(.*)$", t, re.S)
    if not m:
        return {}, "", [f"{p}: missing/invalid frontmatter delimiters (need '---' on line 1 and a closing '---')"]
    raw, body = m.group(1), m.group(2)
    if yaml is None:
        return {}, body, [f"{p}: PyYAML unavailable; refusing to guess-parse frontmatter"]
    try:
        fm = yaml.safe_load(raw)
    except yaml.YAMLError as e:
        return {}, body, [f"{p}: frontmatter is not valid YAML: {str(e).splitlines()[0]}"]
    if not isinstance(fm, dict):
        return {}, body, [f"{p}: frontmatter is not a mapping"]
    for k, v in fm.items():
        if k not in ALLOWED_KEYS:
            errs.append(f"{p}: unexpected key {k!r} (only name/description/tools; model intentionally omitted)")
        if not isinstance(v, str):
            errs.append(f"{p}: value of {k!r} must be a string, got {type(v).__name__}")
    fm = {k: (v.strip() if isinstance(v, str) else v) for k, v in fm.items()}
    return fm, body, errs


def tools_of(fm):
    t = fm.get("tools", "")
    return [x.strip() for x in t.split(",") if x.strip()] if isinstance(t, str) else []


def _allowlist_tools(name):
    try:
        a = json.loads(ALLOWLISTS.read_text())
    except (OSError, json.JSONDecodeError):
        return None
    if name not in a:
        return None
    return {re.sub(r"\(.*\)$", "", tok) for tok in a[name].split()}


def validate_dir(d, prefixes=None):
    d = pathlib.Path(d)
    errs, names = [], {}
    files = sorted(d.glob("*.md"))
    if not files:
        errs.append(f"no agent files in {d}")
    for p in files:
        fm, body, e = parse_agent_file(p)
        errs += e
        if not fm:
            continue
        n = fm.get("name", "")
        if not isinstance(n, str) or not re.fullmatch(r"[a-z0-9]+(-[a-z0-9]+)*", n):
            errs.append(f"{p}: bad or missing name {n!r}"); continue
        if p.stem != n:
            errs.append(f"{p}: filename stem {p.stem!r} != name {n!r}")
        if prefixes and not n.startswith(tuple(prefixes)):
            errs.append(f"{p}: name lacks required prefix {prefixes}")
        if n in names:
            errs.append(f"{p}: duplicate name {n!r} (also {names[n]})")
        names[n] = str(p)
        desc = fm.get("description", "")
        if not isinstance(desc, str) or len(desc) < 20:
            errs.append(f"{p}: description missing or too short")
        tools = tools_of(fm)
        bad = [x for x in tools if x not in ALLOWED_TOOLS]
        if bad:
            errs.append(f"{p}: unknown tools {bad}")
        if n.startswith("definition-") and FORBIDDEN_FOR_DEFINITION & set(tools):
            errs.append(f"{p}: definition agents may not declare {sorted(FORBIDDEN_FOR_DEFINITION & set(tools))} (R1)")
        granted = _allowlist_tools(n)
        if granted is not None:
            missing = [x for x in tools if x not in granted]
            if missing:
                errs.append(f"{p}: frontmatter tools {missing} not granted in allowlists.json")
        if len(body.split()) < MIN_BODY_WORDS:
            errs.append(f"{p}: body under {MIN_BODY_WORDS} words")
    return names, errs


def _sha(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


def cross_check(names, other_dirs):
    errs = []
    for od in other_dirs:
        for p in sorted(pathlib.Path(od).glob("*.md")):
            fm, _, _ = parse_agent_file(p)
            n = fm.get("name") if fm else None
            if n in names and _sha(p) != _sha(names[n]):
                errs.append(f"cross-set duplicate name {n!r}: {names[n]} and {p} differ")
    return errs


def main(argv):
    if not argv:
        print(__doc__, file=sys.stderr); return 2
    against = []
    if "--against" in argv:
        i = argv.index("--against"); against = argv[i + 1:]; argv = argv[:i]
    d = argv[0]
    prefixes = [x for x in argv[1].split(",") if x] if len(argv) > 1 else None
    names, errs = validate_dir(d, prefixes)
    errs += cross_check(names, [a for a in against if pathlib.Path(a).is_dir()])
    print(json.dumps({"dir": str(d), "prefixes": prefixes, "against": against, "files": len(names),
                      "names": sorted(names), "errors": errs}, indent=1))
    return 1 if errs else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
