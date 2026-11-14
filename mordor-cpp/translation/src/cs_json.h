// System.Text.Json, as Mordor.Core uses it: a parsed document (Node), a writer that reproduces
// JsonSerializer's output (default encoder escaping, indented or compact), and generic
// ToJson/FromJson for the .NET built-in types. Each Core type's own contract (which properties,
// in which order, under which names, with which ignore conditions and converters) is generated
// from the .NET contract into gen/JsonContracts.cpp as JsonOut/JsonIn, found here by ADL.
#pragma once
namespace cs {
namespace Json {

struct Node {
    enum Kind { Null, True, False, Number, Str, Array, Object } kind = Null;
    String text;                                   // Number: the literal; Str: the value (Latin-1)
    std::vector<Node> items;                       // Array
    std::vector<std::pair<String, Node>> props;    // Object, in document order
    static Node Obj() { Node n; n.kind = Object; return n; }
    static Node Arr() { Node n; n.kind = Array; return n; }
    static Node Num(String t) { Node n; n.kind = Number; n.text = std::move(t); return n; }
    static Node S(String t) { Node n; n.kind = Str; n.text = std::move(t); return n; }
    static Node B(bool b) { Node n; n.kind = b ? True : False; return n; }
    void Add(const String& k, Node v) { props.emplace_back(k, std::move(v)); }
    // JsonElement.TryGetProperty: the first property of that name
    const Node* Find(const String& k) const { for (auto& p : props) if (p.first == k) return &p.second; return nullptr; }
};

// ---------------------------------------------------------------- reading (RFC 8259, as Utf8JsonReader with default options)
struct Parser {
    const String& s; size_t i = 0;
    explicit Parser(const String& t) : s(t) {}
    [[noreturn]] void fail(const char* what) const { throw JsonException(String("invalid JSON: ") + what + " at byte " + std::to_string(i)); }
    void ws() { while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) i++; }
    bool lit(const char* w) { size_t n = std::strlen(w); if (s.compare(i, n, w) == 0) { i += n; return true; } return false; }
    static void put(String& o, unsigned cp) { o += cp <= 0xFF ? (char)cp : '?'; }  // String is Latin-1
    String str() {
        if (s[i] != '"') fail("expected string"); i++;
        String o;
        while (true) {
            if (i >= s.size()) fail("unterminated string");
            unsigned char c = (unsigned char)s[i];
            if (c == '"') { i++; return o; }
            if (c < 0x20) fail("control character in string");
            if (c == '\\') {
                if (++i >= s.size()) fail("bad escape");
                char e = s[i++];
                switch (e) {
                case '"': o += '"'; break; case '\\': o += '\\'; break; case '/': o += '/'; break;
                case 'b': o += '\b'; break; case 'f': o += '\f'; break; case 'n': o += '\n'; break;
                case 'r': o += '\r'; break; case 't': o += '\t'; break;
                case 'u': {
                    auto hex4 = [&]() { if (i + 4 > s.size()) fail("bad \\u escape"); unsigned v = 0;
                        for (int k = 0; k < 4; k++) { char h = s[i++]; v <<= 4;
                            if (h >= '0' && h <= '9') v |= (unsigned)(h - '0'); else if (h >= 'a' && h <= 'f') v |= (unsigned)(h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') v |= (unsigned)(h - 'A' + 10); else fail("bad \\u escape"); }
                        return v; };
                    unsigned v = hex4();
                    if (v >= 0xD800 && v <= 0xDBFF && i + 1 < s.size() && s[i] == '\\' && s[i + 1] == 'u') { i += 2; unsigned lo = hex4(); v = 0x10000 + ((v - 0xD800) << 10) + (lo - 0xDC00); }
                    put(o, v); break;
                }
                default: fail("bad escape");
                }
                continue;
            }
            o += (char)c; i++;   // the text is already decoded (File.ReadAllText), one character per byte
        }
    }
    Node value(int depth) {
        if (depth > 64) fail("depth exceeds 64");
        ws(); if (i >= s.size()) fail("unexpected end");
        char c = s[i];
        if (c == '{') {
            i++; Node n = Node::Obj(); ws();
            if (i < s.size() && s[i] == '}') { i++; return n; }
            while (true) { ws(); if (i >= s.size()) fail("unexpected end"); String k = str(); ws(); if (i >= s.size() || s[i] != ':') fail("expected ':'"); i++;
                Node v = value(depth + 1); n.props.emplace_back(std::move(k), std::move(v)); ws();
                if (i < s.size() && s[i] == ',') { i++; continue; } if (i < s.size() && s[i] == '}') { i++; return n; } fail("expected ',' or '}'"); }
        }
        if (c == '[') {
            i++; Node n = Node::Arr(); ws();
            if (i < s.size() && s[i] == ']') { i++; return n; }
            while (true) { n.items.push_back(value(depth + 1)); ws();
                if (i < s.size() && s[i] == ',') { i++; continue; } if (i < s.size() && s[i] == ']') { i++; return n; } fail("expected ',' or ']'"); }
        }
        if (c == '"') return Node::S(str());
        if (lit("true")) return Node::B(true);
        if (lit("false")) return Node::B(false);
        if (lit("null")) return Node();
        size_t b = i;
        if (s[i] == '-') i++;
        if (i < s.size() && s[i] == '0') i++;
        else if (i < s.size() && s[i] >= '1' && s[i] <= '9') while (i < s.size() && isdigit((unsigned char)s[i])) i++;
        else fail("unexpected character");
        if (i < s.size() && s[i] == '.') { i++; if (i >= s.size() || !isdigit((unsigned char)s[i])) fail("bad number"); while (i < s.size() && isdigit((unsigned char)s[i])) i++; }
        if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) { i++; if (i < s.size() && (s[i] == '+' || s[i] == '-')) i++;
            if (i >= s.size() || !isdigit((unsigned char)s[i])) fail("bad number"); while (i < s.size() && isdigit((unsigned char)s[i])) i++; }
        return Node::Num(s.substr(b, i - b));
    }
};
// JsonDocument.Parse / the reader behind JsonSerializer.Deserialize: one value, then only whitespace
inline Node Parse(const String& text) {
    Parser p(text);
    p.ws(); if (p.i >= text.size()) p.fail("empty input");
    Node n = p.value(0); p.ws();
    if (p.i != text.size()) p.fail("text after the value");
    return n;
}

// ---------------------------------------------------------------- writing (Utf8JsonWriter, JavaScriptEncoder.Default)
inline void Escape(String& o, const String& v) {
    static const char* hx = "0123456789ABCDEF";
    o += '"';
    for (unsigned char c : v) {
        switch (c) {
        case '"': o += "\\u0022"; continue;      // the default encoder writes the HTML-sensitive set as \uXXXX
        case '\\': o += "\\\\"; continue;
        case '\n': o += "\\n"; continue; case '\r': o += "\\r"; continue; case '\t': o += "\\t"; continue;
        case '\b': o += "\\b"; continue; case '\f': o += "\\f"; continue;
        default: break;
        }
        if (c < 0x20 || c >= 0x7F || c == '<' || c == '>' || c == '&' || c == '\'' || c == '+' || c == '`') {
            o += "\\u00"; o += hx[c >> 4]; o += hx[c & 15];
        } else o += (char)c;
    }
    o += '"';
}
inline const char* NewLine() {
#ifdef _WIN32
    return "\r\n";  // Utf8JsonWriter (net8.0) indents with Environment.NewLine
#else
    return "\n";
#endif
}
inline void Write(String& o, const Node& n, bool indent, int depth) {
    auto nl = [&](int d) { if (indent) { o += NewLine(); o.append((size_t)d * 2, ' '); } };
    switch (n.kind) {
    case Node::Null: o += "null"; return;
    case Node::True: o += "true"; return;
    case Node::False: o += "false"; return;
    case Node::Number: o += n.text; return;
    case Node::Str: Escape(o, n.text); return;
    case Node::Array:
        o += '['; if (n.items.empty()) { o += ']'; return; }
        for (size_t k = 0; k < n.items.size(); k++) { if (k) o += ','; nl(depth + 1); Write(o, n.items[k], indent, depth + 1); }
        nl(depth); o += ']'; return;
    case Node::Object:
        o += '{'; if (n.props.empty()) { o += '}'; return; }
        for (size_t k = 0; k < n.props.size(); k++) { if (k) o += ','; nl(depth + 1); Escape(o, n.props[k].first); o += indent ? ": " : ":"; Write(o, n.props[k].second, indent, depth + 1); }
        nl(depth); o += '}'; return;
    }
}
inline String Write(const Node& n, bool indent) { String o; Write(o, n, indent, 0); return o; }

// ---------------------------------------------------------------- numbers as .NET writes and reads them
inline String DoubleText(double d) {
    if (!std::isfinite(d)) throw ArgumentException(".NET number values such as positive and negative infinity cannot be written as valid JSON.");
    return ToStr(d);
}
inline String SingleText(float f) {
    if (!std::isfinite(f)) throw ArgumentException(".NET number values such as positive and negative infinity cannot be written as valid JSON.");
    return ToStr(f);
}
inline bool IsIntegerText(const String& t) { return t.find_first_of(".eE") == String::npos; }
// Utf8JsonReader.TryGetInt64 and friends: an integer literal in range
template <class I> bool TryGetInteger(const Node& n, I& out) {
    if (n.kind != Node::Number || !IsIntegerText(n.text)) return false;
    errno = 0;
    if constexpr (std::is_signed<I>::value) { char* e; long long v = std::strtoll(n.text.c_str(), &e, 10);
        if (errno || *e || v < (long long)std::numeric_limits<I>::min() || v > (long long)std::numeric_limits<I>::max()) return false; out = (I)v; }
    else { if (n.text[0] == '-') { if (n.text == "-0") { out = 0; return true; } return false; } char* e; unsigned long long v = std::strtoull(n.text.c_str(), &e, 10);
        if (errno || *e || v > (unsigned long long)std::numeric_limits<I>::max()) return false; out = (I)v; }
    return true;
}
inline double GetDouble(const Node& n) { return std::strtod(n.text.c_str(), nullptr); }
// TryGetDecimal: the literal read exactly (exponent applied to the scale)
inline bool TryGetDecimal(const Node& n, Decimal& out) {
    if (n.kind != Node::Number) return false;
    String t = n.text; int exp = 0; size_t e = t.find_first_of("eE");
    if (e != String::npos) { exp = std::atoi(t.c_str() + e + 1); t = t.substr(0, e); }
    Decimal d; if (!Decimal::TryParse(t, d)) return false;
    if (exp > 0) d = d * Decimal::Raw(Decimal::P10(exp), 0);
    else if (exp < 0) d = Decimal::Raw(d.m, d.scale - exp).Limit();
    out = d; return true;
}

// ---------------------------------------------------------------- generic contracts for the built-in types
template <class T> struct IsList : std::false_type {}; template <class T> struct IsList<List<T>> : std::true_type {};
template <class T> struct IsHashSet : std::false_type {}; template <class T> struct IsHashSet<HashSet<T>> : std::true_type {};
template <class T> struct IsDict : std::false_type {}; template <class K, class V> struct IsDict<Dictionary<K, V>> : std::true_type {};
template <class T> struct IsNullable : std::false_type {}; template <class T> struct IsNullable<std::optional<T>> : std::true_type {};
template <class T> struct IsRef : std::false_type {}; template <class T> struct IsRef<std::shared_ptr<T>> : std::true_type {}; template <class T> struct IsRef<Ref<T>> : std::true_type {};

[[noreturn]] inline void Mismatch(const char* want, const Node& n) {
    static const char* k[] = {"Null", "True", "False", "Number", "String", "StartArray", "StartObject"};
    throw JsonException(String("The JSON value could not be converted to ") + want + ". Token: " + k[n.kind]);
}

template <class K> String KeyText(const K& k) {
    if constexpr (std::is_same<K, String>::value) return k;
    else if constexpr (std::is_enum<K>::value) return ToStr(k);
    else return ToStr(k);
}
template <class K> K KeyFrom(const String& t) {
    if constexpr (std::is_same<K, String>::value) return t;
    else if constexpr (std::is_integral<K>::value) { Node n = Node::Num(t); K v{}; if (!TryGetInteger(n, v)) throw JsonException("The JSON property name '" + t + "' could not be converted to a number."); return v; }
    else { static_assert(sizeof(K) == 0, "dictionary key type not used by Core"); }
}

template <class T> Node ToJson(const T& v);
template <class T> void FromJson(const Node& n, T& out);

template <class T> Node ToJson(const T& v) {
    if constexpr (std::is_same<T, bool>::value) return Node::B(v);
    else if constexpr (std::is_same<T, float>::value) return Node::Num(SingleText(v));
    else if constexpr (std::is_floating_point<T>::value) return Node::Num(DoubleText((double)v));
    else if constexpr (std::is_integral<T>::value) return Node::Num(std::to_string(v));
    else if constexpr (std::is_enum<T>::value) return Node::Num(std::to_string((long long)static_cast<std::underlying_type_t<T>>(v)));  // no JsonStringEnumConverter in Core
    else if constexpr (std::is_same<T, String>::value) return Node::S(v);
    else if constexpr (std::is_same<T, Decimal>::value) return Node::Num(v.ToString());
    else if constexpr (IsNullable<T>::value) return v ? ToJson(*v) : Node();
    else if constexpr (IsList<T>::value || IsHashSet<T>::value) { if (v.IsNull()) return Node(); Node a = Node::Arr(); for (const auto& x : v) a.items.push_back(ToJson(x)); return a; }
    else if constexpr (IsDict<T>::value) { if (v.IsNull()) return Node(); Node o = Node::Obj(); for (const auto& kv : v) o.Add(KeyText(kv.Key), ToJson(kv.Value)); return o; }
    else if constexpr (IsRef<T>::value) { if (!v) return Node(); return JsonOut(*v); }
    else return JsonOut(v);   // a Core struct (generated, found by ADL)
}

template <class T> void FromJson(const Node& n, T& out) {
    if constexpr (std::is_same<T, bool>::value) { if (n.kind == Node::True) out = true; else if (n.kind == Node::False) out = false; else Mismatch("System.Boolean", n); }
    else if constexpr (std::is_floating_point<T>::value) { if (n.kind != Node::Number) Mismatch("a floating-point number", n); out = (T)GetDouble(n); }
    else if constexpr (std::is_integral<T>::value) { if (!TryGetInteger(n, out)) Mismatch("an integer of this size", n); }
    else if constexpr (std::is_enum<T>::value) { std::underlying_type_t<T> u{}; if (!TryGetInteger(n, u)) Mismatch("an enum", n); out = static_cast<T>(u); }
    else if constexpr (std::is_same<T, String>::value) { if (n.kind == Node::Null) out = String(); else if (n.kind == Node::Str) out = n.text; else Mismatch("System.String", n); }  // a C# null string is "" here
    else if constexpr (std::is_same<T, Decimal>::value) { if (!TryGetDecimal(n, out)) Mismatch("System.Decimal", n); }
    else if constexpr (IsNullable<T>::value) { if (n.kind == Node::Null) out.reset(); else { typename T::value_type v{}; FromJson(n, v); out = v; } }
    else if constexpr (IsList<T>::value) {
        if (n.kind == Node::Null) { out = T(); return; } if (n.kind != Node::Array) Mismatch("a collection", n);
        T l = T::New(); for (const auto& x : n.items) { std::decay_t<decltype(l[0])> e{}; FromJson(x, e); l.Add(e); } out = l;
    }
    else if constexpr (IsHashSet<T>::value) {
        if (n.kind == Node::Null) { out = T(); return; } if (n.kind != Node::Array) Mismatch("a collection", n);
        T h = T::New(); for (const auto& x : n.items) { std::decay_t<decltype(*h.begin())> e{}; FromJson(x, e); h.Add(e); } out = h;
    }
    else if constexpr (IsDict<T>::value) {
        if (n.kind == Node::Null) { out = T(); return; } if (n.kind != Node::Object) Mismatch("a dictionary", n);
        T d = T::New();
        for (const auto& kv : n.props) { using KV = std::decay_t<decltype(*d.begin())>; decltype(KV::Value) v{}; FromJson(kv.second, v); d.Set(KeyFrom<decltype(KV::Key)>(kv.first), v); }
        out = d;
    }
    else if constexpr (IsRef<T>::value) {
        if (n.kind == Node::Null) { out = nullptr; return; }
        auto o = New<typename T::element_type>(); JsonIn(n, *o); out = o;
    }
    else { JsonIn(n, out); }
}

// [JsonIgnore(Condition = WhenWritingNull / WhenWritingDefault)]
template <class T> bool IsNullValue(const T& v) {
    if constexpr (IsNullable<T>::value) return !v.has_value();
    else if constexpr (IsRef<T>::value) return !v;
    else if constexpr (IsList<T>::value || IsHashSet<T>::value || IsDict<T>::value) return v.IsNull();
    else return false;   // value types; a String is never null here
}
template <class T> bool IsDefaultValue(const T& v) {
    if constexpr (std::is_arithmetic<T>::value || std::is_enum<T>::value) return v == T{};
    else return IsNullValue(v);
}

// JsonSerializerOptions as Core sets them
enum class JsonIgnoreCondition { Never, Always, WhenWritingDefault, WhenWritingNull };
}  // namespace Json

struct JsonSerializerOptions { bool WriteIndented = false; Json::JsonIgnoreCondition DefaultIgnoreCondition = Json::JsonIgnoreCondition::Never; };

namespace Json {
// the options of the current Serialize/Deserialize call, for generated contracts that honour DefaultIgnoreCondition
inline const JsonSerializerOptions*& CurrentOptions() { static thread_local const JsonSerializerOptions* o = nullptr; return o; }
inline bool IgnoreNullsByDefault() { auto o = CurrentOptions(); return o && o->DefaultIgnoreCondition == JsonIgnoreCondition::WhenWritingNull; }
struct OptionsScope { const JsonSerializerOptions* prev; explicit OptionsScope(const JsonSerializerOptions& o) : prev(CurrentOptions()) { CurrentOptions() = &o; } ~OptionsScope() { CurrentOptions() = prev; } };

// Utf8JsonReader / Utf8JsonWriter as a JsonConverter sees them: positioned on one value / writing one value
enum class JsonTokenType { None, StartObject, EndObject, StartArray, EndArray, PropertyName, Comment, String, Number, True, False, Null };
}  // namespace Json
struct Utf8JsonReader {
    const Json::Node* node = nullptr;
    Json::JsonTokenType TokenType() const {
        using K = Json::Node; using T = Json::JsonTokenType;
        switch (node->kind) { case K::Null: return T::Null; case K::True: return T::True; case K::False: return T::False; case K::Number: return T::Number;
            case K::Str: return T::String; case K::Array: return T::StartArray; case K::Object: return T::StartObject; }
        return T::None;
    }
    void Skip() {}  // the converter's value is consumed whole either way
    bool TryGetInt64(int64_t& v) const { return Json::TryGetInteger(*node, v); }
    bool TryGetInt32(int32_t& v) const { return Json::TryGetInteger(*node, v); }
    double GetDouble() const { if (node->kind != Json::Node::Number) Json::Mismatch("System.Double", *node); return Json::GetDouble(*node); }
    bool TryGetDecimal(Decimal& d) const { return Json::TryGetDecimal(*node, d); }
};
struct Utf8JsonWriter {
    Json::Node* node = nullptr;
    void WriteNumberValue(int64_t v) const { *node = Json::Node::Num(std::to_string(v)); }
    void WriteNumberValue(int32_t v) const { *node = Json::Node::Num(std::to_string(v)); }
    void WriteNumberValue(double v) const { *node = Json::Node::Num(Json::DoubleText(v)); }
    void WriteNumberValue(const Decimal& v) const { *node = Json::Node::Num(v.ToString()); }
    void WriteNullValue() const { *node = Json::Node(); }
    void WriteValue(Json::Node v) const { *node = std::move(v); }   // JsonSerializer.Serialize(writer, value, options)
};
namespace Json {
// JsonSerializer.Deserialize<T>(ref reader, options) inside a converter
template <class T> T DeserializeAt(const Utf8JsonReader& r) { T out{}; FromJson(*r.node, out); return out; }

// JsonSerializer.Serialize(value, options)
template <class T> String Serialize(const T& v, const JsonSerializerOptions& o = JsonSerializerOptions()) {
    OptionsScope sc(o);
    if constexpr (std::is_pointer<T>::value) return Write(v ? JsonOut(*v) : Node(), o.WriteIndented);   // Serialize(this)
    else return Write(ToJson(v), o.WriteIndented);
}
template <class T> String Serialize(const T& v, const Ref<JsonSerializerOptions>& o) { return Serialize(v, o ? *o : JsonSerializerOptions()); }
inline JsonSerializerOptions Indented() { JsonSerializerOptions o; o.WriteIndented = true; return o; }   // new JsonSerializerOptions { WriteIndented = true }
// JsonSerializer.Deserialize<T>(text, options)
template <class T> T Deserialize(const String& text, const JsonSerializerOptions& o = JsonSerializerOptions()) {
    OptionsScope sc(o);
    Node n = Parse(text); T out{}; FromJson(n, out); return out;
}
template <class T> T Deserialize(const String& text, const Ref<JsonSerializerOptions>& o) { return Deserialize<T>(text, o ? *o : JsonSerializerOptions()); }
}  // namespace Json
}  // namespace cs
