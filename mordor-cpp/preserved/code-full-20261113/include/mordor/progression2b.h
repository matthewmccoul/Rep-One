#pragma once
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"

namespace mordor {

// Fn13ED experience cap = xpThreshold(level+1) - 1; ±1 vs Fn12A1 at about 0.2% of cases [c3][c34]
i64 experienceCap(i32 level, double guildExpFactor, double raceExpFactor);
// Gold to take from level L: cumulative sum of (2i-1)*30.1 times (1+ggf/10), returned as Currency (1/10000 gold) [c24][c27][c104]
// UNKNOWN: per-term rounding; CONFLICT: anchor "exact" unverified [c104]
Currency levelUpGoldCost(i32 currentLevel, double guildGoldFactor);
// T13 join cost for a guild, as Currency (1/10000 gold) [c104]
Currency guildJoinCost(Guild guild);
// T10 clamp MaxHits to kMaxHitsCap [c2]
i32 clampMaxHits(i32 maxHits);
// [c34] announce "Can make a level" only when XP crosses the threshold
void announceLevelThresholdCrossed(Ctx& ctx, const Character& ch, Guild guild, i64 xpBefore, i64 xpAfter, i64 xpRequired);
// [c2] highest-guild test: guild level strictly greater than every other guild (tie: false)
bool isAheadOfEveryGuild(const Character& ch, Guild guild);

// ---- T11 hit gain schedule (archive) ----
// HP gained when guild reaches newLevel. avgHits is the guild's AH value (Warrior 6) [c106]
// UNKNOWN: bonus HP for each level below ML with Con >= kBonusHpConMin; amount not stated, excluded here [c106]
i32 hitsGainForLevel(Guild guild, i32 newLevel, i32 maxLevel, i32 avgHits, i32 con);

// ---- T9 rest, ailment and aging rules ----
// Town rest age cost in days: B((100 - 100*cur/max)/3.25); full HP gives 0, zero HP gives 31 [c24][c25]
// UNKNOWN: rounding of B; roundHalfEven chosen as it gives 0 and 31 at the anchors [c25]
i32 restAgeCostDays(i32 curHits, i32 maxHits);
// Adds restAgeCostDays to ch.ageDays and announces it [c24]. UNKNOWN: whether town rest also heals [c24][c48]
void applyRestAging(Ctx& ctx, Character& ch);
// Poison clear test: result > 185 [c23]
bool poisonClearSucceeds(i32 result);
// Poison clear on a roll result: clears poisonLevel and costs 2 HP if result > 185 [c23]
void applyPoisonCure(Ctx& ctx, Character& ch, i32 result);
// Disease clear test: result > 195 [c23]
bool diseaseClearSucceeds(i32 result);
// Disease clear on a roll result: clears diseaseLevel and costs 1 HP if result > 195 [c23]
void applyDiseaseCure(Ctx& ctx, Character& ch, i32 result);
// Old-age onset: ageDays >= 98% of racial lifespan [c23][c25][c97]
bool isOldAgeOnset(i32 ageDays, i32 racialLifespanDays);
// Natural death chance in percent: 5% + Log(excess) [c25]. CONFLICT: comparison direction UNKNOWN [c34]
double naturalDeathChancePercent(i32 ageDays, i32 racialLifespanDays);
// Natural death roll using naturalDeathChancePercent [c23][c25]
bool rollNaturalDeath(Ctx& ctx, i32 ageDays, i32 racialLifespanDays);
// Sanctuary ages target by kSanctuaryAgeDays. UNKNOWN: caster or party target [c31][c34][c106]
void applySanctuary(Ctx& ctx, Character& target);

} // namespace mordor
