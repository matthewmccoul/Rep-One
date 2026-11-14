#include "core.h"

namespace mc {

cs::List<cs::String> CompanionCarry::Run(cs::Ref<Character> who, bool inCommand, cs::Func<cs::Ref<Monster>(cs::Ref<Companion>)> monsterOf) {
    auto said = cs::List<cs::String>::New();
    auto slots = who->Companions;
    const int32_t four = CompanionSlots::Slots;  // mc00E0

    if (who->CarryingCompanion < four && who->CarryingCharacter == "") {
        int32_t best = -1;  // l0364
        for (int32_t i = 0; i < four; i++) {
            auto c = slots->Index(i);
            if (c == nullptr || c->Hits > 0) continue;
            c->Hits = 0;
            if (best > -1) {
                if (c->MaxHits > slots->Index(best)->MaxHits) {
                    // fn128B(pv0360, M5943) with M5943 = -1 reads Companions(-1): the error
                    // handler resumes at the End Sub (mdrcharGetCarry, MAINMOD2.BAS:2079-2082)
                    if (who->CarryingCompanion < 0) return said;
                    said.Add("\"" + cs::Str::TrimEnd(who->Name) + "\" dropped " + cs::ToStr(who->CompanionName(who->CarryingCompanion)) + " to carry " + cs::ToStr(who->CompanionName(i)) + ".");
                    best = i;
                }
            } else
                best = i;
        }

        if (best > -1 && who->CarryingCompanion != best) {
            // Monsters(Ref).M5565 - Races(RaceIndex).M5565 <= 1 (MAINMOD2.BAS:2050); a monster
            // the data cannot find would be Monsters() out of range, the same handler again
            auto def = monsterOf(slots->Index(best));
            if (def == nullptr) return said;
            if ((int32_t)def->Size - static_cast<int32_t>(RaceData::ByName(who->Race)->Size) <= 1) {
                who->CarryingCompanion = best;
                said.Add(inCommand
                    ? "You are now carrying " + cs::ToStr(who->CompanionName(best)) + "."
                    : cs::Str::TrimEnd(who->Name) + ", you are now carrying " + cs::ToStr(who->CompanionName(best)) + ".");
            } else {
                cs::String name = who->CompanionName(best);
                slots->Release(best);
                said.Add(inCommand
                    ? name + " was too big to carry."
                    : cs::Str::TrimEnd(who->Name) + ", " + name + " was too big to carry.");
            }
            // mdrcharShowCompanions pv0360 (MAINMOD2.BAS:2067), which blanks the empty slots' names
            // when the owner is the one on screen - before the sweep below lets any more go
            if (inCommand) slots->BlankEmptyNames();
        }
    }

    // the sweep (MAINMOD2.BAS:2071-2077): Hits = 0 and not the one carried. A morgue body is
    // four and up, so no slot number matches it; the port keeps it apart as CarryingCharacter
    for (int32_t i = 0; i < four; i++) {
        auto c = slots->Index(i);
        if (c != nullptr && c->Hits == 0
            && (who->CarryingCharacter != "" || who->CarryingCompanion != i))
            slots->Release(i);
    }
    return said;
}

}  // namespace mc
