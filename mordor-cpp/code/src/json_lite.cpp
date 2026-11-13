#include "mordor/json_lite.h"

#include <cstdlib>

namespace mordor {
namespace {

struct Parser {
  const Str& s;
  std::size_t i = 0;
  Str err;
  explicit Parser(const Str& text) : s(text) {}

  void ws() { while (i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')) ++i; }
  bool fail(const char* m) { if (err.empty()) err = std::string(m) + " at offset " + std::to_string(i); return false; }

  static void putUtf8(Str& out, unsigned cp) {
    if (cp < 0x80) out += static_cast<char>(cp);
    else if (cp < 0x800) { out += static_cast<char>(0xC0 | (cp >> 6)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
    else { out += static_cast<char>(0xE0 | (cp >> 12)); out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
  }

  bool parseString(Str& out) {
    if (i >= s.size() || s[i] != '"') return fail("expected string");
    ++i;
    while (i < s.size()) {
      char c = s[i++];
      if (c == '"') return true;
      if (c != '\\') { out += c; continue; }
      if (i >= s.size()) return fail("bad escape");
      char e = s[i++];
      switch (e) {
        case '"': out += '"'; break; case '\\': out += '\\'; break; case '/': out += '/'; break;
        case 'b': out += '\b'; break; case 'f': out += '\f'; break; case 'n': out += '\n'; break;
        case 'r': out += '\r'; break; case 't': out += '\t'; break;
        case 'u': {
          if (i + 4 > s.size()) return fail("bad unicode escape");
          unsigned cp = static_cast<unsigned>(std::strtoul(s.substr(i, 4).c_str(), nullptr, 16));
          i += 4; putUtf8(out, cp); break;
        }
        default: return fail("unknown escape");
      }
    }
    return fail("unterminated string");
  }

  bool parseValue(JsonValue& v, int depth) {
    if (depth > 200) return fail("nesting too deep");
    ws();
    if (i >= s.size()) return fail("unexpected end");
    char c = s[i];
    if (c == '{') {
      v.type = JsonValue::Type::Object; ++i; ws();
      if (i < s.size() && s[i] == '}') { ++i; return true; }
      for (;;) {
        ws(); Str key;
        if (!parseString(key)) return false;
        ws(); if (i >= s.size() || s[i] != ':') return fail("expected ':'");
        ++i;
        JsonValue child;
        if (!parseValue(child, depth + 1)) return false;
        v.obj.emplace_back(std::move(key), std::move(child));
        ws();
        if (i < s.size() && s[i] == ',') { ++i; continue; }
        if (i < s.size() && s[i] == '}') { ++i; return true; }
        return fail("expected ',' or '}'");
      }
    }
    if (c == '[') {
      v.type = JsonValue::Type::Array; ++i; ws();
      if (i < s.size() && s[i] == ']') { ++i; return true; }
      for (;;) {
        JsonValue child;
        if (!parseValue(child, depth + 1)) return false;
        v.arr.push_back(std::move(child));
        ws();
        if (i < s.size() && s[i] == ',') { ++i; continue; }
        if (i < s.size() && s[i] == ']') { ++i; return true; }
        return fail("expected ',' or ']'");
      }
    }
    if (c == '"') { v.type = JsonValue::Type::String; return parseString(v.str); }
    if (s.compare(i, 4, "true") == 0) { v.type = JsonValue::Type::Bool; v.b = true; i += 4; return true; }
    if (s.compare(i, 5, "false") == 0) { v.type = JsonValue::Type::Bool; v.b = false; i += 5; return true; }
    if (s.compare(i, 4, "null") == 0) { v.type = JsonValue::Type::Null; i += 4; return true; }
    const char* start = s.c_str() + i;
    char* end = nullptr;
    double d = std::strtod(start, &end);
    if (end == start) return fail("unexpected character");
    v.type = JsonValue::Type::Number; v.num = d; i += static_cast<std::size_t>(end - start);
    return true;
  }
};

}  // namespace

bool parseJson(const Str& text, JsonValue& out, Str& err) {
  Parser p(text);
  out = JsonValue();
  if (!p.parseValue(out, 0)) { err = p.err; return false; }
  p.ws();
  if (p.i != text.size()) { err = "trailing characters at offset " + std::to_string(p.i); return false; }
  return true;
}

}  // namespace mordor
