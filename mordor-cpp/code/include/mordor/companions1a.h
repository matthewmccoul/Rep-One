#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"

namespace mordor {

struct GameState;  // defined by the game state layer; passed by reference only

// ---- Shared companion types ----

// Per-owner companion slots 0..3 (kCompanionSlotsPerCharacter, see state_companion.h)
using CompanionSlots = Arr<Companion, kCompanionSlotsPerCharacter>;

// R3 / R4 constants
inline constexpr i32 kCompanionId307 = 307;
inline constexpr i32 kId307NeutralRef = 278;  // Morgul
inline constexpr i32 kId307EvilRef = 389;     // Shadow Ninja

// R7 / R11 binding constants (kBindingMax == 100 is ForLife, see state_companion.h)
inline constexpr i32 kFreeWillBinding = -10;

// R8 starting and bought binding
inline constexpr i32 kBindingBase = 5;
inline constexpr i32 kStartBindingCap = 10;
inline constexpr i32 kBoughtBindingCap = 10;

// R9 binding cost
inline constexpr i32 kBindingStep = 4;
inline constexpr i32 kBindingPurchaseMax = 12;
inline constexpr i32 kBindingCostConst = 1237;
inline constexpr i32 kBindingCostDivisor = 4;

// R10 / R13 join offers
inline constexpr i32 kJoinRollMax = 3000;
inline constexpr i32 kJoinBarChaScale = 10;
inline constexpr i32 kJoinBarLevelScale = 10;
inline constexpr i32 kJoinOfferGroupCap = 4;
inline constexpr f64 kVoluntaryJoinDivisor = 300.0;

// R15 subtype draw limits
inline constexpr i32 kSubtypeResetTries = 150;
inline constexpr i32 kSubtypeFailTries = 300;


// R23 breath
inline constexpr i32 kBreathSweepLast = 4;  // sweep index limit; the fifth kill gives up
inline constexpr i32 kBreathChanceStrong = 80;
inline constexpr i32 kBreathChanceWeak = 20;
inline constexpr f64 kBreathLogBase = 1.08;
inline constexpr f64 kBreathSeedDivisor = 15.0;


// R25 spit retry cap (port only)
inline constexpr i32 kSpitRetryCap = 1000;

// R24 mishap kinds: factor and clamp range per kind
enum class MishapKind : i32 {
    Breath = 0,
    Spell = 1,
    Spit = 2
};

struct MishapRow {
    f64 factor;
    f64 low;
    f64 high;
};

inline constexpr MishapRow kMishapRows[3] = {
    {2.0, 2.0, 40.0},   // Breath
    {1.5, 1.0, 40.0},   // Spell
    {1.0, 0.5, 20.0}    // Spit
};

// R19 pen stock. UNKNOWN: pen layout is not given in this SRD part; one shelf per monster and alignment is assumed.
struct PenShelf {
    i32 monsterRef = -1;
    CompanionAlignment alignment = CompanionAlignment::Good;
    i32 stock = 0;
};

struct Pen {
    Vec<PenShelf> shelves{};
};

// R15 result of a subtype draw limit check
struct SubtypePick {
    i32 subtype = 0;
    i32 tries = 0;
    bool failed = false;
};

// R16 spawn window (levels below and above the lead)
struct SpawnWindow {
    i32 below = 0;
    i32 above = 0;
};

// R16 result of a spawn mode lookup
struct SpawnPick {
    SpawnWindow window{};
    i32 tableEntry = -1;   // -1 means no table entry
    bool leadItself = false;
};

// ---- Trivial accessors ----

// R7 bound companion: positive binding, can turn against its owner
inline bool companionBound(const Companion& c) { return c.bindingLevel > 0; }

// R7 ForLife binding (100): never drifts, leaves or is bought
inline bool companionForLifeBinding(const Companion& c) { return c.bindingLevel == kBindingMax; }

// ---- Declarations ----

// R2 first free slot for a new companion; -1 when all four slots are occupied
i32 firstFreeCompanionSlot(const CompanionSlots& slots);

// R3 resolve a Ref to the creature table index that matches monsterId (keeps ref if it still matches)
i32 resolveMonsterRef(const Ctx& ctx, i32 ref, i32 monsterId);

// R3 FromId lookup: one match, or the match with the known alignment, else first match
i32 monsterRefFromId(const Ctx& ctx, i32 monsterId, bool alignmentKnown, CompanionAlignment alignment);

// R3 COMPANION SORTER: repoint a companion whose MonsterId disagrees with its Ref
void resyncCompanionRef(const Ctx& ctx, Companion& c);

// R4 Id 307 resolves by alignment: Neutral Morgul 278, Evil Shadow Ninja 389, Good or Unaligned first Morgul
// UNKNOWN: index of the first Morgul entry for the Good/Unaligned case is not given; looked up from ctx.data
i32 id307Ref(const Ctx& ctx, CompanionAlignment alignment);

// R5 stored alignment (character scheme 0..2) to companion scheme; other values use the creature definition
CompanionAlignment companionAlignmentFromStored(i32 storedAlignment, i32 monsterAlignmentValue);

// R5 AlignmentOf(def): 0 Good, 1 Neutral, 2 Evil, other Neutral
CompanionAlignment alignmentOfDefinition(i32 alignmentValue);

// R6 owner's knowledge level of a creature (CharmIdLevel)
OwnerIdLevel charmIdLevel(const Ctx& ctx, const Character& owner, i32 monsterRef);

// R6 sharpen a companion's IdLevel to the owner's knowledge; never blurs it
void sharpenIdLevel(const Ctx& ctx, const Character& owner, Companion& c);

// R8 starting binding for a joiner or stock creature: min(5 + monsterFloor, 10)
i32 startingBinding(i32 monsterFloor);

// R8 binding given to a companion bought at Confinement: min(10, 5 + LevelFound)
i32 boughtBinding(i32 levelFound);

// R9 binding after one purchase step: |cur| + 4, capped at 12
i32 nextBindingLevel(i32 current);

// R9 cost factor f for the current signed binding (a = |current|)
// UNKNOWN: identity of f with the docs' "next" factor is not confirmed
i32 bindingCostFactor(i32 current);

// R9 charge for raising binding from current: round_half_even(guildLevel * 1237 * f * current / 4)
// UNKNOWN: negative current gives a negative charge, as the SRD states; port behaviour not specified
Currency bindingRaiseCharge(i32 monsterGuildLevel, i32 current);

// R17 pay gold from the character's wallet; false when short
// UNKNOWN: charPay source account (on-hand before bank) is not given in this part
bool payCharacterGold(Ctx& ctx, Character& c, Currency amount);

// R18 raise binding at Confinement by one step, paying bindingRaiseCharge; refuses ForLife; returns true if raised
// UNKNOWN: port blocks buying at binding 12 (cost 0) or not; default here is to allow it
bool raiseBinding(Ctx& ctx, Character& owner, Companion& c);

// R10 non-engaged group may offer: alignment 1 (Neutral) or matches viewer; guild level <= viewer top guild level
// UNKNOWN: "engaged" read as GroupState::Engaged
bool groupMayOfferJoin(const MonsterGroup& group, Align viewerAlign, i32 viewerTopGuildLevel);

// R10 join bar: max(0, Cha*10 - (LevelFound-1)*10)
i32 joinBar(i32 cha, i32 levelFound);

// R10 join offer roll: returns members offered (0 for none); only group 0's offer is ever shown
i32 rollJoinOffer(Ctx& ctx, i32 cha, i32 levelFound, i32 groupCount);

// R11 joiner built from the front monster: copies hits and max hits, BindingLevel -10, owner index set
Companion makeJoinerFromFront(const Creature& front, i32 ownerIndex, i32 slot);

// R11 R12 accept a join offer: adds up to group.joinOffer members while slots and group members remain
// R12 when no slot is free, emits "Join - No room for more Companions!" and the offer stands
// R11 only the first group can join; nothing is reported on success
i32 acceptJoinOffer(Ctx& ctx, const Character& owner, i32 ownerIndex, CompanionSlots& slots, MonsterGroup& group);

// R12 dead joiner is refused while monsters are present
bool joinRefusedDead(const Creature& joiner, bool monstersPresent);

// R13 wiki join roll: rand(1,3000) <= joinBar(cha, mLvl)
// CONFLICT: R13 says monsters offer only when they are the sole group in a room; R10 (engine) takes precedence and this roll ignores group count
bool monsterJoinRoll(Ctx& ctx, i32 cha, i32 mLvl);

// R13 proposed voluntary join probability: max(0, CHA - M + 1) / 300
// UNKNOWN: player level, hostility and group count constraints are not specified
f64 voluntaryJoinProbability(i32 cha, i32 monsterLevel);

// R13 Charm forces a join when inflicted damage exceeds the monster's remaining hits
// UNKNOWN: research instead compares magic potency to remaining target HP; correct quantity not confirmed
bool charmForcesJoin(i64 inflicted, i64 remainingHits);

// R14 monsters that join of their own will are bound at -10
// UNKNOWN: whether a Bind spell can raise a joiner's -10 binding is not stated; no routine declared here

// R15 count a rejected draw: over 150 sets subtype 0; over 300 fails
// The counter is shared between the two limits and resets only per group
SubtypePick rejectSubtypeDraw(SubtypePick pick);

// R16 spawn mode to level window and table entry; mode 7 or negative reuses the previous window
// Mode 3 is the lead itself; modes above 7 use mode - 7 as the table entry
SpawnPick spawnPickForMode(i32 mode, SpawnWindow previous);

// R17 buy a companion at Confinement: needs a free slot, gold and stock at the alignment
// Charges first, then sets the companion to full hits, no name, IdLevel 0, binding boughtBinding(def)
// UNKNOWN: buy price is not given in this SRD part, so it is supplied by the caller
// UNKNOWN: hits on purchase come from the creature definition (M4EAC); the port clamps rolled hits to it
bool buyCompanion(Ctx& ctx, Character& owner, CompanionSlots& slots, Pen& pen,
                  CompanionAlignment alignment, i32 monsterRef, Currency price);

// R19 pen replenish toward kPenShelfCap for each aligned shelf
// UNKNOWN: ShelfCap value; "capped at 9" may refer to this constant
void replenishPen(Ctx& ctx, Pen& pen);

// R19 mdrReplenishStore variant: alignment index is assigned before the test, so the result can differ from replenishPen
void replenishPenStoreVariant(Ctx& ctx, Pen& pen);

// R20 sell the companion in slot for price (from pen.PriceToSell); returns it to pen unless floor 0, releases the slot
// Other companions keep their slots; dead companions are not checked
// UNKNOWN: mod-designated bank account for proceeds; proceeds go to the seller here
bool sellCompanion(Ctx& ctx, Character& seller, CompanionSlots& slots, Pen& pen, i32 slot, Currency price);

// R22 companion takes part in combat: occupied, not dead, hits > 0
bool companionCanAct(const Companion& c);

// R22 count of living companions in slots 0..3
i32 livingCompanionCount(const CompanionSlots& slots);

// R23 breath gate: fire bit, or cold bit while no earlier cold breath (coldBlocked)
// CONFLICT: engine prose says "earlier cold breath"; formula uses the stop flag l0208; formula is followed here
bool companionBreathGuard(bool fireBit, bool coldBit, bool coldBlocked);

// R23 companion breath chance percent: 80 if lead foe GL is more than a quarter above, 20 if more than a quarter below, else 50
// UNKNOWN: "by more than a quarter" as ratio or difference; the in-between case is UNKNOWN in c64, base 50 per c28
i32 companionBreathPercent(i32 companionGuildLevel, i32 leadFoeGuildLevel);

// R23 companion breath: chance roll, mishap check, then sweep of groups; returns true if breath fired
// A fatal mishap ends the routine before its caption
bool companionBreath(Ctx& ctx, GameState& game, Character& owner, Companion& c, bool coldBlocked);

// R23 companion breath range: a = atk + 1; Int((Log(a) + a/15) / Log(1.08) - tgt/10 + 1)
// UNKNOWN: whether tgt/10 is real or integer division


// R23 monster's own breath range (sub0E85): seed = round_half_even(monLvl * Log(monLvl+1))
i32 monsterBreathRange(i32 monLvl, i32 charLevel);

// R24 mishap chance: clamp(999 / max(1, guildLevel) * bodies / 20 * factor, low, high)
f64 mishapChance(MishapKind kind, i32 guildLevel, i32 bodies);

// R24 bodies: every character plus every living companion in slots 0..3
// UNKNOWN: whether dead characters count
i32 countBodies(i32 partySize, i32 livingCompanions);

// R24 mishap roll: rng.nextDouble() * 100 < chance
bool rollMishap(Ctx& ctx, f64 chance);

// R24 victim draw: round_half_even(rng * (partySize - 1)) + 1; caller compares it with owner + 1
i32 drawMishapVictim(Ctx& ctx, i32 partySize);

// R25 spit target slot: lands on a companion with chance count/(count+1), where count is the attacker's living companions
// Port quirk: the slot indexes the victim's list; an empty slot is a miss; retries are capped at kSpitRetryCap (port only)
// Returns the slot, or -1 for the character
i32 drawSpitTarget(Ctx& ctx, const CompanionSlots& attackerSlots, const CompanionSlots& victimSlots);

// R25 spit: off a mishap hits the front of the owner's group, with no sweep
void companionSpit(Ctx& ctx, GameState& game, Character& owner, Companion& c);

// R26 friendly-fire draw made at spit odds before spit; returns true when it kills a party member
// That companion is the owner's last to act this round
bool companionFriendlyFire(Ctx& ctx, GameState& game, Character& owner, Companion& c);

// R22 spell or blow step (mishap uses the Spell row of kMishapRows)
// UNKNOWN: blow mishap factor is not given; the spell row is assumed
void companionSpellOrBlow(Ctx& ctx, GameState& game, Character& owner, Companion& c);

}  // namespace mordor
