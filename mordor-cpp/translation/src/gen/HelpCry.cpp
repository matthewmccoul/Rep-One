#include "core.h"

namespace mc {

bool HelpCry::Needed(cs::Ref<Character> c) { return c->CurHits < 10 || c->CurHits <= c->MaxHits * 0.1; }

cs::String HelpCry::Line(cs::Ref<Character> c) {
    return cs::Str::TrimEnd(c->Name) + " - \"Help! My hits are " + cs::ToStr(c->CurHits) + "/" + cs::ToStr(c->MaxHits) + "!";
}

cs::String HelpCry::Paralysis(cs::Ref<Character> c) { return cs::Str::TrimEnd(c->Name) + " - \"I've been Paralyzed!"; }

}  // namespace mc
