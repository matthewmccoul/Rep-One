"""Deterministic halving of an SRD part at a section/rule boundary near the middle."""
import re
def split_half(text):
    n = len(text); mid = n // 2
    cands = [m.start() for m in re.finditer(r"(?m)^(?=#{2,4} |\*\*R\d+\.|\*\*Q\d+\.|\*\*CONFLICT|\*\*UNKNOWN)", text)]
    cands = [c for c in cands if n * 0.3 < c < n * 0.7]
    cut = min(cands, key=lambda c: abs(c - mid)) if cands else (text.rfind("\n\n", 0, mid) if text.rfind("\n\n", 0, mid) > 0 else mid)
    return text[:cut], text[cut:]

SLUG = {"02": "attributes", "03": "races", "04": "guilds", "05": "progression", "06": "clock", "07": "combat", "08": "status", "09": "casting", "10": "spells", "11": "monsters", "12": "companions",
        "13": "items", "14": "loot", "15": "dungeon", "16": "city", "17": "party", "18": "death", "19": "options", "20": "dataformats", "21": "modding", "22": "misc"}


def condense(text):
    """Drop condensed duplicate sections (summary, quick reference, terms, worked example) from an SRD part."""
    units = re.split(r"(?m)^(?=#{2,3} )", text)
    keep = []
    for u in units:
        head = u.split("\n", 1)[0].lower()
        if re.match(r"#{2,3} (summary|quick reference|terms|worked example)", head): continue
        keep.append(u)
    return "".join(keep)

def header_view(h):
    """Header without full-line comments (trailing rule tags stay)."""
    out = []
    for l in h.split("\n"):
        if l.strip().startswith("//") or not l.strip(): continue
        out.append(l)
    return "\n".join(out)


def split_k(text, k):
    """Cut text into k pieces at section/rule boundaries (repeated halving of the largest piece)."""
    pieces = [text]
    while len(pieces) < k:
        i = max(range(len(pieces)), key=lambda j: len(pieces[j])); a, b = split_half(pieces[i]); pieces[i:i + 1] = [a, b]
    return pieces
