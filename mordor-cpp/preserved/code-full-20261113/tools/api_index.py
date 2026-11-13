"""Compact index of declared types and functions in include/mordor/*.h (names and signatures only)."""
import glob, os, re, sys
ROOT = "/tmp/claude-0/-home-user-Rep-One/17a752bb-9e88-5884-a5bf-3b7fc4ab8c87/scratchpad/mordor/code"
SPINE = ["types.h", "math_util.h", "rng.h", "events.h", "ctx.h", "game_data.h", "json_lite.h"]
def strip_comments(t):
    t = re.sub(r"/\*.*?\*/", "", t, flags=re.S)
    return "\n".join(re.sub(r"//.*", "", l).rstrip() for l in t.split("\n"))
def digest(path, comments=False):
    t = open(path).read()
    t = t if comments else strip_comments(t)
    return "\n".join(l for l in t.split("\n") if l.strip() and not l.startswith("#pragma"))
def spine_digest(include_game=True):
    """State headers (comment-stripped) - the data model every module shares. state_game.h only when asked."""
    out = []
    for f in sorted(glob.glob(f"{ROOT}/include/mordor/state_*.h")):
        if os.path.basename(f) == "state_game.h" and not include_game: continue
        out.append(f"// ---- include/mordor/{os.path.basename(f)}\n" + digest(f))
    return "\n".join(out)
def core_digest():
    """Foundation headers' public surface (short)."""
    return ("// ---- foundation (already written): types.h: i8..i64,u8..u64,Str,Vec<T>,Arr<T,N>, enum Stat{STR,INT,WIS,CON,CHA,DEX}, Currency=i64 (1/10000), enum Alignment{ALIGN_GOOD,ALIGN_NEUTRAL,ALIGN_EVIL}\n"
            "// math_util.h: vbInt(double)->i64 (floor), vbFix, roundHalfEven(double)->i64, clampv(v,lo,hi), minv, maxv, currencyWhole, currencyFromWhole\n"
            "// rng.h: IRng{nextU32,nextDouble,below(n)=Int(Rnd*n),range(lo,hi),percent(p)}, Rng(seed), ScriptedRng(Vec<double>)\n"
            "// events.h: Event{code,text,nums,strs}, IEventSink, EventLog; ctx.h: struct Ctx{const GameData& data; IRng& rng; IEventSink& events; say(code,text,nums,strs)}\n"
            "// game_data.h (generated): RaceDef,GuildDef,SpellDef,ItemDef,MonsterDef,NamedEntry,FloorField,FloorArea,FloorTeleporter,FloorChute,FloorDef,enum FloorFlag{FLAG_WALL_EAST...}, GameData{races,guilds,spells,items,monsters,itemTypes,itemDescriptions,monsterClasses,monsterGroups,floors}\n")
def api_index(exclude=()):
    out = []
    for f in sorted(glob.glob(f"{ROOT}/include/mordor/*.h")):
        b = os.path.basename(f)
        if b in SPINE or b.startswith("state_") or b in exclude: continue
        t = strip_comments(open(f).read())
        lines = []
        for l in t.split("\n"):
            s = l.strip()
            if re.match(r"^(struct|enum class|enum|class|namespace|constexpr|inline|using)\b", s) or (re.search(r"\w\(", s) and s.endswith(";") and not l.startswith(" ")):
                lines.append(s)
        out.append(f"// {b}\n" + "\n".join(lines))
    return "\n".join(out)
if __name__ == "__main__":
    print(api_index())


def relevant_index(part_text, exclude=(), cap=14000):
    """API index lines relevant to a text: functions whose name (>=6 chars) appears in the text, plus type declarations."""
    low = part_text.lower(); out = []
    for blk in api_index(exclude=exclude).split("\n// "):
        hdr, _, body = blk.partition("\n"); keep = []
        for l in body.split("\n"):
            m = re.search(r"(\w+)\(", l)
            if (m and len(m.group(1)) >= 6 and m.group(1).lower() in low) or re.match(r"^(struct|enum)", l): keep.append(l)
        if keep: out.append(f"// {hdr.lstrip('/ ').strip()}\n" + "\n".join(keep))
    return "\n".join(out)[:cap]


def spine_for_header(header_text):
    """Digest of only the state headers a module header includes (transitively), plus state_common.h."""
    need, todo = set(), re.findall(r'#include "mordor/(state_\w+)\.h"', header_text)
    todo.append("state_common")
    while todo:
        n = todo.pop()
        if n in need: continue
        need.add(n)
        path = f"{ROOT}/include/mordor/{n}.h"
        if os.path.exists(path): todo += re.findall(r'#include "mordor/(state_\w+)\.h"', open(path).read())
    out = []
    for n in sorted(need):
        path = f"{ROOT}/include/mordor/{n}.h"
        if os.path.exists(path): out.append(f"// ---- include/mordor/{n}.h\n" + digest(path))
    return "\n".join(out)
