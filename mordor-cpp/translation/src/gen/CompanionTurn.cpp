#include "core.h"

namespace mc {

cs::String CompanionTurn::NameOf(cs::Ref<Character> who, int32_t index) { return who->CompanionName(index); }

cs::Ref<CompanionTurn_Outcome> CompanionTurn::Check(cs::Ref<Character> who, int32_t index, bool forced, cs::Ref<cs::Random> rng,
                                                    cs::Func<cs::Ref<Monster>(cs::Ref<Companion>)> monsterDef,
                                                    cs::List<cs::Ref<CombatMonster>> fight,
                                                    bool isLeader, int32_t groupsPresent) {
    cs::Ref<Companion> c = who->Companions->Index(index);
    if (c == nullptr) return cs::New<CompanionTurn_Outcome>();
    if (c->BindingLevel <= 0 && !forced) return cs::New<CompanionTurn_Outcome>();

    auto def = monsterDef(c);   // Monsters(Companions(p0170).Ref)
    if (def == nullptr) return cs::New<CompanionTurn_Outcome>();

    int32_t mine = static_cast<int32_t>(who->Alignment);   // 0 Good, 1 Neutral, 2 Evil
    int32_t theirs = def->AlignmentValue;                 // Monsters(l0178).Alignment (540-542)
    bool turns = cs::Math::Abs(theirs - mine) > 1;                                // 540
    double evilRoll = rng->NextDouble() * 100;                                    // 541, drawn always
    if (theirs == static_cast<int32_t>(Alignment::Evil) && evilRoll < 75) turns = true;
    double neutralRoll = rng->NextDouble() * 100;                                 // 542, drawn always
    if (theirs == static_cast<int32_t>(Alignment::Neutral) && neutralRoll < 15) turns = true;
    if (forced) turns = true;                                                     // 543

    // A fight already going: the companion will not join monsters two steps from its own
    // alignment, and the whole thing is called off - it does not leave either.
    if (fight != nullptr && fight.Any([&](auto& m) { return m->Alive(); })) {
        // Foes(0): the first slot's record, standing or not; a list built by hand has no slots
        auto first = fight.FirstOrDefault([&](auto& m) { return m->Group == 0; });
        if (!first) first = fight.First([&](auto& m) { return m->Alive(); });
        int32_t foes0 = first->Alignment >= 0 ? first->Alignment : (int32_t)first->Def->AlignmentValue;
        if (cs::Math::Abs(foes0 - theirs) > 1)
            return [&] {
                auto _o = cs::New<CompanionTurn_Outcome>();
                _o->Refused = true;
                _o->Message = isLeader
                    ? cs::String("Your companion refuses to join the current monsters!")
                    : "\"" + cs::Str::TrimEnd(who->Name) + "\" - Your companion refuses to join the current monsters!";
                return _o;
            }();
    }

    // no free slot, no turning: the original looks for an empty one of four and gives up quietly
    if (!turns || groupsPresent >= GroupSlots) return cs::New<CompanionTurn_Outcome>();

    cs::String name = who->CompanionName(index);
    return [&] {
        auto _o = cs::New<CompanionTurn_Outcome>();
        _o->Turned = true;
        _o->Became = [&] {
            auto _m = cs::New<CombatMonster>();
            _m->Def = def;
            _m->set_Hits(c->Hits);
            // Foes(l0176).Alignment = Companions(p0170).Alignment (MONSMOD.BAS:569)
            _m->Alignment = MonsterAlignment(def, c);
            _m->Hostile = true;
            _m->Identified = true;
            return _m;
        }();
        _o->Message = isLeader
            ? (forced ? "You've attacked " + name + "!" : name + " has turned on you!")
            : "\"" + cs::Str::TrimEnd(who->Name) + "\": " + name + " has turned on the party!";
        return _o;
    }();
}

int32_t CompanionTurn::MonsterAlignment(cs::Ref<Monster> def, cs::Ref<Companion> c) {
    if (c != nullptr) {
        int32_t a = static_cast<int32_t>(c->Alignment);
        if (a == static_cast<int32_t>(ItemAlignment::Good)) return static_cast<int32_t>(Alignment::Good);
        if (a == static_cast<int32_t>(ItemAlignment::Neutral)) return static_cast<int32_t>(Alignment::Neutral);
        if (a == static_cast<int32_t>(ItemAlignment::Evil)) return static_cast<int32_t>(Alignment::Evil);
        return def->AlignmentValue;
    }
    return def->AlignmentValue;
}

cs::String CompanionTurn::KickOutProblem(cs::Ref<Character> who, int32_t index, bool inDungeon) {
    cs::Ref<Companion> c = who->Companions->Index(index);
    if (c == nullptr) return "";   // one button per companion in the original: nothing to pick, and nothing said
    if (c->Hits == 0)
        return "You must drop a dead companion you're carrying - not kick them out!";
    if (!inDungeon) return "You cannot kick out companions in the city!";
    return cs::String();
}

cs::String CompanionTurn::KickOutQuestion(cs::Ref<Character> who, int32_t index) {
    cs::String q = "Kick out " + who->CompanionName(index) + "?";
    cs::Ref<Companion> c = who->Companions->Index(index);
    if (c != nullptr && c->BindingLevel > 0)
        q += "\r\n\r\nWARNING: Since the companion is bound, you will risk them turning on you!";
    return q;
}

}  // namespace mc
