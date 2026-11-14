#include "core.h"

namespace mc {

int32_t Rules::StatsAD(cs::List<int32_t> s) {
    auto Term = [](int32_t v, int32_t lo, int32_t hi, int32_t w) { return v > hi ? w * (v - hi) : v < lo ? w * (v - lo) : 0; };
    return Term(s[static_cast<int32_t>(Stat::Str)], 8, 12, 2) + Term(s[static_cast<int32_t>(Stat::Dex)], 8, 12, 2) +
           Term(s[static_cast<int32_t>(Stat::Int)], 8, 14, 1) + Term(s[static_cast<int32_t>(Stat::Wis)], 8, 14, 1);
}

int32_t Rules::AdInterval(cs::Ref<GuildDef> g, int32_t level, bool isNomad) {
    float acc;
    if (level > g->HitsCapLevel) {
        acc = g->HitsCapLevel;
        double bonus = isNomad ? 3 : 0;
        for (int32_t i = g->HitsCapLevel; i <= level; i++) {
            // l0434 is declared Integer, so the step is rounded before it is added
            int32_t step = cs::CastDouble<int32_t>(cs::Math::Round(((int32_t)g->HitsCapLevel / 2.0 + acc / 18.0f) / (g->AdEvery + bonus),
                                                                   cs::MidpointRounding::ToEven));
            acc += step;
            if (acc > level) break;
        }
        if (cs::Math::Abs(acc - g->HitsCapLevel) < 1e-9) acc = level + 1;
    } else
        acc = 0;
    if (level > g->AdMaxLevel) acc = (float)(acc * 1.1);  // 1.1# is a Double literal in the source
    return cs::CastDouble<int32_t>(cs::Math::Round((double)acc));
}

Tup_att_def_ac50a Rules::GuildAD(cs::String guild, int32_t level) {
    auto t = GuildADSingle(guild, level);
    auto fa = t.att;
    auto fd = t.def;
    return Tup_att_def_ac50a{cs::CastDouble<int32_t>(fa), cs::CastDouble<int32_t>(fd)};
}

Tup_att_def_2ad65 Rules::GuildADSingle(cs::String guild, int32_t level) {
    auto g = GuildData::All.FirstOrDefault([&](auto& x) { return x->Name == guild; });
    if (g == nullptr || g->AttPerLevel <= 0) return Tup_att_def_2ad65{0.0f, 0.0f};
    bool isNomad = GuildData::All.Count() > 0 && GuildData::All[0]->Name == guild;

    float att = 0, def = 0;
    int32_t cached = 0;  // the character's M59A1
    int32_t every = cs::Math::Max(1, cs::CastDouble<int32_t>(g->AdEvery));
    for (int32_t l = 1; l <= level && l <= g->AdMaxLevel; l++) {
        int32_t r;
        if (l <= g->HitsCapLevel)
            r = l % every;
        else {
            if (cached < l) cached = AdInterval(g, l, isNomad);
            r = cached <= 0 ? 0 : l % cached;
        }
        if (r == 0) {
            att += g->AttPerLevel;
            def += g->DefPerLevel;
            if (l >= g->HitsCapLevel) cached = AdInterval(g, l, isNomad);
        }
        cached = AdInterval(g, l, isNomad);  // recomputed every level, as FRMCTYG does
    }
    // Single, and truncated - and it has to be both (FRMCTYG.BAS:1019)
    return Tup_att_def_2ad65{att, def};
}

Tup_att_def_ac50a Rules::BestGuildAD(cs::Ref<Character> c) {
    int32_t a = 0, d = 0;
    for (auto& m : c->Guilds) {
        auto t = GuildAD(m->Guild, m->Level);
        auto ga = t.att;
        auto gd = t.def;
        a = cs::Math::Max(a, ga);
        d = cs::Math::Max(d, gd);
    }
    return Tup_att_def_ac50a{a, d};
}

double Rules::HitThreshold(int32_t atk, int32_t def) {
    double p = atk == 0 && def == 0 ? 0.5
             : atk > def ? 0.5 + 0.5 * (atk - def) / (double)atk
             : def == 0 ? 0.5 + 0.5 * atk
             : 0.5 - 0.5 * (def - atk) / (double)def;
    p *= 100;
    if (p > 98) p = 98;
    if (p < 2) p = 2;
    return p;
}

int32_t Rules::PitDamage(cs::Ref<cs::Random> rng, int32_t dungeonLevel, int32_t raceSize) {
    // l0270 is an Integer: rounded (MAZEMOD.BAS:1512)
    return cs::CastDouble<int32_t>(cs::Math::Round(cs::Math::Floor(rng->NextDouble() * (((dungeonLevel + 1) / 2.0) * 15.5)) * cs::Math::Log(raceSize + 2) + 1,
                                                   cs::MidpointRounding::ToEven));
}

int32_t Rules::ChuteDamage(cs::Ref<cs::Random> rng, int32_t dungeonLevel, int32_t raceSize) {
    return cs::CastDouble<int32_t>(cs::Math::Round(cs::Math::Floor(rng->NextDouble() * (dungeonLevel * 10.5)) * cs::Math::Log(raceSize + 2) + 5,
                                                   cs::MidpointRounding::ToEven));
}

bool Rules::HoversOverPit(cs::Ref<cs::Random> rng, int32_t dungeonLevel) {
    return 5 + dungeonLevel / 4.0 < rng->NextDouble() * 100;
}

int32_t Rules::EngageChance(Alignment monster, Alignment character, int32_t charisma) {
    int32_t b;
    if (monster == Alignment::Good)
        b = character == Alignment::Neutral ? 15 : character == Alignment::Evil ? 100 : 0;
    else if (monster == Alignment::Neutral)
        b = character == Alignment::Neutral ? 15 : 25;
    else
        b = character == Alignment::Evil ? 25 : 35;
    return cs::CastDouble<int32_t>(b - 7.5 * cs::Math::Log(cs::Math::Max(1, charisma)));
}

bool Rules::MonstersEngage(cs::Ref<cs::Random> rng, Alignment monster, Alignment character, int32_t charisma) {
    return rng->Next(100) + 1 < EngageChance(monster, character, charisma);
}

double Rules::IdentifySkill(int32_t topGuildLevel, int32_t intelligence, int32_t wisdom) {
    if (topGuildLevel <= 0) return 0;
    return (intelligence + wisdom) / 2.0 * cs::Math::Log((topGuildLevel + 5) / 10.0 + 1);
}

int32_t Rules::IdentifyChance(double skill, bool dark) {
    return cs::Math::Clamp(cs::CastDouble<int32_t>((dark ? skill * 0.75 : skill) + 20), 10, 90);
}

int32_t Rules::RecogniseChance(double skill, bool dark) {
    return cs::Math::Clamp(cs::CastDouble<int32_t>((dark ? skill : skill * 1.25) + 20), 5, 80);
}

bool Rules::QuicksandTakesSomething(cs::Ref<cs::Random> rng, int32_t dungeonLevel) {
    return rng->Next(100) < dungeonLevel / 25.0 * 100;
}

int32_t Rules::Fn0DEE(cs::Ref<cs::Random> rng, int32_t atk, int32_t def, int32_t level, int32_t strength, double damMod) {
    double threshold = HitThreshold(atk, def);
    if (damMod <= 0) damMod = 0.1;
    int32_t str = strength < 1 ? 1 : strength;
    // CInt rounds half to even; Int() floors
    str = cs::CastDouble<int32_t>(cs::Math::Round(str - cs::Math::Log(str) * ((cs::Math::Log(str) - 2.4) * 2.25), cs::MidpointRounding::ToEven)) + 1;
    if (damMod > 1) damMod -= cs::Math::Log(damMod) * cs::Math::Log(damMod);
    double lvlTerm = (cs::Math::Log(level + 5) + 1) / cs::Math::Log(1.2);
    double strTerm = str / 30.0;
    double roll = cs::Math::Floor(rng->NextDouble() * ((str + 10) / 2.0) + (str + 10) / 4.0) / 10;
    int32_t dmg = cs::CastDouble<int32_t>(cs::Math::Floor(lvlTerm * strTerm * roll * damMod));
    int32_t adj = cs::CastDouble<int32_t>(cs::Math::Floor(((def - atk) / 2.0) / 4.0));
    adj = adj - cs::CastDouble<int32_t>(cs::Math::Floor(rng->NextDouble() * (adj + 1))) + 1;
    if (cs::CastDouble<int32_t>(rng->NextDouble() * 1000) + 1 > 1) dmg -= adj;
    if (dmg < 1) dmg = 0;
    if (cs::CastDouble<int32_t>(rng->NextDouble() * 100) + 1 >= threshold) dmg = 0;
    return dmg;
}

int32_t Rules::CharacterDamage(cs::Ref<cs::Random> rng, int32_t guildLevel, double fightingMod, double weaponMod, int32_t strength, int32_t sizeDiff, bool crit, bool backstab, int32_t atk, bool invisibleTarget, int32_t def) {
    double damMod = FightingBase + ((cs::Math::Log(100 + guildLevel) / 1.75 - 2.3) * fightingMod * fightingMod / 2);
    damMod *= weaponMod <= 0 ? 1 : weaponMod;
    if (crit) damMod += 5;
    if (backstab) damMod += 2;
    if (invisibleTarget) damMod -= damMod / 2;
    if (sizeDiff < 0)
        damMod -= -sizeDiff / 10.0;
    else if (sizeDiff > 0)
        damMod += sizeDiff / 10.0;
    return Fn0DEE(rng, atk, def, guildLevel, strength, damMod);
}

int32_t Rules::MonsterBlow(cs::Ref<cs::Random> rng, int32_t monAtt, int32_t targetDef, int32_t monGuildLevel, int32_t monStr, double damMod) {
    return Fn0DEE(rng, monAtt, targetDef, monGuildLevel, monStr, damMod);
}

int32_t Rules::MonsterDamage(cs::Ref<cs::Random> rng, cs::Ref<Monster> m, int32_t sizeDiff, bool crit, bool backstab, int32_t charDef, bool openingChest) {
    double damMod = m->DamageMod <= 0 ? 1 : m->DamageMod;
    if (openingChest) damMod += 0.75;
    if (crit) damMod += 2;
    if (backstab) damMod += 1;
    if (sizeDiff > 0)
        damMod += sizeDiff / 10.0;
    else if (sizeDiff < 0)
        damMod -= -sizeDiff / 10.0;
    int32_t str = m->Stats.Length() > 0 ? m->Stats[0] : 0;
    return Fn0DEE(rng, m->Att, charDef, m->GuildLevel, str, damMod);
}

double Rules::MonsterCritChance(int32_t attackerGuildLevel, int32_t attackerFloor, int32_t defenderLevel, bool backstab) {
    double p = (cs::Math::Log(attackerGuildLevel / (backstab ? 500.0 : 650.0) + 1) * 100) * cs::Math::Log(attackerFloor + 1)
             - cs::Math::Log(defenderLevel / 999.0 + 1) * 100;
    double floor = backstab ? 4 : 2, cap = backstab ? 75 : 50;
    if (p < floor) p = floor;
    if (p > cap) p = cap;
    return p;
}

bool Rules::ParalysisPasses(cs::Ref<cs::Random> rng, int32_t paralysisLevel, int32_t guildLevel, int32_t con, int32_t resistPct, bool monstersPresent) {
    if (!monstersPresent) return true;
    double p = cs::Math::Max(1, paralysisLevel);
    double hold = cs::Math::Log(p / 2 + p / 10) / cs::Math::Log(1.02);
    double lvl = cs::Math::Max(1, guildLevel);
    double fight = ((cs::Math::Log(lvl) + lvl / 10) / cs::Math::Log(1.05) / 10 + con * 1.5) + resistPct;
    double chance = hold - fight;
    if (chance < 5) chance = 5;
    if (chance > 90) chance = 90;
    return cs::CastDouble<int32_t>(rng->NextDouble() * 100) + 1 > chance;
}

double Rules::CritChance(int32_t guildLevel, double critMod, int32_t monLevel, int32_t weaponFloor, bool weaponCrits) {
    return SpecialHitChance(guildLevel, critMod, monLevel, weaponFloor, weaponCrits, 3, 10, 1, 50);
}

double Rules::BackstabChance(int32_t guildLevel, double bsMod, int32_t monLevel, int32_t weaponFloor, bool weaponBackstabs) {
    return SpecialHitChance(guildLevel, bsMod, monLevel, weaponFloor, weaponBackstabs, 2, 100, 2, 75);
}

double Rules::SpecialHitChance(int32_t guildLevel, double mod, int32_t monLevel, int32_t weaponFloor,
                               bool weaponHasIt, double abilityDivisor, double weaponLog,
                               double floor, double cap) {
    double p = ((cs::Math::Log(guildLevel / 999.0 + 1) * 100) * cs::Math::Log(mod + 1)) / abilityDivisor;
    p -= cs::Math::Log(monLevel / 999.0 + 1) * 100;
    p /= 2;

    if (weaponHasIt) {
        double fromWeapon = cs::Math::Log(weaponFloor + 1) * cs::Math::Log(weaponLog);
        p = cs::Math::Max(p + fromWeapon, fromWeapon);
    }
    // the floor only applies to someone who can do it at all - by guild level or by weapon
    if (p < floor && (guildLevel > 0 || weaponHasIt)) p = floor;
    if (p > cap) p = cap;
    // MODULE4.BAS:394 compares Int(Rnd * 100) against this Single without rounding it
    return p;
}

int64_t Rules::ChestExp(double disarmChance, int32_t monsterGuildLevel, int32_t firstCharacterGuildLevel, int32_t partySize) {
    double xp = disarmChance * (monsterGuildLevel / 2.0) / (1 + firstCharacterGuildLevel / 333.0);
    double party = 1 - (partySize - 1) * 0.04;
    return cs::CastDouble<int64_t>(cs::Math::Round(party * cs::Math::Round(xp, cs::MidpointRounding::ToEven), cs::MidpointRounding::ToEven));
}

int64_t Rules::ExpAwarded(int32_t hitsInflicted, int32_t monGuildLevel, int32_t currentGuildLevel, bool byCompanion, int32_t partySize) {
    if (hitsInflicted <= 0) return 0;
    double h = hitsInflicted;
    double xp = h * (9 - cs::Math::Log(h + 1)) * (20 - cs::Math::Log(h)) * (monGuildLevel + 50) * (byCompanion ? 0.25 : 1.0) * (1.0 / (1 + currentGuildLevel / 333.0)) / 1000;
    double party = 1 - (partySize - 1) * 0.04;
    return cs::CastDouble<int64_t>(cs::Math::Round(party * cs::Math::Round(xp, cs::MidpointRounding::ToEven), cs::MidpointRounding::ToEven));
}

int64_t Rules::Fn12A1(int32_t level, float guildExp, float raceExp) {
    auto Cur = [](double x) { return cs::CastDouble<int64_t>(cs::Math::Round(x * 10000.0, cs::MidpointRounding::ToEven)); };  // ten-thousandths
    int64_t l0392 = 0;
    float l038A = guildExp;
    float l038C = raceExp + 1;
    for (int32_t l0386 = 1; l0386 <= level; l0386++) {
        int64_t l038E = Cur((float)(l0386 + l0386) * l038A);     // Integer * Single is a Single
        int64_t l0390 = Cur((float)l0386 / 2 * 2 + (l0386 - 1));  // l0386 / 2: Integer / Integer is a Single
        // (l038E@ / 12) * 500 * l0386 is a Double; 344 * l0390@ stays Currency until the sum
        l0392 = Cur((l038E / 10000.0 / 12 * 500 * l0386 + 344 * l0390 / 10000.0 - 2) / 10);
    }
    // the line is one typed expression the x87 evaluates without narrowing (see Fn1485Price)
    double f = ((cs::Math::Log(l038C) / cs::Math::Log((float)(l038C + 1))) + 1) - .7 + l038C / 25.0;
    l0392 = Cur(l0392 / 10000.0 * f);
    return cs::Decimal::Round(cs::Decimal(l0392) / cs::Decimal(10000), 0, cs::MidpointRounding::ToEven).ToInt64();  // fn12A1 = l0392@
}

int64_t Rules::Fn13ED(int32_t level, float guildExp, float raceExp) {
    return Fn12A1(level + 1, guildExp, raceExp) - 1;
}

int64_t Rules::Fn12B2(int32_t level, int32_t guildGold) {
    int16_t l039E = cs::Wrap<int16_t>(level);  // Integer
    auto L = X87::Of((int64_t)l039E);
    auto halfL = L / X87::Of((int64_t)2);  // l039E / 2, unnarrowed
    auto rate = X87::Of(60.2);             // 60.2#
    int64_t l03A8 = 0;
    for (int32_t l039C = 1; l039C <= l039E; l039C++) {
        int64_t l03A6 = cs::CastDouble<int64_t>(cs::Math::Round((double)l039C / 2 * 2 + (l039C - 1), cs::MidpointRounding::ToEven));
        // l03A8& = l03A8& + (((l039E / 2) * l03A6&) * 60.2#) / l039E
        l03A8 = (X87::Of(l03A8) + halfL * X87::Of(l03A6) * rate / L).RoundToLong();
    }
    // fn12B2 = l03A8& + (l03A8& * (Gold / 10))
    auto acc = X87::Of(l03A8);
    return (acc + acc * (X87::Of((int64_t)cs::Wrap<int16_t>(guildGold)) / X87::Of((int64_t)10))).RoundToLong();
}

int64_t Rules::GuildJoinCost(cs::Ref<GuildDef> g) {
    double sum = 0;
    // abilities 0 to 5 (mc006A = 6) and spell classes 0 to 17 (mc0074 = 18), FRMCTYG.BAS:116-125
    for (auto& a : g->Abilities.Take(6))
        if (a > 0) sum += cs::Math::Round((a + 1) * 1.25, 4);
    for (auto& m : g->SpellMax.Take(18))
        if (m > 0) sum += cs::Math::Round(m / 3.0, 4);
    return cs::CastDouble<int64_t>(cs::Math::Round(sum * sum * 5356));
}

int32_t Rules::SpellLevel(int32_t guildLevel) {
    return cs::Math::Clamp(cs::CastDouble<int32_t>(guildLevel / 2.0 + 0.5), 1, 255);
}

int32_t Rules::Fn1485Price(int32_t level, int32_t u4, float rate, int32_t sl, int32_t& rated) {
    float r = rate == 0 ? 1.0f : rate;  // 2886-2889
    rated = cs::CastDouble<int32_t>(cs::Math::Round((double)level * r, cs::MidpointRounding::ToEven));  // 2890: exact, then the Integer store
    // the Single rate as mant * 2^exp, exactly
    int32_t bits = cs::BitConverter::SingleToInt32Bits(r);
    int32_t e = (bits >> 23) & 0xFF;
    int64_t mant = bits & 0x7FFFFF;
    if (e != 0)
        mant |= 0x800000;
    else
        e = 1;
    if (bits < 0) mant = -mant;
    int32_t exp = e - 127 - 23;
    auto num = cs::BigInteger((long long)rated) * cs::BigInteger((long long)u4) * cs::BigInteger((long long)mant) * cs::BigInteger(19LL);
    auto den = cs::BigInteger(10LL) * cs::BigInteger((long long)cs::Math::Max(1, sl));
    if (exp >= 0)
        num = num << exp;
    else
        den = den << -exp;
    cs::BigInteger rem;
    auto q = cs::BigInteger::DivRem(num, den, rem);
    if (rem.Sign() < 0)
        q = q - cs::BigInteger(1LL);  // a floor, not a truncation
    else if (rem.IsZero() && num.Sign() > 0)
        q = q - cs::BigInteger(1LL);  // whole: the true product is just under it
    return (int32_t)(long long)q;
}

int32_t Rules::SpellDamage(cs::Ref<cs::Random> rng, int32_t baseDmg, int32_t power, int32_t sl, int32_t resistPct) {
    sl = cs::Math::Max(1, sl);
    double mod1 = baseDmg + (baseDmg * cs::Math::Log(sl + 1) / cs::Math::Log(5)) + cs::CastDouble<int32_t>((sl / 10.0) * (power - 1));
    int32_t mod2 = cs::CastDouble<int32_t>(rng->NextDouble() * (mod1 - ((sl - 1) / (double)(sl * sl))) / 2) + 1;
    int32_t dmg = cs::CastDouble<int32_t>(mod1 - mod2);
    // monGetSpellHits (MONSMOD.BAS:2464-2533) holds the damage in an Integer: the resist step rounds
    return cs::Math::Max(0, cs::CastDouble<int32_t>(cs::Math::Round(dmg * (1 - cs::Math::Clamp(resistPct, 0, 100) / 100.0),
                                                                    cs::MidpointRounding::ToEven)));
}

int32_t Rules::SpellHeal(cs::Ref<cs::Random> rng, int32_t baseHeal, int32_t power, int32_t sl) {
    sl = cs::Math::Max(1, sl);
    double mod1 = baseHeal + (baseHeal * cs::Math::Log(sl) / cs::Math::Log(10)) + cs::CastDouble<int32_t>((sl / 10.0) * (power - 1));
    int32_t mod2 = cs::CastDouble<int32_t>(rng->NextDouble() * (mod1 - ((sl - 1) / (double)(sl * sl))) / 2) + 1;
    return cs::Math::Max(1, cs::CastDouble<int32_t>(mod1 - mod2));
}

double Rules::SpellHealAverage(int32_t baseHeal, int32_t power, int32_t sl) {
    sl = cs::Math::Max(1, sl);
    double mod1 = baseHeal + (baseHeal * cs::Math::Log(sl) / cs::Math::Log(10)) + cs::CastDouble<int32_t>((sl / 10.0) * (power - 1));
    double h = (mod1 - ((sl - 1) / (double)(sl * sl))) / 2;
    // only a guard: a heal always has a base above nought
    if (!(h > 0)) return cs::Math::Max(1, cs::CastDouble<int32_t>(mod1 - 1));
    double sum = 0;
    for (int32_t k = 0; k < h; k++)
        sum += (cs::Math::Min(k + 1, h) - k) / h * cs::Math::Max(1, cs::CastDouble<int32_t>(mod1 - (k + 1)));
    return sum;
}

int32_t Rules::EncounterSpread(int32_t level, cs::Ref<cs::Random> rng, bool onStud) {
    int32_t roll = rng->Next(100) + 1;
    int32_t spread = cs::CastDouble<int32_t>(cs::Math::Round((roll < 50 ? 1 : 0) + level / 5.0, cs::MidpointRounding::ToEven));
    // on a stud square, the whole thing is usually replaced by one (MODULE4.BAS:1491)
    if (onStud && rng->NextDouble() * 75 < 50) spread = 1;
    return cs::Math::Min(spread, level);
}

int32_t Rules::EncounterFloor(int32_t level, bool onStud) {
    return onStud ? level + 1 : level;
}

int32_t Rules::AbilityRatingExact(cs::Ref<Character> c, int32_t ability, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    double value = BestAbilityValue(c, ability, gear);
    double lo = AbilityFloor(ability), hi = AbilityCeiling(ability);
    if (hi <= lo) return 1;
    int32_t r = cs::CastDouble<int32_t>(cs::Math::Round((value - lo) / (hi - lo) * 100, cs::MidpointRounding::ToEven));
    return r <= 1 ? 1 : r;
}

cs::String Rules::BestAbilityGuild(cs::Ref<Character> c, int32_t ability, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    auto t = AbilityStats(c, ability, gear);
    auto wis = t.wis;
    auto dex = t.dex;
    double bestChance = -std::numeric_limits<double>::infinity();
    cs::String best = "";
    for (auto& g : GuildData::All) {
        auto m = c->Guilds.FirstOrDefault([&](auto& x) { return x->Guild == g->Name; });
        if (m == nullptr || m->Level <= 0) continue;
        double gm = g->Abilities.Length() > ability ? g->Abilities[ability] : 0;
        double chance = AbilityChance(ability, gm, m->Level, wis, dex, 0, 2);
        if (chance > bestChance) {
            bestChance = chance;
            best = g->Name;
        }
    }
    return best;
}

double Rules::BestAbilityValue(cs::Ref<Character> c, int32_t ability, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    // fn122A is called with its third argument True, which turns the opposition off
    auto t = AbilityStats(c, ability, gear);
    auto wis = t.wis;
    auto dex = t.dex;
    double bestChance = -std::numeric_limits<double>::infinity(), mod = 0;
    int32_t level = 1;
    for (auto& g : GuildData::All) {  // table order: fn122A's ties go to the lower index
        auto m = c->Guilds.FirstOrDefault([&](auto& x) { return x->Guild == g->Name; });
        if (m == nullptr || m->Level <= 0) continue;
        double gm = g->Abilities.Length() > ability ? g->Abilities[ability] : 0;
        double chance = AbilityChance(ability, gm, m->Level, wis, dex, 0, 2);
        if (chance > bestChance) {
            bestChance = chance;
            mod = gm;
            level = m->Level;
        }
    }
    if (std::isinf(bestChance) && bestChance < 0) return 0;

    double lvl = cs::Math::Max(1, level);
    switch (ability) {
        case 0: {
            double stat = wis / 4.0 + dex / 2.0;
            double against = (cs::Math::Log(2) * 4) * (cs::Math::Log(2) * 1.25) * 1.25;
            return (stat + (4.5 + cs::Math::Exp(cs::Math::Log(lvl + 1) / cs::Math::Log(15))) * (mod / 10 * 14)) - against * 1.5;
        }
        case 4:
            return FightingBase + (((cs::Math::Log(100 + lvl) / 1.75) - 2.3) * (mod * mod)) / 2;
        case 5:
            return (mod + cs::Math::Log(lvl + 1) / cs::Math::Log(1.25)) * (mod * 0.3) + wis;
        default:
            return ((cs::Math::Log(lvl / 999.0 + 1) * 100) * cs::Math::Log(mod + 1)) / 3;
    }
}

double Rules::AbilityFloor(int32_t ability) {
    double mod = LowestGuildModifier(ability);
    switch (ability) {
        case 0: {
            // note the hardcoded 5 here: the floor does not use the lowest guild's modifier
            double against = (cs::Math::Log(2) * 4) * (cs::Math::Log(2) * 1.25) * 1.25;
            return (2 / 4.0 + 2 / 2.0 + (4.5 + cs::Math::Exp(cs::Math::Log(2) / cs::Math::Log(15))) * (5 / 10.0 * 14)) - against * 1.5;
        }
        case 4:
            return FightingBase + (((cs::Math::Log(101) / 1.75) - 2.3) * (mod * mod)) / 2;
        case 5:
            return (mod + cs::Math::Log(2) / cs::Math::Log(1.25)) * (mod * 0.3) + 8 / 4.0;
        default:
            return ((cs::Math::Log(1 / 999.0 + 1) * 100) * cs::Math::Log(mod + 1)) / 3;
    }
}

double Rules::AbilityCeiling(int32_t ability) {
    constexpr int32_t Top = 999;
    double mod = HighestGuildModifier(ability);
    switch (ability) {
        case 0: {
            // and a hardcoded 9 here, with stats of 30 and 40 rather than any real maximum
            double against = (cs::Math::Log(2) * 4) * (cs::Math::Log(2) * 1.25) * 1.25;
            return (30 / 4.0 + 40 / 2.0 + (4.5 + cs::Math::Exp(cs::Math::Log(Top + 1) / cs::Math::Log(15))) * (9 / 10.0 * 14)) - against * 1.5;
        }
        case 4:
            return FightingBase + (((cs::Math::Log(100 + Top) / 1.75) - 2.3) * (mod * mod)) / 2;
        case 5:
            return (mod + cs::Math::Log(Top) / cs::Math::Log(1.25)) * (mod * 0.3) + 60 / 4.0;
        default:
            return ((cs::Math::Log(Top / 999.0 + 1) * 100) * cs::Math::Log(mod + 1)) / 3;
    }
}

double Rules::LowestGuildModifier(int32_t ability) {
    double lowest = 0;
    for (auto& g : GuildData::All)
        if (g->Abilities.Length() > ability && g->Abilities[ability] < lowest) lowest = g->Abilities[ability];
    double first = GuildData::All.Count() > 0 && GuildData::All[0]->Abilities.Length() > ability ? GuildData::All[0]->Abilities[ability] : 0;
    return first > lowest ? first : lowest;
}

double Rules::HighestGuildModifier(int32_t ability) {
    double highest = 0;
    for (auto& g : GuildData::All)
        if (g->Abilities.Length() > ability && g->Abilities[ability] > highest) highest = g->Abilities[ability];
    return highest;
}

IdLevel Rules::NextIdLevel(IdLevel now) {
    if (now == IdLevel::Barely) return IdLevel::Partial;
    if (now == IdLevel::Partial) return IdLevel::Mostly;
    return IdLevel::All;
}

int32_t Rules::RaiseConLossChance(int32_t con) {
    return cs::Math::Max(0, 50 - con * 2);
}

int32_t Rules::ChestClass(cs::Ref<Monster> m, double roll0To99, bool isLairMonster) {
    int32_t bonus = isLairMonster && m->BoxChance.Length() > 2 && m->BoxChance[2] > 0 ? 25 : 0;
    int32_t cls = 0;
    for (int32_t i = 1; i <= 2 && i < m->BoxChance.Length(); i++)
        if (roll0To99 < m->BoxChance[i] + bonus) cls = i + 1;
    return cls;
}

double Rules::GroupGold(cs::Ref<Monster> m, int32_t count, cs::Ref<cs::Random> rng) {
    double g = cs::Math::Max(0, m->GoldFactor);
    double a = (g * cs::Math::Pow(10, g - 0.5)) / 4;
    a = (a / 4 + rng->NextDouble() * (a / 2) + rng->NextDouble() * (a / 2))
        * cs::Math::Log((m->LevelFound + 1) / cs::Math::Log(2));
    if (a < 500) a = cs::CastDouble<int32_t>(rng->NextDouble() * 500) + 2;
    double n = cs::Math::Log(count + 1);
    return a * (n * (n / 2));
}

int64_t Rules::EncounterGold(cs::List<cs::Ref<Monster>> kinds, cs::List<int32_t> counts, cs::Ref<cs::Random> rng, int32_t chestClass, bool chestLocked) {
    if (kinds.Count() == 0) return 0;
    double gold = 0;
    for (int32_t i = 0; i < kinds.Count(); i++) gold += GroupGold(kinds[i], counts[i], rng);

    int32_t total = 0;
    for (int32_t i = 0; i < counts.Count(); i++) total += counts[i];
    gold *= total / 2.0;

    // the class and the lock are decided once, by the caller (MONSMOD.BAS:1692-1697)
    if (chestLocked) gold *= 1.5;
    double mult;
    if (chestClass == 0 || chestClass == 1)
        mult = 0.5;
    else if (chestClass == 2)
        mult = 1.0;
    else if (chestClass == 3)
        mult = 1.5;
    else
        mult = 1.0;
    gold *= mult;
    return cs::CastDouble<int64_t>(cs::Math::Max(0.0, gold));
}

bool Rules::DiesOfOldAge(cs::Ref<cs::Random> rng, int32_t ageDays, int32_t raceLifespanYears) {
    if (raceLifespanYears <= 0) return false;
    double pct = (ageDays / 365.0) / raceLifespanYears * 100.0;
    if (pct < 98) return false;
    return rng->Next(100) < 5 + cs::Math::Log(cs::Math::Max(pct - 98, 1e-9));
}

Tup_wis_dex_3bce3 Rules::AbilityStats(cs::Ref<Character> c, int32_t ability, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    int32_t natWis = c->Stats[static_cast<int32_t>(Stat::Wis)], natDex = c->Stats[static_cast<int32_t>(Stat::Dex)];
    if (!gear) return Tup_wis_dex_3bce3{(double)natWis, (double)natDex};
    // Stats + StatMods with no floor: StatMods is never clamped (MAINMOD2.BAS:1769-1774)
    int32_t effWis = c->StatWithMods(static_cast<int32_t>(Stat::Wis), gear), effDex = c->StatWithMods(static_cast<int32_t>(Stat::Dex), gear);
    return ability == 5 ? Tup_wis_dex_3bce3{natWis + (effWis - natWis) / 4.0, (double)effDex}
                        : Tup_wis_dex_3bce3{(double)effWis, (double)effDex};
}

double Rules::AbilityChance(int32_t ability, double guildRating, int32_t guildLevel,
                            double wis, double dex, int32_t dungeonLevel, int32_t monsterGuildLevel) {
    double lvl = cs::Math::Max(1, guildLevel);
    switch (ability) {
        case 0: {  // thieving - disarming, and picking what is locked
            double stat = wis / 4.0 + dex / 2.0;
            // in town there is nothing to oppose you: dungeonLevel is 0, so Log(0 + 1) is 0
            double against = (cs::Math::Log(cs::Math::Max(2, monsterGuildLevel)) * 4)
                           * (cs::Math::Log(dungeonLevel + 1) * 1.25) * 1.25;
            double r = (stat + (4.5 + cs::Math::Exp(cs::Math::Log(lvl + 1) / cs::Math::Log(15))) * (guildRating / 10 * 14))
                     - against * 1.5;
            return dungeonLevel > 0 ? cs::Math::Clamp(r, 5.0, 98.0) : r;
        }
        case 4:  // fighting. mc0080 is 0.6, not zero - MODULE6.BAS:8
            return FightingBase + (((cs::Math::Log(100 + lvl) / 1.75) - 2.3) * (guildRating * guildRating)) / 2;
        case 5: {  // perception
            // the original's precedence: WIS + gearWIS/4, not (WIS + gearWIS)/4
            double stat = wis;
            double climb = cs::Math::Log(lvl) / cs::Math::Log(1.25);
            double r = (guildRating + climb) * (guildRating * 0.3) + stat;
            return cs::Math::Clamp(r - dungeonLevel * 2, 5.0, 90.0);
        }
        default:  // backstab, critical hit, extra swings
            return guildRating * lvl;
    }
}

Tup_Chance_Guild_Modifier_4c9fc Rules::AbilityLead(cs::Ref<Character> c, int32_t ability, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    auto t = AbilityStats(c, ability, gear);
    auto wis = t.wis;
    auto dex = t.dex;
    double best = 0, mod = 0;
    cs::String guild = "";
    for (auto& g : GuildData::All) {
        auto m = c->Guilds.FirstOrDefault([&](auto& x) { return x->Guild == g->Name; });
        if (m == nullptr || m->Level <= 0) continue;
        double gm = g->Abilities.Length() > ability ? g->Abilities[ability] : 0;
        double r = ability == 0
            ? AbilityChance(0, gm, m->Level, wis, dex, 0, 2)
              - (cs::Math::Log(2) * 4) * (cs::Math::Log(c->DungeonLevel + 1) * 1.25) * 1.25 * 1.5
            : AbilityChance(ability, gm, m->Level, wis, dex, c->DungeonLevel, 2);
        if (r > best) {
            best = r;
            mod = gm;
            guild = g->Name;
        }
    }
    return Tup_Chance_Guild_Modifier_4c9fc{best, guild, mod};
}

double Rules::BestAbilityChance(cs::Ref<Character> c, int32_t ability, int32_t dungeonLevel, int32_t monsterGuildLevel, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    auto t = AbilityStats(c, ability, gear);
    auto wis = t.wis;
    auto dex = t.dex;
    double best = 0;
    for (auto& g : GuildData::All) {
        auto m = c->Guilds.FirstOrDefault([&](auto& x) { return x->Guild == g->Name; });
        if (m == nullptr || m->Level <= 0) continue;
        double r = AbilityChance(ability, g->Abilities.Length() > ability ? g->Abilities[ability] : 0,
                                 m->Level, wis, dex, dungeonLevel, monsterGuildLevel);
        if (r > best) best = r;
    }
    return best;
}

int32_t Rules::ChestTrap(cs::Ref<Monster> m, int32_t chestClass, cs::Ref<cs::Random> rng) {
    if (chestClass <= 0 || m->TrapFlags == 0) return 0;
    int32_t chance;
    if (chestClass == 1)
        chance = 25;
    else if (chestClass == 2)
        chance = 75;
    else if (chestClass == 3)
        chance = 90;
    else
        chance = 0;
    if (rng->Next(100) + 1 >= chance) return 0;
    for (int32_t guard = 0; guard < 200; guard++) {
        int32_t t = rng->Next(12);
        if ((m->TrapFlags & (1 << t)) != 0) return t;
    }
    return 0;
}

int32_t Rules::MorgueRaiseChance(int32_t con) {
    return cs::Math::Clamp(cs::CastDouble<int32_t>(cs::Math::Round(35 * (con / 4.5) + 40, cs::MidpointRounding::ToEven)), 10, 98);
}

cs::String Rules::RaiseRefusal(int32_t causeOfDeath, int32_t effect) {
    constexpr int32_t restoreFlesh = 21, resurrect = 22;
    switch (causeOfDeath) {
        case DeathStoned:
            return effect != restoreFlesh && effect > -1 ? cs::String("Character is stoned!  This Spell will not work!") : cs::String();
        case DeathRock:
            if (effect == -1) return "The morgue cannot help characters pulled from Rock.";
            return effect != resurrect ? cs::String("Character is Rock!  This spell will not work!") : cs::String();
        default:
            return cs::String();
    }
}

int32_t Rules::SpellRaiseChance(int32_t con, int32_t effect, int32_t causeOfDeath) {
    int32_t tier = cs::Math::Max(1, effect - 19);
    // l0456 is an Integer (MAINMOD2.BAS:2759), so the Double is rounded half to even, not truncated
    int32_t chance = cs::Math::Clamp(cs::CastDouble<int32_t>(cs::Math::Round(35 * (con / 4.5) + tier * 10, cs::MidpointRounding::ToEven)), 10, 98);
    return causeOfDeath == DeathRock ? 10 : chance;
}

Rules_AilmentStepResult Rules::AilmentStep(cs::Ref<cs::Random> rng, int32_t effectiveCon, int32_t poisonLevel, int32_t diseaseLevel) {
    bool curedP = false, curedD = false;
    int32_t lost = 0;
    if (poisonLevel > 0) {
        int32_t roll = rng->Next(200) + (effectiveCon - 10) * 2 - poisonLevel * 2;
        if (roll > 185)
            curedP = true;
        else
            lost += 2;
    }
    if (diseaseLevel > 0) {
        // Ailments(0) is read again here, so on a tick that just cured the poison it is 0 (FRMDUNM.BAS:538-547)
        int32_t roll = rng->Next(200) + (effectiveCon - 10) * 2 - (curedP ? 0 : poisonLevel) * 2;
        if (roll > 195)
            curedD = true;
        else
            lost += 1;
    }
    return Rules_AilmentStepResult{curedP, curedD, lost};
}

Rules_TrapReadoutResult Rules::TrapReadout(cs::Ref<cs::Random> rng, int32_t naturalWis, int32_t gearWis, int32_t monsterGuildLevel, int32_t monsterDex,
                                           double thievingChance, int32_t thievingGuildLevel, int32_t realTrap) {
    // note the original's precedence: only the gear part is quartered
    double confidence = naturalWis + gearWis / 4.0;
    confidence -= cs::Math::Log(cs::Math::Max(1, monsterGuildLevel)) * cs::Math::Log(cs::Math::Max(1, monsterDex)) * 2.5;

    double disarm = thievingChance;
    if (disarm < 5) disarm = 5;
    if (confidence < 2) confidence = 2;
    if (confidence > 98) confidence = 98;

    double guildTerm = cs::Math::Log(cs::Math::Max(1, thievingGuildLevel));
    confidence = cs::Math::Floor(confidence + disarm / 1.2) + guildTerm * guildTerm;
    if (disarm > 98) disarm = 98;

    int32_t shown = confidence < rng->Next(100) + 1 ? rng->Next(ChestTraps::Names.Length()) : realTrap;

    if (confidence > 100) confidence = 100;
    if (confidence < 1) confidence = 1;
    int32_t diamonds = cs::CastDouble<int32_t>(confidence / 10 / 2.3);
    if (diamonds < 1) diamonds = 1;

    char sign;
    int32_t count;
    if (disarm > 44 && disarm < 56) {
        sign = '*';
        count = 1;
    } else if (disarm < 45) {
        sign = '-';
        count = cs::CastDouble<int32_t>(5 - disarm / 15 - 0.5);
    } else {
        sign = '+';
        count = cs::CastDouble<int32_t>((disarm - 50) / 15);
    }
    if (count < 1) count = 1;
    if (count > 3) count = 3;

    return Rules_TrapReadoutResult{cs::String((size_t)count, sign), diamonds, shown};
}

bool Rules::MagicallyLocked(int32_t lockState) {
    return lockState > 0;
}

double Rules::RenameChance(double identifySkill, bool inFog) {
    double p = (inFog ? identifySkill * 0.75 : identifySkill) + 20;
    if (p < 10) p = 10;
    if (p > 90) p = 90;
    return p;
}

int32_t Rules::RoundedIndex(cs::Ref<cs::Random> rng, int32_t n) {
    return cs::CastDouble<int32_t>(cs::Math::Round(rng->NextDouble() * n, cs::MidpointRounding::ToEven));
}

int32_t Rules::QuestTier(int32_t guildLevel) {
    return cs::Math::Clamp(cs::CastDouble<int32_t>(cs::Math::Round(guildLevel / 25.0, cs::MidpointRounding::ToEven)), 1, 10);
}

int32_t Rules::QuestItemBand(int32_t tier) {
    return cs::CastDouble<int32_t>(tier / 1.5) + 1;
}

Tup_Type_Tier_b59ee Rules::RollQuest(cs::Ref<GuildDef> guild, int32_t guildLevel, cs::Ref<cs::Random> rng) {
    // `Dim l010E As Integer` (FRMCTYG.BAS:211): every `l010E = Rnd * 100` rounds half to even
    auto Roll = [&]() { return cs::CastDouble<int32_t>(cs::Math::Round(rng->NextDouble() * 100, cs::MidpointRounding::ToEven)); };

    int32_t roll = guildLevel < 5 ? 100 : Roll();
    if (roll > guild->QuestPercent) return Tup_Type_Tier_b59ee{0, 0};

    int32_t tier = QuestTier(guildLevel);
    if (Roll() <= 5) return Tup_Type_Tier_b59ee{-1, tier};  // let off the hook: -1, which is said
    int32_t type = Roll() < 25 && guildLevel > 15 ? 2 : 1;
    return Tup_Type_Tier_b59ee{type, tier};
}

cs::Ref<Monster> Rules::QuestMonster(cs::List<cs::Ref<Monster>> all, int32_t tier, cs::Ref<cs::Random> rng) {
    int32_t nc = EncounterGenerator::ClassCount(all);
    if (nc <= 0 || all.Count() == 0) return nullptr;
    auto r = EncounterGenerator::ClassRanges(all);
    // only guards a table with no class of monsters in it, where the original's loop could never end
    bool anyClass = false;
    for (int32_t c = 0; c < nc; c++)
        if (r[c][0] != -1) anyClass = true;
    if (!anyClass) return nullptr;

    int32_t pick = 0, last = 0;
    do {
        int32_t cls = cs::CastDouble<int32_t>(cs::Math::Floor(rng->NextDouble() * nc));      // :491
        int32_t start = cs::Math::Floor(rng->NextDouble() * 100) < 40 ? r[cls][0] : -1;       // :492-496
        last = -1;                                                                             // :497
        if (r[cls][0] != -1)                                                                   // :499
            for (int32_t i = r[cls][0]; i <= r[cls][1]; i++) {                                 // :498
                if (start == -1 && all[i]->LevelFound >= tier) start = i;                      // :502-506
                if (all[i]->LevelFound > tier || i == r[cls][1]) {                             // :507-510
                    last = i - 1;
                    break;
                }
            }
        if (start == -1) start = r[cls][0];                                                    // :512-514
        double span = rng->NextDouble() * (last - start);                                      // :515
        // an empty class: the original's Monsters(-1) at :517 is a subscript error
        if (r[cls][0] == -1) {
            pick = -1;
            continue;
        }
        pick = cs::CastDouble<int32_t>(cs::Math::Round(span, cs::MidpointRounding::ToEven)) + start;  // :515-516
        bool rare = all[pick]->Chance < 10;
        bool turned = rng->NextDouble() * 100 > 25;  // :517, drawn always
        if (rare && turned) pick = -1;
    } while (last == -1 || all[last]->LevelFound > tier || pick == -1);  // :520
    return all[pick];
}

bool Rules::QuestItemReachable(cs::List<cs::Ref<Item>> all, int32_t band, int32_t kinds) {
    int32_t n = all.Count();
    for (int32_t k = 0; k < kinds; k++) {
        int32_t first = -1;
        for (int32_t i = 0; i < n; i++)
            if (all[i]->Type == k) {
                first = i;
                break;
            }
        if (first < 0 || all[first]->Floor > band) continue;
        int32_t last = -1;
        for (int32_t i = first; i < n; i++)
            if (all[i]->Floor > band || i == n - 1 || all[i]->Type != k) {
                last = i - 1;
                break;
            }
        if (last < first) last = first;
        int32_t top = last == first ? first + 1 : last;
        for (int32_t p = first + 1; p <= top; p++)
            if (p < n && all[p]->Price >= 100 && all[p]->Cursed == 0 && all[p]->Floor >= 1) return true;
    }
    return false;
}

cs::Ref<Item> Rules::QuestItem(cs::List<cs::Ref<Item>> all, int32_t tier, cs::Ref<cs::Random> rng) {
    int32_t band = QuestItemBand(tier);
    int32_t n = all.Count();
    int32_t kinds = Mdata1::ItemTypes.Count() > 0 ? Mdata1::ItemTypes.Count()
                  : n == 0 ? 0 : all.Max([&](auto& i) { return (int32_t)i->Type; }) + 1;
    // only a table where no pick can pass could spin for ever; the original would hang there
    if (kinds == 0 || !QuestItemReachable(all, band, kinds)) return nullptr;

    int32_t pick = 0;
    do {
        int32_t first = 0, kind = 0;
        do {
            do {
                first = -1;                                                     // :528
                kind = cs::CastDouble<int32_t>(cs::Math::Floor(rng->NextDouble() * kinds));  // :529
                for (int32_t i = 0; i < n; i++)                                 // :530-535
                    if (all[i]->Type == kind) {
                        first = i;
                        break;
                    }
            } while (first == -1);                                              // :536
        } while (all[first]->Floor > band);                                     // :537

        int32_t last = -1;  // :538-546
        for (int32_t i = first; i < n; i++)
            if (all[i]->Floor > band || i == n - 1 || all[i]->Type != kind) {
                last = i - 1;
                break;
            }
        if (last < first) last = first;  // :547-549

        pick = cs::CastDouble<int32_t>(cs::Math::Floor(rng->NextDouble() * (last - first) + 1)) + first;  // :550-551
        cs::Ref<Item> it = pick < n ? all[pick] : cs::Ref<Item>();  // Items(gv1BEC): blank
        if (it == nullptr || it->Price < 100)
            pick = -1;  // :552
        else if (it->Cursed != 0)
            pick = -1;  // :554
        else if (it->Floor < 1)
            pick = -1;  // :556
        else {
            bool rare = it->Rarity < 5;
            bool turned = rng->NextDouble() * 100 > 25;  // :558
            if (rare && turned) pick = -1;
        }
    } while (pick == -1);  // :561
    return all[pick];
}

int32_t Rules::LevelGapChance(int32_t charGuildLevel, int32_t monsterGuildLevel) {
    if (charGuildLevel > monsterGuildLevel + monsterGuildLevel / 4.0) return 90;
    if (charGuildLevel < monsterGuildLevel - monsterGuildLevel / 4.0) return 10;
    return 50;
}

}  // namespace mc
