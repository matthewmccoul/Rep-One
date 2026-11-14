#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct RaiseOutcome {
    bool Raised = false;
    bool Complications = false;
    bool Unequipped = false;
    cs::List<cs::String> Lines = cs::List<cs::String>::New();
};

struct Raising {
    static constexpr int32_t ByRaiseDead = 20;
    static constexpr int32_t ByRestoreFlesh = 21;
    static constexpr int32_t ByResurrect = 22;
    static constexpr int32_t ByMorgue = 23;

    static constexpr int32_t AgeDaysOrdinary = 54;
    static constexpr int32_t AgeDaysStoned = 84;
    static constexpr int32_t AgeDaysRock = 124;

    static constexpr int32_t ComplicationAgeDays = 5475;

    static constexpr double ComplicationHitsDivisor = 1.2;

    static double ComplicationChance(int32_t ageDays, int32_t raceLifespanYears);
    static bool Complicates(cs::Ref<cs::Random> rng, cs::Ref<Character> c, int32_t raceLifespanYears);
    static int32_t AgeDaysFor(int32_t causeOfDeath);
    static void ApplyComplications(cs::Ref<Character> c, cs::Ref<Race> race, cs::Func<cs::Ref<Item>(int16_t)> lookup,
                                   cs::Ref<RaiseOutcome> outcome, bool inAParty = false);
    static cs::Ref<RaiseOutcome> Resolve(cs::Ref<cs::Random> rng, cs::Ref<Character> c, cs::Ref<Race> race, int32_t kind,
                                         int32_t chance, cs::Func<cs::Ref<Item>(int16_t)> lookup, bool ownBody = false,
                                         bool inAParty = false);
    static cs::String StatDecreased(cs::Ref<Character> c, int32_t stat, int32_t by, bool inAParty = false);
};

}  // namespace mc
