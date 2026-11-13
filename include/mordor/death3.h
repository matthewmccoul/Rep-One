#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- Constants ----

// [c17][c19][c24][c28][c50] Morgue own-body chance bonus.
// CONFLICT: the counter route uses +10 (kMorgueCounterBonus) [c17][c30][c50]; the own-body route uses +40 [c19][c50].
inline constexpr i32 kMorgueOwnBodyBonus = 40;
// [c17][c30][c50] Morgue counter chance bonus ("mode -1").
inline constexpr i32 kMorgueCounterBonus = 10;
// [c19] Stored chance returned by the spell raise path for a rock body. Prepare zeroes it before the offer [c17].
inline constexpr i32 kRockStoredRaiseChance = 10;
// [c17][c27] Age added by a quiet failed raise, by cause.
inline constexpr i32 kAgeQuietFailure = 54;
inline constexpr i32 kAgeStoned = 84;
inline constexpr i32 kAgeRock = 124;
// [c49] Morgue hire level limits.
// CONFLICT: the doc says "below level 15" [c25]; the engine says above 15 [c49]. Engine preferred.
inline constexpr i32 kMorgueMinLevel = 1;
inline constexpr i32 kMorgueMaxLevel = 15;
// [c28][c50] A guild pays a morgue own-body raise when TopGuildLevel is at most this and the purse is short.
inline constexpr i32 kGuildPaysMaxLevel = 5;
// [c17][c28] Revival fee formula: Int(kRevivalFeeFactor * (MaxHits / kRevivalFeeHitDivisor)).
inline constexpr double kRevivalFeeFactor = 2912.32;
inline constexpr double kRevivalFeeHitDivisor = 1.5;
// [c17][c27] Complicated max-hits loss: Int(MaxHits / kComplicationHitDivisor).
inline constexpr double kComplicationHitDivisor = 1.2;
// [c17] Living recovery at the morgue applies when CurHits <= this fraction of MaxHits.
// UNKNOWN: whether the port implements this test [c49].
inline constexpr double kLivingRecoverFraction = 0.20;
// [c18] Old-age threshold percentage and base term.
inline constexpr i32 kOldAgePct = 98;
inline constexpr double kOldAgeBase = 5.0;
// [c17] Stone costs this much CON on any raise, regardless of outcome.
inline constexpr i32 kStoneConLoss = 1;
// [c88] Hall date field: days since 1900-01-01, plus this offset.
// CONFLICT: engine MDATA9 uses a Long day [c35]; archive uses a 7-byte field [c88]. Engine preferred.
inline constexpr i64 kHallDateDayOffset = 2;
// [c88] Hall stat field scale (value * 10000).
// CONFLICT: archive 8-byte stat field [c88] vs engine MDATA9 layout [c35]. Engine preferred for MDATA9/12.
inline constexpr i64 kHallStatScale = 10000;

// ---- Enums and small result structs ----

// [c17][c19] Morgue route: own body (+40) or counter (+10).
enum class MorgueRoute : i32 {
    OwnBody = 0,
    Counter = 1
};

// [c19][c50] Reasons the morgue refuses a body.
enum class MorgueRefusal : i32 {
    None = 0,
    LevelTooLow,
    LevelTooHigh,
    RockCannotHelp
};

// [c17][c27] Outcome of a raise attempt.
enum class RaiseAttemptOutcome : i32 {
    Success = 0,
    QuietFailure,
    Complicated,
    Refused
};

// [c17][c28][c50] Who pays a revival and whether the payer is asked.
struct RevivalPayment {
    Currency fee = 0;
    bool guildPays = false;
    bool asksPayer = false;
};

// [c17][c27][c50] Result of a player raise attempt.
struct PlayerRaiseResult {
    RaiseAttemptOutcome outcome = RaiseAttemptOutcome::Refused;
    i32 chanceUsed = 0;
    i32 ageAdded = 0;
    i32 conLost = 0;
    bool conRollMade = false;
};

// ---- Declarations ----

// [c19][c24][c28][c50] Morgue own-body raise chance, before clamping.
// UNKNOWN: base term is Table 13 CON-based; natural vs modified CON is UNKNOWN [c50].
i32 morgueOwnBodyChance(const Character& ch);

// [c17][c30][c50] Morgue counter raise chance, before clamping.
// CONFLICT: rounding is round_to_int in engine [c17] vs Int() in doc [c30]. Engine preferred.
i32 morgueCounterChance(const Character& ch);

// [c19] Morgue raise chance for a route, clamped to kRaiseChanceMin..kRaiseChanceMax.
i32 morgueChance(const Character& ch, MorgueRoute route);

// [c19] Clamp a raw raise chance to kRaiseChanceMin..kRaiseChanceMax.
i32 clampRaiseChance(i32 raw);

// [c19] Spell raise chance. Returns kRockStoredRaiseChance for a rock body.
// UNKNOWN: values for non-rock tiers; the engine uses tier only, not spell base values [c19][c83].
i32 spellRaiseChance(const Character& ch, RaiseKind kind);

// [c18] Old-age term 5 + Ln(pct - 98).
// UNKNOWN: sign is negative between 98 and about 98.37 and no clamp is given [c18].
double oldAgeTerm(i32 pct);

// [c18] True when the percentage is at or past the old-age threshold (pct >= 98).
bool isOldAgePct(i32 pct);

// [c17][c28][c50] Revival quote: fee, whether the guild pays, and whether the payer is asked.
// CONFLICT: doc asks payment for level 5 or under who cannot afford it [c28]; engine asks nothing for level 5 or under who can afford it [c50]. Engine preferred.
RevivalPayment quoteRevival(const Character& ch);

// [c17][c28] Revival fee in currency units.
// UNKNOWN: whether the formula result is whole gold (currencyFromWhole) or raw currency units.
Currency revivalFee(const Character& ch);

// [c17][c28][c50] True when the guild pays: TopGuildLevel <= kGuildPaysMaxLevel and the purse cannot cover fee.
bool guildPaysRevival(const Character& ch, Currency fee);

// [c19][c49][c50] Morgue refusal for a body: rock, or hire level out of range.
// UNKNOWN: whether the level checked is activeLevel() or Character::level.
// UNKNOWN: whether the morgue offers a raise to a rock body at 0% [c30]; this declaration follows the engine and refuses it [c19].
MorgueRefusal morgueRefusal(const Character& body);

// [c17][c24][c97] True when age >= raceMaxAge / 2 (integer halving), so the complication is guaranteed.
// CONFLICT: archive gives a fixed 50 [c103]; race-scaled threshold used per engine [c17][c24][c97].
bool complicationGuaranteed(i32 age, i32 raceMaxAge);

// [c17][c27] Max hits after a complicated raise: Int(maxHits / kComplicationHitDivisor).
i32 complicatedMaxHits(i32 maxHits);

// [c17][c27] Age added for a failed raise by cause: stoned = kAgeStoned, rock = kAgeRock, otherwise kAgeQuietFailure.
// UNKNOWN: whether the age-by-cause rule applies to complicated failures [c27].
i32 raiseAgeCost(DeathState cause);

// [c17] True when the living body qualifies for recovery at the morgue (CurHits <= kLivingRecoverFraction of MaxHits).
// UNKNOWN: whether the port implements this test [c49].
bool livingRecoversAtMorgue(const Character& ch);

// [c47][c66] True when the character is a prune candidate (hits <= 0).
// CONFLICT: engine formula uses CurHits == 0 [c53]; VB uses Hits <= False [c58]. Prose and test use <= 0 [c47][c66]; <= 0 chosen.
bool isPruneCandidate(const Character& ch);

// [c48] Whether city rest applies. Dead characters are not rested until raised. Engine preferred over the 31-day rest [c31].
bool cityRestApplies(const Character& ch);

// [c10] Demote the character's guild level by one.
// UNKNOWN: the number of levels N is not given [c10]; one level is assumed as the simplest sensible rule.
void demoteGuildLevel(Ctx& ctx, Character& ch, Guild guild);

// [c17][c19][c27][c97] Attempt a raise on a character with the given chance; complication via complication().
// Morgue own-body rock refusal and kind-specific rules are applied; rock with chance zeroed [c17] leads to complication.
// UNKNOWN: the morgue CON roll details [P10.11].
// UNKNOWN: whether Resurrect's full health also restores lost stats; presumed not [c17].
PlayerRaiseResult raiseCharacter(Ctx& ctx, Character& ch, RaiseKind kind, i32 chance);

// [c35][c88] Hall date field for days since 1900-01-01.
// CONFLICT: see kHallDateDayOffset.
i64 hallDateField(i64 daysSince1900);

// [c88] Hall stat field value (value * kHallStatScale).
// CONFLICT: see kHallStatScale.
i64 hallStatField(i64 value);

// [c33] Hall Wealthiest bank record.
// UNKNOWN: the stored record for a half-unit bank is self-contradictory [c33].
Currency hallWealthiestRecord(Currency bank);

}  // namespace mordor
