#include "core.h"

namespace mc {

int32_t CombatEngine::StatOf(cs::Ref<Monster> m, int32_t i) { return i < m->Stats.Length() ? (int32_t)m->Stats[i] : 0; }

cs::List<CombatEngine_CombatLine> CombatEngine::CompanionCasts(cs::Ref<Character> owner, cs::Ref<Companion> c, cs::Ref<Monster> def, cs::Ref<Spell> spell, cs::Ref<CombatEngine_CompanionRound> round)
{
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    int32_t owned = Party->Members.IndexOf(owner);
    int32_t slotOf = owner->Companions->SlotOf(c);
    auto victim = CompanionMishap::Victim(_rng, CompanionMishap_Kind::Spell, def->GuildLevel, Party, owned);
    if (victim.has_value() && victim.value() < Party->Members.Count())
    {
        int32_t v = victim.value();
        auto hurt = Party->Members[v];
        auto foe = FoeOf(owner);
        cs::Ref<Monster> facing = foe ? foe->Def : nullptr;
        int32_t dmg = CompanionSpellOnParty(spell, def, facing, hurt, nullptr);
        if (dmg > 0)
        {
            if (hurt == Party->Selected()) _result.Add(Caption(hurt, "You were accidently damaged for " + cs::ToStr(dmg) + "!", cs::List<cs::String>()));
            for (auto& line : TakeDamage(hurt, dmg)) _result.Add(line);
            if (hurt->CurHits < 1) { SetDead(hurt, Rules::DeathOrdinary); round->Over = true; return _result; }
        }
        for (int32_t i = 0; i <= spell->AffectGroup; i++)
        {
            if (i > CompanionSlotsWalked - 1) break;
            round->Suffix = "";
            auto mine = hurt->Companions->Index(i);
            if (mine == nullptr || mine->Hits <= 0) continue;
            auto mdef = DefOf(mine);
            if (mdef == nullptr) continue;
            round->Suffix = " and " + HisHer(hurt) + " companions";
            mine->Hits -= CompanionSpellOnParty(spell, def, facing, hurt, mine);
            // death cry under the victim's No Party Sound test (MONSMOD.BAS:1068-1073)
            if (mine->Hits < 1)
            {
                _result.Add(CompanionCry(hurt, DeathSound(mdef)));
                mine->Hits = 0;
                for (auto& line : CompanionFalls(hurt)) _result.Add(line);
            }
        }
        _result.Add(FoeMessage(Accidently(owner, c) + " cast \"" + cs::Str::TrimEnd(spell->Name) + "\" on \"" + cs::Str::TrimEnd(hurt->Name) + "\"" + round->Suffix + "!"));
        return _result;
    }

    // no mishap: mdrcharCastSpell on the owner's behalf (MONSMOD.BAS:1080; 829-853)
    CastSpellDraw();
    cs::Ref<CombatEngine_ItemOrder> ownersItem{};
    if (ItemOrders.TryGetValue(owner, ownersItem))
    {
        auto runOut = ItemCharge(owner, Party->OrderFor(owner), ownersItem);
        if (runOut.has_value())
        {
            _result.Add(runOut.value());
            return _result;
        }
    }
    cs::String refused = CastDamage(owner, spell, cs::Nullable<int32_t>{}, 0, slotOf, def);
    if (LastCaptions.Count() == 0 && !refused.empty()) { _result.Add(Message(refused)); return _result; }
    for (auto& line : CaptionLines(owner)) _result.Add(line);
    return _result;
}

int32_t CombatEngine::CompanionSpellOnParty(cs::Ref<Spell> s, cs::Ref<Monster> caster, cs::Ref<Monster> facing, cs::Ref<Character> hurt, cs::Ref<Companion> victimComp)
{
    int32_t lvl = cs::CastDouble<int32_t>(cs::Math::Floor(caster->GuildLevel / 6.0));
    if (lvl < 1) lvl = 1;
    if (lvl > 255) lvl = 255;
    cs::Ref<Monster> vdef = victimComp == nullptr ? nullptr : DefOf(victimComp);
    int32_t resist = s->ResistIndex < 0 ? 0
                   : vdef == nullptr ? Res(hurt, (int32_t)s->ResistIndex)
                   : Res(vdef, (int32_t)s->ResistIndex);
    int32_t dmg = Rules::SpellDamage(_rng, s->Damage2, s->SpecialEffect + 1, lvl, resist);
    if (vdef != nullptr && MonsterProperty::Has(vdef, 2)) dmg = 0;
    if ((s->Effect >= 75 && s->Effect <= 99)
        && (facing == nullptr || facing->Type >= Mdata1::MonsterClasses.Count() || Mdata1::MonsterClasses[facing->Type].Index != s->Effect - 75))
        dmg = 0;
    return dmg;
}

cs::String CombatEngine::CastSpell(cs::Ref<Character> actor, cs::Ref<Spell> s, cs::Nullable<int32_t> targetIndex, bool free, int32_t itemLevel)
{
    LastCaptions.Clear();
    LastSaidSounds = cs::List<cs::String>();
    LastCastTookTurn = true;
    // a charm with no free slot is refused before anything is spent (MAINMOD2.BAS:1410-1416)
    if (Spellbook::IsCharm(s) && actor->Companions->FreeSlots() == 0)
    {
        LastCastTookTurn = false;
        Say("\"" + cs::Str::TrimEnd(s->Name) + "\" - No room for more Companions!");
        return Log[Log.Count() - 1];
    }
    int32_t cost = free ? 0 : Spellbook::Cost(s, actor);
    if (actor->CurSpellPoints < cost)
    {
        // mdrcharCheckCast's refusal (MAINMOD2.BAS:502-510)
        LastCastTookTurn = false;
        Say(actor == Party->Selected() ? "Not enough spell points to cast \"" + cs::Str::TrimEnd(s->Name) + "\"!"
                                       : cs::Str::TrimEnd(actor->Name) + " doesn't have enough spell points to cast \"" + cs::Str::TrimEnd(s->Name) + "\"!");
        return Log[Log.Count() - 1];
    }
    // mdrcharCheckCast's next refusal (MAINMOD2.BAS:512-519)
    auto refused = NonCombatInFight(actor, s);
    if (!refused.empty())
    {
        LastCastTookTurn = false;
        Say(refused);
        return Log[Log.Count() - 1];
    }
    actor->CurSpellPoints -= cost;
    // monGetSpellHits (MONSMOD.BAS:2485-2492)
    int32_t sl = itemLevel > 0 ? itemLevel : itemLevel < 0 ? Rules::SpellLevel(actor->Guild()->Level)
               : free ? 1 : Spellbook::SpellLevelFor(actor, s);

    return CastDamage(actor, s, targetIndex, sl);
}

cs::List<CombatEngine_CombatLine> CombatEngine::CaptionLines(cs::Ref<Character> caster)
{
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    auto caps = LastCaptions.ToList();
    auto aims = LastCaptionAims.ToList();
    int32_t back = LastCastAim;
    bool paced = caster == Party->Selected();
    for (int32_t i = 0; i < caps.Count(); i++)
        _result.Add([&] {
            CombatEngine_CombatLine _o{};
            _o.Text = caps[i];
            _o.Actor = caster;
            _o.Paced = paced;
            _o.Channel = LineChannel::CharacterHit;
            _o.CastSounds = LastCaptionSounds;
            _o.AimAt = i < aims.Count() ? cs::Nullable<int32_t>(aims[i]) : cs::Nullable<int32_t>{};
            return _o;
        }());
    if (caps.Count() > 0)
        _result.Add([&] {
            CombatEngine_CombatLine _o{};
            _o.Text = "";
            _o.Actor = caster;
            _o.Channel = LineChannel::CharacterHit;
            _o.AimAt = back;
            return _o;
        }());
    return _result;
}

cs::String CombatEngine::CastDamage(cs::Ref<Character> actor, cs::Ref<Spell> s, cs::Nullable<int32_t> targetIndex, int32_t level, int32_t compSlot, cs::Ref<Monster> compDef)
{
    LastCaptions.Clear();
    LastCaptionAims.Clear();
    LastCastAim = -1;
    LastCaptionSounds = SpellSounds::For(s);
    // MAINMOD2.BAS:1410-1416: the slots counted are the owner's
    if (s->Category == Spellbook::Charm && actor->Companions->FreeSlots() == 0)
        return "\"" + cs::Str::TrimEnd(s->Name) + "\" - No room for more Companions!";
    const int32_t Slots = 4;   // mc02A6
    auto groups = CombatMonster::BySlot(Monsters);
    auto Alive = [&](int32_t gi) -> int32_t { return gi < groups.Count() ? groups[gi].Count([&](auto& m) { return m->Alive(); }) : 0; };

    // sub1019 (MONSMOD.BAS:297-305)
    int32_t aim = targetIndex.has_value() ? targetIndex.value() : Party->OrderFor(actor)->Target;
    int32_t g0 = aim >= 0 && aim < Monsters.Count() ? groups.FindIndex([&](auto& gr) { return gr.Contains(Monsters[aim]); }) : -1;
    if (g0 < 0 || Alive(g0) < 1) g0 = groups.FindIndex([&](auto& gr) { return gr.Any([&](auto& m) { return m->Alive(); }); });
    if (g0 < 0) g0 = 0;

    bool me = actor == Party->Selected();
    cs::String prefix = !me ? cs::String("") : compSlot < 0 ? cs::String("You ") : "Comp. #" + cs::ToStr(compSlot + 1) + " ";
    int32_t g = g0, done = 0, stepped = 0;   // l0286, l0288, l028E
    do
    {
        if (g > Slots - 1) g = 0;
        if (Alive(g) < 1)
        {
            do
            {
                stepped++;
                g++;
                if (g > Slots - 1) g = 0;
                if (stepped >= Slots) break;
                if (Alive(g) > 0) break;
            } while (stepped < Slots);
            if (stepped >= Slots) break;
        }

        auto group = groups[g];
        auto gdef = group[0]->Def;
        // "CombatOrders(pv01A6).M547A = l0286" (1456)
        int32_t aimNow = Monsters.IndexOf(group.First([&](auto& m) { return m->Alive(); }));
        Party->OrderFor(actor)->Target = aimNow;
        int32_t reach = cs::Math::Min((int32_t)s->AffectGroup, Alive(g));
        int32_t kills = 0, survivor = 0, charms = 0;
        bool charm = s->Category == Spellbook::Charm;   // SpellDefs(s).Kind = mc029E (7)
        for (int32_t i = 1; i <= reach; i++)
        {
            auto front = group.FirstOrDefault([&](auto& m) { return m->Alive(); });
            if (front == nullptr) break;
            int32_t dmg = SpellHitsOn(s, level, gdef, compDef);
            if (s->Effect == 100 && front->Hits() - dmg > 0) { dmg = 0; break; }
            int32_t before = front->Hits();                   // l0296
            // a charm takes only what it would have killed (MAINMOD2.BAS:1473-1477)
            if (charm && before - dmg > 0) dmg = 0;
            Credit(actor, front, dmg, compSlot >= 0, DamageKind::Magical, !charm);
            front->set_Hits(front->Hits() - dmg);
            if (front->Hits() < 0) front->set_Hits(0);
            if (front->Hits() > 0) { survivor = dmg; break; }
            // 1506-1508
            if (compSlot < 0) KilledBy(actor, front->Def, DamageKind::Magical);
            else CompanionKill(actor, front);
            if (charm)
            {
                // l029A, fn1163 (1511-1517)
                charms++;
                if (compSlot < 0)
                {
                    // Exit For (MAINMOD2.BAS:1514-1516) skips the load
                    if (actor->Companions->FreeSlots() == 0) { charms--; KeepRecord(front); break; }
                    // fn1163 writes no name (MAINMOD2.BAS:388-400)
                    actor->Companions->Add(Charmed(actor, front, before, s), true);
                }
                front->Gone = true;
                Sub0D08(front, front);   // 1520-1521
                continue;
            }
            kills++;
            if (compSlot < 0 && front->Identified
                && (!MonsterProperty::Has(front->Def, MonsterProperty::Invisible) || actor->SeeingInvisible))
                RecordableKills.Add(Tup_killer_def_3ffae{actor, front->Def});
            Sub0D08(front, front);   // 1520-1521
        }

        cs::String caption = "";
        if (charms > 0) caption = "You charmed " + CountWords[cs::Math::Min(charms, 11)];   // 1527-1528
        else if (kills > 0) caption = prefix + SpellVerbs[cs::Math::Clamp<int32_t>((int32_t)s->AffectMonster, 0, SpellVerbs.Length() - 1)]
                                 + " " + CountWords[cs::Math::Min(kills, 11)];
        if (charms > 0) { }
        else if (survivor > 0)
            caption = kills > 0 ? caption + " and damaged one for " + cs::ToStr(survivor)
                                : prefix + "damaged one for " + cs::ToStr(survivor);
        else if (kills == 0)
        {
            if (compSlot < 0)
            {
                // sub102F caster, 1
                auto o = Party->OrderFor(actor);
                o->Kind = OrderKind::Fight; o->SpellId = 0;
                ItemOrders.Remove(actor);   // M5766 = -1 (MULTIMOD.BAS:476-479)
                caption = "Failed";
            }
            else caption = prefix + "failed in casting \"" + cs::Str::TrimEnd(s->Name) + "\"";
        }
        LastCaptions.Add(caption + "!");
        LastCaptionAims.Add(aimNow);

        if (!Monsters.Any([&](auto& m) { return m->Alive(); })) break;
        g++; done++; stepped++;
    } while (done < s->Damage1 && stepped < s->Damage1);

    // 1582-1586
    LastCastAim = Alive(g0) > 0 ? Monsters.IndexOf(groups[g0].First([&](auto& m) { return m->Alive(); })) : Monsters.FindIndex([&](auto& m) { return m->Alive(); });
    Party->OrderFor(actor)->Target = LastCastAim;
    return cs::Str::Join(" ", LastCaptions);
}

int32_t CombatEngine::SpellHitsOn(cs::Ref<Spell> s, int32_t level, cs::Ref<Monster> target, cs::Ref<Monster> compDef)
{
    int32_t lvl = compDef != nullptr ? cs::CastDouble<int32_t>(cs::Math::Floor(compDef->GuildLevel / 6.0)) : level;
    if (lvl < 1) lvl = 1;
    if (lvl > 255) lvl = 255;
    int32_t resist = s->ResistIndex > -1 && s->ResistIndex < target->Resistances.Length() ? (int32_t)target->Resistances[s->ResistIndex] : 0;
    int32_t dmg = Rules::SpellDamage(_rng, s->Damage2, s->SpecialEffect + 1, lvl, resist);
    if (compDef == nullptr)
    {
        if (s->Category == 7 && MonsterProperty::Has(target, 3)) dmg = 0;
        if (MonsterProperty::Has(target, 2)) dmg = 0;
    }
    else if (MonsterProperty::Has(compDef, 2)) dmg = 0;
    if ((s->Effect >= 75 && s->Effect <= 99)
        && (target->Type >= Mdata1::MonsterClasses.Count() || Mdata1::MonsterClasses[target->Type].Index != s->Effect - 75))
        dmg = 0;
    return dmg;
}

cs::String CombatEngine::CastSpell(cs::Ref<Spell> s, cs::Nullable<int32_t> targetIndex, bool free, int32_t itemLevel) { return CastSpell(Actor(), s, targetIndex, free, itemLevel); }

cs::String CombatEngine::Who(cs::Ref<Character> c) { return Party->Size() > 1 ? c->Name : cs::String("You"); }

cs::String CombatEngine::Named(cs::Ref<CombatMonster> m, int32_t count)
{
    return EncounterGenerator::NameFor(m->Seen(), Party->Selected(), count, (int32_t)(m->Identified ? 0 : 1));
}

cs::String CombatEngine::NonCombatInFight(cs::Ref<Character> c, cs::Ref<Spell> s)
{
    if (CastGate::UseOf(s) != SpellUse::NonCombat) return cs::String();
    auto r = CastGate::Refuse(c, s, 0, c == Party->Selected(), true, true, false);
    return r ? r->Text : cs::String();
}

cs::Nullable<CombatEngine_CombatLine> CombatEngine::RoundCheckCast(cs::Ref<Character> c, cs::Ref<CombatOrder> order, cs::Ref<Spell> s, int32_t itemLevel)
{
    bool isYou = c == Party->Selected();
    int32_t cost = itemLevel != 0 ? 0 : Spellbook::Cost(s, c);
    auto refused = CastGate::Unfit(c, s, itemLevel, isYou, _item);
    if (!refused) refused = CastGate::Refuse(c, s, cost, isYou, true, true, NoSpellsHere);
    if (refused == nullptr) return cs::Nullable<CombatEngine_CombatLine>{};
    order->Kind = OrderKind::Fight; order->SpellId = 0;
    ItemOrders.Remove(c);
    if (refused->Kind == RefusalKind::AntiMagic) { if (AntiMagicRefused) AntiMagicRefused(c); }
    return Message(refused->Text);
}

cs::Ref<Companion> CombatEngine::Charmed(cs::Ref<Character> caster, cs::Ref<CombatMonster> m, int32_t hitsBefore, cs::Ref<Spell> s)
{
    return [&] {
        auto _o = cs::New<Companion>();
        _o->Ref = m->Def->Ref;                    // p00D6.Ref (MAINMOD2.BAS:388)
        _o->MonsterId = m->Def->Id;               // M57D4 = Monsters(p00D6.Ref).Id (389)
        _o->Name = "";
        _o->Hits = hitsBefore;
        _o->MaxHits = m->MaxHits > 0 ? m->MaxHits : (int32_t)m->Def->Hits;
        _o->Alignment = m->CompanionAlignment();   // fn1163: p00D6.Alignment (MAINMOD2.BAS:392)
        _o->BindingLevel = s->Level;
        _o->IdLevel = CharmIdLevel(caster, m->Def, _item);
        return _o;
    }();
}

int32_t CombatEngine::CharmIdLevel(cs::Ref<Character> c, cs::Ref<Monster> def, cs::Func<cs::Ref<Item>(int16_t)> item) { return IdLevelRoll(c, def, item, true); }

int32_t CombatEngine::IdLevelRoll(cs::Ref<Character> c, cs::Ref<Monster> def, cs::Func<cs::Ref<Item>(int16_t)> item, bool p036E)
{
    int32_t difficulty = cs::CastDouble<int32_t>(cs::Math::Round((30 + def->LevelFound) * cs::Math::Log((def->GuildLevel + 5) / 10.0 + 1), cs::MidpointRounding::ToEven));
    if (!p036E) difficulty = cs::CastDouble<int32_t>(cs::Math::Round(difficulty * 1.5, cs::MidpointRounding::ToEven));
    double skill = !item
        ? Rules::IdentifySkill(c->TopGuildLevel(), c->Stats[static_cast<int32_t>(Stat::Int)], c->Stats[static_cast<int32_t>(Stat::Wis)])
        : Rules::IdentifySkill(c->TopGuildLevel(), c->EffectiveStat(static_cast<int32_t>(Stat::Int), item), c->EffectiveStat(static_cast<int32_t>(Stat::Wis), item));
    int32_t gap = cs::CastDouble<int32_t>(cs::Math::Round(skill - difficulty, cs::MidpointRounding::ToEven));
    return gap >= 0 ? 0 : gap <= -15 ? 3 : gap <= -5 ? 2 : 1;
}

void CombatEngine::WalkAway() { Fled = true; }

}  // namespace mc
