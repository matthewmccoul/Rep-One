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














// [c17][c27] Age added for a failed raise by cause: stoned = kAgeStoned, rock = kAgeRock, otherwise kAgeQuietFailure.
// UNKNOWN: whether the age-by-cause rule applies to complicated failures [c27].
i32 raiseAgeCost(DeathState cause);









}  // namespace mordor
