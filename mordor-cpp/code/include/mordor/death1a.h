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

// R3 AreaOr(fromSquare): the body's stored area if set, else rock rule, else the square's area
// UNKNOWN: "set" is taken as area != kAreaUnset.
i32 bodyAreaOr(const Character& who, i32 fromSquareArea);

// R3 AreaForRockDeath: DungeonLevel <= 0 gives level 0 and area -1; otherwise area 0
void setAreaForRockDeath(Character& who);

// R3 Name(cause) for the death-state name table
Str deathStateName(i32 causeCode);

// R3 record a death: setDead (step 1), area (step 3), count once and +1 Deaths, log, DIE sound (step 4).
// Caller runs sub0930 afterwards (step 5).
// CONFLICT: SRD stores ordinary death as 0; port stores DeathState::Dead (1). This header uses DeathState.
void recordDeath(Ctx& ctx, Character& who, DeathState cause, i32 squareArea, bool& alreadyCounted);

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

// R5 Wait is enabled only when the leader is dead or the party is awaiting rescue
bool waitButtonEnabled(const Party& party);

// R5 pressing Wait does nothing unless enabled. If awaiting rescue, clears the flag and enters the rescue wait.
// Returns true if the button acted.
bool pressWaitButton(Party& party);

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

// R7 rescue wait applies only to a dead character below ground and not rock-pulled
bool needsRescueWait(const Character& who);

// R7 easy-to-find test on floor and position, run first
// UNKNOWN: formula not stated in this part.
bool isEasyToFind(const Character& who);

// R7 found after daysWaited: carried to town, Age += days, told "You have been found after N day(s)!".
// Remains dead. Days come from daysUntilFound (declared elsewhere).
void applyRescueFound(Ctx& ctx, Character& who, i32 daysWaited);

// R7 carry a body to town: DungeonLevel = 0 and Area = -1.
// CONFLICT: R7 step 6 sets Area = -1; R7 step 10 says only DungeonLevel and M59BB change. This header follows step 6.
void carryBodyToTown(Character& who);

// R7 deepest level used to replenish the town: saved deepest if one is saved, else the map's.
// UNKNOWN: sentinel for "none saved" taken as savedDeepest <= 0.
i32 replenishDepth(i32 savedDeepest, i32 mapDeepest);

// R7 replenish the town from the deepest level. Returns false on error; the error is logged and the rescue goes on.
bool replenishTown(Ctx& ctx, i32 deepestLevel);

// R7 the dead do not rest
bool canRest(const Character& who);

// ---- R8 Morgue: own body ----

// R8 Morgue fee guild-pays rule: TopGuildLevel <= 5 and the purse is short of the fee
bool guildPaysRaiseFee(i32 topGuildLevel, Currency purse, Currency fee);

// R8 Morgue fee settlement for a payer. Guild pays: nothing charged. Charged: purse reduced.
// NotEnoughGold: "Not Enough Gold!" is given and nothing happens.
// UNKNOWN: deduction order (goldOnHand before goldInBank) is not stated.
MorgueFeeOutcome settleMorgueFee(Ctx& ctx, Character& payer, Currency fee);

// R8 Morgue fee for a body with the given MaxHits (RaiseCost).
// UNKNOWN: fee formula is not stated in this part; take it from the owning data table.
Currency morgueRaiseCost(i32 maxHits);

// R8/R9 Raising.Resolve: rolls chance on the body. Failure may complicate (complication() from progression2.h).
// Success restores the body. Caller handles the GuildLog entry and city arrival.
// UNKNOWN: whether every failure complicates; this follows the summary (a complicated failed raise).
RaiseResult resolveRaise(Ctx& ctx, Character& body, RaiseKind by, i32 chance, bool ownBody, bool inAParty);

// R8 own-body raise resets natural CON to the race minimum (counter raises do not)
void resetConToRaceMinimum(Ctx& ctx, Character& who);

// R8 the city arrival rests the character
// UNKNOWN: rest effects are not stated in this part; apply the rest rule from its owning module.
void restAtCity(Ctx& ctx, Character& who);

// R8/R11 guild log line for a successful raise
void logResurrection(Ctx& ctx, const Character& who);

// ---- R9 Morgue: counter raise ----

// R9 counter raise refusals: rock-pulled body, or a raise already pending
// CONFLICT 3: rock refusal per SRD R9 step 1.
CounterRaiseRefusal counterRaiseRefusal(const Character& body);

// R9 counter raise chance, percent: clamp(roundHalfEven(35 * (Con / 4.5) + 10), 10, 98)
// CONFLICT 2: engine round_to_int vs doc Int(); this follows the engine's nearest rounding via roundHalfEven.
// cause is not used by the formula as stated (UNKNOWN); it is kept for the RaiseChance signature.
i32 morgueCounterChance(i32 naturalCon, DeathState cause);

// R9 queue a raise (no raise on the spot). The body moves to the raiser's square, level and area.
// Initiate sets chance, RaisedBy and RaiseKind (when chance > 0). Copies the area whatever its value (original quirk).
void initiatePendingRaise(Ctx& ctx, Character& body, const Character& raiser, i32 chance, RaiseKind by);

// ---- R10 Morgue: hire rescuers ----

// R10 typed rescue coordinates: x and y at least 1, level in [1, 15], exactly 3 parts
bool validRescueCoordinates(i32 x, i32 y, i32 level, i32 partCount);

// R10 rescue search cost: trunc(372567.52 * ln(level + 1) * level)
// UNKNOWN: currency units of the result are not stated (assumed 1/10000 gold).
Currency rescueSearchCost(i32 level);

// R10 pay the hire from the party. Returns false on shortfall, which refuses the hire.
// UNKNOWN: which member's purse pays; this uses party.goldLeaderIndex.
bool payRescueHire(Party& party, Currency cost);

// R10 rescuers' return test: returns true when Rnd * 100 > level * 4.
// CONFLICT: source comment says rescuers always return below level 25; the code gives return chance 1 - L/25.
bool rescuersReturn(Ctx& ctx, i32 level);

// R10 CloseEnough: takes 1-based target coordinates (caller adds 1 to the 0-based stored position)
bool closeEnough(i32 rescuerX, i32 rescuerY, i32 rescuerLevel, i32 targetX1, i32 targetY1, i32 targetLevel);

// R10 can the body be brought home. Rock: never. Dead: yes. Living: only at or below 20% of MaxHits.
// UNKNOWN: whether the port implements the 20% living-body test.
bool bodyRecoverable(const Character& who);

// ---- R11 pending raise lifecycle ----

// R11 a body awaits a raise offer when RaiseChance > 0
bool raisePending(const Character& who);

// R11 settle an accepted raise. Failure: clears kind, chance and raiser. Success: clears raiser and chance, keeps kind.
void settleRaiseAccept(Character& who, bool succeeded);

// R11 declining, or a quiet failure, clears chance and raiser
void clearPendingRaise(Character& who);

// R11 Ask to Join refusals: in combat, carrying the body, or no pending raise (needs a Resurrect cast)
AskToJoinRefusal askToJoinRefusal(const Character& body, bool inCombat, bool carryingBody);

// R11 Ask to Join. accepted is the player's Yes. On No the body is left as it stands.
// Does not clear the raise by itself; a rolled raise is settled with settleRaiseAccept.
AskToJoinResult askToJoin(Ctx& ctx, Character& body, bool inCombat, bool carryingBody, bool inAParty, bool accepted);

// R11 a rocked body that is unloaded has its raise wiped (MAINMOD.BAS:419-423, original quirk)
void clearRaiseOnRockedUnload(Character& who);

}  // namespace mordor
