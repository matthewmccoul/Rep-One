#!/usr/bin/env python3
"""Static wrapper checker (QA spec UNR-08, UNR-09, UNR-10).

  check_unreal_wrapper.py <dir> [--log FILE]

Scans the hand-written wrapper files (.h/.cpp) under <dir>. The generated core copies (any
Private/Core/ or Public/Core/ folder) are skipped. For each file it checks:
  UNR-08  includes: only core headers ("dd/<name>.h"), the wrapper's own headers (and their
          .generated.h), standard-library headers (no .h, no slash), and Unreal headers (known
          Unreal folders or CoreMinimal.h). Anything else (a .cpp, "../", an unknown path) is flagged.
  UNR-08  every UCLASS class is listed with its core calls (calls into the dd:: core, found in the
          class's own .cpp: qualified dd::<ns>::<Name>(...) calls plus member calls whose name is a
          public lowerCamel function of a core header found under <dir>, minus common std names).
          A class with 0 core calls is flagged (UNR-10).
  UNR-09  game content or rules in wrapper code: content ids (ctl_*, e<NN>, fig_*, str.*, mon_type_*,
          area_*), any ==/!= against a string literal, indexing Solution/Progress/Order variables,
          comparing Num() with a Solution/Progress value, Stage/Score/Roll/Day compared to a number.
Heuristics, not a parser: a human still reviews. Exit 0 clean; 1 on any violation, on a class
without core calls, on no UCLASS found, or when 0 files are scanned; 2 on usage error.
"""
import os
import re
import sys

STD_NAME_BLACKLIST = {"size", "empty", "c_str", "begin", "end", "push_back", "find", "data", "get", "reset",
                      "clear", "insert", "erase", "count", "at", "back", "front", "str", "substr", "length"}
KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "operator", "catch", "defined", "static_cast",
            "const_cast", "reinterpret_cast", "alignof", "decltype", "noexcept"}
UNREAL_PREFIXES = {"Engine", "Components", "GameFramework", "Kismet", "Misc", "Modules", "UObject", "Materials",
                   "Interfaces", "Containers", "HAL", "Math", "Templates", "Logging", "Async", "Subsystems",
                   "Tickable", "Serialization", "Internationalization", "Delegates", "Stats"}
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*([<"])\s*([^>"]+?)\s*[>"]')
CONTENT_PATTERNS = [
    (r"\bctl_\w*", "control id"),
    (r"\be\d{2}\b", "edge id"),
    (r"\bfig_\w*", "quest/figure key"),
    (r"\bstr\.\w+", "string key"),
    (r"\bmon_type_\w*", "monster type id"),
    (r"\barea_\w+", "area id"),
]
LITERAL = r'(?:TEXT\s*\(\s*"[^"]*"\s*\)|"[^"]*")'
RULE_PATTERNS = [
    (r"(==|!=)\s*" + LITERAL, "comparison against a string literal"),
    (LITERAL + r"\s*(==|!=)", "comparison against a string literal"),
    (r"\b\w*(?:Solution|Progress|Order)\w*\s*\[", "indexing a solution/progress/order sequence"),
    (r"\bNum\s*\(\s*\)\s*(?:==|!=|<=|>=|<|>)\s*\w*(?:Solution|Progress)", "order comparison"),
    (r"\b\w*(?:Solution|Progress)\w*\s*(?:\.|->)\s*Num\s*\(\s*\)\s*(?:==|!=|<=|>=|<|>)", "order comparison"),
    (r"\b(?:Stage|Score|Roll|Day)\s*(?:==|!=|<=|>=|<|>)\s*-?\d", "threshold comparison"),
]
CLASS_RE = re.compile(r"UCLASS\s*\((?:[^()]|\((?:[^()]|\([^()]*\))*\))*\)\s*class\s+(?:\w+_API\s+)?(\w+)", re.S)


def strip_comments(text):
    """Blank comments (keep strings and line structure)."""
    out = []
    i, n = 0, len(text)
    while i < n:
        two = text[i:i + 2]
        c = text[i]
        if two == "//":
            while i < n and text[i] != "\n":
                out.append(" ")
                i += 1
        elif two == "/*":
            out.append("  ")
            i += 2
            while i < n and text[i:i + 2] != "*/":
                out.append("\n" if text[i] == "\n" else " ")
                i += 1
            out.append("  ")
            i += 2
        elif c == '"':
            out.append(c)
            i += 1
            while i < n and text[i] != '"' and text[i] != "\n":
                if text[i] == "\\" and i + 1 < n:
                    out.append(text[i:i + 2])
                    i += 2
                    continue
                out.append(text[i])
                i += 1
            if i < n and text[i] == '"':
                out.append('"')
                i += 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


def is_generated_copy(rel):
    parts = rel.replace("\\", "/").split("/")
    for a, b in zip(parts, parts[1:]):
        if a in ("Private", "Public") and b == "Core":
            return True
    return False


def collect(root):
    files = []
    for dp, dn, fn in os.walk(root):
        dn[:] = [d for d in dn if d not in ("Intermediate", "Binaries", "__pycache__")]
        for name in sorted(fn):
            if name.endswith((".h", ".cpp")):
                p = os.path.join(dp, name)
                files.append((p, os.path.relpath(p, root)))
    return files


def core_method_names(root):
    names = set()
    for dp, _dn, fn in os.walk(root):
        parts = dp.replace("\\", "/").split("/")
        if "dd" not in parts or "Core" not in parts:
            continue
        for name in fn:
            if name.endswith(".h"):
                with open(os.path.join(dp, name), encoding="utf-8", errors="replace") as f:
                    t = strip_comments(f.read())
                for m in re.finditer(r"\b([a-z][A-Za-z0-9]*)\s*\(", t):
                    if m.group(1) not in KEYWORDS:
                        names.add(m.group(1))
    return names - STD_NAME_BLACKLIST


def check_includes(raw_lines, rel, own_names):
    bad = []
    for no, line in enumerate(raw_lines, 1):
        m = INCLUDE_RE.match(line)
        if not m:
            continue
        kind, target = m.group(1), m.group(2).replace("\\", "/")
        base = target.split("/")[-1]
        ok = False
        if target.startswith("dd/") and target.endswith(".h") and ".." not in target:
            ok = True
        elif base in own_names and "/" not in target:
            ok = True
        elif target.endswith(".generated.h") and base[:-len(".generated.h")] + ".h" in own_names:
            ok = True
        elif "/" not in target and "." not in target:
            ok = True  # standard library header such as <string>
        elif target == "CoreMinimal.h":
            ok = True
        elif target.split("/")[0] in UNREAL_PREFIXES and target.endswith(".h") and ".." not in target:
            ok = True
        if not ok:
            bad.append((rel, no, "include not allowed (core header, own header, std or Unreal only): " + line.strip()))
    return bad


def main(argv):
    args = argv[1:]
    log = None
    rest = []
    i = 0
    while i < len(args):
        if args[i] == "--log" and i + 1 < len(args):
            log = args[i + 1]
            i += 2
        else:
            rest.append(args[i])
            i += 1
    if len(rest) != 1 or rest[0].startswith("--"):
        print("usage: check_unreal_wrapper.py <dir> [--log FILE]")
        return 2
    root = rest[0]
    lines = []
    violations = []

    all_files = collect(root) if os.path.isdir(root) else []
    wrappers = [(p, r) for p, r in all_files if not is_generated_copy(r)]
    own_names = {os.path.basename(p) for p, _r in wrappers if p.endswith(".h")}
    methods = core_method_names(root) if os.path.isdir(root) else set()

    classes = []  # (name, header rel, cpp rel or None, calls dict)
    texts = {}
    for p, rel in wrappers:
        with open(p, encoding="utf-8", errors="replace") as f:
            raw = f.read()
        texts[rel] = raw
        raw_lines = raw.split("\n")
        violations += check_includes(raw_lines, rel, own_names)
        code = strip_comments(raw)
        for no, line in enumerate(code.split("\n"), 1):
            for pat, what in CONTENT_PATTERNS:
                m = re.search(pat, line)
                if m:
                    violations.append((rel, no, "hard-coded content (%s): %s" % (what, m.group(0))))
            for pat, what in RULE_PATTERNS:
                if re.search(pat, line):
                    violations.append((rel, no, "game rule in wrapper (%s): %s" % (what, line.strip())))

    def count_calls(code):
        calls = {}
        for m in re.finditer(r"\bdd::(\w+)::(\w+)(?:::(\w+))?\s*\(", code):
            key = "::".join(g for g in m.groups() if g)
            calls["dd::" + key] = calls.get("dd::" + key, 0) + 1
        for m in re.finditer(r"(?:->|\.)\s*([a-z][A-Za-z0-9]*)\s*\(", code):
            if m.group(1) in methods:
                key = "." + m.group(1)
                calls[key] = calls.get(key, 0) + 1
        return calls

    for p, rel in wrappers:
        if not p.endswith(".h"):
            continue
        code = strip_comments(texts[rel])
        for m in CLASS_RE.finditer(code):
            stem = os.path.splitext(os.path.basename(p))[0]
            cpp = None
            for _p2, r2 in wrappers:
                if r2.endswith(".cpp") and os.path.splitext(os.path.basename(r2))[0] == stem:
                    cpp = r2
            calls = count_calls(strip_comments(texts[cpp])) if cpp else {}
            if cpp is None:
                calls = count_calls(code)
            classes.append((m.group(1), rel, cpp, calls))

    lines.append("%d wrapper files scanned (generated Core copies skipped)" % len(wrappers))
    for name, hdr, cpp, calls in classes:
        total = sum(calls.values())
        lines.append("CLASS %s (%s, %s): %d core call(s)" % (name, hdr, cpp or "no .cpp", total))
        for k in sorted(calls):
            lines.append("    %-40s x%d" % (k, calls[k]))
        if total == 0:
            violations.append((hdr, 1, "class %s makes no call into the core" % name))
    for rel, no, msg in violations:
        lines.append("VIOLATION %s:%d: %s" % (rel, no, msg))
    ok = True
    if not wrappers:
        lines.append("FAIL: 0 files scanned (a check that scans nothing does not pass)")
        ok = False
    elif not classes:
        lines.append("FAIL: no UCLASS wrapper class found")
        ok = False
    if violations:
        ok = False
    lines.append("%d class(es), %d violation(s)" % (len(classes), len(violations)))
    lines.append("RESULT " + ("PASS" if ok else "FAIL"))
    text = "\n".join(lines) + "\n"
    sys.stdout.write(text)
    if log:
        with open(log, "w", encoding="utf-8") as f:
            f.write(text)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
