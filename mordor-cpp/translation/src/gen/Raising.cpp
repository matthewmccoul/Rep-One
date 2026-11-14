#include "core.h"

namespace mc {

double Raising::ComplicationChance(int32_t ageDays, int32_t raceLifespanYears) {
    return raceLifespanYears <= 0 ? 0 : ageDays / 365.0 / (raceLifespanYears / 2.0) * 100;
}

bool Raising::Complicates(cs::Ref<cs::Random> rng, cs::Ref<Character> c, int32_t raceLifespanYears) {
    return c->CauseOfDeath == Rules::DeathRock ||
           rng->Next(100) < ComplicationChance(c->Age, raceLifespanYears);
}

int32_t Raising::AgeDaysFor(int32_t causeOfDeath) {
    if (causeOfDeath == Rules::DeathStoned) return AgeDaysStoned;
    if (causeOfDeath == Rules::DeathRock) return AgeDaysRock;
    return AgeDaysOrdinary;
}

void Raising::ApplyComplications(cs::Ref<Character> c, cs::Ref<Race> race, cs::Func<cs::Ref<Item>(int16_t)> lookup,
                                 cs::Ref<RaiseOutcome> outcome, bool inAParty) {
    outcome->Complications = true;
    outcome->Lines.Add("There are complications...");   // the box comes first (MAINMOD2.BAS:1653)
    c->Complications++;   // M5906(2), MAINMOD2.BAS:1654
    for (int32_t i = 0; i < 6; i++) {
        c->Stats[i] -= 5;
        if (i < race->Min.Count() && c->Stats[i] < race->Min[i]) c->Stats[i] = race->Min[i];
    }

    // mdrcharAddAge True, pv02D4, mc02EA, False (MAINMOD2.BAS:1663)
    outcome->Lines.Add(c->AgeBy(ComplicationAgeDays, inAParty));
    c->MaxHits = cs::CastDouble<int32_t>(c->MaxHits / ComplicationHitsDivisor);
    if (c->MaxHits < 1) c->MaxHits = 1;
    c->CurHits = c->MaxHits;

    // fn11B6: the stat arm of the equip check (GearCheck handles the guild one)
    for (int32_t i = 1; i < Character::PackSize; i++) {
        cs::Ref<InventorySlot> slot = c->Pack()[i];
        if (slot == nullptr || !slot->Equipped || slot->ItemId == 0) continue;
        auto it = lookup(slot->ItemId);
        if (it == nullptr) continue;
        bool tooWeak = false;
        for (int32_t s = 0; s < 6 && s < it->StatsRequired.Count(); s++)
            if (c->Stats[s] < it->StatsRequired[s]) { tooWeak = true; break; }
        if (!tooWeak) continue;
        slot->Equipped = false;
        outcome->Unequipped = true;
    }

    if (outcome->Unequipped) outcome->Lines.Add("One or more items were unequipped due to lack of stats...");
}

cs::Ref<RaiseOutcome> Raising::Resolve(cs::Ref<cs::Random> rng, cs::Ref<Character> c, cs::Ref<Race> race, int32_t kind,
                                       int32_t chance, cs::Func<cs::Ref<Item>(int16_t)> lookup, bool ownBody,
                                       bool inAParty) {
    auto outcome = cs::New<RaiseOutcome>();
    int32_t roll = chance;
    if (c->CauseOfDeath == Rules::DeathRock) roll = 0;

    if (rng->Next(100) > roll) {
        outcome->Lines.Add("Raise Failed!");
        if (!Complicates(rng, c, race->MaxAge)) {
            // mdrcharAddAge True, pv02D4, mc00B4, False (MAINMOD2.BAS:1680)
            outcome->Lines.Add(c->AgeBy(AgeDaysOrdinary, inAParty));
            return outcome;                     // still dead
        }
        ApplyComplications(c, race, lookup, outcome, inAParty);
    } else
        outcome->Lines.Add("Success!");

    // Stone costs a point of Constitution either way (MAINMOD2.BAS:352-355), before the morgue's roll.
    if (c->CauseOfDeath == Rules::DeathStoned) {
        c->Stats[static_cast<int32_t>(Stat::Con)] -= 1;
        outcome->Lines.Add(StatDecreased(c, static_cast<int32_t>(Stat::Con), 1, inAParty));
    }

    // The own-body morgue ages you before its Constitution roll (MAINMOD2.BAS:3109, 3112); fn16B5 after it (363).
    if (ownBody) outcome->Lines.Add(c->AgeBy(AgeDaysFor(c->CauseOfDeath), inAParty));

    // Only the morgue asks for this one (MAINMOD2.BAS:357, mc00C6 = 23).
    if (kind == ByMorgue && rng->NextDouble() * 100 < Rules::RaiseConLossChance(c->Stats[static_cast<int32_t>(Stat::Con)])) {
        // Own body in the morgue: MAINMOD2.BAS:3087-3118, its own line ("Constution" misspelling is original)
        outcome->Lines.Add(ownBody ? cs::String("MORGUE: You lost a point of Constitution during the raise.")
                                   : "MORGUE: \"" + cs::Str::Trim(c->Name) + "\" lost a point of Constution during the raise.");
        c->Stats[static_cast<int32_t>(Stat::Con)] -= 1;
    }
    if (ownBody && static_cast<int32_t>(Stat::Con) < race->Min.Count() &&
        c->Stats[static_cast<int32_t>(Stat::Con)] < race->Min[static_cast<int32_t>(Stat::Con)])
        c->Stats[static_cast<int32_t>(Stat::Con)] = race->Min[static_cast<int32_t>(Stat::Con)];

    if (!ownBody) outcome->Lines.Add(c->AgeBy(AgeDaysFor(c->CauseOfDeath), inAParty));
    c->IsDead = false;
    c->Stoned = false;
    c->CauseOfDeath = Rules::DeathOrdinary;
    // MAINMOD2.BAS:364-371: Raise Dead / Restore Flesh give one hit, Resurrect / morgue give all.
    c->CurHits = (kind == ByResurrect || kind == ByMorgue) ? c->MaxHits : 1;
    outcome->Raised = true;
    return outcome;
}

cs::String Raising::StatDecreased(cs::Ref<Character> c, int32_t stat, int32_t by, bool inAParty) {
    cs::String name = stat >= 0 && stat < RaceData::StatNamesLong.Count() ? RaceData::StatNamesLong[stat] : cs::String("stat");
    if (!inAParty) return "Your " + name + " has decreased by " + cs::ToStr(by) + ".";
    return "\"" + cs::Str::Trim(c->Name) + "\" had " + (c->Sex == Sex::Female ? cs::String("her") : cs::String("his")) + " " +
           name + " decreased by " + cs::ToStr(by) + ".";
}

}  // namespace mc
