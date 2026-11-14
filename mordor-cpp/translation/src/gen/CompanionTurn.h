#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CompanionTurn_Outcome {
    bool Turned = false;
    cs::Ref<CombatMonster> Became;
    cs::String Message{};
    bool Refused = false;
};

struct CompanionTurn {
    using Outcome = CompanionTurn_Outcome;

    static constexpr int32_t GroupSlots = 4;

    static cs::String NameOf(cs::Ref<Character> who, int32_t index);
    static cs::Ref<CompanionTurn_Outcome> Check(cs::Ref<Character> who, int32_t index, bool forced, cs::Ref<cs::Random> rng,
                                                cs::Func<cs::Ref<Monster>(cs::Ref<Companion>)> monsterDef,
                                                cs::List<cs::Ref<CombatMonster>> fight,
                                                bool isLeader, int32_t groupsPresent);
    static int32_t MonsterAlignment(cs::Ref<Monster> def, cs::Ref<Companion> c);
    static cs::String KickOutProblem(cs::Ref<Character> who, int32_t index, bool inDungeon);
    static cs::String KickOutQuestion(cs::Ref<Character> who, int32_t index);
};

}  // namespace mc
