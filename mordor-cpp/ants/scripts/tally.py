"""Gate + tally: parse ant blocks; C++ names must exist in the C++ demo (index or src definitions); mark unwired; per-file summary."""
import glob, json, os, re, collections
A2 = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/ants2"; C = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/dl/cpp"
idx = json.load(open(f"{A2}/notes/cpp_index.json")); names = {d["name"] for d in idx}
src = "\n".join(open(f, errors="replace").read() for f in glob.glob(f"{C}/src/*.cpp"))
names |= set(re.findall(r"^[A-Za-z_][\w:<>,&\*\s]*?\b([A-Za-z_]\w*)\s*\([^;{)]*\)\s*(?:const)?\s*\{", src, re.M))
unw = set(json.load(open(f"{A2}/notes/unwired.json")))
man = {m["id"]: m for m in json.load(open(f"{A2}/notes/manifest.json"))}
rows = []; st = collections.Counter()
for pid, m in man.items():
    p = f"{A2}/map/{pid}.md"
    if not os.path.exists(p): st["missing_packet"] += 1; continue
    for b in re.split(r"(?m)^(?=### M )", open(p).read()):
        h = re.match(r"### M (\S+)", b)
        if not h: continue
        s = re.search(r"Status:\s*(\w+)", b); c = re.search(r"C\+\+:\s*(.*)", b); n = re.search(r"Note:\s*(.*)", b)
        status = s.group(1) if s else "?"; raw = c.group(1) if c else "NONE"
        TYPES = {"i32","i64","u32","f64","f32","bool","const","void","auto","Str","Vec","Arr","int","double","float","char","NONE","std"}
        ids = [w for w in re.findall(r"[A-Za-z_]\w{2,}", raw) if w not in TYPES]
        cc = list(dict.fromkeys(w for w in ids if w in names))
        bad = [] if cc else ids[:4]
        if status in ("PRESENT", "PARTIAL") and not cc: st["demoted_unverified"] += 1; status = "UNVERIFIED"
        wired = None if not cc else not all(x in unw for x in cc)
        rows.append({"file": m["file"], "method": h.group(1), "status": status, "cpp": cc, "bad_names": bad, "wired": wired, "note": (n.group(1).strip() if n else "")[:240]})
json.dump(rows, open(f"{A2}/notes/mapping.json", "w"), indent=1)
c = collections.Counter(r["status"] for r in rows); print(dict(st), dict(c), "total", len(rows))
print("PRESENT/PARTIAL but every C++ match unwired:", sum(1 for r in rows if r["status"] in ("PRESENT", "PARTIAL") and r["wired"] is False))
