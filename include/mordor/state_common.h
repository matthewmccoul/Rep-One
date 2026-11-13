// Shared constants and enums used by several state headers (single definition site).
#pragma once
#include "mordor/types.h"

namespace mordor {

inline constexpr i32 kGuildCount = 12;       // guilds in table order
inline constexpr i32 kPackSlots = 40;        // pack array 0..39; index 0 is Hands
inline constexpr i32 kCompanionSlots = 4;    // companion slots 0..3 per character
inline constexpr i32 kPartyMaxSize = 4;      // party maximum

// Death states (SRD ch.18 Table 1): 0 = alive or ordinary death handled by flags, 1..8 named states.
enum class DeathState : i32 { Alive = 0, Dead = 1, Stoned = 2, Destroyed = 3, Immolated = 4, Frozen = 5, Annihilated = 6, Melted = 7, Rock = 8 };

// How well an item is identified (SRD ch.13).
enum IdLevel : i32 { Unknown = 0, Barely = 1, Partial = 2, Mostly = 3, All = 4 };

// Chest trap kinds 0..10 (SRD ch.7 and ch.14): 1-4 hit every member, 5-10 only the opener; 3 and 4 do damage.
enum class ChestTrap : i32 { NotTrapped = 0, PoisonGas = 1, DiseaseSpores = 2, Explosion = 3, Fire = 4, ItemDestruction = 5, Fate = 6, Trap7 = 7, Blackout = 8, Trap9 = 9, Trap10 = 10 };

// Constants and enums that the game state needs but that were first declared in module headers (moved here to break include cycles).
enum class AltPoolOutcome : i32 { Refused = 0, PoolOnHand = 1, PoolAndDeposit = 2 };
inline constexpr i32 kDefaultMessageLines = 20;      // R42 mc04F2 default message window lines
inline constexpr i32 kReplenishPassCap = 500;        // R28 creature restock pass cap
inline constexpr i32 kGuildLogKeepOnClean = 50;         // R65 newest entries kept across all guilds
inline constexpr i32 kSeerLevelGateOriginal = 12;       // R64 original level gate (FRMSEEK.BAS:90, 237)

}  // namespace mordor
