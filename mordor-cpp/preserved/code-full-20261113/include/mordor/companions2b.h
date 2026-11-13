#pragma once
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_party.h"
#include "mordor/companions1a.h"
#include "mordor/companions2a.h"
#include "mordor/combat1b.h"

// Rule ids below are the SRD source chunk tags (cN). Where a tag is cited as
// UNKNOWN, CONFLICT or port-only, the comment says so; grep for those tags.

namespace mordor {


// c2 step cost: cost multiplies current, so binding 0 is free and negative binding multiplies a negative cost.
// Port: a = 12 gives factor 0 (cost 0 at max). Whether buying at 12 is blocked: UNKNOWN.
Currency bindingStepCost(Currency current, i32 bindingLevel);

// c2 chance to leave rises as the companion is hurt; capped at kChanceToLeaveMax.
// UNKNOWN: exact formula (only the cap and the direction are given).
i32 chanceToLeave(const Companion& comp);

// c2 clamp a raw leave chance to [0, kChanceToLeaveMax].
i32 clampChanceToLeave(i32 raw);

// c8 c27 turn chance in percent by creature alignment and master alignment.
// Good creature under Good master: 0 (never turns). Gap <= 1: Evil 75, Neutral 15 (engine).
// Gap of two steps: 100 (docs, c27). Good creature under non-Good master: UNKNOWN.
// Original reads the creature's alignment, not the companion's (c8).
// CONFLICT 11: docs say 3-in-4 and 1-in-7; engine 75 and 15 preferred.
i32 companionTurnPercent(CharAlign creatureAlign, Align masterAlign);

// c8 turn roll using companionTurnPercent.
bool companionTurnRoll(Ctx& ctx, CharAlign creatureAlign, Align masterAlign);

// c8 c27 a kicked-out companion that does not turn leaves with no print. Returns true if it leaves silently.
bool kickOutLeavesSilently(Ctx& ctx, CharAlign creatureAlign, Align masterAlign);

// c5 every companion rolls friendly fire, not only spitters. UNKNOWN: base chance.
bool rollFriendlyFire(Ctx& ctx, const Companion& comp);

// c5 c28 breath stop rule: Fire is never stopped by an earlier breath; Cold is stopped by one.
// Engine formula: bit3 Or (bit4 And not Breathed). CONFLICT 2. Acid and Electrical: UNKNOWN.
bool breathStopped(BreathType type, bool breathedEarlierInRound);

// c28 c64 classify breather power against target power. UNKNOWN: "much stronger" and
// "much weaker" thresholds, ratio or difference (c28 "by more than a quarter").
// FIXED: BreathRelation does not exist in any header; using plain i32 as the relation code.
i32 breathRelation(i32 breatherPower, i32 targetPower);

// c28 c64 breath fire chance in percent: 80, 50 or 20.
// FIXED: parameter type BreathRelation replaced by i32 (no such type in the API index).
i32 breathFireChancePercent(i32 relation);

// c28 fire if the roll is below breathFireChancePercent(relation).
// FIXED: parameter type BreathRelation replaced by i32 (no such type in the API index).
bool breathRollFires(Ctx& ctx, i32 relation);

// c64 special range extra: breath adds a quarter of the range; spit and electrocution add a half.
// UNKNOWN: rounding and how the extra is applied.
i32 specialRangeExtra(SpecialAttackKind kind, i32 range);

// c8 c28 spit slot draw. The draw counts the attacker's companions but indexes the victim's list
// (original). Returns the slot index, or -1 for a miss on an empty slot. PORT: retries capped at kSpitRetryCap.
i32 drawSpitSlot(Ctx& ctx, i32 attackerCompanionCount, const Vec<bool>& victimSlotOccupied);

// c6 c29 c65 " and his companions" suffix. Reset per pass in the caller; names only the last slot visited.
Str companionMishapSuffix(const Companion& lastSlotVisited);

// c28 a fatal mishap ends the routine before its line is printed.
bool mishapEndsPass(const MishapResult& result);

// c6 c28 stone success. CON test reads the stoner's CON against the target.
// UNKNOWN: whose LevelFound the formula subtracts (c6).
bool stoneLands(Ctx& ctx, const Creature& stoner, const Creature& target);

// c28 stone and electrocution follow-ups test Foes(...).Hits > 0 (line 1215), so the next living foe is chosen.
// Returns -1 if none.
i32 nextStoneFoeIndex(const Vec<Creature>& foes, i32 fromIndex);

// c6 c8 stoned companion: its slot is emptied before its carry is read, leaving the carry on a hole (port quirk).
void stoneCompanion(Ctx& ctx, Party& party, Companion& comp);

// c6 c28 c64 electrocution caption. Not inside the caption guard, so the watched character is told even if
// someone else is hit. CONFLICT 7: text "You were accidently electrocuted" (engine) preferred.
void sayElectrocutionCaption(Ctx& ctx, const Character& watched, i32 damage);

// c28 a companion that kills a party member: mdrcharSetDead then Exit Sub. Returns true if the
// owner's remaining companions do not act this round.
bool companionKillEndsOwnerRound(Ctx& ctx, Companion& killer, Character& victim);

// c6 c35 companion blow against a foe. Uses the monster's Att and Def (attackerDef), not per-companion fields.
// No text on an ordinary hit, miss or kill; only special captions print (c8 c64).
void companionBlowFoe(Ctx& ctx, Companion& attacker, Creature& foe, const MonsterDef& attackerDef);

// c28 c35 defence used when a companion blow targets a character. Protection is NOT added (c6 c28).
i32 companionBlowDefence(const Character& target);

// c29 caster level for a companion's spell: guild level / kCasterDivisor (floor).
i32 companionCastLevel(const Companion& comp);

// c29 companion's cast draw. It was missing from all casts until added (c29).
bool companionCastDraw(Ctx& ctx, const Companion& comp);

// c29 heal spells (M5B1D = 0) reach nobody.
bool spellHealsNoOne(i32 m5b1d);

// c8 c30 a carry choice is skipped while a morgue body is carried. Pure query.
bool carryChoiceApplies(const Party& party);

// c8 c30 "dropped" prints before the size test, so it shows even when the new body is kept out.
void announceDropped(Ctx& ctx, const Companion& fallen);

// c8 c30 carry resolution on a companion's death (trigger: Hits reach zero; CONFLICT 10 for join/load).
// Port quirk: with nothing carried and a heavier second candidate, "dropped" uses companion slot -1;
// the error handler ends the sub, so nothing is picked up and the sweep never runs. Repeats each call.
void resolveCarryOnDeath(Ctx& ctx, Party& party, Companion& fallen);

// c30 "Death lets go of every companion and anything carried (sub0930)".
// UNKNOWN: whether this is character death only.
void releaseAllOnDeath(Ctx& ctx, Party& party);

// c28 c51 purchase hits clamp: a rolled figure above the creature definition's M4EAC is clamped down.
i32 clampPurchaseHits(i32 rolledHits, i32 definitionHits);

// c7 c65 joiners from a fight arrive with the front monster's hits and maximum, not their own.
void applyJoinerHitsFromFront(Ctx& ctx, Companion& joiner, const Creature& front);

// c7 only group 0's join offer is shown; multi-group encounters never show an offer.
// c7 companion groups are weighed but their offers are never read.
bool joinOfferVisibleFor(const Encounter& enc, i32 groupIndex);

// c7 join-offer admission: group alignment 1 or matching the viewer. CONFLICT 19: "alignment 1" meaning unconfirmed.
bool joinOfferAdmits(CharAlign groupAlign, Align viewerAlign);

// c91 join chance table, CHA 3-30 and mLvl 1-15. UNKNOWN: only two rows given; table values not supplied.
i32 joinChancePercent(i32 cha, i32 mLvl);

// c91 c82 charm forces a join. CONFLICT: wiki says damage > remaining HP; research proposes potency vs HP.
// UNKNOWN: which is correct and the potency formula. Implement the wiki form pending check.
bool charmForcesJoin(i32 damageInflicted, i32 remainingHits);

// c83 c89 bind spells do not work on companions that joined of their own free will.
// UNKNOWN: whether a bind spell can raise a joiner's -10 bind level.
bool bindSpellApplies(const Companion& comp);

// c27 pen room for a creature of the given alignment. Pens hold only three alignments.
// PORT deviation: the cap of nine is named in a comment but not enforced (c51). CONFLICT 13 on aligned vs unaligned.
bool penHasRoomFor(const Pen& pen, CompanionAlignment align);

// c9 pen files keyed by Id are remapped on load (FromIdKeys).
void penFromIdKeys(Ctx& ctx, Pen& pen);

// c8 a released slot keeps its last occupant's record. The name is blanked only by the character screen
// for the shown character, so this routine leaves the name in place.
void releaseCompanionSlot(Ctx& ctx, Companion& comp);

// c8 c55 drag swap refused if either end is dead.
bool dragSwapBlockedByDeath(const Companion& a, const Companion& b);

// c8 c55 prints "No use re-arranging a dead companion!"
void sayDragSwapRefused(Ctx& ctx);

// c9 c8 an unaligned saved companion (older charm or join code) takes its creature's alignment on load.
// PORT deviation. Unaligned value encoding: UNKNOWN; saved values outside 1..3 are treated as unaligned.
CompanionAlignment alignOnLoad(Ctx& ctx, i32 savedAlign, CharAlign creatureAlign);

// c9 c8 Id 307 is the only shared Id; Unaligned and Good both fall to the Morgul.
bool isSharedMorgulId(i32 monsterId);

// c7 c27 spawn modes 5, 6 and 7 have no monsters in MDATA5. UNKNOWN: exact data field; pure query on GameData.
bool spawnModeInData(const GameData& data, i32 mode);

// c7 the 300-try guard shares a counter with the 150-try reset. The counter resets only per group.
bool spawnTryGuardHit(i32 tries);

// c7 advance the shared spawn try counter by one. m0292 > 10000 is never incremented (c7) and mode 7 reads
// stale static window values (c7); both are port-faithful quirks and are not declared here.
i32 advanceSpawnTries(i32 tries);

// c43 c55 right-click reference alignment; reads kRightClickReferenceAlign (Good) as the port does.
i32 rightClickReferenceAlign();

// c25 c50 raise chance = min(kRaiseCap, kRaiseBase + CON), CON from comp.stats[kRaiseStatIndex].
i32 companionRaiseChance(const Companion& comp);

// c50 gold is taken before the roll and is not refunded. Failure destroys the companion.
// The Raise button hidden-state quirk (c50) is a UI matter and not declared here.
bool payAndRollRaise(Ctx& ctx, Purse& purse, Currency cost, Companion& comp);

// c7 c8 companion group count for the turn check (port: distinct live monster ids). CONFLICT with original count (c43).
i32 distinctLiveCompanionIds(const Vec<Companion>& companions);

// c8 c28 drift floor: content companions drift toward kDriftFloorContent, so full health gives 0.5 (engine).
// CONFLICT 9 (docs: nothing at full health).
f32 companionDriftFloor(const Companion& comp);

// c8 c43 mood drift, once per combat round (engine). CONFLICT 8 (docs: once a second per character).
// UNKNOWN: the state field drifted; Companion has no mood field.
void driftMoodOncePerRound(Ctx& ctx, Companion& comp);

}  // namespace mordor
