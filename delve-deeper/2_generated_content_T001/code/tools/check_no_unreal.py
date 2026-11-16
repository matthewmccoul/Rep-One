#!/usr/bin/env python3
"""Purity checker for the Delve Deeper core (QA spec UNR-01 to UNR-07, plus the step-1b rules).

  check_no_unreal.py <code dir> [--log FILE]   scan core dirs for Unreal includes/macros and for
                                               forbidden constructs (throw, try, catch, dynamic_cast, typeid)
  check_no_unreal.py --cmake <code dir> [--log FILE]
                                               fail if any CMake file names an unreal/ source

Core dirs: common, map, puzzle, quest (those that exist) under <code dir>.
Exit 0 clean; 1 violations (or nothing scanned: 0 files is a failure); 2 usage error.
Comments and string literals are blanked before the construct scan, so words in text do not trip it.
"""
import os
import re
import sys

CORE_DIRS = ["common", "map", "puzzle", "quest"]
EXTS = (".h", ".hpp", ".cpp", ".cc", ".inl", ".c")

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]\s*([^>"]+?)\s*[>"]')
UNREAL_HEADERS = {"CoreMinimal.h", "Engine.h", "GameFramework", "Components", "UObject", "Kismet",
                  "Containers", "HAL", "Misc", "Math", "Templates", "Modules", "Net", "Blueprint"}
UNREAL_MARKERS = [
    r"\bUCLASS\s*\(", r"\bUSTRUCT\s*\(", r"\bUENUM\s*\(", r"\bUINTERFACE\s*\(", r"\bUPROPERTY\s*\(",
    r"\bUFUNCTION\s*\(", r"\bGENERATED_BODY\b", r"\bGENERATED_UCLASS_BODY\b", r"\bFString\b",
    r"\bFName\b", r"\bFText\b", r"\bTArray\b", r"\bTMap\b", r"\bTSet\b", r"\bTSharedPtr\b",
    r"\bTObjectPtr\b", r"\bUE_LOG\b", r"\bTEXT\s*\(", r"\bUObject\b", r"\bAActor\b",
    r"\bUActorComponent\b", r"\bFVector\b", r"\bcheck\s*\(\s*\w", r"\bensure\s*\(",
]
FORBIDDEN = [
    (r"\bthrow\b", "throw"),
    (r"\btry\b", "try"),
    (r"\bcatch\b", "catch"),
    (r"\bdynamic_cast\b", "dynamic_cast"),
    (r"\btypeid\b", "typeid"),
]


def blank_comments_and_strings(text):
    """Keep line structure; replace comment and string/char literal contents by spaces."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        two = text[i:i + 2]
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
        elif c == '"' or c == "'":
            q = c
            out.append(q)
            i += 1
            while i < n and text[i] != q and text[i] != "\n":
                if text[i] == "\\" and i + 1 < n:
                    out.append("  ")
                    i += 2
                    continue
                out.append(" ")
                i += 1
            if i < n and text[i] == q:
                out.append(q)
                i += 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


def scan_file(path, rel):
    findings = []
    with open(path, encoding="utf-8", errors="replace") as f:
        raw = f.read()
    raw_lines = raw.split("\n")
    # Includes are checked on raw lines (an include inside a comment is not an include, but a
    # commented-out line starts with //, which INCLUDE_RE does not match).
    for no, line in enumerate(raw_lines, 1):
        m = INCLUDE_RE.match(line)
        if m:
            target = m.group(1).replace("\\", "/")
            parts = target.split("/")
            if parts[-1] in ("CoreMinimal.h", "Engine.h") or parts[0] in UNREAL_HEADERS or \
                    any(p in ("Runtime", "Engine", "CoreUObject") for p in parts[:-1]) or \
                    re.match(r"^[UAFT][A-Z][A-Za-z]+\.h$", parts[-1]) and not target.startswith("dd/"):
                findings.append((rel, no, "Unreal include: " + line.strip()))
    clean = blank_comments_and_strings(raw)
    for no, line in enumerate(clean.split("\n"), 1):
        for pat in UNREAL_MARKERS:
            m = re.search(pat, line)
            if m:
                findings.append((rel, no, "Unreal marker: " + m.group(0).strip()))
        for pat, name in FORBIDDEN:
            if re.search(pat, line):
                findings.append((rel, no, "forbidden construct in core: " + name))
    return findings


def scan_tree(root):
    files, findings = 0, []
    for d in CORE_DIRS:
        base = os.path.join(root, d)
        if not os.path.isdir(base):
            continue
        for dp, _dn, fn in os.walk(base):
            for name in sorted(fn):
                if name.endswith(EXTS):
                    p = os.path.join(dp, name)
                    files += 1
                    findings += scan_file(p, os.path.relpath(p, root))
    return files, findings


def scan_cmake(root):
    files, findings = 0, []
    for dp, _dn, fn in os.walk(root):
        for name in sorted(fn):
            if name == "CMakeLists.txt" or name.endswith(".cmake"):
                p = os.path.join(dp, name)
                files += 1
                rel = os.path.relpath(p, root)
                with open(p, encoding="utf-8", errors="replace") as f:
                    for no, line in enumerate(f, 1):
                        code = line.split("#", 1)[0]
                        if re.search(r"(^|[\s/\"(])unreal\s*/", code) or re.search(r"add_subdirectory\s*\(\s*unreal\b", code):
                            findings.append((rel, no, "CMake names an unreal/ source: " + code.strip()))
    return files, findings


def main(argv):
    args = argv[1:]
    log = None
    cmake = False
    rest = []
    i = 0
    while i < len(args):
        if args[i] == "--log" and i + 1 < len(args):
            log = args[i + 1]
            i += 2
        elif args[i] == "--cmake":
            cmake = True
            i += 1
        else:
            rest.append(args[i])
            i += 1
    if len(rest) != 1 or rest[0].startswith("--"):
        print("usage: check_no_unreal.py [--cmake] <code dir> [--log FILE]")
        return 2
    root = rest[0]
    if cmake:
        files, findings = scan_cmake(root)
        what = "CMake files"
    else:
        files, findings = scan_tree(root)
        what = "core files"
    lines = []
    for rel, no, msg in findings:
        lines.append("VIOLATION %s:%d: %s" % (rel, no, msg))
    lines.append("%d %s scanned, %d violation(s)" % (files, what, len(findings)))
    ok = files > 0 and not findings
    if files == 0:
        lines.append("FAIL: 0 files scanned (a check that scans nothing does not pass)")
    lines.append("RESULT " + ("PASS" if ok else "FAIL"))
    text = "\n".join(lines) + "\n"
    sys.stdout.write(text)
    if log:
        with open(log, "w", encoding="utf-8") as f:
            f.write(text)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
