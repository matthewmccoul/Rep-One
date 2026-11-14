#include "core.h"

namespace mc {

bool MessageList::Add(cs::List<cs::String> lines, cs::String line, int32_t kept) {
    if (cs::Str::IsNullOrWhiteSpace(line)) return false;
    lines.Add(line);
    kept = cs::Math::Max(1, kept);
    if (lines.Count() > kept) lines.RemoveRange(0, lines.Count() - kept);
    return true;
}

bool MessageList::Add(cs::List<cs::String> lines, cs::List<MessageLook> looks, cs::String line, MessageLook look, int32_t kept) {
    if (looks.Count() != lines.Count()) {
        looks.Clear();
        looks.AddRange(cs::Enumerable::Repeat(MessageLook::Plain, lines.Count()));
    }
    if (!Add(lines, line, kept)) return false;
    looks.Add(look);
    if (looks.Count() > lines.Count()) looks.RemoveRange(0, looks.Count() - lines.Count());
    return true;
}

}  // namespace mc
