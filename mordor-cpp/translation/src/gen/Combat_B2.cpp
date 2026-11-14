#include "core.h"

namespace mc {

cs::String CombatEngine::PlayerAttacks(cs::Nullable<int32_t> targetIndex) { return PlayerAttacks(Actor(), targetIndex); }

cs::List<cs::String> CombatEngine::Round(cs::List<cs::Ref<Spell>> spells) {
    cs::List<cs::String> _result = cs::List<cs::String>::New();
    for (auto& line : RoundPaced(spells)) _result.Add(line.Text);
    return _result;
}

cs::List<CombatEngine_CombatLine> CombatEngine::RoundPaced(cs::List<cs::Ref<Spell>> spells) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    if (_spells == nullptr) _spells = spells;
    if (IsOver()) return _result;

    // mdrmulInitiateCombatRound (MULTIMOD.BAS:511-539): each character whole, then their companions
    for (auto& c : Party->Able().ToList()) {
        if (IsOver() || !Monsters.Any([&](auto& m) { return m->Alive(); })) break;
        for (auto& line : ChestTick(c)) _result.Add(line);
        for (auto& line : WithSaid(CharacterActs(c, spells))) _result.Add(line);
        if (IsOver() || !Monsters.Any([&](auto& m) { return m->Alive(); })) break;
        for (auto& line : WithSaid(CompanionTurn(c))) _result.Add(line);
    }

    // MULTIMOD.BAS:542-553
    for (auto& m : Party->Members.Where([&](auto& m) { return !m->IsDead; }).ToList()) Retarget(m);
    if (Party->Size() > 1 && Party->Members.Any([&](auto& c) { return c->IsDead; })) {
        if (ListUpdate) ListUpdate();
    }

    // MULTIMOD.BAS:556-557: the monsters go whenever any stand
    if (Fled || !Monsters.Any([&](auto& m) { return m->Alive(); }) || Party->Members.Count() == 0) return _result;
    for (auto& line : MonsterAttacks(AttackKind::Round)) _result.Add(line);
    return _result;
}

cs::List<CombatEngine_CombatLine> CombatEngine::WithSaid(cs::List<CombatEngine_CombatLine> lines) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    for (auto& l : lines) {
        _result.Add(l);
        for (auto& s : TakeSaid()) _result.Add(s);
    }
    for (auto& s : TakeSaid()) _result.Add(s);
    return _result;
}

cs::List<CombatEngine_CombatLine> CombatEngine::CharacterActs(cs::Ref<Character> c, cs::List<cs::Ref<Spell>> spells) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    // MONSMOD.BAS:291
    if (ParalyzedRounds(c) > 0) return _result;
    // MONSMOD.BAS:303-305
    Retarget(c);

    auto order = Party->OrderFor(c);
    if (order->Kind == OrderKind::Fight) {
        for (auto& line : PlayerSwings(c, order->Target >= 0 ? cs::Nullable<int32_t>(order->Target) : cs::Nullable<int32_t>()))
            _result.Add(line);
        return _result;
    }
    if (order->Kind == OrderKind::Spell) {
        auto s = spells.FirstOrDefault([&](auto& sp) { return sp->Id == order->SpellId; });
        if (s == nullptr) { order->Clear(); return _result; }
        // MONSMOD.BAS:312-313
        cs::Ref<CombatEngine_ItemOrder> io{};
        ItemOrders.TryGetValue(c, io);
        if (io && io->ItemCast) {
            for (auto& line : ItemOrderActs(c, order, s, io)) _result.Add(line);
            return _result;
        }
        // MAINMOD2.BAS:831
        auto refused = RoundCheckCast(c, order, s, 0);
        if (refused.has_value()) { _result.Add(refused.value()); return _result; }
        // MAINMOD2.BAS:841-851
        CastSpellDraw();   // l01D0 (MAINMOD2.BAS:840)
        if (io != nullptr) {
            auto runOut = ItemCharge(c, order, io);
            if (runOut.has_value()) { _result.Add(runOut.value()); return _result; }
        }
        cs::String said = CastSpell(c, s, order->Target >= 0 ? cs::Nullable<int32_t>(order->Target) : cs::Nullable<int32_t>());
        // MAINMOD2.BAS:1566-1568
        if (LastCaptions.Count() > 0) {
            for (auto& line : CaptionLines(c)) _result.Add(line);
        } else if (said != "") {
            _result.Add([&] { CombatEngine_CombatLine _o{}; _o.Text = said; _o.Actor = c; _o.Channel = LineChannel::Message; _o.CastSounds = LastSaidSounds; _o.Kind = MessageKind::Party; return _o; }());   // a sub085B line
        }
    }
    return _result;
}

void CombatEngine::CastSpellDraw() { (void)_rng->NextDouble(); }

cs::Nullable<CombatEngine_CombatLine> CombatEngine::ItemCharge(cs::Ref<Character> c, cs::Ref<CombatOrder> order, cs::Ref<CombatEngine_ItemOrder> io) {
    if (io->Slot->Charges > 0) { Spellbook::SpendCharge(c, io->Slot, io->Item); return {}; }
    // sub102F pv01A6, -1 (850)
    order->Sub102FMinusOne();
    ItemOrders.Remove(c);
    return Message(c == Party->Selected()
        ? cs::String("Your item has run out of charges!")
        : "The item \"" + cs::Str::TrimEnd(c->Name) + "\" was using ran out of charges!");
}

cs::List<CombatEngine_CombatLine> CombatEngine::ItemOrderActs(cs::Ref<Character> c, cs::Ref<CombatOrder> order, cs::Ref<Spell> s, cs::Ref<CombatEngine_ItemOrder> io) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    int32_t lvl = io->Item->SpellLvl;
    // MONSMOD.BAS:313, MAINMOD2.BAS:439-526
    auto refused = RoundCheckCast(c, order, s, lvl);
    if (refused.has_value()) { _result.Add(refused.value()); return _result; }
    // MAINMOD2.BAS:841-851, MULTIMOD.BAS:476-479
    CastSpellDraw();   // l01D0 (MAINMOD2.BAS:840)
    auto runOut = ItemCharge(c, order, io);
    if (runOut.has_value()) { _result.Add(runOut.value()); return _result; }
    // MAINMOD2.BAS:1355
    cs::String said = CastSpell(c, s, order->Target >= 0 ? cs::Nullable<int32_t>(order->Target) : cs::Nullable<int32_t>(), lvl != 0, lvl);
    if (LastCaptions.Count() > 0) {
        for (auto& line : CaptionLines(c)) _result.Add(line);
    } else if (said != "") {
        _result.Add([&] { CombatEngine_CombatLine _o{}; _o.Text = said; _o.Actor = c; _o.Channel = LineChannel::Message; _o.CastSounds = LastSaidSounds; _o.Kind = MessageKind::Party; return _o; }());
    }
    return _result;
}

cs::List<CombatEngine_CombatLine> CombatEngine::CompanionTurn(cs::Ref<Character> owner) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    auto round = cs::New<CombatEngine_CompanionRound>();
    int32_t owned = Party->Members.IndexOf(owner);

    for (int32_t at = 0; at < CompanionSlotsWalked; at++) {
        // MONSMOD.BAS:873
        auto c = owner->Companions->Index(at);
        if (!c || c->Hits <= 0) continue;
        if (!Monsters.Any([&](auto& m) { return m->Alive(); })) return _result;
        auto def = DefOf(c);
        if (!def) continue;

        if (Bit(def->SpecialAttackFlags, 3) || (Bit(def->SpecialAttackFlags, 4) && !round->Breathed)) {
            for (auto& line : CompanionBreathes(owner, c, def, round)) _result.Add(line);
            if (round->Over) return _result;
        }

        int32_t victim = -1, slot = -1;
        auto vv = CompanionMishap::Victim(_rng, CompanionMishap_Kind::Spit, def->GuildLevel, Party, owned);
        if (vv.has_value() && vv.value() < Party->Members.Count()) {
            int32_t v = vv.value();
            victim = v;
            slot = CompanionSpit::Slot(_rng, owner);
        }

        if (Bit(def->SpecialAttackFlags, 5)) {
            for (auto& line : CompanionSpits(owner, c, def, victim, slot, round)) _result.Add(line);
            if (round->Over) return _result;
        }

        // fn0EEF (MONSMOD.BAS:1029)
        if (!Monsters.Any([&](auto& m) { return m->Alive(); })) { CompanionsCleared(owner); return _result; }

        // MONSMOD.BAS:1038-1083
        bool cast = false;
        cs::Ref<Spell> spell{};
        if (def->SpellFlags > 0 && !round->Cast && !AntiMagicHere && (spell = ChooseSpell(def)) != nullptr) {
            cast = true;
            round->Cast = true;
            for (auto& line : CompanionCasts(owner, c, def, spell, round)) _result.Add(Picked(line));
            PendingPickSounds = cs::List<cs::String>();
            if (round->Over) return _result;
        }
        if (!cast)
            for (auto& line : CompanionStrikes(owner, c, def, victim, slot, round)) _result.Add(line);
        if (round->Over) return _result;
        // MONSMOD.BAS:1327-1335, 1340-1346
        if (!Monsters.Any([&](auto& m) { return m->Alive(); })) { CompanionsCleared(owner); return _result; }
    }
    return _result;
}

void CombatEngine::CompanionsCleared(cs::Ref<Character> owner) {
    if (owner->ParalysisLevel > 0) owner->Unparalyse(_item);
}

cs::Ref<CombatMonster> CombatEngine::FoeOf(cs::Ref<Character> owner) {
    int32_t t = Party->OrderFor(owner)->Target;
    if (t >= 0 && t < Monsters.Count()) {
        if (Monsters[t]->Alive()) return Monsters[t];
        auto next = NextInGroup(Monsters[t]);
        if (next) return next;
    }
    return Monsters.FirstOrDefault([&](auto& m) { return m->Alive(); });
}

void CombatEngine::Sub0D08(cs::Ref<CombatMonster> aimed, cs::Ref<CombatMonster> fallen) {
    if (aimed != nullptr) {
        auto front = NextInGroup(aimed);
        if (front && (front->Unrolled || front->Group >= 0))
            front->LoadRecord(_rng);
    }
    KeepRecord(fallen);
}

void CombatEngine::KeepRecord(cs::Ref<CombatMonster> fallen) {
    auto next = NextInGroup(fallen);
    if (next && next->Unrolled)
        next->TakeRecord(cs::Math::Max(0, fallen->Hits()), fallen->MaxHits);
}

cs::Ref<CombatMonster> CombatEngine::AimedRecord(cs::Ref<Character> who) {
    int32_t t = Party->OrderFor(who)->Target;
    return t >= 0 && t < Monsters.Count() ? Monsters[t] : Monsters.FirstOrDefault([&](auto& m) { return m->Alive(); });
}

void CombatEngine::Retarget(cs::Ref<Character> who) {
    auto o = Party->OrderFor(who);
    if (o->Target < 0 || o->Target >= Monsters.Count() || NextInGroup(Monsters[o->Target]) != nullptr) return;
    o->Target = Monsters.FindIndex([&](auto& m) { return m->Alive(); });
}

cs::Ref<CombatMonster> CombatEngine::NextInGroup(cs::Ref<CombatMonster> m) {
    int32_t i = Monsters.IndexOf(m);
    if (i < 0) return nullptr;
    while (i > 0 && CombatMonster::SameGroup(Monsters[i - 1], m)) i--;
    for (; i < Monsters.Count() && CombatMonster::SameGroup(Monsters[i], m); i++)
        if (Monsters[i]->Alive()) return Monsters[i];
    return nullptr;
}

cs::String CombatEngine::HisHer(cs::Ref<Character> c) { return c->Sex == Sex::Male ? "his" : "her"; }

cs::String CombatEngine::Accidently(cs::Ref<Character> owner, cs::Ref<Companion> c) {
    return cs::Str::TrimEnd(owner->Name) + ": " + owner->CompanionName(owner->Companions->SlotOf(c)) + " accidently";
}

CombatEngine_CombatLine CombatEngine::CompanionCry(cs::Ref<Character> guard, cs::String sound) {
    CombatEngine_CombatLine _o{};
    _o.Text = "";
    _o.Actor = guard;
    _o.HitSounds = cs::List<cs::String>::Of({sound});
    return _o;
}

cs::List<CombatEngine_CombatLine> CombatEngine::CompanionBreathes(cs::Ref<Character> owner, cs::Ref<Companion> c, cs::Ref<Monster> def, cs::Ref<CombatEngine_CompanionRound> round) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    int32_t compLvl = def->GuildLevel;
    auto lead = Monsters.FirstOrDefault([&](auto& m) { return m->Alive(); });
    if (lead == nullptr) return _result;

    int32_t chance = 50;
    if (lead->Def->GuildLevel > compLvl + compLvl / 4.0) chance = 80;
    if (lead->Def->GuildLevel < compLvl - compLvl / 4.0) chance = 20;
    if (!(Floor(_rng->NextDouble() * 100) < chance)) return _result;

    bool fire = Bit(def->SpecialAttackFlags, 3);
    int32_t owned = Party->Members.IndexOf(owner);

    cs::Nullable<int32_t> victim = CompanionMishap::Victim(_rng, CompanionMishap_Kind::Breath, compLvl, Party, owned);
    if (victim.has_value() && victim.value() < Party->Members.Count()) {
        int32_t at = victim.value();
        auto hurt = Party->Members[at];
        int32_t dmg = SpecialHits::AfterResistance(
            SpecialHits::Raw(_rng, SpecialHits_Kind::Breath, compLvl, hurt->TopGuildLevel()),
            Res(hurt, SpecialHits::ResistedBy(SpecialHits_Kind::Breath, fire)));
        if (dmg > 0) {
            _result.Add(Caption(hurt, "You were accidently Breathed on for " + cs::ToStr(dmg) + "!", cs::List<cs::String>::Of({"FIRE"})));
            for (auto& line : TakeDamage(hurt, dmg)) _result.Add(line);
            if (hurt->CurHits < 1) {
                // monCompanionAttack (MONSMOD.BAS:901-1305) calls mdrcharSetDead alone
                SetDead(hurt, Rules::DeathOrdinary);
                round->Over = true;
                return _result;
            }
        }

        round->Suffix = "";
        for (int32_t s = 0; s < CompanionSlotsWalked; s++) {
            auto mine = hurt->Companions->Index(s);
            if (!mine || mine->Hits <= 0) continue;
            auto mdef = DefOf(mine);
            if (!mdef) continue;
            round->Suffix = " and " + HisHer(hurt) + " companions";
            int32_t splash = SpecialHits::AfterResistance(
                SpecialHits::Raw(_rng, SpecialHits_Kind::Breath, compLvl, mdef->GuildLevel),
                Res(mdef, SpecialHits::ResistedBy(SpecialHits_Kind::Breath, fire)));
            mine->Hits -= splash;
            // MONSMOD.BAS:913-918
            if (mine->Hits < 0) {
                _result.Add(CompanionCry(owner, DeathSound(mdef)));
                mine->Hits = 0;
                for (auto& line : CompanionFalls(hurt)) _result.Add(line);
            }
        }
        if (round->Kills > 0) round->Breathed = true;
        _result.Add(FoeMessage(Accidently(owner, c) + " breathed on \"" + cs::Str::TrimEnd(hurt->Name) + "\"" + round->Suffix + "!"));
        return _result;
    }

    // MONSMOD.BAS:925-953; a kill calls "sub0D08 pv01E4" (948) on the owner's target group
    auto record = AimedRecord(owner);   // M547A, which nothing in the sweep moves
    for (auto& group : AliveGroups()) {
        round->Kills = 0;
        do {
            auto front = group.FirstOrDefault([&](auto& m) { return m->Alive(); });
            if (front == nullptr) break;
            int32_t dmg = SpecialHits::AfterResistance(
                SpecialHits::Raw(_rng, SpecialHits_Kind::Breath, compLvl, front->Def->GuildLevel),
                Res(front->Def, SpecialHits::ResistedBy(SpecialHits_Kind::Breath, fire)));
            if (dmg > 0) _result.Add([&] { CombatEngine_CombatLine _o{}; _o.Text = ""; _o.Actor = owner; _o.HitSounds = cs::List<cs::String>::Of({"FIRE"}); return _o; }());
            Credit(owner, front, dmg, true, DamageKind::Companion);
            front->set_Hits(front->Hits() - dmg);
            if (front->Hits() > 0) break;
            front->set_Hits(0);
            QuestKill(front->Def); CompanionKill(owner, front);   // MONSMOD.BAS:945
            round->Kills++;
            Sub0D08(record, front); // 948
        } while (group.Any([&](auto& m) { return m->Alive(); }) && round->Kills <= 4);
    }
    if (round->Kills > 0) round->Breathed = true;
    return _result;
}

cs::List<CombatEngine_CombatLine> CombatEngine::CompanionSpits(cs::Ref<Character> owner, cs::Ref<Companion> c, cs::Ref<Monster> def, int32_t victim, int32_t slot, cs::Ref<CombatEngine_CompanionRound> round) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    int32_t compLvl = def->GuildLevel;
    if (victim >= 0) {
        auto hurt = Party->Members[victim];
        if (slot < 0) {
            int32_t dmg = SpecialHits::AfterResistance(
                SpecialHits::Raw(_rng, SpecialHits_Kind::AcidSpit, compLvl, hurt->TopGuildLevel()),
                Res(hurt, SpecialHits::Acid));
            if (dmg <= 0) return _result;
            // MONSMOD.BAS:971-987
            _result.Add(FoeMessage(Accidently(owner, c) + " spit on \"" + cs::Str::TrimEnd(hurt->Name) + "\"" + round->Suffix + "!"));
            _result.Add(Caption(hurt, "You were accidently Spit on for " + cs::ToStr(dmg) + "!", cs::List<cs::String>::Of({"SPIT"})));
            for (auto& line : TakeDamage(hurt, dmg)) _result.Add(line);
            if (hurt->CurHits < 1) { SetDead(hurt, Rules::DeathOrdinary); round->Over = true; }
            return _result;
        }

        // MONSMOD.BAS:991: the slot is read in the victim's list with no test of its own
        auto poor = hurt->Companions->Left(slot);
        int32_t splash;
        if (hurt->Companions->Index(slot) == nullptr) {
            // MONSMOD.BAS:2402-2405, 2434-2438: an empty slot reads as Chars(0)
            auto first = Party->Members[0];
            splash = SpecialHits::AfterResistance(
                SpecialHits::Raw(_rng, SpecialHits_Kind::AcidSpit, compLvl, first->TopGuildLevel()),
                Res(first, SpecialHits::Acid));
        } else {
            auto pdef = DefOf(poor);
            if (!pdef) return _result;
            splash = SpecialHits::AfterResistance(
                SpecialHits::Raw(_rng, SpecialHits_Kind::AcidSpit, compLvl, pdef->GuildLevel),
                Res(pdef, SpecialHits::Acid));
        }
        if (splash <= 0) return _result;
        // fn128B, MONSMOD.BAS:993
        _result.Add(FoeMessage(Accidently(owner, c) + " spit on " + hurt->CompanionName(slot) + " belonging to \"" + cs::Str::TrimEnd(hurt->Name) + "\"!"));
        poor->Hits -= splash;
        if (poor->Hits < 0) {
            // MONSMOD.BAS:996-999, MUSIC.BAS:99-102, MONSMOD.BAS:1355-1359
            bool sounds = !SoundFxOff && (!NoPartySound || owner == Party->Selected());
            if (hurt->Companions->Index(slot) == nullptr && sounds) { round->Over = true; return _result; }
            // MONSMOD.BAS:996-998
            if (hurt->Companions->Index(slot) != nullptr) {
                auto dead = DefOf(poor);
                if (dead) _result.Add(CompanionCry(owner, DeathSound(dead)));
            }
            // MONSMOD.BAS:1000-1001
            poor->Hits = 0;
            for (auto& line : CompanionFalls(hurt)) _result.Add(line);
        }
        return _result;
    }

    auto front = FoeOf(owner);
    if (front == nullptr) return _result;
    int32_t hit = SpecialHits::AfterResistance(
        SpecialHits::Raw(_rng, SpecialHits_Kind::AcidSpit, compLvl, front->Def->GuildLevel),
        Res(front->Def, SpecialHits::Acid));
    if (hit > 0) _result.Add([&] { CombatEngine_CombatLine _o{}; _o.Text = ""; _o.Actor = owner; _o.HitSounds = cs::List<cs::String>::Of({"SPIT"}); return _o; }());
    Credit(owner, front, hit, true, DamageKind::Companion);
    front->set_Hits(front->Hits() - hit);
    if (front->Hits() < 0) front->set_Hits(0);
    if (front->Hits() <= 0) {
        QuestKill(front->Def); CompanionKill(owner, front);   // MONSMOD.BAS:1022
        Sub0D08(front, front);  // 1023-1024
    }
    return _result;
}

cs::List<CombatEngine_CombatLine> CombatEngine::CompanionStrikes(cs::Ref<Character> owner, cs::Ref<Companion> c, cs::Ref<Monster> def, int32_t victim, int32_t slot, cs::Ref<CombatEngine_CompanionRound> round) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    auto foe = FoeOf(owner);
    if (foe == nullptr) { round->Over = true; return _result; }

    cs::Ref<Character> hurt = victim >= 0 ? Party->Members[victim] : cs::Ref<Character>();
    cs::Ref<Companion> poor = nullptr;
    cs::Ref<Monster> pdef = nullptr;
    if (hurt != nullptr && slot >= 0) {
        // MONSMOD.BAS:1102
        auto p = hurt->Companions->Index(slot);
        if (!p || p->Hits <= 0) return _result;
        poor = p;
        if ((pdef = DefOf(poor)) == nullptr) return _result;
    }

    int32_t targetDef = hurt == nullptr ? (int32_t)foe->Def->Def : pdef == nullptr ? DefenseOf(hurt) : (int32_t)pdef->Def;
    int32_t targetLevel = hurt == nullptr ? (int32_t)foe->Def->GuildLevel : pdef == nullptr ? hurt->TopGuildLevel() : (int32_t)pdef->GuildLevel;
    bool unseen = hurt == nullptr ? MonsterProperty::Has(foe->Def, MonsterProperty::Invisible)
                : pdef == nullptr ? hurt->InvisibleNow
                : MonsterProperty::Has(pdef, MonsterProperty::Invisible);

    double damMod = def->DamageMod;
    if (Bit(def->SpecialAttackFlags, 10)
        && _rng->Next(100) + 1 <= Rules::MonsterCritChance(def->GuildLevel, def->LevelFound, targetLevel, false))
        damMod += 4;
    if (Bit(def->SpecialAttackFlags, 11)
        && _rng->Next(100) + 1 <= Rules::MonsterCritChance(def->GuildLevel, def->LevelFound, targetLevel, true))
        damMod += 1;
    if (unseen && !MonsterProperty::Has(def, 0)) damMod -= damMod / 2;

    int32_t dmg = Rules::Fn0DEE(_rng, def->Att, targetDef, def->GuildLevel, def->Stats.Length() > 0 ? (int32_t)def->Stats[0] : 0, damMod);
    // sub0D35 follows the blow whatever it did (MONSMOD.BAS:1212-1214)
    auto attackCry = CompanionCry(owner, AttackSound(def));
    if (dmg < 1) { _result.Add(attackCry); return _result; }

    cs::String whose = hurt == nullptr ? cs::String("") : "\"" + cs::Str::TrimEnd(hurt->Name) + "\"";
    if (hurt == nullptr) {
        Credit(owner, foe, dmg, true, DamageKind::Companion);
        foe->set_Hits(foe->Hits() - dmg);
        if (foe->Hits() < 0) foe->set_Hits(0);
        if (foe->Hits() <= 0) {
            // MONSMOD.BAS:1175-1178
            _result.Add(CompanionCry(owner, DeathSound(foe->Def)));
            QuestKill(foe->Def); CompanionKill(owner, foe);
            Sub0D08(foe, foe);   // 1179-1180
        }
    } else if (poor == nullptr) {
        _result.Add(Caption(hurt, "You were accidently hit for " + cs::ToStr(dmg) + "!", cs::List<cs::String>()));
        for (auto& line : TakeDamage(hurt, dmg)) _result.Add(line);
        _result.Add(FoeMessage(Accidently(owner, c) + " hit " + whose + round->Suffix + "!"));
        if (hurt->CurHits < 1) { SetDead(hurt, Rules::DeathOrdinary); round->Over = true; return _result; }
    } else {
        poor->Hits -= dmg;
        _result.Add(FoeMessage(Accidently(owner, c) + " hit " + hurt->CompanionName(slot) + " belonging to " + whose + "!"));
        // MONSMOD.BAS:1201-1207
        if (poor->Hits < 0) {
            _result.Add(CompanionCry(owner, DeathSound(pdef)));
            poor->Hits = 0;
            for (auto& line : CompanionFalls(hurt)) _result.Add(line);
            return _result;
        }
    }
    _result.Add(attackCry);

    auto standing = FoeOf(owner);
    if (standing == nullptr || standing->Hits() <= 0 || (hurt == nullptr && !CombatMonster::SameGroup(standing, foe))) return _result;
    // MONSMOD.BAS:1215
    cs::Ref<CombatMonster> mon = hurt == nullptr ? standing : cs::Ref<CombatMonster>();

    if (Bit(def->SpecialAttackFlags, 8)) {
        int32_t res = mon != nullptr ? Res(mon->Def, 7) : pdef == nullptr ? Res(hurt, 7) : Res(pdef, 7);
        if (_rng->Next(100) + 1 > res) {
            int32_t con = mon != nullptr ? StatOf(def, 3)                      // the companion's own
                    : pdef == nullptr ? hurt->EffectiveStat(static_cast<int32_t>(Stat::Con), _item)
                    : StatOf(pdef, 3);
            float chance = (float)(cs::Math::Log((double)cs::Math::Max(1, con)) / cs::Math::Log(2.5));
            chance = con * chance - def->LevelFound;
            if (chance > 95) chance = 95;
            if (_rng->NextDouble() * 100 > chance) {
                if (mon != nullptr) {
                    Credit(owner, mon, mon->Hits(), true, DamageKind::Companion);
                    cs::String caption = owner == Party->Selected() ? Caption(owner, owner->CompanionName(owner->Companions->SlotOf(c)) + " Stoned one!") : cs::String("");
                    _result.Add([&] { CombatEngine_CombatLine _o{}; _o.Text = caption; _o.Actor = owner; _o.Paced = caption != ""; _o.HitSounds = cs::List<cs::String>::Of({"STONE"}); _o.Channel = LineChannel::CharacterHit; return _o; }());
                    int32_t staleHits = mon->Hits();
                    mon->set_Hits(0);
                    QuestKill(mon->Def); CompanionKill(owner, mon);   // MONSMOD.BAS:1247
                    // 1248-1249, 1268-1296: with nobody left the stale record is struck by the electrocution
                    Sub0D08(mon, mon);
                    auto nextMon = NextInGroup(mon);
                    mon = nextMon ? nextMon : [&] { auto _o = cs::New<CombatMonster>(); _o->Def = mon->Def; _o->set_Hits(staleHits); return _o; }();
                } else if (pdef == nullptr) {
                    // STONE is the owner's to hear (MONSMOD.BAS:1236-1237)
                    _result.Add(FoeMessage(Accidently(owner, c) + " stoned " + whose + round->Suffix + "!", cs::List<cs::String>::Of({"STONE"}), owner));
                    hurt->Stoned = true;
                    SetDead(hurt, Rules::DeathStoned);
                    round->Over = true;
                    return _result;
                } else {
                    _result.Add(FoeMessage(Accidently(owner, c) + " stoned " + hurt->CompanionName(slot) + " belonging to " + whose + "!", cs::List<cs::String>::Of({"STONE"}), owner));
                    // MONSMOD.BAS:1256-1258
                    _result.Add(CompanionCry(owner, DeathSound(pdef)));
                    // MONSMOD.BAS:1260-1262: the slot is emptied before the carry is looked at
                    poor->Hits = 0;
                    hurt->Companions->Release(slot);
                    for (auto& line : CompanionFalls(hurt)) _result.Add(line);
                    return _result;
                }
            }
        }
    }

    if (Bit(def->SpecialAttackFlags, 6)) {
        int32_t level = mon != nullptr ? (int32_t)mon->Def->GuildLevel : pdef == nullptr ? hurt->TopGuildLevel() : (int32_t)pdef->GuildLevel;
        int32_t zap = SpecialHits::AfterResistance(
            SpecialHits::Raw(_rng, SpecialHits_Kind::Electrocute, def->GuildLevel, level),
            mon != nullptr ? Res(mon->Def, SpecialHits::Electrical)
            : pdef == nullptr ? Res(hurt, SpecialHits::Electrical) : Res(pdef, SpecialHits::Electrical));
        cs::List<cs::String> shock = zap > 0 ? cs::List<cs::String>::Of({"SHOCK"}) : cs::List<cs::String>();

        if (mon != nullptr) {
            Credit(owner, mon, zap, true, DamageKind::Companion);
            mon->set_Hits(mon->Hits() - zap);
            if (mon->Hits() < 0) mon->set_Hits(0);
            if (mon->Hits() <= 0) {
                QuestKill(mon->Def); CompanionKill(owner, mon);   // MONSMOD.BAS:1294
                Sub0D08(mon, mon);    // 1295-1296
            }
            cs::String caption = mon->Hits() <= 0 && owner == Party->Selected()
                ? Caption(owner, owner->CompanionName(owner->Companions->SlotOf(c)) + " Electrocuted one!") : cs::String("");
            _result.Add([&] { CombatEngine_CombatLine _o{}; _o.Text = caption; _o.Actor = owner; _o.Paced = caption != ""; _o.HitSounds = shock; _o.Channel = LineChannel::CharacterHit; return _o; }());
        } else if (pdef == nullptr) {
            // MONSMOD.BAS:1300 has no "l0214 = Me": the character on screen is told it was them
            auto sel = Party->Selected();
            auto watching = sel ? sel : hurt;
            cs::String text = "You were accidently electrocuted for " + cs::ToStr(zap) + "!";
            // MONSMOD.BAS:1276-1278: SHOCK is the owner's to hear
            if (shock != nullptr) _result.Add(CompanionCry(owner, "SHOCK"));
            Say(text);
            _result.Add([&] { CombatEngine_CombatLine _o{}; _o.Text = text; _o.Actor = watching; _o.Paced = true; _o.Channel = LineChannel::MonsterHit; return _o; }());
            for (auto& line : TakeDamage(hurt, zap)) _result.Add(line);
            if (hurt->CurHits < 1) { SetDead(hurt, Rules::DeathOrdinary); round->Over = true; return _result; }
            _result.Add(FoeMessage(Accidently(owner, c) + " electrocuted " + whose + round->Suffix + "!"));
        } else {
            poor->Hits -= zap;
            _result.Add(FoeMessage(Accidently(owner, c) + " electrocuted " + hurt->CompanionName(slot) + " belonging to " + whose + "!", shock, owner));
            // MONSMOD.BAS:1314-1319
            if (poor->Hits < 0) {
                _result.Add(CompanionCry(owner, DeathSound(pdef)));
                poor->Hits = 0;
                for (auto& line : CompanionFalls(hurt)) _result.Add(line);
            }
        }
    }
    return _result;
}

}  // namespace mc
