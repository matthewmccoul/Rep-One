"""Apply the found errors (code/SRD_ERRATA.md rows) to the SRD chapters in place, one ant per row; every change is logged for review.
usage: apply_found_errors.py [row numbers...]   (DRY=1 writes packets only)"""
import asyncio, glob, json, math, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
args = sys.argv[1:]; sys.argv = sys.argv[:1]
import phase_investigate as pi
from cant import ROOT, run_all, est_tokens
M = os.path.dirname(ROOT); SRDDIR = f"{M}/out/srd"
rows = []
for sec in re.split(r"\n(?=## |# )", open(f"{ROOT}/SRD_ERRATA.md").read()):
    for l in sec.split("\n"):
        if l.startswith("| ") and not l.startswith("| Ch") and not l.startswith("|---"):
            c = [x.strip() for x in l.strip().strip("|").split("|")]
            if len(c) >= 4: rows.append({"ch": c[0], "type": c[1], "text": c[2], "evidence": c[3]})
print(len(rows), "found errors parsed", flush=True)
paras = []
for p in sorted(glob.glob(f"{SRDDIR}/[0-2][0-9]_*.md")):
    base = os.path.basename(p)
    for para in re.split(r"\n\s*\n", open(p, encoding="utf-8").read()):
        if 40 < len(para.strip()) < 2600: paras.append((base, para.strip(), para.lower()))
def top(tokens, k=7, maxchars=14000):
    df = {w: sum(1 for x in paras if w in x[2]) for w in tokens}
    sc = sorted(((sum(math.log(1 + len(paras) / (1 + df[w])) for w in tokens if w in lw), b, para) for b, para, lw in paras), key=lambda x: -x[0])
    out, used = [], 0
    for s, b, para in sc[:k * 3]:
        if s <= 0 or used + len(para) > maxchars: continue
        out.append(f"[FILE {b}]\n{para}"); used += len(para)
        if len(out) >= k: break
    return "\n\n".join(out)
SYS = ("You correct a game-rules specification (the SRD, Markdown chapters) against a finding. The REFERENCE implementation is canon; the SRD is not. You get one FOUND ERROR (what the reference does, with evidence) and the SRD paragraphs most likely to contain the wrong or missing statement, each labelled with its file. "
       "Make the SRD state the correct rule. Edit the wrong statement IN PLACE (do not add a note saying it was wrong; do not mention errata or corrections). Keep the SRD's voice: short rule statements, existing markup and numbering (never renumber; where a new rule is needed add it right after the closest rule as **R<n>a.**). Where an UNKNOWN/CONFLICT line is answered by the finding, replace it. Name the reference function once in parentheses, e.g. (reference: Rules.QuestTier). "
       "Output ONLY edit blocks (one or several):\n=== EDIT <file> ===\n<old text copied EXACTLY from the paragraph above, occurring once in that file>\n=== TO ===\n<new text>\n=== END EDIT ===\nor, to add text:\n=== INSERT <file> ===\n<anchor text copied exactly, occurring once>\n=== TEXT ===\n<new paragraph(s) placed after the anchor's paragraph>\n=== END INSERT ===\n"
       "If no paragraph above is the right place, say `NO_PLACE: <reason>` and nothing else.")
tasks = []
for i, r in enumerate(rows, 1):
    if args and str(i) not in args: continue
    toks = pi.tokens_of(r["text"], r["ch"])
    user = f"FOUND ERROR #{i} (chapter hint {r['ch']}; kind: {r['type']}):\n{r['text']}\nEvidence: {r['evidence']}\n\nSRD PARAGRAPHS\n{top(toks)}"
    while est_tokens(SYS, user) > 31000: user = user[:int(len(user) * .9)]
    if os.environ.get("DRY"): print(i, est_tokens(SYS, user)); continue
    tasks.append({"id": str(i), "system": SYS, "user": user, "allow": [], "save": f"spec/investigate/fe_{i}.out"})
if os.environ.get("DRY"): sys.exit(0)
asyncio.run(run_all(tasks, "found_errors", 4))
log = []
for t in tasks:
    i = t["id"]; p = f"{ROOT}/{t['save']}"
    if not os.path.exists(p): log.append((i, "NO OUTPUT", "")); continue
    out = open(p).read()
    if "NO_PLACE" in out and "=== EDIT" not in out and "=== INSERT" not in out: log.append((i, "NO_PLACE", out.strip()[:200])); continue
    n = 0
    for m in re.finditer(r"=== EDIT (\S+) ===\n(.*?)\n=== TO ===\n(.*?)\n=== END EDIT ===", out, re.S):
        f, old, new = m.groups(); fp = f"{SRDDIR}/{f}"
        if not re.match(r"[0-2]\d_[\w\-]+\.md$", f) or not os.path.exists(fp): log.append((i, "BAD FILE " + f, "")); continue
        txt = open(fp, encoding="utf-8").read()
        if txt.count(old) != 1: log.append((i, f"EDIT old text found {txt.count(old)}x in {f}", old[:120])); continue
        open(fp, "w", encoding="utf-8").write(txt.replace(old, new)); n += 1; log.append((i, "EDIT " + f, old[:110].replace("\n", " ") + "  ==>  " + new[:160].replace("\n", " ")))
    for m in re.finditer(r"=== INSERT (\S+) ===\n(.*?)\n=== TEXT ===\n(.*?)\n=== END INSERT ===", out, re.S):
        f, anchor, new = m.groups(); fp = f"{SRDDIR}/{f}"
        if not re.match(r"[0-2]\d_[\w\-]+\.md$", f) or not os.path.exists(fp): log.append((i, "BAD FILE " + f, "")); continue
        txt = open(fp, encoding="utf-8").read()
        if txt.count(anchor) != 1: log.append((i, f"INSERT anchor found {txt.count(anchor)}x in {f}", anchor[:120])); continue
        k = txt.index(anchor) + len(anchor); e = txt.find("\n\n", k); e = len(txt) if e < 0 else e
        open(fp, "w", encoding="utf-8").write(txt[:e] + "\n\n" + new.strip() + txt[e:]); n += 1; log.append((i, "INSERT " + f, new[:200].replace("\n", " ")))
    if n == 0 and not any(x[0] == i for x in log): log.append((i, "NOTHING APPLIED", out.strip()[:150]))
json.dump(log, open(f"{ROOT}/logs/found_errors_apply.json", "w"), indent=1)
for l in log: print(f"#{l[0]:>2} {l[1][:60]:60} {l[2][:230]}")
