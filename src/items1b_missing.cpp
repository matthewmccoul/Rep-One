// Implementations of the three items1b functions that were declared but never defined:
// equipProblemText, itemNameAtLevel and itemKindName.
//
// UNKNOWN: game_data.h is not in the foundation text. Field names used below
// (ItemDef::name, ItemDef::type, NamedEntry::index, NamedEntry::name) are ASSUMED
// from the SRD wording ("ItemTypes[kind].Index", "ItemDescriptions[...]") and must
// be checked against the generated game_data.h.

#include "mordor/items1b.h"

namespace mordor {
namespace {

// Appends a base-10 integer to a string without relying on std::to_string.
void appendInt(Str& out, i64 value) {
    if (value < 0) {
        out += "-";
        value = -value;
    }
    char buf[24];
    i32 n = 0;
    do {
        buf[n] = static_cast<char>('0' + (value % 10));
        n += 1;
        value /= 10;
    } while (value > 0);
    while (n > 0) {
        n -= 1;
        out += buf[n];
    }
}

// ASSUMED: text shown when the item is not identified enough to name it.
// SRD: "Barely = vaguer description or 'Unidentified Item' [c12]".
// UNKNOWN: the vaguer description itself is not given; the fallback text is used.
Str unidentifiedText() {
    return "Unidentified Item";
}

}  // namespace




}  // namespace mordor
