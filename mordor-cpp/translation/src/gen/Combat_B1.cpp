#include "core.h"

namespace mc {

namespace {
CombatEngine_CombatLine MakeLine(const cs::String& text, cs::Ref<Character> actor) {
    CombatEngine_CombatLine l{};
    l.Text = text;
    l.Actor = actor;
    return l;
}
}  // namespace

void CombatEngine::UnlockChest() { ChestLocked = -1; }

int64_t CombatEngine::ExpEarned() { return Experience.Values().Sum(); }

int32_t CombatEngine_ParalysisView::Index(cs::Ref<Character> c) { return c->ParalysisLevel; }

bool CombatEngine_ParalysisView::Remove(cs::Ref<Character> c) {
    bool was = c->ParalysisLevel != 0;
    c->ParalysisLevel = 0;
    return was;
}

int64_t CombatEngine::GoldStolen() { return GoldRecovered.Values().Sum(); }

CombatEngine::CombatEngine(cs::Ref<mc::Party> party, cs::List<cs::Ref<CombatMonster>> monsters, cs::Func<cs::Ref<Item>(int16_t)> itemLookup, cs::Ref<cs::Random> rng, bool isLair) {
    Party = party; _item = itemLookup; _rng = rng ? rng : cs::New<cs::Random>();
    Monsters.AddRange(monsters);
    for (auto& m : Monsters) if (m->MaxHits == 0) m->MaxHits = m->Hits();
    // The original fixes an encounter's treasure when it generates the monsters (MONSMOD.BAS)
    auto kinds = Monsters.GroupBy([&](auto& m) { return m->Def; }).ToList();
    // the chest the encounter rolled (MONSMOD.BAS:1651-1697)
    cs::Ref<Monster> lead = Monsters.Count() > 0 ? Monsters[0]->Def : cs::Ref<Monster>();
    // `monsters is Encounter`: a cs::List handle cannot carry the derived Encounter type
    cs::Ref<Encounter> rolled = nullptr;
    if (rolled)
    {
        Source = rolled;
        // fn0D4D has already rolled all of it (MONSMOD.BAS:1656-1702 and 1859-1978)
        ChestClass = rolled->TreasureClass;
        ChestTrap = rolled->ChestTrap;
        ChestLocked = rolled->ChestLock;
        ChestGuard = rolled->LeadMonster ? (int32_t)rolled->LeadMonster->GuildLevel : (lead ? (int32_t)lead->GuildLevel : 0);
        GoldEarned = GoldAfterClass(rolled->Gold, rolled->TreasureClass);
        return;
    }
    if (lead != nullptr)
    {
        ChestClass = Rules::ChestClass(lead, _rng->NextDouble() * 99, isLair);
        ChestTrap = Rules::ChestTrap(lead, ChestClass, _rng);
        ChestGuard = lead->GuildLevel;
        if (_rng->Next(100) + 1 <= lead->LockedChance && ChestClass > 1) ChestLocked = 1;
    }
    GoldEarned = Rules::EncounterGold(kinds.Select([&](auto& k) { return k.Key; }).ToList(),
                                      kinds.Select([&](auto& k) { return (int32_t)k.Count(); }).ToList(), _rng,
                                      ChestClass, ChestLocked == 1);
}

cs::Ref<Monster> CombatEngine::ChestLead() {
    cs::Ref<Monster> s = Source ? Source->LeadMonster : cs::Ref<Monster>();
    return s ? s : (Monsters.Count() > 0 ? Monsters[0]->Def : cs::Ref<Monster>());
}

int64_t CombatEngine::GoldAfterClass(cs::Decimal fightGold, int32_t treasureClass) {
    int64_t g = cs::Decimal::Round(fightGold, 0, cs::MidpointRounding::ToEven).ToInt64();
    double f;
    if (treasureClass == 0 || treasureClass == 1) f = .5;
    else if (treasureClass == 2) f = 1;
    else if (treasureClass == 3) f = 1.5;
    else f = 1;
    return treasureClass >= 0 && treasureClass <= 3 ? cs::CastDouble<int64_t>(cs::Math::Round(g * f, cs::MidpointRounding::ToEven)) : g;
}

CombatEngine::CombatEngine(cs::Ref<Character> lone, cs::List<cs::Ref<CombatMonster>> monsters, cs::Func<cs::Ref<Item>(int16_t)> itemLookup, cs::Ref<cs::Random> rng, bool isLair)
    : CombatEngine(cs::New<mc::Party>(cs::List<cs::Ref<Character>>::Of({lone})), monsters, itemLookup, rng, isLair) { }

cs::List<cs::Ref<Character>> CombatEngine::NewlyDead() {
    if (_deadAtStart.IsNull()) _deadAtStart = cs::HashSet<cs::Ref<Character>>::New();
    return Party->Members.Where([&](auto& c) { return c->IsDead && !_deadAtStart.Contains(c); });
}

void CombatEngine::NoteWhoIsAlive() {
    _deadAtStart = cs::HashSet<cs::Ref<Character>>::From(Party->Members.Where([&](auto& c) { return c->IsDead; }));
}

void CombatEngine::UseMonsterTable(cs::List<cs::Ref<Monster>> table) {
    _monsterDef = [=](cs::Ref<Companion> c) -> cs::Ref<Monster> { return CompanionRef::Def(c, table); };
}

void CombatEngine::UseCompanionDefs(cs::Func<cs::Ref<Monster>(cs::Ref<Companion>)> lookup) { _monsterDef = lookup; }

void CombatEngine::UseSpellTable(cs::List<cs::Ref<Spell>> spells) { _spells = spells; }

cs::Ref<Character> CombatEngine::Actor() { return Party->Selected(); }

bool CombatEngine::IsOver() { return Fled || !Party->AnyAble() || Monsters.All([&](auto& m) { return !m->Alive(); }); }
bool CombatEngine::Won() { return !Fled && Party->AnyAble() && Monsters.All([&](auto& m) { return !m->Alive(); }); }
bool CombatEngine::PartyWiped() { return !Party->AnyAble(); }
int32_t CombatEngine::Killed() { return Monsters.Count([&](auto& m) { return m->Hits() <= 0 && !m->Alive(); }); }

int32_t CombatEngine::ParalyzedRounds(cs::Ref<Character> c) { return c->ParalysisLevel; }

int32_t CombatEngine::StatsADOf(cs::Ref<Character> c) { return Rules::StatsAD(StatsWithGear(c)) + c->StoredADShift(_item); }

cs::List<int32_t> CombatEngine::StatsWithGear(cs::Ref<Character> c) {
    return cs::Enumerable::Range(0, 6).Select([&](auto& i) { return c->EffectiveStat(i, _item); }).ToArray();
}

int32_t CombatEngine::AttackOf(cs::Ref<Character> c) {
    return cs::Math::Max(0, Rules::BaseAD + StatsADOf(c) + Rules::BestGuildAD(c).att + EquippedSum(c, [&](cs::Ref<Item> i) { return (int32_t)i->Att; }));
}

int32_t CombatEngine::DefenseOf(cs::Ref<Character> c) {
    return cs::Math::Max(0, Rules::BaseAD + StatsADOf(c) + Rules::BestGuildAD(c).def + EquippedSum(c, [&](cs::Ref<Item> i) { return (int32_t)i->Def; }));
}

int32_t CombatEngine::EquippedSum(cs::Ref<Character> c, cs::Func<int32_t(cs::Ref<Item>)> f) { return WornSum(c, _item, f); }

int32_t CombatEngine::WornSum(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item, cs::Func<int32_t(cs::Ref<Item>)> f) {
    return c->Inventory().Where([&](auto& s) { return s->Equipped; })
        .Select([&](auto& s) { return std::make_pair(s, item(s->ItemId)); })
        .Where([&](auto& t) { return t.second != nullptr; })
        .Sum([&](auto& t) { return ItemAlignments::AttDefWorth(f(t.second), ItemAlignments::FitFor(t.first->Alignment, c->Alignment)); });
}

cs::Ref<Item> CombatEngine::WeaponOf(cs::Ref<Character> c) {
    return c->Inventory().Where([&](auto& s) { return s->Equipped; }).Select([&](auto& s) { return _item(s->ItemId); })
        .Where([&](auto& i) { return i != nullptr && i->Type <= 7 && i->Att > 0; })
        .OrderByDescending([&](auto& i) { return i->Att; }).FirstOrDefault();
}

int32_t CombatEngine::SizeOf(cs::Ref<Character> c) { return static_cast<int32_t>(RaceData::ByName(c->Race)->Size); }

int32_t CombatEngine::Clamp(int32_t v, int32_t lo, int32_t hi) { return cs::Math::Max(lo, cs::Math::Min(hi, v)); }

Tup_def_level_ba376 CombatEngine::BestAbilityGuild(cs::Ref<Character> c, cs::Func<float(cs::Ref<GuildDef>)> pick) {
    cs::Ref<GuildDef> best = nullptr; int32_t lvl = 1;
    for (auto& m : c->Guilds)
    {
        auto d = GuildData::ByName(m->Guild);
        if (best == nullptr || pick(d) > pick(best)) { best = d; lvl = m->Level; }
    }
    return Tup_def_level_ba376{best ? best : GuildData::ByName("Nomad"), lvl};
}

Tup_mod_level_cd3a8 CombatEngine::WinningGuild(cs::Ref<Character> c, int32_t ability) {
    double best = 0, bestMod = 0; int32_t bestLevel = 0;
    for (auto& g : GuildData::All)                 // table order, so ties go to the lower index
    {
        auto m = c->Guilds.FirstOrDefault([&](auto& x) { return x->Guild == g->Name; });
        if (m == nullptr || m->Level <= 0) continue;
        double mod = g->Abilities.ElementAtOrDefault(ability);
        double value = ability == 4
            ? Rules::FightingBase + (((cs::Math::Log(100 + m->Level) / 1.75) - 2.3) * (mod * mod)) / 2
            : mod * m->Level;
        if (value > best) { best = value; bestMod = mod; bestLevel = m->Level; }
    }
    return Tup_mod_level_cd3a8{bestMod, bestLevel};
}

void CombatEngine::Credit(cs::Ref<Character> who, cs::Ref<CombatMonster> target, int32_t dmg, bool byCompanion, DamageKind kind, bool removes) {
    int32_t inflicted = cs::Math::Min(dmg, cs::Math::Max(0, target->Hits()));
    if (removes && inflicted > 0 && DamageDealt != nullptr && Monsters.Contains(target))
        Dealt(who, inflicted, byCompanion ? DamageKind::Companion : kind, 0);
    int64_t xp = Rules::ExpAwarded(inflicted, target->Def->GuildLevel, who->Guild()->Level, byCompanion, Party->Size());
    Experience[who] = Experience.GetValueOrDefault(who) + xp;
    // paid there and then, blow by blow (sub0CDF -> sub0F11, MONSMOD.BAS:468, 495)
    GiveExperience(who, xp);
}

void CombatEngine::GiveExperience(cs::Ref<Character> who, int64_t xp, ExperienceSource source) {
    int64_t before = who->Guild()->Experience;
    // sub0F11 judges both lines by before/after this award (291-292)
    int64_t cap = who->ExperienceCap();
    bool wasPinned = who->Guild()->Experience >= cap;
    bool hadEnough = who->Guild()->Experience >= who->ExpForNextLevel();
    who->Guild()->Experience += xp;
    if (who->Guild()->Experience >= cap)
    {
        hadEnough = true;
        who->Guild()->Experience = cap;
        if (!wasPinned)
            _said.Add(Message(Party->Size() > 1 ? "\"" + cs::Str::TrimEnd(who->Name) + "\" is now pinned." : cs::String("You are now pinned.")));
    }
    // fn12A1 alone: the guild's level ceiling is not asked here
    if (who->Guild()->Experience >= who->ExpForNextLevel() && !hadEnough)
        _said.Add(Message(Party->Size() > 1 ? "\"" + cs::Str::TrimEnd(who->Name) + "\" now has enough experience to make a level."
                                            : cs::String("You now have enough experience to make a level.")));
    if (ExperienceGained)
        ExperienceGained([&] { CombatExperience _o{}; _o.Character = who; _o.Amount = cs::Math::Max((int64_t)0, who->Guild()->Experience - before); _o.Source = source; return _o; }());
}

void CombatEngine::AwardSpoils() {
    // sub0C53 (MONSMOD.BAS:783-789) calls monGetGold only when the fight left no chest
    if (ChestClass == 0) PayGold();
}

cs::List<CombatEngine_CombatLine> CombatEngine::TakeSaid() {
    auto l = _said.ToList();
    _said.Clear();
    return l;
}

void CombatEngine::KilledBy(cs::Ref<Character> killer, cs::Ref<Monster> def, DamageKind how) {
    killer->Kills++;
    Dealt(killer, 0, how, 1);
    QuestKill(def);
}

void CombatEngine::CompanionKill(cs::Ref<Character> owner, cs::Ref<CombatMonster> victim) {
    if (DamageDealt != nullptr && Monsters.Contains(victim)) Dealt(owner, 0, DamageKind::Companion, 1);
}

void CombatEngine::Dealt(cs::Ref<Character> who, int32_t amount, DamageKind kind, int32_t kills) {
    if (DamageDealt)
        DamageDealt([&] { CombatDamage _o{}; _o.Character = who; _o.Amount = amount; _o.Kind = kind; _o.Kills = kills; _o.Shown = who == Party->Selected(); return _o; }());
}

void CombatEngine::QuestKill(cs::Ref<Monster> def) {
    for (auto& c : Party->Members)
    {
        auto g = c->Guild();
        if (g->QuestType != 1 || g->QuestDone || g->QuestTarget != def->Id) continue;
        c->MonsterQuestsDone++;
        _said.Add(Message(Party->Members.Count() == 1 ? cs::String("You have completed your quest!")
                                                      : "\"" + cs::Str::TrimEnd(c->Name) + "\" completed " + HisHer(c) + " quest!"));
        g->QuestDone = true;
    }
}

void CombatEngine::PayGold() {
    GoldLine = "";
    // monGetGold (MONSMOD.BAS:2144-2178): everyone, the dead included, gets CLng(total / count)
    int64_t total = GoldEarned + GoldStolen();
    GoldEarned = 0;
    if (total > 0)
    {
        int64_t each = GoldShare::Share(total, Party->Members.Count());
        // an overflowing purse leaves the routine (2133, 2184-2187)
        for (auto& m : Party->Members)
        {
            VbCurrency paid{};
            if (VbCurrency::TryAdd(m->GoldOnHand, VbCurrency::FromRaw(each * VbCurrency::Scale), paid)) m->GoldOnHand = paid;
            else return;
        }
    }
    // the selected character's own recovered gold (MONSMOD.BAS:2160, 2168)
    GoldLine = GoldShare::Line(total, GoldRecovered.GetValueOrDefault(Party->Selected()), Party->Members.Count());
    // MONSMOD.BAS:2179-2181
    GoldRecovered.Clear();
}

void CombatEngine::TrapSprung() { ChestTrap = 0; }

void CombatEngine::ChestEmptied() {
    ChestClass = 0; ChestTrap = 0; ChestLocked = 0; GoldEarned = 0;
}

int32_t CombatEngine::ChestRoundsOf(cs::Ref<Character> c) {
    int32_t n{};
    return ChestRounds.TryGetValue(c, n) ? n : -1;
}

cs::Nullable<int32_t> CombatEngine::OpeningRounds(double disarm) {
    if (disarm - 10 <= 0) return {};
    int32_t n = cs::CastDouble<int32_t>(cs::Math::Round(10 - cs::Math::Log(disarm - 10) * 2, cs::MidpointRounding::ToEven));
    return n < 2 ? 2 : n;
}

CombatEngine_CombatLine CombatEngine::ChestCaption() {
    auto me = Party->Selected();
    int32_t n = ChestRoundsOf(me);
    auto l = MakeLine(n > 0 ? "Opening Chest (" + cs::ToStr(n) + " round(s) to go)" : cs::String(""), me);
    l.Channel = LineChannel::CharacterHit;
    return l;
}

cs::List<CombatEngine_CombatLine> CombatEngine::ChestTick(cs::Ref<Character> c) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    int32_t n = ChestRoundsOf(c);
    if (n <= 0)
    {
        if (c == Party->Selected()) { auto l = MakeLine("", c); l.Channel = LineChannel::CharacterHit; _result.Add(l); }
        return _result;
    }
    if (GoldEarned <= 0) { ChestRounds[c] = -1; return _result; }
    n--;
    if (n <= 0)
    {
        ChestRounds[c] = -1;
        auto l = MakeLine("", c); l.OpensChest = true;
        _result.Add(l);
    }
    else ChestRounds[c] = n;
    if (Monsters.Any([&](auto& m) { return m->Alive(); })) _result.Add(ChestCaption());
    return _result;
}

cs::List<CombatEngine_CombatLine> CombatEngine::PlayerSwings(cs::Ref<Character> actor, cs::Nullable<int32_t> targetIndex) {
    cs::List<CombatEngine_CombatLine> _result = cs::List<CombatEngine_CombatLine>::New();
    auto weapons = WeaponsOf(actor);
    // MODULE4.BAS:325-327 - no weapon, or a swing count that comes to nothing
    if (weapons.Count() == 0 || weapons.All([&](auto& w) { return actor->Swings + w->Swings < 1; }))
    {
        Party->OrderFor(actor)->Kind = OrderKind::Defend;
        ItemOrders.Remove(actor);   // sub102F sets M5766 = -1 too (MULTIMOD.BAS:476-479)
        auto l = MakeLine(Caption(actor, "Using no weapon!"), actor); l.Channel = LineChannel::CharacterHit;
        _result.Add(l);
        return _result;
    }

    for (auto& w : weapons)
    {
        int32_t swings = actor->Swings + w->Swings;
        for (int32_t i = 0; i < swings; i++)
        {
            if (Fled || Monsters.All([&](auto& m) { return !m->Alive(); })) return _result;   // timCombat.Enabled = False aborts sub1019
            auto target = TargetFor(targetIndex);
            if (target == nullptr) return _result;

            // MODULE4.BAS:347-363: both `Exit For` - ends this hand only
            if (MonsterProperty::Has(target->Def, MonsterProperty::NeedsGoodWeapon)
                && w->Floor < (int32_t)(target->Def->LevelFound / 2))
            {
                Party->OrderFor(actor)->Kind = OrderKind::Defend;   // sub102F ... gc1DB4
                ItemOrders.Remove(actor);
                auto l = MakeLine(Caption(actor, "Weapon has no effect!"), actor); l.Channel = LineChannel::CharacterHit;
                _result.Add(l);
                break;
            }
            if (MonsterProperty::Has(target->Def, MonsterProperty::ImmuneToWeapons))
            {
                // no order change here
                auto l = MakeLine(Caption(actor, "Weapon has no effect!"), actor); l.Channel = LineChannel::CharacterHit;
                _result.Add(l);
                break;
            }

            auto sw = SwingOnce(actor, target, w);
            auto blow = sw.blow; auto said = sw.said; auto hitSounds = sw.sounds;
            {
                auto l = MakeLine(said, actor);
                l.Paced = true; l.SwingSound = SwingSoundFor(w); l.HitSounds = hitSounds; l.Channel = LineChannel::CharacterHit;
                _result.Add(l);
            }

            // MODULE4.BAS:436-437 and 462-464: only a kill lets the next swing go
            // (MONSMOD.BAS:509-511)
            if (blow == CombatEngine_Blow::Kill) Retarget(actor);
            if (blow != CombatEngine_Blow::Kill) break;
        }
    }
    return _result;
}

cs::Ref<CombatMonster> CombatEngine::TargetFor(cs::Nullable<int32_t> targetIndex) {
    if (targetIndex.has_value())
    {
        int32_t ti = targetIndex.value();
        if (ti >= 0 && ti < Monsters.Count())
        {
            auto aimed = NextInGroup(Monsters[ti]);
            if (aimed != nullptr) return aimed;
        }
    }
    return Monsters.FirstOrDefault([&](auto& m) { return m->Alive(); });
}

cs::String CombatEngine::PlayerAttacks(cs::Ref<Character> actor, cs::Nullable<int32_t> targetIndex) {
    cs::String last = "";
    for (auto& line : PlayerSwings(actor, targetIndex)) last = line.Text;
    return last;
}

cs::List<cs::Ref<Item>> CombatEngine::WeaponsOf(cs::Ref<Character> c) {
    return c->Inventory().Where([&](auto& s) { return s->Equipped; })
        .Select([&](auto& s) { return _item(s->ItemId); }).Where([&](auto& i) { return i != nullptr && i->Type <= 7; })   // M5997 holds anything of a weapon type
        .ToList();
}

Tup_blow_said_sounds_bb3b1 CombatEngine::SwingOnce(cs::Ref<Character> actor, cs::Ref<CombatMonster> target, cs::Ref<Item> weapon) {
    auto fight = WinningGuild(actor, 4);
    double fightMod = fight.mod; int32_t fightLvl = fight.level;
    auto critG = WinningGuild(actor, 2);
    double critMod = critG.mod; int32_t critLvl = critG.level;
    auto backG = WinningGuild(actor, 1);
    double backMod = backG.mod; int32_t backLvl = backG.level;
    int32_t dropLevel = weapon->Floor;
    bool wCrit = (weapon->Abilities & (1 << Effects::CriticalHit)) != 0;
    bool wBack = (weapon->Abilities & (1 << Effects::Backstab)) != 0;
    bool crit = _rng->Next(100) < Rules::CritChance(critLvl, critMod, target->Def->GuildLevel, dropLevel, wCrit);
    bool backstab = _rng->Next(100) < Rules::BackstabChance(backLvl, backMod, target->Def->GuildLevel, dropLevel, wBack);
    // MODULE4.BAS:422-426 - an unseen target takes half
    bool unseen = MonsterProperty::Has(target->Def, MonsterProperty::Invisible) && !actor->SeeingInvisible;
    // the monster's defence is an unassigned local, 0 (MODULE4.BAS:433)
    int32_t dmg = Rules::CharacterDamage(_rng, fightLvl, fightMod, weapon->DamageMod, actor->EffectiveStat(static_cast<int32_t>(Stat::Str), _item),
                                         SizeOf(actor) - target->Def->Size, crit, backstab, AttackOf(actor), unseen);
    if (dmg < 1)
        return Tup_blow_said_sounds_bb3b1{CombatEngine_Blow::Miss, Caption(actor, "You missed!"), cs::List<cs::String>::New()};

    Credit(actor, target, dmg, false, DamageKind::Physical);
    target->set_Hits(target->Hits() - dmg);
    // MODULE4.BAS:441-447
    auto sounds = cs::List<cs::String>::New();
    if (crit && backstab) sounds.Add("RIP"); else if (backstab) sounds.Add("BSTAB"); else if (crit) sounds.Add("CHIT");
    sounds.Add("HIT");

    // MODULE4.BAS:453-460 for a survivor, 481-489 for a kill
    if (target->Alive())
    {
        cs::String hit = crit && backstab ? "You Severed one for " + cs::ToStr(dmg) + " damage!"
                       : backstab ? "You Backstabbed for " + cs::ToStr(dmg) + " damage!"
                       : crit ? "You Critically hit for " + cs::ToStr(dmg) + " damage!"
                       : "You hit for " + cs::ToStr(dmg) + " damage!";
        return Tup_blow_said_sounds_bb3b1{CombatEngine_Blow::Hit, Caption(actor, hit), sounds.ToArray()};
    }

    cs::String kill = crit && backstab ? "You Severed one!"
                    : backstab ? "You Backstabbed one!"
                    : crit ? "You Critically hit one!"
                    : "You Killed One!";
    if (target->Identified && (!MonsterProperty::Has(target->Def, MonsterProperty::Invisible) || actor->SeeingInvisible))
        RecordableKills.Add(Tup_killer_def_3ffae{actor, target->Def});
    KilledBy(actor, target->Def, DamageKind::Physical);   // MONSMOD.BAS:475 and 496
    Sub0D08(target, target);       // 497: the next one steps forward with its hits
    sounds.Add(DeathSound(target->Def));
    return Tup_blow_said_sounds_bb3b1{CombatEngine_Blow::Kill, Caption(actor, kill), sounds.ToArray()};
}

cs::String CombatEngine::Caption(cs::Ref<Character> actor, cs::String text) {
    if (actor == Party->Selected()) Say(text);
    return text;
}

cs::String CombatEngine::SwingSoundFor(cs::Ref<Item> weapon) {
    auto t = weapon->Type;
    return (t == 0 || t == 1 || t == 2 || t == 4 || t == 5 || t == 7) ? "SWING" : "SLICE";
}

cs::String CombatEngine::DeathSound(cs::Ref<Monster> def) {
    auto k = def->Kind();
    if (k == MonsterKind::Humanoid || k == MonsterKind::Thief || k == MonsterKind::Mage || k == MonsterKind::Warrior) return "DI-HUMAN";
    if (k == MonsterKind::Demon || k == MonsterKind::Dragon || k == MonsterKind::Giant) return "DI-DEMON";
    if (k == MonsterKind::Devil || k == MonsterKind::Animal || k == MonsterKind::Mythical || k == MonsterKind::Lycanthrope) return def->PicId == 18 ? "BIRDDIE" : "DI-ANIM";
    if (k == MonsterKind::Insect) return "BUGDIE";
    if (k == MonsterKind::Undead) return "DI-UNDEA";
    return "KILL";
}

}  // namespace mc
