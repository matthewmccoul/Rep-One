"""Check every quotation in a critic report against the file and line it cites. Standard library only.

Usage: python3 -I orchestration/verify_quotes.py <report.md>
A quote is:  "exact text" (path:LINE)  or  "exact text" (path:LINE-LINE)   (straight double quotes).
Paths are relative to the project folder. A quote may contain "..." to mark an omission; each part must appear
in order inside the cited lines. Prints one line per quote, then a total. Exit 0 only if at least one quote was
found and every quote matches; exit 1 otherwise.
"""
import pathlib, re, sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
QUOTE = re.compile(r'"([^"\n]+)"\s*\(([^():\s]+):(\d+)(?:-(\d+))?\)')


def main(report):
    text = pathlib.Path(report).read_text(encoding="utf-8")
    found = bad = 0
    for m in QUOTE.finditer(text):
        quote, path, a, b = m.group(1), m.group(2), int(m.group(3)), int(m.group(4) or m.group(3))
        found += 1
        p = (ROOT / path).resolve()
        if ROOT not in p.parents or not p.is_file():
            print(f"FAIL  {path}:{a}: file not found inside the project: {quote[:60]!r}")
            bad += 1
            continue
        lines = p.read_text(encoding="utf-8", errors="replace").splitlines()
        window = "\n".join(lines[a - 1:b])
        pos, ok = 0, True
        for part in [x.strip() for x in quote.split("...") if x.strip()]:
            i = window.find(part, pos)
            if i < 0:
                ok = False
                break
            pos = i + len(part)
        print(("PASS  " if ok else "FAIL  ") + f"{path}:{a}" + ("" if ok else f": not found in those lines: {quote[:60]!r}"))
        bad += 0 if ok else 1
    print(f"{found - bad} of {found} quotes verified")
    return 0 if found and not bad else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]) if len(sys.argv) == 2 else 2)
