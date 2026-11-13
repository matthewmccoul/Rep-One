#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- constants ----

// R3 death cause codes (SRD Table 1). The port stores causes as DeathState; see CONFLICT in recordDeath.
inline constexpr i32 kDeathCodeOrdinary = 0;
inline constexpr i32 kDeathCodeStoned = 2;
inline constexpr i32 kDeathCodeRock = 8;

// R3 Name(cause) = Names[max(1, cause) - 1]
inline constexpr i32 kDeathNameCount = 8;
// "Annihalated" is a misspelling kept on purpose (original quirk, R3).
inline constexpr Arr<const char*, kDeathNameCount> kDeathStateNames = {
    "Dead", "Stoned", "Destroyed", "Immolated", "Frozen", "Annihalated", "Melted", "Rock"};

// R3 area sentinel and rock-death areas
inline constexpr i32 kAreaUnset = -1;
inline constexpr i32 kRockAreaAboveGround = -1;
inline constexpr i32 kRockAreaBelowGround = 0;

// R8/R9 raise chance formula terms and clamp
inline constexpr double kMorgueConWeight = 35.0;     // R9 CON term weight
inline constexpr double kMorgueConDivisor = 4.5;     // R9 CON term divisor
inline constexpr i32 kMorgueCounterBase = 10;        // R9 flat term (own-body flat is 40, see morgueRaiseChance)
inline constexpr i32 kRaiseChanceMin = 10;           // R8/R9 clamp low
inline constexpr i32 kRaiseChanceMax = 98;           // R8/R9 clamp high

// R8 guild pays the Morgue fee when TopGuildLevel is at or below this
inline constexpr i32 kGuildPaysMaxTopLevel = 5;

// R10 rescue constants
inline constexpr i32 kRescueLevelMin = 1;
inline constexpr i32 kRescueLevelMax = 15;
inline constexpr i32 kRescueCoordParts = 3;
inline constexpr i32 kRescueReturnPercentPerLevel = 4;   // return test threshold per level, out of 100
inline constexpr i32 kRescueCloseRange = 8;              // CloseEnough: strict |dx| < 8 and |dy| < 8
inline constexpr double kRescueSearchCostFactor = 372567.52;
inline constexpr double kRescueRecoverFraction = 0.2;    // living body recovery at or below 20% of MaxHits

// ---- result types ----

// R8/R9 outcome of paying a Morgue fee
enum class MorgueFeeOutcome : i32 {
    GuildPays = 0,
    Charged,
    NotEnoughGold
};

// R9 reasons a counter raise is turned away
enum class CounterRaiseRefusal : i32 {
    None = 0,
    Rock,
    AlreadyPending
};

// R8 result of a raise roll
struct RaiseResult {
    bool raised = false;
    bool complicated = false;
};

// R11 reasons Ask to Join is refused before any offer
enum class AskToJoinRefusal : i32 {
    None = 0,
    InCombat,
    Carried,
    NoPendingRaise
};

// R11 result of Ask to Join
enum class AskToJoinOutcome : i32 {
    Refused = 0,
    Declined,
    Failed,
    Raised
};

struct AskToJoinResult {
    AskToJoinOutcome outcome = AskToJoinOutcome::Refused;
    AskToJoinRefusal refusal = AskToJoinRefusal::None;
    bool complicated = false;
};

// ---- R3 death recording ----





// R3 guild log line "death" for who
void logDeath(Ctx& ctx, const Character& who);

// ---- R4 dead-state cleanup ----

// R4 the dead screen is owed only while the character is dead and the party holds one member.
// Screen display itself is out of scope.
bool deadScreenOwed(const Character& who, const Party& party);

// ---- R5 party wipe in combat ----

// R5 a wipe casualty: at or below zero hits, or stoned
bool isWipeCasualty(const Character& who);

// R5 on PartyWiped: store the room (caller persists), mark casualties dead, log per cause,
// and count each newly dead character once. Pruning and sub0930 are run by the caller.
void onPartyWiped(Ctx& ctx, Party& party, Arr<bool, kPartyMaxSize>& countedDead);



// ---- R6 party member pruning ----

// R6 UPDATE casualty test: at or below zero hits
// CONFLICT: rule text says "zero or below"; engine tests == 0 (CONFLICT 10). This header follows the rule text.
bool isUpdateCasualty(const Character& who);

// R6 ordinary-death bookkeeping for a pruned member: dead, Deaths +1, guild log at DiedOnLevel or DiedSomewhere.
// Saving is the caller's job.
void applyPruneDeath(Ctx& ctx, Character& who);

// R6 remove the member at index, shifting the rest up. Never removes the last member; returns false then.
bool removePartyMember(Party& party, i32 index);

// ---- R7 rescue waiting ----








// ---- R8 Morgue: own body ----








// ---- R9 Morgue: counter raise ----




// ---- R10 Morgue: hire rescuers ----







// ---- R11 pending raise lifecycle ----







}  // namespace mordor
