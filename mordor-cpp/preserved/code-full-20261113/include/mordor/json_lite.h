// Minimal JSON reader used only to load the data tables (the console build). Unreal fills GameData from DataTables instead.
#pragma once
#include <utility>
#include "mordor/types.h"

namespace mordor {

struct JsonValue {
  enum class Type { Null, Bool, Number, String, Array, Object };
  Type type = Type::Null;
  bool b = false;
  double num = 0;
  Str str;
  Vec<JsonValue> arr;
  Vec<::std::pair<Str, JsonValue>> obj;

  const JsonValue* find(const char* key) const {
    for (const auto& kv : obj) if (kv.first == key) return &kv.second;
    return nullptr;
  }
  i64 asInt(i64 def = 0) const { return type == Type::Number ? static_cast<i64>(num) : (type == Type::Bool ? (b ? 1 : 0) : def); }
  double asDouble(double def = 0) const { return type == Type::Number ? num : def; }
  bool asBool(bool def = false) const { return type == Type::Bool ? b : (type == Type::Number ? num != 0 : def); }
  Str asString(const Str& def = Str()) const { return type == Type::String ? str : def; }
};

// Returns false and fills err on malformed input. No exceptions.
bool parseJson(const Str& text, JsonValue& out, Str& err);

}  // namespace mordor
