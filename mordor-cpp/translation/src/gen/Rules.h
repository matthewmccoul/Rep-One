#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Rules_AilmentStepResult {
    bool CuredPoison = false;
    bool CuredDisease = false;
    int32_t HitsLost = 0;
};

struct Rules_TrapReadoutResult {
    cs::String Gauge{};
    int32_t Diamonds = 0;
    int32_t ShownTrap = 0;
};

struct Rules {
    static int32_t StatsAD(cs::List<int32_t> s);

    // Guild A/D by level (A-D_For_all_Guild_Levels): the five tables the community completed, "att/def:level".
    inline static cs::Dictionary<cs::String, cs::String> GuildTables = cs::Dictionary<cs::String, cs::String>::Of({
        {"Nomad", "10/10:1 13/13:2 16/16:4 19/19:6 22/22:8 25/25:10 28/28:12 31/31:14 34/34:16 37/37:18 40/40:20 43/43:22 46/46:24 49/49:26 52/52:28 55/55:30 58/58:33 61/61:36 64/64:39 67/67:42 70/70:45 73/73:49 76/76:53 79/79:57 82/82:61 85/85:65 88/88:69 91/91:73 94/94:77 97/97:81 100/100:85 103/103:89 106/106:93 109/109:97 112/112:101 115/115:105 118/118:109 121/121:113 124/124:117 127/127:121 130/130:125 133/133:129 136/136:133 139/139:137 142/142:142 145/145:147 148/148:152 151/151:157 154/154:162 157/157:167 160/160:172 163/163:177 166/166:182 169/169:187 172/172:192 175/175:197"},
        {"Warrior", "13/13:4 16/17:8 19/20:12 23/24:16 26/28:20 29/31:24 33/35:28 36/38:32 39/42:36 42/46:40 46/49:44 49/53:48 52/56:52 56/60:56 59/63:60 62/67:64 66/71:68 69/74:72 72/78:76 75/81:81 79/85:86 82/89:91 85/92:96 89/96:101 92/99:106 95/103:111 99/107:116 102/110:121 105/114:126 109/117:131 112/121:136 115/125:141 118/128:146 122/132:152 125/135:158 128/139:164 132/143:170 135/146:176 138/150:182 142/153:188 145/157:194 148/161:200 151/164:206 155/168:212 158/172:218 161/175:225 165/179:232 168/182:239 171/186:246 175/190:253 178/193:260 181/197:267 184/200:274 188/204:281 191/208:288 194/211:296 198/215:304 201/218:312 204/222:320 208/226:328 211/229:336 214/233:344 217/236:352 221/240:360 224/244:368 227/247:377 231/251:386 234/254:395 237/258:404 241/262:413 244/265:422 247/269:431 250/272:440 254/276:450"},
        {"Paladin", "13/13:3 16/17:6 19/20:9 22/24:12 25/28:15 28/31:18 31/35:21 34/38:24 37/42:27 41/46:33 44/49:37 47/53:41 50/56:45 53/60:49 56/63:54 59/67:59 62/71:64 65/74:69 68/78:74 71/81:79 75/85:84 78/89:89 81/92:94 84/96:99 87/99:104 90/103:109 93/107:114 96/110:119 99/114:125 102/117:131 106/121:137 109/125:143 112/128:149 115/132:155 118/135:161 121/139:167 124/143:173 127/146:179 130/150:185 133/153:192 137/157:199 140/161:206 143/164:213 146/168:220 149/172:227 152/175:234 155/179:241 158/182:248 161/186:255 165/190:263 168/193:271 171/197:279 174/200:287 177/204:295 180/208:303 183/211:311 186/215:319 189/218:327 192/222:336 196/226:345 199/229:354 202/233:363 205/236:372 208/240:381 211/244:390 214/247:400 217/251:410 220/254:420 223/258:430"},
        {"Seeker", "10/10:1 13/13:3 16/17:6 19/21:9 22/25:12 26/29:15 29/33:18 32/37:21 35/41:24 38/45:27 42/49:32 45/52:37 48/56:42 51/60:47 54/64:52 58/68:57 61/72:62 64/76:67 67/80:72 70/84:77 74/88:83 77/91:89 80/95:95 83/99:101 86/103:107 89/107:113 93/111:119 96/115:125 99/119:131 102/123:137 105/127:144 109/130:151 112/134:158 115/138:165 118/142:172 121/146:179 125/150:186 128/154:193 131/158:201 134/162:209 137/165:217 141/169:225 144/173:233 147/177:241 150/181:249 153/185:258 157/189:267 160/193:276 163/197:285 166/201:294 169/204:303 173/208:312 176/212:322 179/216:332 182/220:342 185/224:352 189/228:362 192/232:373 195/236:384 198/240:395 201/243:406 205/247:417 208/251:428"},
        {"Thief", "12/12:3 14/15:6 16/18:9 18/20:12 21/23:15 23/26:18 25/28:21 27/31:24 29/34:27 32/37:30 34/39:33 36/42:37 38/45:41 40/47:45 43/50:49 45/53:53 47/55:57 49/58:61 51/61:65 54/64:69 56/66:73 58/69:77 60/72:81 62/74:85 65/77:89 67/80:93 69/82:97 71/85:102 73/88:107 76/90:112 78/93:117 80/96:122 82/99:127 84/101:132 87/104:137 89/107:142 91/109:147 93/112:152 95/115:157 97/117:162 100/120:168 102/123:174 104/126:180 106/128:186 108/131:192 111/134:198 113/136:204 115/139:210 117/142:216 119/144:222 122/147:229 124/150:236 126/153:243 128/155:250 130/158:257 133/161:264 135/163:271 137/166:278 139/169:285 141/171:293 144/174:301 146/177:309 148/180:317 150/182:325 152/185:333 155/188:341 157/190:349 159/193:358 161/196:367 163/198:376 166/201:385 168/204:394"},
    });

    static constexpr int32_t BaseAD = 10;
    static constexpr int32_t WaterHurts = 1;

    inline static cs::List<Tup_Name_Index_51f4b> Abilities = cs::List<Tup_Name_Index_51f4b>::Of({
        Tup_Name_Index_51f4b{"Thieving", 0}, Tup_Name_Index_51f4b{"Backstabbing", 1}, Tup_Name_Index_51f4b{"Critical Hit", 2},
        Tup_Name_Index_51f4b{"Multiple Swings", 3}, Tup_Name_Index_51f4b{"Fighting", 4}, Tup_Name_Index_51f4b{"Perception", 5},
    });

    static constexpr double FightingBase = 0.6;

    inline static cs::List<cs::String> AbilityNames = cs::List<cs::String>::Of(
        {"Thieving", "Backstabbing", "Critical Hit", "Multiple Swings", "Fighting", "Perception"});

    inline static cs::List<cs::String> TrapNames = cs::List<cs::String>::Of({
        "Not Trapped", "Poison Gas", "Disease Spores", "Exploding Trap", "Fire Trap",
        "Slime", "Fate", "Teleporter", "Blackout", "Fear", "Withering",
    });

    static constexpr int32_t DeathOrdinary = 0;
    static constexpr int32_t DeathStoned = 2;
    static constexpr int32_t DeathRock = 8;

    inline static const cs::String LockedCaption = "Magically Locked";

    static int32_t AdInterval(cs::Ref<GuildDef> g, int32_t level, bool isNomad);
    static Tup_att_def_ac50a GuildAD(cs::String guild, int32_t level);
    static Tup_att_def_2ad65 GuildADSingle(cs::String guild, int32_t level);
    static Tup_att_def_ac50a BestGuildAD(cs::Ref<Character> c);
    static double HitThreshold(int32_t atk, int32_t def);
    static int32_t PitDamage(cs::Ref<cs::Random> rng, int32_t dungeonLevel, int32_t raceSize);
    static int32_t ChuteDamage(cs::Ref<cs::Random> rng, int32_t dungeonLevel, int32_t raceSize);
    static bool HoversOverPit(cs::Ref<cs::Random> rng, int32_t dungeonLevel);
    static int32_t EngageChance(Alignment monster, Alignment character, int32_t charisma);
    static bool MonstersEngage(cs::Ref<cs::Random> rng, Alignment monster, Alignment character, int32_t charisma);
    static double IdentifySkill(int32_t topGuildLevel, int32_t intelligence, int32_t wisdom);
    static int32_t IdentifyChance(double skill, bool dark);
    static int32_t RecogniseChance(double skill, bool dark);
    static bool QuicksandTakesSomething(cs::Ref<cs::Random> rng, int32_t dungeonLevel);
    static int32_t Fn0DEE(cs::Ref<cs::Random> rng, int32_t atk, int32_t def, int32_t level, int32_t strength, double damMod);
    static int32_t CharacterDamage(cs::Ref<cs::Random> rng, int32_t guildLevel, double fightingMod, double weaponMod, int32_t strength, int32_t sizeDiff, bool crit, bool backstab, int32_t atk, bool invisibleTarget = false, int32_t def = 0);
    static int32_t MonsterBlow(cs::Ref<cs::Random> rng, int32_t monAtt, int32_t targetDef, int32_t monGuildLevel, int32_t monStr, double damMod);
    static int32_t MonsterDamage(cs::Ref<cs::Random> rng, cs::Ref<Monster> m, int32_t sizeDiff, bool crit, bool backstab, int32_t charDef, bool openingChest = false);
    static double MonsterCritChance(int32_t attackerGuildLevel, int32_t attackerFloor, int32_t defenderLevel, bool backstab);
    static bool ParalysisPasses(cs::Ref<cs::Random> rng, int32_t paralysisLevel, int32_t guildLevel, int32_t con, int32_t resistPct, bool monstersPresent);
    static double CritChance(int32_t guildLevel, double critMod, int32_t monLevel, int32_t weaponFloor, bool weaponCrits = false);
    static double BackstabChance(int32_t guildLevel, double bsMod, int32_t monLevel, int32_t weaponFloor, bool weaponBackstabs = false);
    static double SpecialHitChance(int32_t guildLevel, double mod, int32_t monLevel, int32_t weaponFloor,
                                   bool weaponHasIt, double abilityDivisor, double weaponLog,
                                   double floor, double cap);
    static int64_t ChestExp(double disarmChance, int32_t monsterGuildLevel, int32_t firstCharacterGuildLevel, int32_t partySize = 1);
    static int64_t ExpAwarded(int32_t hitsInflicted, int32_t monGuildLevel, int32_t currentGuildLevel, bool byCompanion, int32_t partySize = 1);
    static int64_t Fn12A1(int32_t level, float guildExp, float raceExp);
    static int64_t Fn13ED(int32_t level, float guildExp, float raceExp);
    static int64_t Fn12B2(int32_t level, int32_t guildGold);
    static int64_t GuildJoinCost(cs::Ref<GuildDef> g);
    static int32_t SpellLevel(int32_t guildLevel);
    static int32_t Fn1485Price(int32_t level, int32_t u4, float rate, int32_t sl, int32_t& rated);
    static int32_t SpellDamage(cs::Ref<cs::Random> rng, int32_t baseDmg, int32_t power, int32_t sl, int32_t resistPct);
    static int32_t SpellHeal(cs::Ref<cs::Random> rng, int32_t baseHeal, int32_t power, int32_t sl);
    static double SpellHealAverage(int32_t baseHeal, int32_t power, int32_t sl);
    static int32_t EncounterSpread(int32_t level, cs::Ref<cs::Random> rng, bool onStud);
    static int32_t EncounterFloor(int32_t level, bool onStud);
    static int32_t AbilityRatingExact(cs::Ref<Character> c, int32_t ability, cs::Func<cs::Ref<Item>(int16_t)> gear = {});
    static cs::String BestAbilityGuild(cs::Ref<Character> c, int32_t ability, cs::Func<cs::Ref<Item>(int16_t)> gear = {});
    static double BestAbilityValue(cs::Ref<Character> c, int32_t ability, cs::Func<cs::Ref<Item>(int16_t)> gear = {});
    static double AbilityFloor(int32_t ability);
    static double AbilityCeiling(int32_t ability);
    static double LowestGuildModifier(int32_t ability);
    static double HighestGuildModifier(int32_t ability);
    static IdLevel NextIdLevel(IdLevel now);
    static int32_t RaiseConLossChance(int32_t con);
    static int32_t ChestClass(cs::Ref<Monster> m, double roll0To99, bool isLairMonster);
    static double GroupGold(cs::Ref<Monster> m, int32_t count, cs::Ref<cs::Random> rng);
    static int64_t EncounterGold(cs::List<cs::Ref<Monster>> kinds, cs::List<int32_t> counts, cs::Ref<cs::Random> rng, int32_t chestClass, bool chestLocked);
    static bool DiesOfOldAge(cs::Ref<cs::Random> rng, int32_t ageDays, int32_t raceLifespanYears);
    static Tup_wis_dex_3bce3 AbilityStats(cs::Ref<Character> c, int32_t ability, cs::Func<cs::Ref<Item>(int16_t)> gear);
    static double AbilityChance(int32_t ability, double guildRating, int32_t guildLevel,
                                double wis, double dex, int32_t dungeonLevel, int32_t monsterGuildLevel);
    static Tup_Chance_Guild_Modifier_4c9fc AbilityLead(cs::Ref<Character> c, int32_t ability, cs::Func<cs::Ref<Item>(int16_t)> gear = {});
    static double BestAbilityChance(cs::Ref<Character> c, int32_t ability, int32_t dungeonLevel, int32_t monsterGuildLevel = 2, cs::Func<cs::Ref<Item>(int16_t)> gear = {});
    static int32_t ChestTrap(cs::Ref<Monster> m, int32_t chestClass, cs::Ref<cs::Random> rng);
    static int32_t MorgueRaiseChance(int32_t con);
    static cs::String RaiseRefusal(int32_t causeOfDeath, int32_t effect);
    static int32_t SpellRaiseChance(int32_t con, int32_t effect, int32_t causeOfDeath);
    static Rules_AilmentStepResult AilmentStep(cs::Ref<cs::Random> rng, int32_t effectiveCon, int32_t poisonLevel, int32_t diseaseLevel);
    static Rules_TrapReadoutResult TrapReadout(cs::Ref<cs::Random> rng, int32_t naturalWis, int32_t gearWis, int32_t monsterGuildLevel, int32_t monsterDex,
                                               double thievingChance, int32_t thievingGuildLevel, int32_t realTrap);
    static bool MagicallyLocked(int32_t lockState);
    static double RenameChance(double identifySkill, bool inFog);
    static int32_t RoundedIndex(cs::Ref<cs::Random> rng, int32_t n);
    static int32_t QuestTier(int32_t guildLevel);
    static int32_t QuestItemBand(int32_t tier);
    static Tup_Type_Tier_b59ee RollQuest(cs::Ref<GuildDef> guild, int32_t guildLevel, cs::Ref<cs::Random> rng);
    static cs::Ref<Monster> QuestMonster(cs::List<cs::Ref<Monster>> all, int32_t tier, cs::Ref<cs::Random> rng);
    static bool QuestItemReachable(cs::List<cs::Ref<Item>> all, int32_t band, int32_t kinds);
    static cs::Ref<Item> QuestItem(cs::List<cs::Ref<Item>> all, int32_t tier, cs::Ref<cs::Random> rng);
    static int32_t LevelGapChance(int32_t charGuildLevel, int32_t monsterGuildLevel);
};

}  // namespace mc
