#pragma once
// progression2a.h - SRD c05p2 (R28-R36).
// Planning projections (R28), archive route helpers (R29-R30), sanctuary aging (R31),
// port-deviation helpers (R33), companion and roster helpers (R34), and the
// constants of Table T10 and the SRD tables T1, T2, T11 and T12.
// Declarations only. Conflicts and unknowns are tagged for grep.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"

namespace mordor {

// ---- Tables and constants (SRD T1, T2, T10, T11, T12, R30) ----

// T1 raw Exp per race, indexed by static_cast<i32>(Race) (Human..Troll)
inline constexpr i32 kRaceRawExp[9] = {3, 7, 7, 7, 5, 6, 4, 8, 9};

// T1 / test RaceMult, indexed by raw Exp (0..9). Raw Exp <= 3 gives 1.0.
inline constexpr double kRaceMultByRawExp[10] = {
    1.0, 1.0, 1.0, 1.0,
    1.05819132268, 1.10551994263, 1.14714602440,
    1.18544741085, 1.22165868435, 1.25647909518};

// T2 guild Parameter (guild Experience Parameter, GEF), indexed by Guild order.
// CONFLICT: Wizard EP 3.75 x 8 = 30, but the table prints 27; the printed value is used.
// UNKNOWN: Healer Parameter is not printed; 32 is derived from EP 4.0 x 8.
inline constexpr i32 kGuildParameter[kGuildCount] = {8, 16, 19, 21, 21, 18, 15, 18, 27, 27, 27, 32};

// T2 guild gold factor (GGF), indexed by Guild order
inline constexpr i32 kGuildGoldFactor[kGuildCount] = {3, 1, 2, 5, 5, 3, 9, 7, 3, 3, 4, 4};

// R30 post-route guild order (archive player advice, not engine mechanics)
inline constexpr Guild kPostRouteOrder[5] = {
    Guild::Warrior, Guild::Ninja, Guild::Paladin, Guild::Villain, Guild::Seeker};

// R29 Con needed for bonus HP
inline constexpr i32 kRouteConBonusMin = 17;
// R29 route step 2: Warrior target level (Warrior AH = 6)
inline constexpr i32 kRouteWarriorTarget = 28;
// R29 route step 3: Nomad target level
inline constexpr i32 kRouteNomadTarget = 30;
// R29 route step 4: Wizard target level, also Wizard ML
inline constexpr i32 kRouteWizardTarget = 41;
// R29 Nomad ML (bonus-HP levels end here)
inline constexpr i32 kRouteNomadMaxLevel = 30;
// R29 Warrior average hits per level up to level 28 (archive T11)
inline constexpr i32 kWarriorAvgHits = 6;
// R29 Nomad extra hits at levels 29-30
// UNKNOWN: per level or total (R35)
inline constexpr i32 kNomadExtraHits = 5;
// R29 Nomad hits per level past level 30 (archive T11)
inline constexpr i32 kNomadPostHits = 1;
// R29 other guilds' hits per level past ML (archive T11)
inline constexpr i32 kOtherPostHits = 2;
// R29 HP lost when reversing Warrior and Nomad (archive)
inline constexpr i32 kReversalHitLoss = 28;

// R31 sanctuary aging amount, days (original, agreed)
// CONFLICT: port SpellEffect.cs ~50 uses 28 days; the original 365 is used here.
inline constexpr i32 kSanctuaryAgeDays = 365;
// R31 stat item Kind 25 aging, days
// UNKNOWN: item identity of Kind 25 (R35)
inline constexpr i32 kKind25AgeDays = 28;

// R36 constant mc00BC
// UNKNOWN: meaning of mc00BC (R36 lists only the value)
inline constexpr i32 kMc00BC = 999;
// R36 hits cap: MaxHits = min(MaxHits, 999)
inline constexpr i32 kMaxHitsCap = 999;
// R36 wiki pin cap: min(currentLevel + 1, 998)
inline constexpr i32 kWikiPinCapLevel = 998;
// R36 guild maximum level
inline constexpr i32 kGuildMaxLevel = 999;
// R36 kill XP constants (expAwarded, [c18]): 9, 20, 50, 1000
inline constexpr double kKillXpA = 9.0;
inline constexpr double kKillXpB = 20.0;
inline constexpr double kKillXpGuildOffset = 50.0;
inline constexpr double kKillXpDivisor = 1000.0;
// R36 companion factor
inline constexpr double kCompanionXpFactor = 0.25;
// R36 guild level scale 333 (GuildLevelFactor and chest award)
inline constexpr double kGuildLevelScale = 333.0;
// R36 chest award factor
inline constexpr double kChestXpFactor = 0.5;
// R36 archive level XP constants (266, 177)
// UNKNOWN: archive formula using these constants (R35, [c103])
inline constexpr double kArchiveLevelXpA = 266.0;
inline constexpr double kArchiveLevelXpB = 177.0;
// R36 XP_Total constants: level XP = a*l*l*(GEF/8) + b*(2l-1) - c
inline constexpr double kXpTotalSquare = 88.09020776;
inline constexpr double kXpTotalLinear = 45.45454;
inline constexpr double kXpTotalOffset = 0.264;
// R36 gold constant 30.1
// UNKNOWN: role of 30.1 in the gold formula (R35, Fn12B2 inputs)
inline constexpr double kGoldConstant = 30.1;
// R36 gold divisor: P = 1 + GGF / 10
inline constexpr double kGoldGgfDivisor = 10.0;
// R36 join cost constants 1.25, 3, 5356
// UNKNOWN: join cost formula (R35, [c104])
inline constexpr double kJoinC1 = 1.25;
inline constexpr double kJoinC2 = 3.0;
inline constexpr double kJoinC3 = 5356.0;

// ---- R28 Level-XP and gold planning ----



// ---- R29 Optimal route (archive) ----





// ---- R31 Sanctuary cast (archive) and aging ----

// R31 age a character by days and print the aging message (AgeBy, [c4])



// ---- R33 Documented port deviations ----



// ---- R34 Other notes ----




// ---- R32 and R35: archive provenance and known gaps ----
// No executable rules. Items are tracked in the comments above (UNKNOWN / CONFLICT tags).

}  // namespace mordor
