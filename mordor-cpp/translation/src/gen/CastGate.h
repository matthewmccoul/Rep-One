#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CastGate {
    static constexpr int32_t HealSchool = 9;
    static SpellUse UseOf(cs::Ref<Spell> s);
    static cs::Ref<Refusal> Unfit(cs::Ref<Character> caster, cs::Ref<Spell> s, int32_t itemLevel, bool isYou, cs::Func<cs::Ref<Item>(int16_t)> item);
    static cs::Ref<Refusal> Refuse(cs::Ref<Character> caster, cs::Ref<Spell> s, int32_t cost, bool isYou,
                                   bool monstersPresent, bool engaged, bool antiMagic);
    inline static const cs::String AntiMagic = "Anti-Magic area!";
    static constexpr int32_t PeacedSpellLevel = 7;
};

struct Refusal {
    RefusalKind Kind{};
    cs::String Text{};
    Refusal() = default;
    Refusal(RefusalKind Kind, cs::String Text) : Kind(Kind), Text(Text) {}
    bool ClearsItemSlot();
    bool operator==(const Refusal& o) const { return Kind == o.Kind && Text == o.Text; }
    bool operator!=(const Refusal& o) const { return !(*this == o); }
};

}  // namespace mc
