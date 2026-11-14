#include "core.h"

namespace mc {

cs::String SpellThrown::Line(cs::Ref<Party> party, cs::Ref<Spell> s, cs::Ref<SpellThrown_Target> t, cs::Ref<Character> commander) {
    cs::String q = "\"" + cs::Str::TrimEnd(s->Name) + "\"";
    auto me = commander ? commander : party->Selected();
    if (std::dynamic_pointer_cast<SpellThrown_Target_Party>(t)) return q + " thrown on the party.";
    if (std::dynamic_pointer_cast<SpellThrown_Target_Nobody>(t)) return q + " thrown.";
    if (auto c = std::dynamic_pointer_cast<SpellThrown_Target_Companion>(t))
        return q + " thrown on " + me->CompanionName(c->Slot) + ".";
    if (auto p = std::dynamic_pointer_cast<SpellThrown_Target_Person>(t)) {
        if (party->Members.Count() <= 1) return q + " thrown on yourself.";
        if (p->Who == me)
            return "\"" + cs::Str::TrimEnd(me->Name) + "\" threw " + q + " on " + (me->Sex == Sex::Female ? "herself" : "himself") + ".";
        return "\"" + cs::Str::TrimEnd(me->Name) + "\" threw " + q + " on \"" + cs::Str::TrimEnd(p->Who->Name) + "\".";
    }
    return q + " was thrown...";
}

bool SpellThrown::Handles(cs::Ref<Spell> s) {
    int32_t e = SpellEffect::Of(s);
    return (e == SpellEffect::Damage || e == SpellEffect::Cure || e == SpellEffect::CurePoison || e == SpellEffect::CureDisease
            || e == SpellEffect::CureParalysis || e == SpellEffect::Levitate || e == SpellEffect::Invisible
            || e == SpellEffect::SeeInvisible || e == SpellEffect::Protect || e == SpellEffect::Fate || e == SpellEffect::SetSanctuary)
        || SpellEffect::ResistKind(e) >= 0;
}

cs::String SpellThrown::Apply(cs::Ref<Party> party, cs::Ref<Character> caster, cs::Ref<Spell> s, cs::Ref<SpellThrown_Target> t, int32_t healLevel,
                              cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> item, cs::Ref<CombatEngine> fight, cs::Ref<Character> commander) {
    int32_t e = SpellEffect::Of(s);
    cs::List<cs::Ref<Character>> people;
    if (std::dynamic_pointer_cast<SpellThrown_Target_Party>(t)) people = party->Members.ToList();
    else if (auto p = std::dynamic_pointer_cast<SpellThrown_Target_Person>(t)) people = cs::List<cs::Ref<Character>>::Of({p->Who});
    else people = cs::List<cs::Ref<Character>>::New();

    if (e == SpellEffect::Damage || e == SpellEffect::Cure) {
        // "If SpellDefs(pv01A8).M5B33 > 0 Then" (MAINMOD2.BAS:931): with none, the line names the first character
        do {
            if (s->Damage2 <= 0) {
                if (std::dynamic_pointer_cast<SpellThrown_Target_Nobody>(t)) t = cs::New<SpellThrown_Target_Person>(party->Members[0]);
                break;
            }
            if (auto comp = std::dynamic_pointer_cast<SpellThrown_Target_Companion>(t)) {
                cs::Ref<Companion> c = caster->Companions->Index(comp->Slot);
                if (c != nullptr) {
                    c->Hits = cs::Math::Min(c->MaxHits, c->Hits + Rules::SpellHeal(rng, s->Damage2, s->SpecialEffect + 1, healLevel));
                }
                break;
            }
            for (auto& who : people) {
                who->CurHits = cs::Math::Min(who->MaxHits, who->CurHits + Rules::SpellHeal(rng, s->Damage2, s->SpecialEffect + 1, healLevel));
                // Ailments(0) and (1) = False (MAINMOD2.BAS:974-977)
                if (e == SpellEffect::Cure) { who->CurePoison(); who->CureDisease(); }
            }
        } while (false);
    } else if (e == SpellEffect::CurePoison) {
        for (auto& who : people) who->CurePoison();    // 989
    } else if (e == SpellEffect::CureDisease) {
        for (auto& who : people) who->CureDisease();   // 996
    } else if (e == SpellEffect::CureParalysis) {
        // MAINMOD2.BAS:925-927: the caster, fight or no fight
        if (caster->ParalysisLevel > 0) caster->Unparalyse(item);
    } else if (e == SpellEffect::Levitate || e == SpellEffect::Invisible || e == SpellEffect::SeeInvisible || e == SpellEffect::Protect) {
        // MAINMOD2.BAS:1008-1009, 1023-1024, 1033-1034, 1046-1047: no recomputeEffects
        for (auto& who : people) {
            who->SpellEffects |= 1 << e;
            if (e == SpellEffect::Levitate) who->Levitating = true;              // Ailments(3) = 1
            else if (e == SpellEffect::Invisible) who->InvisibleNow = true;      // Ailments(4) = 1
            else if (e == SpellEffect::SeeInvisible) who->SeeingInvisible = true; // Ailments(5) = True
            // Ailments(6) = Int(casterSpellLevel(pv01A6, pv01A8) / 2) + 1 (1046)
            else who->Protection = Spellbook::CasterSpellLevel(caster, s) / 2 + 1;
        }
    } else if (e == SpellEffect::Fate) {
        rng->Next(3);                                            // l031E, never used
        caster->CurHits = cs::CastDouble<int32_t>(rng->NextDouble() * caster->MaxHits) + 1;
        caster->CurSpellPoints = cs::CastDouble<int32_t>(rng->NextDouble() * caster->MaxSpellPoints) + 1;
        t = cs::New<SpellThrown_Target_Person>(party->Members[0]);
    } else if (e == SpellEffect::SetSanctuary) {
        // stored 1-based, so that a zero means the spell has never been cast
        caster->SanctuaryX = caster->X + 1; caster->SanctuaryY = caster->Y + 1; caster->SanctuaryLevel = caster->DungeonLevel;
        t = cs::New<SpellThrown_Target_Nobody>();
    } else {
        int32_t kind = SpellEffect::ResistKind(e);
        if (kind >= 0)
            for (auto& who : people)
                if (who->SpellResist[kind] < SpellEffect::ResistGranted) {
                    who->SpellResist[kind] = SpellEffect::ResistGranted;
                    Effects::Recompute(who, item);
                }
    }
    return Line(party, s, t, commander);   // commander: see Line
}

SpellThrown_Ask SpellThrown::WhoToAsk(cs::Ref<Spell> s) {
    int32_t e = SpellEffect::Of(s);
    bool party = SpellEffect::IsParty(s);
    // only "If SpellDefs(pv01A8).M5B33 > 0" (MAINMOD2.BAS:931)
    if (e == SpellEffect::Damage || e == SpellEffect::Cure) return party || s->Damage2 <= 0 ? Ask::None : Ask::PeopleAndCompanions;
    if (e == SpellEffect::CureParalysis) return Ask::PeopleAndCompanions;
    if (e == SpellEffect::CurePoison || e == SpellEffect::CureDisease || e == SpellEffect::SeeInvisible) return Ask::People;
    if (e == SpellEffect::Levitate || e == SpellEffect::Invisible || e == SpellEffect::Protect) return party ? Ask::None : Ask::People;
    if (SpellEffect::ResistKind(e) >= 0) return party ? Ask::None : Ask::People;
    return Ask::None;
}

}  // namespace mc
