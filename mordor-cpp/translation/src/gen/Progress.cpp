#include "core.h"

namespace mc {

Tup_State_Amount_0a0cd Progress::ToolbarNeed(cs::Ref<Character> c) {
    int64_t needed = c->ExpForNextLevel();
    if (c->Guild()->Experience < needed) return Tup_State_Amount_0a0cd{NeedState::Short, needed - c->Guild()->Experience};
    int64_t cap = c->ExperienceCap();
    return c->Guild()->Experience == cap ? Tup_State_Amount_0a0cd{NeedState::Pinned, 0} : Tup_State_Amount_0a0cd{NeedState::Ready, cap - c->Guild()->Experience};
}

cs::String Progress::NextLevel(cs::Ref<Character> c) {
    int32_t next = c->Guild()->Level + 1;
    int64_t gold = c->GoldForNextLevel();
    int64_t needed = c->ExpForNextLevel();
    if (c->Guild()->Experience < needed)
        return "You need " + cs::Format(needed - c->Guild()->Experience, "N0") + " more experience and " + cs::Format(gold, "N0") + " gold for level " + cs::ToStr(next) + ".";

    int64_t cap = c->ExperienceCap();
    if (c->Guild()->Experience == cap)
        return "You are Pinned and need " + cs::Format(gold, "N0") + " gold for level " + cs::ToStr(next) + ".";

    return "You need " + cs::Format(cap - c->Guild()->Experience, "N0") + " more experience to pin and " + cs::Format(gold, "N0") + " gold for level " + cs::ToStr(next) + ".";
}

cs::String Progress::Quest(cs::Ref<Character> c, cs::Func<cs::Ref<Monster>(int16_t)> monster, cs::Func<cs::Ref<Item>(int16_t)> item) {
    auto g = c->Guild();
    if (g->QuestType <= 0) return cs::String{};
    if (g->QuestDone) return "Your quest has been completed.";
    if (g->QuestType == 1) {
        auto m = monster(g->QuestTarget);
        return "You must defeat one " + (m ? cs::Str::Trim(m->Name) : cs::String("creature")) + ".";
    } else if (g->QuestType == 2) {
        auto it = item(g->QuestTarget);
        return "You must return with one " + (it ? cs::Str::Trim(it->Name) : cs::String("item")) + ".";
    } else {
        return cs::String{};
    }
}

Progress_QuestState Progress::QuestStateOf(cs::Ref<Character> c) {
    auto g = c->Guild();
    if (!(g->QuestType == 1 || g->QuestType == 2)) return QuestState::None;
    return g->QuestDone ? QuestState::Done : QuestState::Active;
}

}  // namespace mc
