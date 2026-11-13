#pragma once
#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_creature.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_item.h"
#include "mordor/state_party.h"
#include "mordor/companions1a.h"  // FIXED: MishapKind and SpawnWindow are declared here

namespace mordor {

// Base for starting and bought bindings. R8
inline constexpr i32 kBindingBaseLevel = 5;
// Cap on starting binding: min(5 + monsterFloor, cap). R8
inline constexpr i32 kBindingStartCap = 10;
// Binding cost: Cost = monsterGuildLevel * unit * f * current / divisor. R9
inline constexpr i32 kBindingCostUnit = 1237;
// Upper end of the binding-factor table (R54: f = a + 4 if a + 4 <= 12, else 12 - a).
inline constexpr i32 kBindingFactorMax = 12;
inline constexpr f64 kDriftBoundGuildBase = 3.0;
inline constexpr f64 kDriftBoundTopBase = 12.0;
inline constexpr f64 kDriftBoundGuildScale = 2.0;
inline constexpr f64 kDriftBoundBindingDivisor = 2.0;
inline constexpr i32 kJoinChaScale = 10;
inline constexpr i32 kJoinLevelScale = 10;
inline constexpr i32 kJoinLevelOffset = 1;
// Stone chance: con * (Log(Max(1, con)) / Log(2.5)) - LevelFound, capped. R30
inline constexpr i32 kCompanionStoneCap = 95;
inline constexpr i32 kStoneConFloor = 1;
// Raise chance: min(98, 85 + CON). R42
inline constexpr i32 kCompanionRaiseBase = 85;
inline constexpr i32 kCompanionRaiseCap = 98;
inline constexpr i32 kMishapLevelFloor = 1;
// Breath fire roll percentages by relative strength. R23 [c28]
inline constexpr i32 kBreathFireBase = 50;
inline constexpr i32 kBreathFireStrong = 80;
inline constexpr i32 kBreathFireWeak = 20;
// UNKNOWN: exact comparison for "by more than a quarter" (ratio or difference). [c28]
inline constexpr f64 kBreathLeadRatio = 1.25;
// Companion blow crit bits and bonuses. R28
inline constexpr u32 kSpecialBitCrit10 = 1u << 10;
inline constexpr u32 kSpecialBitCrit11 = 1u << 11;
inline constexpr i32 kCritBit10Bonus = 4;
inline constexpr i32 kCritBit11Bonus = 1;
// Breath gives up after the fifth kill. [c28]
// UNKNOWN: engine constant mc0230 = 4 (R23); docs give fifth kill. Docs used.
inline constexpr i32 kBreathSweepGiveUpKills = 5;
// Companion caster failure percentage. [c2][c65]  CONFLICT R56 (whether companions cast).
inline constexpr i32 kCompanionCastFailPercent = 50;
// Domination charms up to this many creatures in one group. [c83]
inline constexpr i32 kDominationTargets = 4;
// Item 307 split. [c8][c9][c55]
inline constexpr i32 kId307Item = 307;
inline constexpr i32 kId307MorgulRef = 278;
inline constexpr i32 kId307ShadowNinjaRef = 389;
// Companion kills count toward owner's Kills? Engine says no. R55 (engine preferred).
inline constexpr bool kCompanionKillsCountForOwner = false;


// Factor and clamp range for a mishap kind. R24 R57
struct MishapRange {
    f64 factor;
    f64 low;
    f64 high;
};

inline constexpr MishapRange kMishapRanges[3] = {
    {2.0, 2.0, 40.0},   // Breath
    {1.5, 1.0, 40.0},   // Spell (single application of 1.5, R57)
    {1.0, 0.5, 20.0},   // Spit
};

// Result of a mishap roll against a party. R24
struct MishapResult {
    bool happened = false;
    i32 victimIndex = -1;
    f64 chance = 0.0;
};


// ---- inline trivial accessors ----
// R7 binding classes: positive = bound; <= 0 = content (free will); kBindingMax = ForLife.
inline bool isBound(const Companion& c) { return c.bindingLevel > 0; }
inline bool isContent(const Companion& c) { return c.bindingLevel <= 0; }
inline bool isForLife(const Companion& c) { return c.bindingLevel == kBindingMax; }
// R11 binding given to a companion that joined of its own free will.
// FIXED: kFreeWillBinding is declared nowhere; value -10 taken from R10 (joiner enters slot with binding -10).
inline i32 joinerBinding() { return -10; }

// ---- rules ----

// R2 first empty companion slot (Ref == -1); returns 0..3, or -1 when all are used.
i32 firstFreeCompanionSlot(const Arr<Companion, kCompanionSlots>& slots);

// R8 starting binding for a companion met at a monster floor.
i32 startingBinding(i32 monsterFloor);
// R8 binding given to a companion bought from the pen.
i32 boughtBinding(i32 levelFound);

// R54 binding cost factor f from current binding (a = |current|; engine formula).
i32 bindingCostFactor(i32 currentBinding);
// R9 unrounded binding cost.
// UNKNOWN: sign of current in the cost product; magnitude used (see R59 identity of `next`).
f64 bindingCostRaw(i32 monsterGuildLevel, i32 currentBinding);
// R9 charge = round_half_even(binding cost).
Currency bindingCharge(i32 monsterGuildLevel, i32 currentBinding);

// R10 join bar = max(0, Cha * 10 - (LevelFound - 1) * 10).
i32 joinBar(i32 cha, i32 levelFound);
// R10 R13 join offer roll: succeeds when the roll is <= the bar.
bool joinOfferRoll(Ctx& ctx, i32 cha, i32 levelFound);
// [c7] a monster group may offer to join the viewer.
// UNKNOWN: whether "alignment 1" means Neutral on the character scheme (R59); and only-group rule [c91] vs first-group rule [c65].
bool groupMayOffer(const MonsterGroup& group, Align viewer);
// [c91] alignment compatibility for a monster joining a character.
// UNKNOWN: Evil monsters' joining rule not stated; returns false (never joins).
bool monsterJoinsCharacter(CharAlign monsterAlign, Align charAlign);
// R10 R11 joiner enters slot with binding -10; hits taken from the front monster.
// UNKNOWN: joiner hits source per engine (c7), docs cite M4EAC for purchase (no direct conflict).
void acceptJoin(Ctx& ctx, const Creature& front, Companion& slot, i32 ownerIndex);

// R19 true when the pen shelf still has room for another companion.
// CONFLICT: cap not enforced in port (c51); see kPenShelfCap.
bool penHasRoom(i32 shelfCount);
// [c51] price the pen pays for a companion sold by the owner.
// UNKNOWN: pen.PriceToSell formula not supplied (R59); returns 0 until confirmed.
Currency companionSalePrice(const Companion& c);
// [c51] owner sells a companion; clears the slot and credits the seller.
// CONFLICT: sale with handler (c51) vs port selling nothing on overflow; overflow unresolved.
void sellCompanion(Ctx& ctx, Character& seller, Companion& slot);
// R8 R9 buy a companion from the pen into a free slot, charging `price`.
// UNKNOWN: price source (ModItemCost, R59); caller supplies the charge.
bool buyCompanion(Ctx& ctx, Character& buyer, Companion& slot, const Companion& penEntry, Currency price);

// [c83] Bind renews a fading binding.
void bindRenew(Ctx& ctx, Companion& c);
// [c83] Control increases the Bind level.
// UNKNOWN: increase amount not stated in the source.
void bindControl(Ctx& ctx, Companion& c, i32 amount);
// [c83] Domination charms min(kDominationTargets, groupCount) creatures in one group.
i32 dominationCharmCount(i32 groupCount);
// [c89] a charm makes a creature a companion of the caster; returns true when a slot is filled.
// UNKNOWN: charm mechanics are not quantified in the source.
bool charmIntoSlot(Ctx& ctx, Character& caster, Companion& slot, const Creature& target);

// R25 spit share count / (count + 1) for living companions.
f64 spitShare(i32 livingCount);

// R23 breath fire percent: 50 base; 80 if lead foe is much stronger; 20 if much weaker. [c28]
i32 breathFirePercent(i32 leadGuildLevel, i32 companionGuildLevel);
// R23 breath fire roll against breathFirePercent.
bool breathFireRoll(Ctx& ctx, i32 leadGuildLevel, i32 companionGuildLevel);
// R23 breath sweep continues while no survivor stopped it and kills < kBreathSweepGiveUpKills. [c28]
bool breathSweepContinues(i32 killsSoFar, bool survivorStopsIt);

// R24 mishap chance: clamp((999 / max(1, guildLevel)) * (bodies / 20) * factor, low, high).
// Worked check: breath GL 100, bodies 5 gives 4.995 (in range).
f64 mishapChance(MishapKind kind, i32 guildLevel, i32 bodies);
// R24 mishap roll; ownerIndex is the owner (no mishap if the drawn victim is the owner). [c8]
MishapResult rollMishap(Ctx& ctx, const Party& party, i32 ownerIndex, MishapKind kind, i32 guildLevel, i32 bodies);
// [c8] victim draw: round_half_even(rng * (partySize - 1)) + 1.
// UNKNOWN: whether the draw is 0-based or can reach partySize; caller compares to owner.
i32 mishapVictimIndex(Ctx& ctx, i32 partySize);

// R28 crit bonus from blow special bits (bit 10 +4, bit 11 +1).
i32 companionCritBonus(u32 specialPropertyFlags);
// R28 companion blow damage modifier: damMod plus crit bonus.
i32 companionBlowDamageMod(i32 damMod, u32 specialPropertyFlags);

// R29 resistance: after = damage - damage * (resist / 2) / 100.
// UNKNOWN: integer versus real division for odd resist values (engine uses integer form in examples).
i32 resistedCompanionDamage(i32 damage, i32 resistPercent);

// R30 stone chance (percent, capped at kCompanionStoneCap).
// UNKNOWN: whose LevelFound is subtracted (R59); ln is natural log (R59).
f64 companionStoneChance(i32 con, i32 levelFound);
// R30 stone roll.
bool companionStoneRoll(Ctx& ctx, i32 con, i32 levelFound);

// R34 content drift step per tick: max(kDriftContentFloor, (MaxHits - Hits) / MaxHits * 15).
f64 driftContentStep(i32 hits, i32 maxHits);
// R34 bound drift step: max(1, GL / ln(3) * 2 - TopGL / ln(12) * |Binding / 2|).
f64 driftBoundStep(i32 guildLevel, i32 topGuildLevel, i32 bindingLevel);
// R34 apply one drift tick to a companion (content vs bound branch).
// UNKNOWN: drift cadence in port (c8 vs c28); whether drift applies out of combat.
void driftCompanion(Ctx& ctx, Companion& c, i32 topGuildLevel);

// R35 chance to leave clamped to 0..kChanceToLeaveMax.
i32 clampChanceToLeave(i32 x);
// R35 leave roll. ForLife never leaves.
bool companionLeaves(Ctx& ctx, const Companion& c);

// R36 turn check: |creature.Align - owner.Align| > 1.
bool alignmentTurns(CharAlign creatureAlign, Align ownerAlign);
// R36 turn roll. Neutral creatures turn at a measured 15.3% [c27] (UNKNOWN mechanism; see R59).
bool companionTurns(Ctx& ctx, const Creature& creature, Align ownerAlign);
// [c8][c43] companion becomes a monster and joins the fight.
void turnCompanion(Ctx& ctx, Character& owner, Companion& c);
// [c43] owner releases a companion; forced = right-click Attack.
void letCompanionGo(Ctx& ctx, Character& owner, Companion& c, bool forced);
// [c29][c65] owner's order after an empty item.
// CONFLICT: docs (c29) say owner refuses and runs sub102F; test (c65) says Defend or Fight. Docs preferred; sub102F effect unknown.
void ownerOrderAfterEmptyItem(Ctx& ctx, Character& owner, i32 slot);

// R37 carry pickup allowed when Monsters(Ref).Size - Race.Size <= 1.
bool carryPickupAllowed(i32 monsterSize, i32 raceSize);

// R42 raise chance for a companion: min(98, 85 + CON).
i32 companionRaiseChance(i32 con);

// Alignment scheme mapping: character 0/1/2 to item 1/2/3. [c5][c8][c9][c35]
CompanionAlignment itemAlignOf(Align a);
Align charAlignOf(CompanionAlignment a);

// Item 307 creature split. [c8][c9][c55]
i32 id307CreatureRef(ItemAlign alignment);

// Spawn mode window and table entry. [c7]
// UNKNOWN: window for mode 2 (below 0, above 0) is empty as written; modes 4 to 6 keep previous window.
SpawnWindow spawnWindowFor(i32 mode, const SpawnWindow& previous);

// R56 companion caster failure roll (kCompanionCastFailPercent).
// CONFLICT: engine (c5) says companions with spells swing; docs (c29) say they cast. Engine preferred.
bool companionCastFails(Ctx& ctx);

// [c65] sound name for a companion cry: death cry (DI-UNDEA) or attack cry (DRAGON).
Str companionCrySound(bool deathCry);

}  // namespace mordor
