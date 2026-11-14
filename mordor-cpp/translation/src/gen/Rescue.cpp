#include "core.h"

namespace mc {

int32_t Rescue::DaysUntilFound(cs::Ref<cs::Random> rng, int32_t dungeonLevel, int32_t areaId, bool easyToFind) {
    if (dungeonLevel < 1) return 0;
    int32_t days = 0;
    double threshold = cs::Math::Log(dungeonLevel) * cs::Math::Log(dungeonLevel) * 100 + 21;
    while (true) {
        int32_t ceiling = days < threshold ? 10000 : 1000;
        if (dungeonLevel == 1 && areaId <= 18) ceiling = 100;
        days++;

        double chance = 16 - dungeonLevel;
        if (chance < 0.1) chance = 0.1;
        if (easyToFind) chance += 16 - dungeonLevel;

        if (rng->NextDouble() * ceiling <= chance) return days;
        if (days > 2000000) return days;   // the original would spin here; nothing else can
    }
}

cs::String Rescue::Found(int32_t days) {
    return "You have been found after " + cs::ToStr(days) + " day(s)!";
}

bool Rescue::EasyToFind(cs::Ref<Floor> floor, int32_t x, int32_t y) {
    return floor->Has(x, y, Floor::RESCUE);
}

void Rescue::CarryHome(cs::Ref<Character> body, int32_t days) {
    body->Age += days;
    body->DungeonLevel = 0;
    body->Area = -1;
    // X and Y are left as they were: FRMDUNM.BAS:187-188 sets only DungeonLevel and M59BB
}

void Rescue::TurnOverTown(cs::String savesDir, cs::List<cs::Ref<Item>> items, cs::List<cs::Ref<Monster>> monsters,
                          cs::Ref<cs::Random> rng, int32_t deepestReached) {
    auto shelf = StoreStock::Load(savesDir, items);
    shelf->Replenish(items, 0, rng, deepestReached);
    shelf->Save(savesDir);
    auto pens = Confinement::Load(savesDir, monsters);
    pens->Replenish(monsters, 0, rng, deepestReached);
    pens->Save(savesDir);
}

// ---- the morgue's rescuers, FRMMORG.BAS:104-176 ----

int64_t Rescue::SearchCost(int32_t level) {
    return cs::CastDouble<int64_t>(372567.52 * cs::Math::Log(level + 1) * level);
}

cs::String Rescue::Offer(int64_t cost, cs::String name) {
    return "Cost will be " + cs::Format(cost, "N0") + " to search for " + name + ".\nDo you accept?";
}

bool Rescue::RescuersReturn(cs::Ref<cs::Random> rng, int32_t level) {
    return rng->NextDouble() * 100 > level * 4;
}

int32_t Rescue::Val(cs::String s) {
    auto t = cs::New<cs::StringBuilder>();
    bool point = false;
    for (auto& ch : s) {
        if (ch == ' ' || ch == '\t') continue;
        if ((ch == '-' || ch == '+') && t->Length() == 0) { t->Append(ch); continue; }
        if (ch >= '0' && ch <= '9') { t->Append(ch); continue; }
        if (ch == '.' && !point) { point = true; t->Append(ch); continue; }
        break;
    }
    double v = 0;
    return cs::Parse::TryParse(t->ToString(), v)
        ? cs::CastDouble<int32_t>(cs::Math::Round(v, cs::MidpointRounding::ToEven)) : 0;
}

cs::List<int32_t> Rescue::SplitCoords(cs::String s) {
    auto pieces = cs::List<int32_t>::New();
    int32_t at = 0, start = 1;
    do {
        at++;
        if (cs::Str::Substring(s, at - 1, 1) == "," || at == (int32_t)s.size()) {
            if (at == (int32_t)s.size()) at++;
            int32_t len = cs::Math::Min(at - start, (int32_t)s.size() - (start - 1));
            pieces.Add(Val(len > 0 ? cs::Str::Substring(s, start - 1, len) : cs::String("")));
            start = at + 1;
        }
    } while (at < (int32_t)s.size());
    return pieces;
}

bool Rescue::CloseEnough(int32_t guessX, int32_t guessY, int32_t guessLevel, int32_t x, int32_t y, int32_t level) {
    return cs::Math::Abs(guessX - x) < 8 && cs::Math::Abs(guessY - y) < 8 && guessLevel == level;
}

Rescue_Outcome Rescue::Report(cs::Ref<Character> who, bool foundAtAll) {
    if (!foundAtAll)
        return Rescue_Outcome(cs::Str::TrimEnd(who->Name) + " was not found at or around the specified location.", false);

    if (!who->IsDead)
        return who->CurHits <= who->MaxHits * 0.2
            ? Rescue_Outcome(cs::Str::TrimEnd(who->Name) + " was found alive, and in pretty bad shape, so we healed them and brought them back!", true)
            : Rescue_Outcome(cs::Str::TrimEnd(who->Name) + " was found, but was alive and well!", false);

    if (who->CauseOfDeath == Rules::DeathRock)
        return Rescue_Outcome("The only thing they saw were feet sticking out of rock.", false);

    // one-based, as typed
    return Rescue_Outcome(cs::Str::TrimEnd(who->Name) + " was found at " + cs::ToStr(who->X + 1) + "," + cs::ToStr(who->Y + 1) + "," + cs::ToStr(who->DungeonLevel) + " and brought to the morgue!", true);
}

}  // namespace mc
