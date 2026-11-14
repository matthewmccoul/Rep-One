#include "core.h"

namespace mc {

namespace {
CombatEngine_CombatLine MkLine(const cs::String& text, cs::Ref<Character> actor) {
    CombatEngine_CombatLine _o{};
    _o.Text = text;
    _o.Actor = actor;
    return _o;
}
CombatEngine_CombatLine SoundLine(cs::Ref<Character> actor, cs::List<cs::String> sounds) {
    CombatEngine_CombatLine _o = MkLine("", actor);
    _o.HitSounds = sounds;
    return _o;
}
}  // namespace

bool CombatEngine::Bit(int32_t flags, int32_t bit) { return (flags & (1 << bit)) != 0; }

int32_t CombatEngine::Floor(double v) { return cs::CastDouble<int32_t>(cs::Math::Floor(v)); }

int32_t CombatEngine::VbInt(double v) { return cs::CastDouble<int32_t>(cs::Math::Round(v, cs::MidpointRounding::ToEven)); }

int32_t CombatEngine::Res(cs::Ref<Character> c, int32_t kind) { return Effects::ResistancePercent(c, kind, _item); }

int32_t CombatEngine::Res(cs::Ref<Monster> m, int32_t kind) { return kind < m->Resistances.Length() ? (int32_t)m->Resistances[kind] : 0; }

int32_t CombatEngine::MaxGuildLevel(cs::Ref<Character> c) {
    return c->Guilds.Count() == 0 ? 0 : c->Guilds.Max([&](auto& g) { return g->Level; });
}

cs::Ref<Monster> CombatEngine::DefOf(cs::Ref<Companion> comp) { return _monsterDef ? _monsterDef(comp) : cs::Ref<Monster>(); }

cs::List<cs::List<cs::Ref<CombatMonster>>> CombatEngine::AliveGroups() {
    auto groups = cs::List<cs::List<cs::Ref<CombatMonster>>>::New();
    for (auto& m : Monsters) {
        if (!m->Alive()) continue;
        if (groups.Count() > 0 && CombatMonster::SameGroup(groups[groups.Count() - 1][0], m)) groups[groups.Count() - 1].Add(m);
        else groups.Add(cs::List<cs::Ref<CombatMonster>>::Of({m}));
    }
    return groups;
}

int32_t CombatEngine::AttackChance(AttackKind kind, int32_t monDex, int32_t charDex) {
    int32_t chance = 50 + (monDex - charDex) * 5;
    if (kind == AttackKind::ChangedLevel && chance > 98) chance = 98;
    if (chance < 5) chance = 5;
    return chance;
}

cs::List<cs::String> CombatEngine::MonstersAttack() {
    cs::List<cs::String> _result = cs::List<cs::String>::New();
    for (auto& line : MonsterAttacks(AttackKind::Round)) if (line.Text != "") _result.Add(line.Text);
    return _result;
}

cs::List<CombatEngine_CombatLine> CombatEngine::MonsterAttacks(AttackKind kind) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    auto members = Party->Members;
    if (members.Count() == 0) return _result;
    // l0096: weights halving as an Integer, over the party as it stood (MULTIMOD.BAS:305-308)
    double total = 0; int32_t share = 100;
    for (int32_t i = 0; i < members.Count(); i++) { total += share; share /= 2; }

    for (auto& group : AliveGroups()) {
        // UBound(Chars) read afresh for every group (MULTIMOD.BAS:320-324)
        int32_t last = members.Count() - 1;
        if (last < 0) return _result;
        auto def = group[0]->Def;
        int32_t target = 0, roll = _rng->Next(100) + 1, acc = 100, step = 100;
        int32_t guard = 0;
        while (acc / total * 100 < roll && guard++ < 32) {
            target++;
            if (target > last) target = last;
            step /= 2;
            acc += step;
        }
        if (target > last) target = last;
        auto c = members[target];

        bool attack = true;
        if (kind != AttackKind::Round) {
            int32_t charDex = c->EffectiveStat(static_cast<int32_t>(Stat::Dex), _item);
            int32_t monDex = def->Stats.Length() > 5 ? (int32_t)def->Stats[5] : 0;
            attack = _rng->Next(100) + 1 < AttackChance(kind, monDex, charDex);
        }
        if (attack)
            for (auto& line : GroupAttacks(c, group)) _result.Add(line);
        // sub0A64, MULTIMOD.BAS:362-363
        if (c->CurHits <= 0) {
            // sub0930's head (MAINMOD2.BAS:3026-3033)
            c->Sub0930Head(_item);
            ShowCompanions(c);   // MAINMOD2.BAS:3029
            // FRMGROUP.BAS:326
            if (members.Count() > 1) { if (PartyUpdate) PartyUpdate(c); }
            // MULTIMOD.BAS:364
            if (members.Count() <= 1) return _result;
        }
    }
    return _result;
}

int32_t CombatEngine::TheftSlot(cs::Ref<cs::Random> rng) { return Floor(rng->NextDouble() * 39) + 2; }

cs::Nullable<int64_t> CombatEngine::GoldToSteal(VbCurrency onHand, int64_t rolled) {
    VbCurrency left{};
    if (!VbCurrency::TrySubtract(onHand, VbCurrency::Saturate(rolled), left)) return {};
    return left < VbCurrency::Zero ? cs::Nullable<int64_t>(onHand.RoundToWhole()) : cs::Nullable<int64_t>(rolled);
}

int32_t CombatEngine::AcidTarget(cs::Ref<cs::Random> rng, cs::Ref<CompanionSlots> slots) {
    // MONSMOD.BAS:2775: with every slot empty there is no draw
    if (!slots->Any()) return 0;
    for (int32_t guard = 0; guard < 1000; guard++) {
        int32_t t = cs::CastDouble<int32_t>(cs::Math::Floor(rng->NextDouble() * 5));
        if (t == 0 || t == 4) return t;      // the character, or the phantom fifth slot
        if (slots->Index(t) != nullptr) return t;
        // an empty slot reads -1, and the original draws again
    }
    return 0;
}

cs::List<CombatEngine_CombatLine> CombatEngine::GroupAttacks(cs::Ref<Character> c, cs::List<cs::Ref<CombatMonster>> group) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    if (c->IsDead || group.Count() == 0) return _result;
    auto def = group[0]->Def;
    bool watched = c == Party->Selected();
    bool party = Party->Size() > 1;
    cs::String name = NameFor(def, c, group[0], 1);

    if (watched) _result.Add([&] { auto _o = MkLine("", c); _o.Paced = true; _o.Channel = LineChannel::MonsterHit; return _o; }());

    int32_t monAtt = def->Att;
    int32_t charDef = DefenseOf(c);                                   // x1.5 when defending, gc1DB4
    int32_t monStr = def->Stats.Length() > 0 ? (int32_t)def->Stats[0] : 0;
    int32_t monLvl = def->GuildLevel;
    int32_t attackers = cs::Math::Min(group.Count(), 5);              // mc0448
    int32_t flags = def->SpecialAttackFlags;
    auto comps = c->Companions;
    // l0450: slot 0 the character, 1..4 the companions
    auto dealt = cs::List<int32_t>::New(5);
    int32_t hitsOnChar = 0;                                           // l045E
    bool charTargeted = false, anyHit = false;                        // l0470, l0462
    int64_t critFlags = 0;                                            // l0466

    // ---- breath (bits 3 fire, 4 cold), MONSMOD.BAS:2692-2757 ----
    bool fire = Bit(flags, 3), cold = Bit(flags, 4);
    if (fire || cold) {
        int32_t chance = 50, top = MaxGuildLevel(c);
        if (top > monLvl + monLvl / 4.0) chance = 80;
        if (top < monLvl - monLvl / 4.0) chance = 20;
        if (_rng->Next(100) < chance) {
            int32_t l046C = VbInt(monLvl * cs::Math::Log(monLvl + 1));
            // Log(0) raises an error: the handler (MONSMOD.BAS:3414-3417) ends the turn
            if (l046C <= 0) return _result;
            int32_t range = Floor(((cs::Math::Log(l046C) + l046C / 15.0) / cs::Math::Log(1.08) - c->Guild()->Level / 20.0) + 1);
            int32_t dmg = VbInt(((Floor((_rng->NextDouble() * range + 1) / 2) + range / 4.0) * 0.7) / 2);
            auto sounds = cs::List<cs::String>::New();
            for (int32_t slot = 0; slot < CompanionSlots::Slots; slot++) {
                // MONSMOD.BAS:2711: a fallen companion is not breathed on
                auto comp = comps->Index(slot);
                if (!comp || comp->Hits <= 0) continue;
                auto cdef = DefOf(comp);
                if (!cdef) continue;
                int32_t res = fire ? Res(cdef, 0) : Res(cdef, 1);
                if (res > 0) dmg = VbInt(dmg - dmg * ((res / 2.0) * 0.01));
                comp->Hits -= dmg;
                if (comp->Hits < 0) {
                    sounds.Add(DeathSound(cdef));
                    comp->Hits = 0;
                    for (auto& line : CompanionFalls(c)) _result.Add(line);
                }
            }
            if (Res(c, 0) > 0 && fire) dmg = VbInt(dmg - dmg * ((Res(c, 0) / 2.0) * 0.01));
            else if (Res(c, 1) > 0 && cold) dmg = VbInt(dmg - dmg * ((Res(c, 1) / 2.0) * 0.01));
            if (dmg > 0) {
                sounds.Add("FIRE");
                _result.Add(Caption(c, "You were Breathed on for " + cs::ToStr(dmg) + "!", sounds));
                for (auto& line : TakeDamage(c, dmg)) _result.Add(line);
                if (c->CurHits < 1) {
                    _result.Add(FoeMessage(party ? "\"" + c->Name + "\" was killed by the breath attack of a " + name + "." : "You were killed by the breath attack of a " + name + "."));
                    if (CharacterKilled) CharacterKilled(c, group[0]);
                    Dies(c, Rules::DeathOrdinary);
                    return _result;
                }
            } else if (sounds.Count() > 0) _result.Add(SoundLine(c, sounds));
        }
    }

    // ---- acid (bit 5), MONSMOD.BAS:2759-2831 ----
    if (Bit(flags, 5)) {
        int32_t chance = 50;
        if (c->Guild()->Level > monLvl + monLvl / 4.0) chance = 90;
        if (c->Guild()->Level < monLvl - monLvl / 4.0) chance = 10;
        // MONSMOD.BAS:2769-2778: slot 0 never spat on, phantom slot 4 lands on nobody
        int32_t t = AcidTarget(_rng, comps);
        if (_rng->Next(100) < chance) {
            int32_t l046C = monLvl + 1;
            int32_t range = Floor(((cs::Math::Log(l046C) + l046C / 15.0) / cs::Math::Log(1.08) - c->Guild()->Level / 10.0) + 1);
            int32_t dmg = VbInt((Floor((_rng->NextDouble() * range + 1) / 2) + range / 2.0) * 0.7);
            if (t == 0) {
                if (Res(c, 10) > 0) dmg = VbInt(dmg - dmg * ((Res(c, 10) / 2.0) * 0.01));
                if (dmg > 0) {
                    _result.Add(Caption(c, "You were Spit on for " + cs::ToStr(dmg) + "!", cs::List<cs::String>::Of({"SPIT"})));
                    for (auto& line : TakeDamage(c, dmg)) _result.Add(line);
                    if (c->CurHits < 1) {
                        _result.Add(FoeMessage(party ? "\"" + c->Name + "\" was killed by the acidic attack of a " + name + "." : "You were killed by the acidic attack of a " + name + "."));
                        if (CharacterKilled) CharacterKilled(c, group[0]);
                        Dies(c, Rules::DeathOrdinary);
                        return _result;
                    }
                }
            } else {
                // MONSMOD.BAS:2778: a fallen companion still in its slot can be spat on
                auto spat = comps->Index(t);
                cs::Ref<Monster> cdef = spat ? DefOf(spat) : cs::Ref<Monster>();
                if (spat && cdef) {
                    if (Res(cdef, 10) > 0) dmg = VbInt(dmg - dmg * ((Res(cdef, 10) / 2.0) * 0.01));
                    spat->Hits -= dmg;
                    if (spat->Hits < 0) {
                        _result.Add(SoundLine(c, cs::List<cs::String>::Of({DeathSound(cdef)})));
                        spat->Hits = 0;
                        // no repaint here (MONSMOD.BAS:2800-2806)
                        for (auto& line : CompanionFalls(c, false)) _result.Add(line);
                    }
                }
            }
            // t == 4 is the slot nobody occupies: the damage goes nowhere
        }
    }

    // ---- the attackers, MONSMOD.BAS:2832-2970 ----
    cs::Ref<Spell> cast = nullptr;
    for (int32_t n = 1; n <= attackers; n++) {
        double damMod = def->DamageMod <= 0 ? 1 : def->DamageMod;   // l0492
        if (def->SpellFlags > 0 && !NoSpellsHere) {
            auto spell = ChooseSpell(def);
            if (spell != nullptr) {
                cast = spell;
                int32_t dmg = MonsterSpellHits(c, spell, def, nullptr);
                // MONSMOD.BAS:2839 assigns rather than adds. Reproduced.
                dealt[0] = dmg;
                charTargeted = true;
                hitsOnChar++;
                // MONSMOD.BAS:2847-2852
                _result.Add(Picked(SoundLine(c, cs::List<cs::String>::Of({AttackSound(def)}))));
                if (dmg > 0) _result.Add(SoundLine(nullptr, HurtSounds(def).ToList()));
                for (int32_t i = 0; i <= spell->AffectGroup && i < 4; i++) {
                    auto ci = comps->Index(i);
                    if (!ci || ci->Hits <= 0) continue;
                    auto cdef = DefOf(ci);
                    if (!cdef) continue;
                    ci->Hits -= MonsterSpellHits(c, spell, def, ci);
                    if (ci->Hits < 1) {
                        _result.Add(SoundLine(c, cs::List<cs::String>::Of({DeathSound(cdef)})));
                        ci->Hits = 0;
                        for (auto& line : CompanionFalls(c)) _result.Add(line);
                    }
                }
                break;   // Exit For
            }
        }

        int32_t alive = comps->All().Count([&](auto& x) { return x->Hits > 0; });
        int32_t t = 0;
        if (alive > 0) {
            if (_rng->Next(100) + 1 < 100.0 / (alive + 1)) t = 0;
            else {
                for (int32_t tries = 0; tries < 100 && t == 0; tries++) {
                    int32_t pick = _rng->Next(4) + 1;
                    auto p = comps->Index(pick - 1);
                    if (p && p->Hits > 0) t = pick;
                }
            }
        }
        cs::Ref<Companion> comp = t == 0 ? nullptr : comps->Index(t - 1);
        cs::Ref<Monster> compDef = comp == nullptr ? nullptr : DefOf(comp);

        if (t == 0) {
            if (c->InvisibleNow && !Bit(def->SpecialPropertyFlags, 0)) damMod -= 0.5;
        } else if (compDef != nullptr && Bit(compDef->SpecialPropertyFlags, MonsterProperty::Invisible) && !Bit(def->SpecialPropertyFlags, 0))
            damMod -= 0.5;
        // MONSMOD.BAS:2907-2909: opening the chest makes you easier to hurt
        if (ChestRoundsOf(c) > 0) damMod += .75;

        int32_t targetDef;
        if (t == 0) { targetDef = charDef + ProtectionBonus(c); charTargeted = true; }
        else targetDef = compDef ? (int32_t)compDef->Def : 0;

        critFlags = 0;
        int32_t charLvl = c->Guild()->Level;
        if (Bit(flags, 10) && _rng->Next(100) + 1 <= Rules::MonsterCritChance(monLvl, def->LevelFound, charLvl, false))
        { damMod += 2; critFlags |= 1 << 10; }
        if (Bit(flags, 11) && _rng->Next(100) + 1 <= Rules::MonsterCritChance(monLvl, def->LevelFound, charLvl, true))
        { damMod += 1; critFlags |= 1 << 11; }
        int32_t targetSize = t == 0 ? SizeOf(c) : (compDef ? (int32_t)compDef->Size : 0);
        int32_t sizeDiff = cs::Math::Abs(targetSize - (int32_t)def->Size);
        if (targetSize < def->Size) damMod += sizeDiff / 10.0;
        else if (targetSize > def->Size) damMod -= sizeDiff / 10.0;

        int32_t blow = Rules::MonsterBlow(_rng, monAtt, targetDef, monLvl, monStr, damMod);
        if (t == 0 && blow > 0) hitsOnChar++;
        if (blow > 0) {
            anyHit = true;
            dealt[t] += blow;
            if (comp != nullptr) {
                comp->Hits -= blow;
                if (comp->Hits < 1) {
                    if (compDef != nullptr) _result.Add(SoundLine(c, cs::List<cs::String>::Of({DeathSound(compDef)})));
                    comp->Hits = 0;
                    for (auto& line : CompanionFalls(c)) _result.Add(line);
                }
            }
        }
    }

    // ---- what it came to, MONSMOD.BAS:2971-3030 ----
    cs::String caption;
    auto afterSounds = cs::List<cs::String>::New();
    if (dealt[0] > 0) {
        if (cast != nullptr) caption = "Monster cast \"" + cast->Name + "\" for " + cs::ToStr(dealt[0]) + " damage!";
        else {
            caption = cs::ToStr(hitsOnChar) + " " + NameFor(def, c, group[0], hitsOnChar);
            bool cr = Bit(static_cast<int32_t>(critFlags), 10), bs = Bit(static_cast<int32_t>(critFlags), 11);
            if (cr && bs) caption += " delivered a Lethal";
            else {
                if (cr) caption += " Critically";
                if (bs) caption += " Backstabbed and";
            }
            caption += " hit for " + cs::ToStr(dealt[0]) + "!";
        }
    } else if (cast != nullptr) caption = "Monster's attempt to cast \"" + cast->Name + "\" failed!";
    else caption = group.Count() > 1 ? "Monsters missed." : "Monster missed.";

    if (hitsOnChar > 0 || charTargeted) {
        afterSounds.Add(AttackSound(def));
        if (dealt[0] > 0) {
            bool cr = Bit(static_cast<int32_t>(critFlags), 10), bs = Bit(static_cast<int32_t>(critFlags), 11);
            if (cr && bs) afterSounds.Add("RIP"); else if (cr) afterSounds.Add("CHIT"); else if (bs) afterSounds.Add("BSTAB");
        }
    }
    if (!charTargeted) caption = "";
    if (dealt[0] > 0) afterSounds.AddRange(HurtSounds(def));
    if (dealt[0] > 0) for (auto& line : TakeDamage(c, dealt[0], true)) _result.Add(line);
    _result.Add(Caption(c, caption, afterSounds));
    if (dealt[0] > 0) for (auto& line : OuchLine(c, dealt[0])) _result.Add(line);
    if (c->CurHits < 1) {
        _result.Add(FoeMessage(party ? "\"" + c->Name + "\" was killed by a " + name + "." : "You were killed by a " + name + "."));
        if (CharacterKilled) CharacterKilled(c, group[0]);
        Dies(c, Rules::DeathOrdinary);
        return _result;
    }

    // ---- electrocution (bit 6), MONSMOD.BAS:3032-3105 ----
    if (Bit(flags, 6)) {
        for (int32_t t = 4; t >= 0; t--) {
            if (dealt[t] <= 0) continue;
            cs::Ref<Companion> comp = t == 0 ? nullptr : comps->Index(t - 1);
            // Monsters(-1).GuildLevel (MONSMOD.BAS:3055) is out of range: the turn is lost
            if (t > 0 && comp == nullptr) return _result;
            cs::Ref<Monster> compDef = comp == nullptr ? nullptr : DefOf(comp);
            if (t > 0 && compDef == nullptr) continue;
            int32_t tLvl = t == 0 ? c->Guild()->Level : (int32_t)compDef->GuildLevel;
            int32_t chance = 50;
            if (tLvl > monLvl + monLvl / 4.0) chance = 90;
            if (tLvl < monLvl - monLvl / 4.0) chance = 10;
            if (_rng->Next(100) >= chance) continue;
            int32_t l046C = monLvl + 1;
            int32_t range = Floor(((cs::Math::Log(l046C) + l046C / 15.0) / cs::Math::Log(1.08) - tLvl / 10.0) + 1);
            int32_t dmg = VbInt((Floor((_rng->NextDouble() * range + 1) / 2) + range / 2.0) * 0.7);
            int32_t res = t == 0 ? Res(c, 2) : Res(compDef, 2);
            if (res > 0) dmg = VbInt(dmg - dmg * ((res / 2.0) * 0.01));
            if (t == 0) _result.Add(Caption(c, "You were Electrocuted for " + cs::ToStr(dmg) + "!", cs::List<cs::String>::Of({"SHOCK"})));
            if (dmg <= 0) continue;
            if (t == 0) {
                for (auto& line : TakeDamage(c, dmg)) _result.Add(line);
                if (c->CurHits < 1) {
                    _result.Add(FoeMessage(party ? "\"" + c->Name + "\" was electrocuted to death by a " + name + "." : "You were electrocuted to death by a " + name + "."));
                    if (CharacterKilled) CharacterKilled(c, group[0]);
                    Dies(c, Rules::DeathOrdinary);
                    return _result;
                }
            } else {
                comp->Hits -= dmg;
                if (comp->Hits < 1) {
                    _result.Add(SoundLine(c, cs::List<cs::String>::Of({DeathSound(compDef)})));
                    comp->Hits = 0;
                    for (auto& line : CompanionFalls(c)) _result.Add(line);
                }
            }
        }
    }

    int32_t con = c->EffectiveStat(static_cast<int32_t>(Stat::Con), _item);
    // ---- ageing (bit 9), MONSMOD.BAS:3106-3130 ----
    if (Bit(flags, 9) && dealt[0] > 0 && anyHit) {
        if (_rng->Next(100) + 1 > con * 2.5 - def->LevelFound * 2) {
            int32_t years = VbInt(monLvl / 50.0);
            if (years < 1) years = 1;
            if (years > 5) years = 5;
            int32_t days = Floor(_rng->NextDouble() * (365 * 5)) + 1;
            if (days > years * 365) days = years * 365;
            // mdrcharAddAge says its own line first (MONSMOD.BAS:3129, MAINMOD2.BAS:6400-6405)
            cs::String older = c->AgeBy(days, party);
            if (older != "") _result.Add(FoeMessage(older));
            if (watched) _result.Add(Caption(c, "You are now older!", cs::List<cs::String>::Of({"WITHER"})));
            // WITHER under the No Party Sound test (MONSMOD.BAS:3130-3132)
            else _result.Add(FoeMessage("\"" + c->Name + "\" is now older!", cs::List<cs::String>::Of({"WITHER"}), c));
        }
    }

    // ---- drain (bit 7), MONSMOD.BAS:3131-3186 ----
    if (Bit(flags, 7) && dealt[0] > 0 && anyHit) {
        bool drained = false;
        int32_t tries = Floor(_rng->NextDouble() * VbInt(def->LevelFound / 2.0 + 0.5)) + 1;
        auto race = RaceData::ByName(c->Race);
        for (int32_t i = 0; i < tries; i++) {
            if (_rng->Next(100) + 1 <= Res(c, 9)) continue;
            if (_rng->Next(100) + 1 <= con * 2.5 - def->LevelFound * 2) continue;
            int32_t pick = Floor(_rng->NextDouble() * 6 + 1);   // 1..6: five stats, then hits
            drained = true;
            if (pick <= 5) {
                // MONSMOD.BAS:3152-3162: held, the stored Att/Def do not move with the stat
                int32_t shift = c->StoredADShift(_item), heldBefore = c->StatsADNow(_item);
                c->Stats[pick] -= 1;
                if (c->Stats[pick] < race->Min[pick]) c->Stats[pick] = race->Min[pick];
                if (c->Paralysed()) c->ADShift = shift - (c->StatsADNow(_item) - heldBefore);
                _result.Add(FoeMessage(party
                    ? "\"" + c->Name + "\" had " + (c->Sex == Sex::Male ? cs::String("his") : cs::String("her")) + " " + RaceData::StatNamesLong[pick] + " decreased by 1."
                    : "Your " + RaceData::StatNamesLong[pick] + " has decreased by 1."));
            } else {
                c->MaxHits -= 2;
                if (c->MaxHits < MaxGuildLevel(c)) c->MaxHits = MaxGuildLevel(c);
                if (c->CurHits > c->MaxHits) c->CurHits = c->MaxHits;
            }
        }
        if (drained) {
            _result.Add(FoeMessage(watched ? cs::String("You have been drained!") : "\"" + c->Name + "\" has been drained!"));
            if (watched) _result.Add(Caption(c, "You have been drained!", cs::List<cs::String>::Of({"DRAIN"})));
            else _result.Add(SoundLine(c, cs::List<cs::String>::Of({"DRAIN"})));

            // MONSMOD.BAS:3190 - mdrcharCheckItems with the announcement on
            for (auto& line : GearCheck::Run(c, _item, party, true))
                _result.Add(Message(line));
        }
    }

    // ---- stoning (bit 8), MONSMOD.BAS:3187-3234 ----
    if (Bit(flags, 8)) {
        for (int32_t t = 4; t >= 0; t--) {
            if (dealt[t] <= 0) continue;
            cs::Ref<Companion> comp = t == 0 ? nullptr : comps->Index(t - 1);
            // MONSMOD.BAS:3199 out of range, handler ends the turn (3414-3417)
            if (t > 0 && comp == nullptr) return _result;
            cs::Ref<Monster> compDef = comp == nullptr ? nullptr : DefOf(comp);
            if (t > 0 && compDef == nullptr) continue;
            int32_t res = t == 0 ? Res(c, 7) : Res(compDef, 7);
            if (_rng->Next(100) + 1 <= res) continue;
            int32_t tCon = t == 0 ? con : (compDef->Stats.Length() > 3 ? (int32_t)compDef->Stats[3] : 0);
            double chance = tCon * (cs::Math::Log(cs::Math::Max(1, tCon)) / cs::Math::Log(2.5)) - def->LevelFound;
            if (chance > 95) chance = 95;
            if (_rng->Next(100) + 1 <= chance) continue;
            if (t == 0) {
                if (watched) _result.Add(Caption(c, "You are now a statue.", cs::List<cs::String>::Of({"STONE"})));
                else _result.Add(SoundLine(c, cs::List<cs::String>::Of({"STONE"})));
                _result.Add(FoeMessage(party ? "\"" + c->Name + "\" was turned to stone by a " + name + "." : "You were turned to stone by a " + name + "."));
                c->Stoned = true;
                if (CharacterKilled) CharacterKilled(c, group[0]);
                Dies(c, Rules::DeathStoned);
                return _result;
            }
            auto sounds = cs::List<cs::String>::Of({"STONE", DeathSound(compDef)});
            if (watched) _result.Add(Caption(c, CompanionName(comp, t - 1) + " is now a statue.", sounds));
            else _result.Add(SoundLine(c, sounds));
            // Ref = -1 and nothing else (MONSMOD.BAS:3239)
            comps->Release(t - 1);
            ShowCompanions(c);   // MONSMOD.BAS:3240
        }
    }

    // ---- poison (bit 0) and disease (bit 1), MONSMOD.BAS:3235-3270 ----
    if (Bit(flags, 0) && dealt[0] > 0 && anyHit && _rng->Next(100) + 1 > Res(c, 5) && c->PoisonLevel == 0) {
        c->Poisoned = true; c->PoisonLevel = def->LevelFound;
        _result.Add(Caption(c, "You have been Poisoned!", cs::List<cs::String>::Of({"POISON"})));
    }
    if (Bit(flags, 1) && dealt[0] > 0 && anyHit && _rng->Next(100) + 1 > Res(c, 4) && c->DiseaseLevel == 0) {
        c->Diseased = true; c->DiseaseLevel = def->LevelFound;
        _result.Add(Caption(c, "You have been Diseased!", cs::List<cs::String>::Of({"DISEASE"})));
    }

    // ---- paralysis (bit 2), MONSMOD.BAS:3271-3287 ----
    if (Bit(flags, 2) && dealt[0] > 0 && anyHit) {
        int32_t chance = Res(c, 8) + con;
        if (chance > 95) chance = 95;
        if (_rng->Next(100) + 1 > chance && ParalyzedRounds(c) == 0) {
            // MONSMOD.BAS:3286: the monster's guild level as it stands
            c->Paralyse(monLvl, _item);
            // MONSMOD.BAS:3285-3291
            if (watched) _result.Add(Caption(c, HelpCry::ParalysisWatched, cs::List<cs::String>::Of({"PARAL"})));
            // PARAL under the No Party Sound test (MONSMOD.BAS:3287-3289)
            else _result.Add(FoeMessage(HelpCry::Paralysis(c), cs::List<cs::String>::Of({"PARAL"}), c));
        }
    }

    // ---- item destruction (bit 12), MONSMOD.BAS:3288-3315 ----
    if (Bit(flags, 12)) {
        // MONSMOD.BAS:3295-3296 rolls a slot and looks in it
        int32_t slot = Floor(_rng->NextDouble() * 39) + 1;    // pack slots 1..39
        auto inSlot = c->Pack()[slot];
        cs::Ref<Item> it = (inSlot && inSlot->ItemId > 0) ? _item(inSlot->ItemId) : cs::Ref<Item>();
        if (inSlot && inSlot->ItemId > 0 && it) {
            int32_t dex = c->Stats[static_cast<int32_t>(Stat::Dex)], monDex = def->Stats.Length() > 5 ? (int32_t)def->Stats[5] : 0;
            int32_t chance = ParalyzedRounds(c) == 0
                ? (45 - def->LevelFound) + (dex + dex) * 2 - monDex * 2
                : VbInt((45 - def->LevelFound) + (dex + dex) / 2.0 - monDex * 2);
            if (chance > 90) chance = 90;
            if (chance < 25) chance = 25;
            if (_rng->Next(100) + 1 > chance && it->Floor - 2 <= def->LevelFound && ChestTraps::Destructible(it)) {
                _result.Add(FoeMessage(watched ? name + ": " + it->Name + " destroyed!." : c->Name + ", " + name + ": " + it->Name + " destroyed!.", cs::List<cs::String>::Of({"SLIMETR"}), c));   // guarded, 3318-3320
                RemoveItem(c, inSlot);
                ItemsDestroyed.Add(it->Name);
            }
        }
    }

    // ---- theft (bit 13), MONSMOD.BAS:3316-3392 ----
    if (Bit(flags, 13)) {
        int32_t score = _rng->Next(100);
        // the thieving check is made against the character being watched
        auto _wg = WinningGuild(Party->Selected(), 0);
        double thiefMod = _wg.mod; int32_t thiefLvl = _wg.level;
        score += Floor(_rng->NextDouble() * (thiefLvl * thiefMod) + thiefLvl / 2.0 * 1.5);
        if (Party->OrderFor(Party->Selected())->Kind == OrderKind::Defend) score += score / 2;
        score -= VbInt((double)dealt[0] / cs::Math::Max(1, c->MaxHits) * 100);
        if (score - monLvl * 2 < 50) {
            int64_t gold = cs::CastDouble<int64_t>(cs::Math::Floor(_rng->NextDouble() * (((c->DungeonLevel + monAtt) * (3353.452 * monLvl)) / cs::Math::Max(1, c->Guild()->Level))));
            auto take = GoldToSteal(c->GoldOnHand, gold);
            if (!take.has_value()) return _result;   // sub0E85's trap (MONSMOD.BAS:2669, 3414-3417)
            gold = take.value();
            if (gold > 0) {
                if (watched) _result.Add(Caption(c, CharStatFile::FormatNumber(gold) + " Gold Stolen!", cs::List<cs::String>()));
                else _result.Add(FoeMessage("\"" + c->Name + "\" had " + CharStatFile::FormatNumber(gold) + " Gold Stolen!"));
                c->GoldOnHand = c->GoldOnHand - VbCurrency::FromRaw(gold * VbCurrency::Scale);
                if (c->GoldOnHand < VbCurrency::Zero) c->GoldOnHand = VbCurrency::Zero;
            }
            cs::Ref<InventorySlot> taken = nullptr; cs::Ref<Item> takenItem = nullptr;
            if (score - monLvl * 2 <= 10) {
                // MONSMOD.BAS:3351, wrong at both ends - see TheftSlot
                int32_t slot = TheftSlot(_rng);
                cs::Ref<InventorySlot> s = slot < Character::PackSize ? c->Pack()[slot] : cs::Ref<InventorySlot>();
                if (slot < Character::PackSize && s && s->ItemId > 0) {
                    auto it = _item(s->ItemId);
                    if (it != nullptr && ChestTraps::Destructible(it) && !s->Equipped) {
                        _result.Add(FoeMessage(name + ": \"Thanks for the " + it->Name + ", " + c->Name + "!\""));
                        taken = s; takenItem = it;
                        RemoveItem(c, s);
                        ItemsStolen.Add(s->ItemId);
                    }
                }
            }
            if (gold > 0 || taken != nullptr) _result.Add(SoundLine(c, cs::List<cs::String>::Of({"STEAL"})));
            if ((_rng->Next(100) + 1 > 30 && gold > 0) || taken != nullptr) {
                // the thief makes off with it: one of the group leaves
                _result.Add(SoundLine(c, cs::List<cs::String>::Of({"BYE"})));
                auto left = group[group.Count() - 1];
                left->Gone = true;
                group.RemoveAt(group.Count() - 1);
                // MONSMOD.BAS:3380-3386
                if (group.Count() > 0) Sub0D08(AimedRecord(c), left);
                else Retarget(c);
            } else GoldRecovered[c] = GoldRecovered.GetValueOrDefault(c) + gold;   // still in its pockets when it dies
        }
    }

    // ---- "Help!", MONSMOD.BAS:3404-3409
    if (party && HelpCry::Needed(c))
        _result.Add(Message(HelpCry::Line(c), cs::List<cs::String>::Of({HelpCry::Sound})));
    return _result;
}

cs::Ref<Spell> CombatEngine::ChooseSpell(cs::Ref<Monster> def) {
    if (_spells == nullptr) return nullptr;
    int32_t school = -1;
    for (int32_t tries = 0; tries <= 1000 && school < 0; tries++) {
        int32_t pick = _rng->Next(18);
        if (Bit(def->SpellFlags, pick)) school = pick;
    }
    if (school < 0) return nullptr;
    int32_t top = def->LevelFound + 1;
    int32_t low = VbInt(def->LevelFound - def->LevelFound / 3.0);
    if (low < 1) low = 1;
    auto pool = _spells.Where([&](auto& s) { return s->Category == school && s->Level >= low && s->Level <= top; }).ToList();
    if (pool.Count() == 0) return nullptr;
    if (_rng->Next(100) < 10) return nullptr;
    if (!cs::Str::Contains(Mdata1::MonsterClassName(def->Type), "Mage") && _rng->NextDouble() * 100 < 50) return nullptr;
    int32_t l00CC = _rng->Next(pool.Count());
    auto chosen = pool[l00CC];
    if (!(chosen->Damage2 > 0 && chosen->SpecialEffect > 0)) return nullptr;
    // MONSMOD.BAS:244-245: the sound is SpellDefs(l00CC)'s, the pool index. Reproduced.
    PendingPickSounds = l00CC < _spells.Count() ? SpellSounds::For(_spells[l00CC]) : cs::List<cs::String>();
    return chosen;
}

CombatEngine_CombatLine CombatEngine::Picked(CombatEngine_CombatLine line) {
    if (PendingPickSounds == nullptr) return line;
    auto l = line;
    l.PickSounds = PendingPickSounds;
    PendingPickSounds = nullptr;
    return l;
}

int32_t CombatEngine::MonsterSpellHits(cs::Ref<Character> c, cs::Ref<Spell> spell, cs::Ref<Monster> caster, cs::Ref<Companion> comp) {
    int32_t lvl = Floor(caster->GuildLevel / 4.0);
    if (lvl < 1) lvl = 1;
    if (lvl > 255) lvl = 255;
    double l042C = cs::Math::Log(lvl + 1) / cs::Math::Log(5);
    double power = spell->Damage2 + spell->Damage2 * l042C;
    power += Floor(lvl / 10.0 * spell->SpecialEffect);
    int32_t cut = Floor(_rng->NextDouble() * (power - (lvl - 1) / cs::Math::Pow(lvl, 2)) / 2) + 1;
    int32_t dmg = Floor(power - cut);
    cs::Ref<Monster> compDef = comp == nullptr ? nullptr : DefOf(comp);
    if (spell->ResistIndex > -1) {
        int32_t res = comp == nullptr ? Res(c, spell->ResistIndex) : compDef == nullptr ? 0 : Res(compDef, spell->ResistIndex);
        dmg = VbInt(dmg * (1 - res / 100.0));
    }
    if (dmg < 1) dmg = 0;
    if (comp != nullptr && compDef != nullptr) {
        // checked against the group the owner is fighting (MONSMOD.BAS:2561-2564)
        if (Bit(compDef->SpecialPropertyFlags, 2)) dmg = 0;
        if (spell->Effect >= 75 && spell->Effect <= 99) {
            auto fo = FoeOf(c);
            cs::Ref<Monster> facing = fo ? fo->Def : cs::Ref<Monster>();
            if (facing != nullptr && facing->Type < Mdata1::MonsterClasses.Count() && Mdata1::MonsterClasses[facing->Type].Index != spell->Effect - 75) dmg = 0;
        }
    }
    if (spell->Effect == 100) {
        // all or nothing: it only counts if it would kill
        int32_t hits = comp ? comp->Hits : c->CurHits;
        if (hits - dmg > 0) dmg = 0;
    }
    return dmg;
}

int32_t CombatEngine::ProtectionBonus(cs::Ref<Character> c) {
    return c->Protection > 0 ? VbInt(cs::Math::Log(c->Protection) * (cs::Math::Log(c->Protection) * 3)) : 0;
}

cs::String CombatEngine::AttackSound(cs::Ref<Monster> def) {
    if (Asmodeus::Is(def)) return Asmodeus::Sound;
    switch (def->Kind()) {
        case MonsterKind::Humanoid: case MonsterKind::Thief: return "GOBLIN";
        case MonsterKind::Cleanup: case MonsterKind::Elemental: return "SLIME";
        case MonsterKind::Demon: return "DEMON";
        case MonsterKind::Devil: return "DEVIL";
        case MonsterKind::Reptile: return "SNAKE";
        case MonsterKind::Dragon: return "DRAGON";
        case MonsterKind::Animal: return def->PicId == 18 ? "BIRD" : "DOG";
        case MonsterKind::Insect: return "BUG";
        case MonsterKind::Undead: return "UNDEAD";
        case MonsterKind::Aquatic: return "WATERD";
        case MonsterKind::Giant: return "GIANT";
        case MonsterKind::Mythical: return "MYTHIC";
        case MonsterKind::Lycanthrope: return "LYCAN";
        case MonsterKind::Mage: return "MAGE";
        case MonsterKind::Warrior: return "WARRIOR";
        case MonsterKind::Indigini: return "PLANTATT";
        default: return "";
    }
}

cs::List<cs::String> CombatEngine::HurtSounds(cs::Ref<Monster> def) {
    cs::List<cs::String> _result = cs::List<cs::String>::New();
    auto k = def->Kind();
    if (k == MonsterKind::Demon || k == MonsterKind::Devil || k == MonsterKind::Elemental || k == MonsterKind::Animal) _result.Add("RIP");
    _result.Add("OUCH");
    return _result;
}

cs::String CombatEngine::CompanionName(cs::Ref<Companion> comp, int32_t index) {
    return cs::Str::Trim(comp->Name) == "" ? "Companion #" + cs::ToStr(index + 1) : "\"" + cs::Str::Trim(comp->Name) + "\"";
}

cs::String CombatEngine::NameFor(cs::Ref<Monster> def, cs::Ref<Character> viewer, cs::Ref<CombatMonster> m, int32_t count) {
    return EncounterGenerator::NameFor(def, viewer, count, m->Identified ? 0 : 1);
}

CombatEngine_CombatLine CombatEngine::Caption(cs::Ref<Character> c, cs::String text, cs::List<cs::String> sounds) {
    bool watched = c == Party->Selected();
    if (watched && text != "") Say(text);
    auto _o = MkLine(watched ? text : cs::String(""), c);
    _o.Paced = watched;
    _o.HitSounds = sounds;
    _o.Channel = LineChannel::MonsterHit;
    return _o;
}

CombatEngine_CombatLine CombatEngine::Message(cs::String text, cs::List<cs::String> sounds, cs::Ref<Character> soundOf) {
    Say(text);
    auto _o = MkLine(text, soundOf);
    _o.HitSounds = sounds;
    _o.Channel = LineChannel::Message;
    return _o;
}

CombatEngine_CombatLine CombatEngine::FoeMessage(cs::String text, cs::List<cs::String> sounds, cs::Ref<Character> soundOf) {
    auto l = Message(text, sounds, soundOf);
    l.Kind = MessageKind::Foes;
    return l;
}

cs::List<CombatEngine_CombatLine> CombatEngine::TakeDamage(cs::Ref<Character> c, int32_t dmg, bool silent) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    c->CurHits -= dmg;
    if (c->CurHits < 0) c->CurHits = 0;
    if (silent) return _result;
    for (auto& line : OuchLine(c, dmg)) _result.Add(line);
    return _result;
}

cs::List<CombatEngine_CombatLine> CombatEngine::OuchLine(cs::Ref<Character> c, int32_t dmg) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    if (Party->Size() <= 1 || c == Party->Selected()) return _result;
    if (c->CurHits == 0) _result.Add(FoeMessage(c->Name + ", \"Aigh!\""));
    else if (dmg >= c->MaxHits / 3.0) _result.Add(FoeMessage(c->Name + ", \"Ouch!  Hit for " + cs::ToStr(dmg) + "!\""));
    return _result;
}

void CombatEngine::Dies(cs::Ref<Character> c, int32_t cause) {
    // mdrcharSetDead inside sub0E85 (MONSMOD.BAS:2757-3228); sub0930 is MonsterAttacks'
    SetDead(c, cause);
}

void CombatEngine::SetDead(cs::Ref<Character> c, int32_t cause) {
    c->CurHits = 0;
    c->IsDead = true;
    c->CauseOfDeath = cause;
}

cs::List<CombatEngine_CombatLine> CombatEngine::CompanionFalls(cs::Ref<Character> c, bool repaint) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    cs::Func<cs::Ref<Monster>(cs::Ref<Companion>)> lookup = _monsterDef ? _monsterDef : cs::Func<cs::Ref<Monster>(cs::Ref<Companion>)>([](cs::Ref<Companion>) { return cs::Ref<Monster>(); });
    for (auto& line : CompanionCarry::Run(c, c == Party->Selected(), lookup))
        _result.Add(Message(line));
    if (repaint) ShowCompanions(c);
    return _result;
}

void CombatEngine::ShowCompanions(cs::Ref<Character> c) {
    if (c == Party->Selected()) c->Companions->BlankEmptyNames();
}

void CombatEngine::RemoveItem(cs::Ref<Character> c, cs::Ref<InventorySlot> slot) { c->RemoveFromPack(slot); }

}  // namespace mc
