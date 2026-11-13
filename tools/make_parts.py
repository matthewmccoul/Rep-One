"""Split SRD chapters into 'parts' (<= MAXCH chars each) at section/rule boundaries. usage: make_parts.py <M>"""
import glob, json, os, re, sys
M = sys.argv[1]; MAXCH = 40_000
out = f"{M}/code/spec/parts"; os.makedirs(out, exist_ok=True)
for f in glob.glob(out + "/*.md"): os.remove(f)
parts = []
for p in sorted(glob.glob(f"{M}/out/srd/[0-2][0-9]_*.md")):
    n = os.path.basename(p)[:2]
    if n in ("01",): continue
    txt = open(p, encoding="utf-8").read()
    title = txt.split("\n")[0].lstrip("# ").strip()
    # units: split at h2/h3 headings, then oversize units at rule starts or blank lines
    units = [u for u in re.split(r"(?m)^(?=#{2,3} )", txt.split("\n", 1)[1]) if u.strip()]
    flat = []
    for u in units:
        if len(u) <= MAXCH: flat.append(u); continue
        pieces = re.split(r"(?m)^(?=\*\*R\d+\.|\*\*Q\d+\.|\*\*CONFLICT|\*\*UNKNOWN|(?:[A-Z]\d+\. ))", u)
        cur = ""
        for pc in pieces:
            if len(cur) + len(pc) > MAXCH and cur: flat.append(cur); cur = ""
            if len(pc) > MAXCH:   # last resort: split on blank lines
                for q in pc.split("\n\n"):
                    if len(cur) + len(q) > MAXCH and cur: flat.append(cur); cur = ""
                    cur += q + "\n\n"
            else: cur += pc
        if cur.strip(): flat.append(cur)
    cur, k = "", 0
    for u in flat:
        if len(cur) + len(u) > MAXCH and cur:
            k += 1; parts.append((f"c{n}p{k}", title, cur)); cur = ""
        cur += u
    if cur.strip(): k += 1; parts.append((f"c{n}p{k}", title, cur))
info = []
for pid, title, text in parts:
    open(f"{out}/{pid}.md", "w").write(f"# {title} (part {pid})\n\n{text}")
    info.append({"id": pid, "chapter": title, "chars": len(text), "est_tokens": int(len(text) / 2.1)})
json.dump(info, open(f"{M}/code/spec/parts.json", "w"), indent=1)
print(len(info), "parts;", sum(i["chars"] for i in info) // 1000, "k chars; max", max(i["chars"] for i in info))
for i in info: print(i["id"], i["est_tokens"], i["chapter"][:50])
