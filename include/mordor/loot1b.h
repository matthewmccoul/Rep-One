#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_companion.h"
#include "mordor/state_character.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_item.h"
#include "mordor/state_party.h"

// Part c14p1: chest generation, chest traps, chest gold and gold handling.
// Already declared elsewhere and called, not redeclared: withering() (progression2.h),
// setDead() (status1b.h), sub0930() (status1b.h), identifyFee() and sellPrice() (items1a.h).
// Slime item destruction wraps the SlimeInput/SlimeResult logic of items1b.h.
// Community named laired drops (T14) are unverified and carry no rule, so they are not declared.

namespace mordor {

// ---- Constants and tables ----

inline constexpr i32 kChestClassCount = 4;            // T2 classes 0..3
inline constexpr i32 kChestTrapBitCount = 11;         // T3 bits 0..10 (bit 11 UNKNOWN-2)

// FIXED: ChestClassMod is not in the API index; defined locally because only this file uses it.
// Field meanings are not given in this part, so they are named by position.
struct ChestClassMod {
    f64 goldFactor = 0.0;
    i32 rollAdjust = 0;
    i32 bandAdjust = 0;
    i32 trapChancePercent = 0;
};

// T2 class modifier table, index = ChestClass value.
inline constexpr Arr<ChestClassMod, kChestClassCount> kChestClassMods{
    ChestClassMod{0.5, 0, 0, 0},
    ChestClassMod{0.5, 15, -1, 25},
    ChestClassMod{1.0, 0, 0, 75},
    ChestClassMod{1.5, -25, 0, 90}
};

// T4 item band low end by drop level 1..16 (community tier). Equals
// max(1, roundHalfEven(level/2)); see itemBandLowEnd().
// FIXED: kItemBandLevels is not in the API index; the table has 16 entries (levels 1..16).
inline constexpr Arr<i32, 16> kItemBandLowEnd{
    1, 1, 2, 2, 2, 3, 4, 4, 4, 5, 6, 6, 6, 7, 8, 8
};

// T1 BoxChance thresholds as the rule uses them: BoxChance[1] and BoxChance[2].
struct BoxChanceThresholds {
    i32 chance1 = 0;
    i32 chance2 = 0;
};


// ---- Chest class and treasure generation ----

// T1 bonus added to both thresholds: kLairTreasureBonus if lair monster and BoxChance[2] > 0
i32 treasureClassBonus(bool lairMonster, i32 boxChance2);

// T1/Q3 chest class from BoxChance thresholds; later k overwrites (no break); class 1 never produced
// CONFLICT-2: bonus target (which threshold(s) receive the bonus) as written in the table
ChestClass rollTreasureClass(Ctx& ctx, BoxChanceThresholds boxChance, bool lairMonster);

// T2 gold factor f for a class
double chestGoldFactor(ChestClass cls);

// T2 item roll adjustment for a class
i32 chestItemRollAdjust(ChestClass cls);

// Q2 item band adjustment for a class (class 1 only is -1)
i32 chestItemBandAdjust(ChestClass cls);

// T2 trap chance percent for a class
i32 chestTrapChancePercent(ChestClass cls);

// T2/T3 whether a generated chest is trapped at all (uses chestTrapChancePercent)
bool rollChestTrapped(Ctx& ctx, ChestClass cls);

// Q4 generation pass clears the chest trap only; the lock is never cleared
void beginChestGenerationPass(Ctx& ctx, SquareState& chest);

// Q1 lock roll increase for a magic-locked chest: Log(level+10)*5 (lowers drop chance)
// CONFLICT-21: community notes say the opposite
double magicLockRollIncrease(i32 level);

// T4 item band low end for a drop level; max(1, roundHalfEven(level/2))
i32 itemBandLowEnd(i32 dropLevel);

// T5/Q5 gold depth term exactly as written: Log((LevelFound+1) / Log(2))
// CONFLICT-5: intended form is Log(LevelFound+1)/Log(2); reproduced as written
double goldDepthTermAsWritten(i32 levelFound);

// Q6 group gold under 500 gold is replaced by Int(Rnd*500)+2 gold (2..501 gold)
Currency applyChestGoldFloor(Ctx& ctx, Currency groupGold);

// ---- Chest opening ----

// T6 opening rounds count by disarm (10 -> 0 meaning none set; 60+ -> kMinOpeningRounds)
// UNKNOWN: the Log formula between table rows is not given; table values only
i32 openingRoundsForDisarm(i32 disarm);

// T6 engine turn on which the chest opens for a count (count 2 opens on turn 3)
// CONFLICT-22: count+1 turn index as stated in T6
i32 openingTurnFromCount(i32 count);

// Q27 disarm shown to the player, capped at kDisarmDisplayCap (confidence uses the uncapped value)
i32 capDisarmForDisplay(i32 disarmUncapped);

// Q27 shown-trap test using the uncapped confidence
// UNKNOWN: the roll formula for the shown-trap test is not in this part
bool chestTrapSeen(Ctx& ctx, i32 confidenceUncapped);

// Q20 opener eligibility: original tests DeathState only (not CurHits)
// CONFLICT-20: port refuses CurHits <= 0; preferred behaviour is the original test
bool canOpenChest(const Character& opener);

// Q19/Q48 lock read from the chest's area record
// UNKNOWN-12: whether the current port reads the live fight instead
ChestLock chestLockOf(const SquareState& chest);

// Q29 magic-lock test; gv1D7E never assigned, so the test is a comparison with zero
bool isMagicLockedChest(const SquareState& chest);

// Q43 trap panel visibility from the viewer's own DEX against group 0's monster DEX
// A DEX of zero on either side logs an error and keeps previouslyShown
bool chestTrapPanelShown(Ctx& ctx, const Character& viewer, i32 group0MonsterDex, bool previouslyShown);

// Q43 Open key is allowed only when the trap label is showing
bool openKeyEnabled(bool trapLabelShowing);

// Q41 a disarm awards experience; a chest with no trap gives none
bool disarmAwardsXp(bool chestWasTrapped);

// Q7/Q16/Q40 chest gold is payable only if the chest was opened and its trap did not kill the opener
// UNKNOWN-16: whether a non-leader opener killed by the trap stops payout (CONFLICT-8)
bool chestGoldPayable(bool opened, bool openerKilledByTrap);

// Q47 victory gold is paid at victory only when no chest is present (fixed port behaviour)
bool fightGoldPaidAtVictory(bool chestPresent);

// Q17 explosion leaves the leader at nought: the opener is killed; trap kill is not declared dead on this path
// CONFLICT-17: mdrcharSetDead writes only the guild log line in the original path
void settleExplosionDeaths(Ctx& ctx, Party& party, i32 opener);

// Q37 receiver index for a random party member
// CONFLICT-9: original CInt(Rnd*UBound) weights ends by half; port draws uniformly here
i32 pickRandomReceiver(Ctx& ctx, i32 memberCount);

// ---- Chest traps (T3) ----

// T3 trap flag test; bit index = trap value
bool chestTrapFlagSet(i32 trapFlags, ChestTrap trap);

// T3/Q28 false trap name for the display; draws only from the 11 names (never bit 11)
// UNKNOWN-2: bit 11 has no name
ChestTrap pickFalseTrapName(Ctx& ctx);

// T3 fire damage after resistance: reduce by fireResist/2 percent, then round
// CONFLICT-7: original rounds (MONSMOD.BAS:3530); ChestTraps.cs truncates
i32 fireTrapDamageAfterResist(i32 baseDamage, i32 fireResist);

// T3 chest trap damage roll for parameter k
// UNKNOWN: damage(k) formula is not in this part
i32 chestTrapDamageRoll(Ctx& ctx, i32 k);

// T3/Q11 poison gas test: poisoned when Rnd*100 < Resist(5)/2 (as written)
// CONFLICT-6: polarity inverted relative to the intended test
bool poisonGasCatches(Ctx& ctx, i32 poisonResist);

// T3/UNKNOWN-5 disease test: roll < Resistances[4]/2
bool diseaseCatches(Ctx& ctx, i32 diseaseResist);

// T3 Poison Gas (index 1): each member poisoned on test; Ailments(0) = DungeonLevel
// Manual: poisons all characters near the chest (CONFLICT-27)
void trapPoisonGas(Ctx& ctx, Party& party);

// T3 Disease Spores (index 2): afflicts each member at floor strength
void trapDiseaseSpores(Ctx& ctx, Party& party);

// T3 Exploding Trap (index 3): damage with k = kExplodingTrapK to all in group
void trapExplodingBox(Ctx& ctx, Party& party);

// T3 Fire Trap (index 4): damage with k = kFireTrapK, reduced by Resistances[0] fire resistance
void trapFireBall(Ctx& ctx, Party& party, i32 opener);

// T3 Slime (index 5): may destroy one item in slots 1..39; message "SLIME: [item] Destroyed!"
// Wraps the SlimeInput/SlimeResult logic of items1b.h; returns true if an item was destroyed
bool trapSlime(Ctx& ctx, Character& opener);

// T3 Fate (index 6): rerolls the opener's HP and SP
// UNKNOWN: the HP/SP reroll formula is not in this part
void trapFate(Ctx& ctx, Character& opener);

// T3 Teleporter (index 7): teleports the opener to a random location on the same level; chest becomes untrapped
// UNKNOWN: eligible squares for the random location
void trapTeleportOpener(Ctx& ctx, Character& opener, const FloorState& floor, SquareState& chest);

// T3 Blackout (index 8): removes spell-granted effects and modifiers; keeps equipped-item and natural
// CONFLICT-26: manual says all resistances and modifiers are removed except equipped-item and natural
void trapBlackout(Ctx& ctx, Character& opener);

// T3 Fear (index 9): removes living companions below kBindingMax; message "Your companions were scared away!"
// Returns the number removed
i32 trapFearCompanions(Ctx& ctx, Vec<Companion>& companions);

// T3 Withering (index 10): handled by withering() in progression2.h (adds kWitheringDays)
// CONFLICT-28: message count (4 weeks older message is a third message)

// Q16/Q15 chest trap dispatch; the trap is resolved before any gold or items are paid
// Only the slime and fear messages come from the trap routine itself (plus the withering message via withering())
// FIXED: ChestTrapResult is not declared anywhere; replaced by plain i32 (closest available result type).
i32 resolveChestTrap(Ctx& ctx, ChestTrap trap, Party& party, i32 opener,
                     FloorState& floor, SquareState& chest);

// ---- Chest gold, split and gold handling (T7-T9) ----

// T7/T8 share paid to each member: roundHalfEven(total / shares), whole gold
// Q35 monster gold uses the same even split
Currency splitSharePaid(Currency total, i32 shares);

// T7/Q8 share displayed to the player: round half up, whole gold (differs at a half)
Currency splitShareQuoted(Currency total, i32 shares);

// T7/Q44 whole-gold display string; a negative value that rounds to nothing keeps its sign
// CONFLICT-12: sign rule from VB Format$ documentation, not observed
Str formatWholeGold(Currency amount);

// T7/Q32 thief take when the thief's roll is rolled: CLng(roll) taken; purse negative after take set to 0 where rounding went up
// UNKNOWN: distribution of the thief roll (only the roll-over-purse case is given)
Currency thiefTakeGold(Ctx& ctx, Purse& purse, Currency rolled);

// Q33 give takes CLng(amount) in whole gold; the fraction stays in the giver's purse
Currency giveGold(Ctx& ctx, Purse& giver, Purse& receiver, Currency amount);

// T9/Q34 in-town exchange: takes the amount from hand gold first, shortfall from bank
void exchangeGoldInTown(Ctx& ctx, Purse& wallet, Currency amount);

// T9 withdrawal without a range check (negative amounts allowed)
// UNKNOWN: bank side of the withdrawal is not given; only the hand side is in T9
// FIXED: the original name withdrawGold collided with an existing declaration in mordor::mordor
// (not in the API index). Renamed to withdrawGoldUnchecked to keep this declaration and its meaning.
void withdrawGoldUnchecked(Ctx& ctx, Purse& wallet, Currency amount);

// Q42 gold shown on the exchange panel: purse at DungeonLevel 0, otherwise hand gold
// CONFLICT-18: comment says on hand plus bank in town; code shows purse
Currency shownGold(const Character& ch, i32 dungeonLevel);

// Q8/T7 gold share announcement; the two wordings are the only outputs
// "Your share is N Gold!" or "Your share is N and you recovered M Gold!"
void announceGoldShare(Ctx& ctx, Currency share, Currency recovered, bool recoveredAny);

// ---- Chest contents and charms (T10, T13, T15, Q47) ----

// T10/Q47 a cursed item attaches itself only for cursed kind 2
bool chestItemAttaches(const Item& item);

// Q47 curse attach test (same rule as chestItemAttaches, kind 2 only)
bool curseAttachesItself(const Item& item);

// T13 whether the character holds one of the Charm of Opening items (T13 list)
// UNKNOWN: ItemDef name field is not given in this part; match by item id from GameData
bool holdsCharmOfOpening(const Ctx& ctx, const Character& ch);

// T15 Charm of Opening opens magically locked chests (protective-field entry is not an unlock)
// CONFLICT-30: stat table row conflicts with the stat text; only the unlock effect is declared
bool charmOpensChest(const Ctx& ctx, const Item& item);

}  // namespace mordor
