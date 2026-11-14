"""C# type -> C++ type, mechanically, for Mordor.Core. Also builds the per-type C++ signature catalog."""
import json, re, sys, collections
X = sys.argv[1] if len(sys.argv) > 1 else "."
T = json.load(open(f"{X}/outline.json"))
# registry: flattened C++ names for nested types; kinds
reg = {}       # C# qualified (Outer.Inner or Name) -> (cpp name, kind)
by_simple = collections.defaultdict(list)
for t in T:
    q = (t["outer"] + "." if t["outer"] else "") + t["name"]
    cpp = q.replace(".", "_")
    k = t["kind"] if not t["isStatic"] else "static"
    reg[q] = (cpp, k); by_simple[t["name"]].append(q)
PRIM = {"int": "int32_t", "long": "int64_t", "short": "int16_t", "byte": "uint8_t", "sbyte": "int8_t", "ushort": "uint16_t",
        "uint": "uint32_t", "ulong": "uint64_t", "float": "float", "double": "double", "bool": "bool", "char": "char",
        "string": "cs::String", "decimal": "cs::Decimal", "void": "void", "object": "std::shared_ptr<void>", "nint": "intptr_t"}
SEQ = {"List", "IList", "IReadOnlyList", "IEnumerable", "ICollection", "IReadOnlyCollection", "Span", "ReadOnlySpan", "Memory", "ReadOnlyMemory"}
DICT = {"Dictionary", "IReadOnlyDictionary", "IDictionary", "SortedDictionary"}
SETS = {"HashSet", "ISet", "IReadOnlySet", "SortedSet"}
VALUE_SYS = {"Utf8JsonReader": "cs::Utf8JsonReader", "Utf8JsonWriter": "cs::Utf8JsonWriter", "Type": "cs::Type", "IntPtr": "intptr_t", "ZipArchiveEntry": "cs::ZipArchiveEntry", "Guid": "cs::Guid", "DirectoryInfo": "cs::DirectoryInfo", "IFormatProvider": "int", "BigInteger": "cs::BigInteger", "TimeSpan": "cs::TimeSpan", "DateTime": "cs::DateTime", "Decimal": "cs::Decimal", "Int32": "int32_t", "Int64": "int64_t"}
REF_SYS = {"JsonSerializerOptions": "cs::JsonSerializerOptions", "MemoryStream": "cs::Stream", "FileStream": "cs::Stream", "Stream": "cs::Stream", "ZipArchive": "cs::ZipArchive", "Random": "cs::Random", "StringBuilder": "cs::StringBuilder", "Regex": "cs::Regex", "Stream": "std::iostream", "Exception": "cs::Exception"}
unknown = collections.Counter()
tuples = {}   # struct name -> [(type, name)]
def split_args(s):
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch in "<([": depth += 1
        if ch in ">)]": depth -= 1
        if ch == "," and depth == 0: out.append(cur.strip()); cur = ""
        else: cur += ch
    if cur.strip(): out.append(cur.strip())
    return out
def resolve(name, scope=""):
    """C# type name (maybe qualified) -> (cpp, kind) or None. scope = enclosing type path for nested lookups."""
    name = re.sub(r"^(global::)?(System\.(Collections\.Generic|Numerics|Text|IO|Text\.Json|Text\.Json\.Serialization|IO\.Compression|Runtime\.CompilerServices)\.)", "", name)
    name = re.sub(r"^Mordor\.Core\.(Import\.|Extraction\.)?", "", name)
    if name in reg: return reg[name]
    parts = scope.split(".") if scope else []
    while parts:  # nested lookup from inside a type
        q = ".".join(parts) + "." + name
        if q in reg: return reg[q]
        parts.pop()
    if name in by_simple:
        tops = [q for q in by_simple[name] if "." not in q]
        q = tops[0] if tops else by_simple[name][0]
        return reg[q]
    return None
def cpp(t, scope="", generic_params=()):
    t = re.sub(r"^(global::)?System\.Text\.Json\.(Serialization\.)?", "", t.strip())
    if not t: return t
    nullable = False
    if t.endswith("?"): nullable = True; t = t[:-1].strip()
    if t.startswith("(") and t.endswith(")"):
        els = split_args(t[1:-1]); typed = []
        for i, e in enumerate(els):
            m = re.match(r"^(.*\S)\s+(\w+)$", e)
            typed.append((cpp(m.group(1), scope, generic_params), m.group(2)) if m and not m.group(1).endswith(",") else (cpp(e, scope, generic_params), f"Item{i+1}"))
        import hashlib; name = "Tup_" + "_".join(n for _, n in typed) + "_" + hashlib.md5(repr(typed).encode()).hexdigest()[:5]
        tuples[name] = typed; r = name
        return f"cs::Nullable<{r}>" if nullable else r
    if t.endswith("[]"):
        r = f"cs::List<{cpp(t[:-2], scope, generic_params)}>"; return r
    if t.endswith("[,]"):
        r = f"cs::List<cs::List<{cpp(t[:-3], scope, generic_params)}>>"; return r
    m = re.match(r"^([\w.]+)<(.*)>$", t)
    if m:
        base, args = m.group(1).split(".")[-1], [cpp(a, scope, generic_params) for a in split_args(m.group(2))]
        if base in SEQ: r = f"cs::List<{args[0]}>"
        elif base in DICT: r = f"cs::Dictionary<{args[0]}, {args[1]}>"
        elif base in SETS: r = f"cs::HashSet<{args[0]}>"
        elif base == "KeyValuePair": r = f"cs::KeyValuePair<{args[0]}, {args[1]}>"
        elif base == "Func": r = f"cs::Func<{args[-1]}({', '.join(args[:-1])})>"
        elif base == "Action": r = f"cs::Func<void({', '.join(args)})>"
        elif base == "Comparison": r = f"cs::Func<int32_t({args[0]}, {args[0]})>"
        elif base == "Predicate": r = f"cs::Func<bool({args[0]})>"
        elif base in ("IComparer", "IEqualityComparer"): r = f"cs::Func<int32_t({args[0]}, {args[0]})>"
        elif base == "Nullable": r = f"cs::Nullable<{args[0]}>"
        elif base == "Lazy": r = args[0]
        elif base == "ConditionalWeakTable": r = f"cs::Dictionary<{args[0]}, {args[1]}>"
        elif base == "Task": r = args[0]
        else: unknown[base + "<>"] += 1; r = f"/*?{base}*/{base}<{', '.join(args)}>"
        return f"cs::Nullable<{r}>" if nullable and not r.startswith(("cs::List", "cs::Dict", "cs::HashSet", "cs::Func")) else r
    if t in PRIM:
        r = PRIM[t]; return f"cs::Nullable<{r}>" if nullable and t not in ("string", "object") else r
    if t in generic_params: return t
    if t in ("Action",): return "cs::Func<void()>"
    if t in VALUE_SYS: r = VALUE_SYS[t]; return f"cs::Nullable<{r}>" if nullable else r
    if t in REF_SYS: return f"cs::Ref<{REF_SYS[t]}>"
    res = resolve(t, scope)
    if res is None:
        unknown[t] += 1; return f"/*?*/{t}"
    name, kind = res
    if kind in ("class", "interface", "static"): return f"cs::Ref<{name}>"
    return f"cs::Nullable<{name}>" if nullable else name
if __name__ == "__main__":
    types = set()
    for t in T:
        scope = (t["outer"] + "." if t["outer"] else "") + t["name"]
        for s in t["fields"] + t["autoProps"] + t["compProps"]:
            m = re.match(r"^((?:public|private|internal|protected|static|readonly|const|required|override|virtual|abstract|sealed|new|volatile)\s+)*(.+?)\s+(\w+)(\s*\[.*\])?$", s)
            if m: cpp(m.group(2), scope)
        for s in t["methods"]:
            m = re.match(r"^((?:public|private|internal|protected|static|override|virtual|abstract|sealed|new|extern|unsafe|partial|async|explicit|implicit)\s+)*(.*?)\s*(\w+|ctor \w+|operator \S+)(<[^>]*>)?\((.*)\)$", s)
            if not m: continue
            gp = tuple(re.findall(r"\w+", m.group(4) or ""))
            if m.group(2) and m.group(2) != "ctor": cpp(m.group(2), scope, gp)
            for p in split_args(m.group(5)):
                p = re.sub(r"^(this|ref|out|in|params)\s+", "", p.split("=")[0].strip())
                mm = re.match(r"^(.*\S)\s+(\w+)$", p)
                if mm: cpp(mm.group(1), scope, gp)
    print("unmapped types:", unknown.most_common(40))
    print("tuple structs:", len(tuples))
