#!/usr/bin/env python3
"""Lore guard (PZL-30, PZL-31, QST-17, QST-18): no statement about who built the mine or what happened to
its makers, in the puzzle's carving text, in every other `str.*` string, and in the quest strings CSV.

  check_text_guard.py <puzzle.json | strings.csv> [--words a,b,c]

JSON mode (any file not ending in .csv): scans every value under a key that starts with "str." (anywhere in
the document, so the "strings" block and any nested copy) for QA's word list: dwarf, dwarves, builder,
built, maker, makers, ancient, colonist, newcomer. Matching is case-insensitive, at a word start (so
"makers", "dwarfs", "newcomers" are caught), after Unicode NFKC and removal of invisible format characters.

CSV mode (file name ends in .csv): the quest strings CSV, header row then one row per line. Scans the `text`
column of every row, keyed by the `key` column. The word list is the list above plus the reason phrases the
Figure must never use: because, in order to, so that, the reason, my master, my plan (same match rules).
`--words` replaces the whole list in either mode.

Exit 0 clean; 1 leak or bad input (first line `FAIL <CODE>: <detail>`); 2 usage or unreadable file.
JSON codes: TEXT_LEAK (detail names the key and the word), JSON_SYNTAX, JSON_DUP_KEY (a repeated key could
hide a leak behind a later value), BAD_STRING (a str.* key whose value holds no string), NO_STRINGS
(nothing was scanned, so nothing can pass).
CSV codes: TEXT_LEAK (detail: the row key and the word), and, for a malformed file (not a leak):
CSV_SYNTAX (bad quoting or not UTF-8), CSV_EMPTY, CSV_NO_COLUMN (no `key` or no `text` column),
CSV_RAGGED (a row with a different number of cells than the header), CSV_DUP_KEY (a repeated key could hide
a leak), CSV_NO_ROWS (header only: nothing was scanned).
"""
import csv
import io
import json
import re
import sys
import unicodedata

WORDS = ["dwarf", "dwarves", "builder", "built", "maker", "makers", "ancient", "colonist", "newcomer"]
CSV_EXTRA = ["because", "in order to", "so that", "the reason", "my master", "my plan"]
USAGE = "usage: check_text_guard.py <puzzle.json|strings.csv> [--words a,b,c]"


class DupKey(Exception):
    pass


def _pairs(pairs):
    seen = set()
    for k, _ in pairs:
        if k in seen:
            raise DupKey(k)
        seen.add(k)
    return dict(pairs)


def _norm(s):
    s = unicodedata.normalize("NFKC", s)
    return "".join(ch for ch in s if unicodedata.category(ch) != "Cf")


def _regex(words):
    return re.compile(r"(?<![^\W_])(" + "|".join(re.escape(w) for w in words) + r")", re.IGNORECASE)


def _strings_in(value):
    """Yields every string inside value (the value itself, or inside lists and objects, keys excluded)."""
    if isinstance(value, str):
        yield value
    elif isinstance(value, list):
        for v in value:
            yield from _strings_in(v)
    elif isinstance(value, dict):
        for v in value.values():
            yield from _strings_in(v)


def _collect(node, out, path=""):
    if isinstance(node, dict):
        for k, v in node.items():
            p = path + "." + k if path else k
            if k.startswith("str."):
                out.append((k, v))
            else:
                _collect(v, out, p)
    elif isinstance(node, list):
        for i, v in enumerate(node):
            _collect(v, out, "%s[%d]" % (path, i))


def check(doc, words):
    """JSON mode. Returns (code, detail) for the first problem, or None when clean."""
    found = []
    _collect(doc, found)
    if not found:
        return "NO_STRINGS", "no str.* key found: nothing was scanned"
    rx = _regex(words)
    for key, value in found:
        texts = list(_strings_in(value))
        if not texts:
            return "BAD_STRING", "%s holds no string" % key
        for t in texts:
            m = rx.search(_norm(t))
            if m:
                return "TEXT_LEAK", "%s: contains '%s'" % (key, m.group(1).lower())
    return None


def check_csv(text, words):
    """CSV mode. Returns (code, detail) for the first problem, or None when clean."""
    if text.startswith("﻿"):
        text = text[1:]
    if not text.strip():
        return "CSV_EMPTY", "file is empty"
    try:
        rows = list(csv.reader(io.StringIO(text, newline=""), strict=True))
    except csv.Error as e:
        return "CSV_SYNTAX", str(e)
    rows = [r for r in rows if r]  # blank lines carry no data
    if not rows:
        return "CSV_EMPTY", "file has no rows"
    header = [h.strip() for h in rows[0]]
    if "key" not in header or "text" not in header:
        return "CSV_NO_COLUMN", "header has no 'key' and 'text' column: %s" % ",".join(header)
    ki, ti = header.index("key"), header.index("text")
    body = rows[1:]
    if not body:
        return "CSV_NO_ROWS", "header only: nothing was scanned"
    seen = set()
    for n, r in enumerate(body, start=2):
        if len(r) != len(header):
            return "CSV_RAGGED", "row %d has %d cells, header has %d" % (n, len(r), len(header))
        if r[ki] in seen:
            return "CSV_DUP_KEY", r[ki]
        seen.add(r[ki])
    rx = _regex(words)
    for r in body:
        m = rx.search(_norm(r[ti]))
        if m:
            return "TEXT_LEAK", "%s: contains '%s'" % (r[ki], m.group(1).lower())
    return None


def main(argv):
    words = None
    args = []
    i = 0
    while i < len(argv):
        if argv[i] == "--words":
            if i + 1 >= len(argv):
                print(USAGE)
                return 2
            words = [w.strip() for w in argv[i + 1].split(",") if w.strip()]
            i += 2
        else:
            args.append(argv[i])
            i += 1
    if len(args) != 1 or words == []:
        print(USAGE)
        return 2
    try:
        with open(args[0], "rb") as f:
            raw = f.read()
    except OSError as e:
        print("ERROR cannot read file: %s (%s)" % (args[0], e))
        return 2
    if args[0].lower().endswith(".csv"):
        try:
            text = raw.decode("utf-8")
        except UnicodeDecodeError as e:
            print("FAIL CSV_SYNTAX: not UTF-8: %s" % e)
            return 1
        res = check_csv(text, words if words is not None else WORDS + CSV_EXTRA)
        if res:
            print("FAIL %s: %s" % res)
            return 1
        print("OK text guard: no listed word in the text column of any row")
        return 0
    try:
        doc = json.loads(raw.decode("utf-8"), object_pairs_hook=_pairs)
    except DupKey as e:
        print("FAIL JSON_DUP_KEY: %s" % e)
        return 1
    except (ValueError, UnicodeDecodeError) as e:
        print("FAIL JSON_SYNTAX: %s" % e)
        return 1
    res = check(doc, words if words is not None else WORDS)
    if res:
        print("FAIL %s: %s" % res)
        return 1
    print("OK text guard: no listed word in any str.* string")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
