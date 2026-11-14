#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CombatOrder {
    OrderKind Kind = OrderKind::None;
    int16_t SpellId = 0;
    int32_t Target = -1;

    cs::String Describe(cs::Func<cs::Ref<Spell>(int16_t)> spell);
    void ApplyDefault(cs::Ref<Character> c, cs::List<cs::Ref<Spell>> spells = {});
    static cs::List<cs::String> SettingNames();
    void Clear();
    bool CheckCastClearsAtt();
    void Sub102FMinusOne();
    bool ClearAtt();
};

}  // namespace mc
