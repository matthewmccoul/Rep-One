#include "core.h"

namespace mc {

bool PendingRaise::IsPending(cs::Ref<Character> c) { return c->RaiseChance > 0; }

void PendingRaise::Initiate(cs::Ref<Character> body, cs::Ref<Character> raiser, int32_t chance, int32_t kind) {
    body->X = raiser->X;
    body->Y = raiser->Y;
    body->DungeonLevel = raiser->DungeonLevel;
    body->Area = raiser->Area;   // "l0830.M59BB = Chars(Me).M59BB" (MAINMOD2.BAS:5992)
    if (chance > 0) {
        body->RaiseChance = chance;
        body->RaisedBy = cs::Str::TrimEnd(raiser->Name);
        body->RaiseKind = kind;
    }
}

void PendingRaise::Prepare(cs::Ref<Character> c) {
    if (c->CauseOfDeath == Rules::DeathRock) c->RaiseChance = 0;
}

cs::String PendingRaise::Offer(cs::Ref<Character> c, bool alone) {
    cs::String by = "\"" + cs::Str::TrimEnd(c->RaisedBy) + "\"";
    cs::String name = "\"" + cs::Str::TrimEnd(c->Name) + "\"";
    cs::String chance = "Chance of success is" + VbStr(c->RaiseChance) + "%.";
    return alone
        ? by + " has initiated a raise on your character.\n" + chance + "\nDo you accept?"
        : by + " has initiated a raise on " + name + ".\n" + chance + "\nDoes " + name + " accept?";
}

cs::Ref<RaiseOutcome> PendingRaise::Accept(cs::Ref<cs::Random> rng, cs::Ref<Character> c, cs::Ref<Race> race, cs::Func<cs::Ref<Item>(int16_t)> lookup, bool alone, cs::String& raiser) {
    raiser = cs::Str::TrimEnd(c->RaisedBy);
    auto o = Raising::Resolve(rng, c, race, c->RaiseKind, c->RaiseChance, lookup, false, !alone);
    if (!o->Raised) {
        c->RaiseKind = 0;
        c->RaiseChance = 0;
        c->RaisedBy = "";
        return o;
    }
    c->RaisedBy = " ";
    c->RaiseChance = 0;
    return o;
}

void PendingRaise::Forget(cs::Ref<Character> c) {
    c->RaiseChance = 0;
    c->RaisedBy = " ";
}

cs::String PendingRaise::BoxTitleFor(cs::String line) {
    if (line == "Raise Failed!") return "Raise Info";
    if (line == "Success!") return "RAISE";
    if (line == "There are complications...") return "Raise Info";
    if (line == "One or more items were unequipped due to lack of stats...") return "Notice!";
    return cs::String();
}

cs::String PendingRaise::VbStr(int64_t n) {
    return n < 0 ? cs::ToStr(n) : " " + cs::ToStr(n);
}

int64_t Morgue::RaiseCost(int32_t maxHits) {
    return cs::CastDouble<int64_t>(cs::Math::Round(2912.32 * (maxHits / 1.5), cs::MidpointRounding::ToEven));
}

int32_t Morgue::RaiseChance(int32_t con, int32_t causeOfDeath) { return Rules::SpellRaiseChance(con, -1, causeOfDeath); }

cs::String Morgue::CostOffer(cs::String name, int64_t cost) {
    return "Cost to raise " + cs::Str::TrimEnd(name) + " is " + FormatNumber(cost) + ".\nDo you wish to pay?";
}

int64_t Morgue::CompanionCost(int32_t maxHits, int32_t floor) {
    return cs::CastDouble<int64_t>(cs::Math::Round(maxHits * 1244.34 * floor, cs::MidpointRounding::ToEven));
}

cs::String Morgue::CompanionCaption(cs::String companionName, int64_t cost) {
    return "Raise " + companionName + " for " + FormatNumber(cost);
}

int32_t Morgue::CompanionChance(int32_t con) { return cs::Math::Min(98, 75 + con); }

cs::String Morgue::FormatNumber(int64_t v) {
    cs::String t = cs::Format(v, "###,###,###,###,###");
    return t == "" ? "0" : t;
}

}  // namespace mc
