#pragma once
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"
#include "mordor/state_companion.h"
#include "mordor/companions1a.h"  // provides the forward declaration of GameState
#include "mordor/casting1.h"      // provides CastRefusal (used by refusedOrder)

namespace mordor {

// ---- constants, tables and small result types ----

inline constexpr i32 kNoGuild = -1;        // R11 a score of zero wins no guild
inline constexpr i32 kBreathKillCap = 4;   // R16 sweep loop continues while Kills <= 4

// Per-ability parameters for the player chance formula (P2 table, c18/c23/c25).
struct ProcRule {
    i32 divisor;
    i32 weaponLog;
    i32 floor;
    i32 cap;
    i32 increment;   // R10 damage modifier increment
    i32 effectBit;   // item ability bit in field 0x0E
};

// Critical: divisor 3, weaponLog 10, floor 1, cap 50, +5, bit 4 (0x10).
inline constexpr ProcRule kCritRule{3, 10, 1, 50, 5, 4};
// Backstab: divisor 2 (engine), weaponLog 100, floor 2, cap 75, +2, bit 5 (0x20).
// CONFLICT: backstab divisor is 2 in engine, audit and fidelity, but 3 in archive (c97). Engine preferred.
inline constexpr ProcRule kBackstabRule{2, 100, 2, 75, 2, 5};

// Monster crit and backstab parameters (R12, c18).
inline constexpr i32 kMonsterCritDivisor = 650;
inline constexpr i32 kMonsterBackstabDivisor = 500;
inline constexpr i32 kMonsterCritFloor = 2;
inline constexpr i32 kMonsterCritCap = 50;
inline constexpr i32 kMonsterBackstabFloor = 4;
inline constexpr i32 kMonsterBackstabCap = 75;

// Which ability a player proc is for (R11).
enum class CombatProc : i32 {
    Critical = 0,
    Backstab = 1
};

// Guild coefficient (CritMod or BSMod) for R11 (wiki, c97).
// UNKNOWN: full coefficient table; guilds not listed here score zero.
struct GuildCoefficient {
    Guild guild;
    i32 coefficient;
};
inline constexpr GuildCoefficient kCritCoefficients[2] = {
    {Guild::Warrior, 9},
    {Guild::Seeker, 3}
};
inline constexpr GuildCoefficient kBackstabCoefficients[2] = {
    {Guild::Thief, 9},
    {Guild::Ninja, 2}
};


// Inputs to the player blow procedure (P2).
struct PlayerBlowInput {
    i32 monLevel = 0;          // M in the player formula
    i32 weaponFloor = 0;       // F in the weapon term; UNKNOWN: equals archive ItemDropLevel (c5, c23, c97)
    bool weaponCrit = false;   // weapon carries the critical bit
    bool weaponBackstab = false; // weapon carries the backstab bit
};

// Results of the player blow procedure (P2 steps 1, 2, 4, 5).
struct PlayerBlowRolls {
    bool crit = false;
    bool backstab = false;
    f64 critChance = 0.0;
    f64 backstabChance = 0.0;
    i32 damageModifier = 0;    // R10
    i32 critGuild = kNoGuild;  // R11
    i32 backstabGuild = kNoGuild; // R11
};

// Inputs to the player chance formula (R7, R8, c18).
struct PlayerChanceParams {
    i32 guildLevel = 0;        // GLvl of the guild selected by R11
    i32 coefficient = 0;       // CritMod or BSMod of that guild
    i32 monLevel = 0;          // M
    bool weaponHasAbility = false;
    i32 weaponFloor = 0;       // F
};

// ---- R7 and R8 player chance and roll (P2 steps 1 and 2) ----



// ---- R10 damage modifier ----


// ---- R11 guild selection per ability ----


// ---- P2 player blow (CombatEngine.SwingOnce) ----


// ---- R12 and R13 monster crit and backstab (P3) ----



// ---- R14 to R17 companion breath sweep (P4) ----



// ---- R18 to R21 combat-setting spell refusal (P5) ----



// ---- R22 to R24 fight-end Asmodeus check (P6) ----




// ---- R25 and R26 quest settlement (P7) ----


// ---- R27 and R28 combat experience per kill (P8) ----





// ---- R29 monster specials ----
// R29 monster crit, backstab and spells are speculation in the manual; no engine evidence was supplied, so nothing is declared.

}  // namespace mordor
