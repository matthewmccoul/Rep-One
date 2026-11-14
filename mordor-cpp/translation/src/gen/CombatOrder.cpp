#include "core.h"

namespace mc {

cs::String CombatOrder::Describe(cs::Func<cs::Ref<Spell>(int16_t)> spell) {
    if (Kind == OrderKind::Fight) return "Fight";
    if (Kind == OrderKind::Defend) return "Defend";
    if (Kind == OrderKind::Spell) {
        auto s = spell(SpellId);
        return s ? "\"" + cs::ToStr(s->Name) + "\"" : cs::String("None");
    }
    return "None";
}

void CombatOrder::ApplyDefault(cs::Ref<Character> c, cs::List<cs::Ref<Spell>> spells) {
    SpellId = 0;
    Target = -1;
    if (c->CombatSetting == 1) { Kind = OrderKind::Fight; return; }
    if (c->CombatSetting == 2) { Kind = OrderKind::Defend; return; }
    if (c->CombatSetting >= 3) {
        // 3 and up name a spell buffer; a spell buffer is a standing cast order (MULTIMOD.BAS:176-185).
        // A pack-slot buffer is sub15CD (174-175), run by the caller (DungeonWindow.CombatSettingItem).
        Kind = OrderKind::None;
        if (spells == nullptr) return;
        int32_t b = c->Buffers.ElementAtOrDefault(c->CombatSetting - 3);
        if (!SpellBuffers::IsSpell(b)) return;
        int32_t idx = SpellBuffers::SpellIndexOf(b);
        if (idx < 0 || idx >= spells.Count()) return;
        Kind = OrderKind::Spell;
        SpellId = spells[idx]->Id;
        return;
    }
    Kind = OrderKind::None;
    return;
}

cs::List<cs::String> CombatOrder::SettingNames() {
    return cs::List<cs::String>::Of({"None", "Fight", "Defend"})
        .Concat(cs::Enumerable::Range(1, SpellBuffers::Count).Select([&](auto& i) { return cs::String("Spell - buffer ") + cs::ToStr(i == 10 ? 0 : i); }))
        .ToArray();
}

void CombatOrder::Clear() { Kind = OrderKind::None; SpellId = 0; Target = -1; }

bool CombatOrder::CheckCastClearsAtt() { return ClearAtt(); }

void CombatOrder::Sub102FMinusOne() {
    Kind = (Kind == OrderKind::None || Kind == OrderKind::Fight) ? OrderKind::Defend : OrderKind::Fight;
    SpellId = 0;
}

bool CombatOrder::ClearAtt() {
    if (!(Kind == OrderKind::Fight || Kind == OrderKind::Defend)) return false;
    Kind = OrderKind::None;
    return true;
}

}  // namespace mc
