#pragma once
// casting1.h: SRD c09p1, magic and casting.
// Covers spell use classification, the cast gate and refusal order, spell level,
// reach, spell cost, spell points, item and buffer casting, monster spells,
// breath, charms, protection, map and utility effects, and seer fees.
// Declarations only. Routines whose source is not engine-verified are marked UNKNOWN or CONFLICT.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"
#include "mordor/state_companion.h"
#include "mordor/state_item.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- Constants ----------------------------------------------------------

inline constexpr i32 kSpellSchoolCount = 16;          // R65 schools Fire 0 .. Opening 15
inline constexpr i32 kSpellClassCount = 18;           // R65 spell classes
inline constexpr i32 kSpellLevelMin = 1;              // R21 clamp floor
inline constexpr i32 kSpellLevelMax = 255;            // R21 clamp ceiling
inline constexpr i32 kPeacedSpellLevel = 7;           // R6 TooLongPeaced threshold (spell level > 7)
inline constexpr i32 kMapSize = 30;                   // R55 30x30 grid
inline constexpr i32 kBitNotRock = 23;                // R56 "not rock" square bit
inline constexpr i32 kNoSchool = -1;                  // R49 no school sampled (attempt cap)
inline constexpr i32 kMonsterSchoolAttemptCap = 1001; // R49 sampling cap (what follows is UNKNOWN)
inline constexpr i32 kMonsterCastSkipPercent = 10;    // R49 flat skip check
inline constexpr i32 kMonsterNonMageSkipPercent = 50; // R49 extra skip check for non-mage classes
inline constexpr i32 kBreathBaseChance = 50;          // R50 percent
inline constexpr i32 kBreathHighChance = 80;          // R50 percent when caster top level is high
inline constexpr i32 kBreathLowChance = 20;           // R50 percent when caster top level is low
inline constexpr i32 kCategoryCharm = 7;              // R65 category id Charm
inline constexpr i32 kCategoryHeal = 9;               // R65 category id Heal
inline constexpr i32 kGroupAffectMin = 6;             // R65 AffectGroup >= 6 is group spell
inline constexpr i32 kItemOnlySpellLevel = 50;        // R67 spells at level 50 are item-only
inline constexpr i32 kFateRollSides = 3;              // R73 unused Int(Rnd * 3) roll
inline constexpr i32 kCastRollSides = 100;            // R12 Int(Rnd * 100)
inline constexpr i32 kEffectWholePartyOffset = 200;   // R64 whole-party variant offset
inline constexpr i32 kEffectPromptId = 18;            // R64 "Cast on Who?" prompt effect
inline constexpr i32 kEffectBindingFirst = 40;        // R64 binding effects 40..42
inline constexpr i32 kEffectBindingLast = 42;
inline constexpr i32 kEffectLifeDomination = 150;     // R64 binding, sets ForLife
inline constexpr i32 kEffectResistFirst = 50;         // R64 "Resists" effects 50..60
inline constexpr i32 kEffectResistLast = 60;
inline constexpr i32 kEffectCharmFirst = 75;          // R64 charm effects 75..99
inline constexpr i32 kEffectCharmLast = 99;
inline constexpr i32 kEffectKillTargets = 100;        // R64 "Kills target(s) or has no effect"
inline constexpr f64 kSeerFeeConstant = 132567.52;    // R63 seer fee constant

// ---- Enums and small result structs -------------------------------------

// R1 spell use class (derived, never stored)
enum class SpellUse : i32 { NonCombat = 0, Combat = 1, Either = 2 };

// R4-R9 cast refusal kinds, in the order the gates test them
enum class CastRefusal : i32 {
    None = 0,
    NotKnown,       // R4 unfit gate, step 1
    SpellLevel,     // R4 unfit gate, step 2
    Stat,           // R4 unfit gate, step 3
    SpellPoints,    // R5 row 1
    NonCombat,      // R5 row 2
    AntiMagic,      // R5 row 3
    CombatOnly,     // R5 row 4a
    TooLongPeaced   // R5 row 4b
};

// R13 whether a cast happens now or becomes the standing order
enum class CastDisposition : i32 { CastNow = 0, BecomesOrder };

// R50 breath attack result
enum class BreathOutcome : i32 {
    NoBreath = 0,      // chance roll missed
    Breathed,          // damage applied
    EndTurn,           // monster level 0 ends the whole turn
    EndGroupTurn       // Log(0) raised, resume at End Sub, skip rest of group turn
};

// R61 what a buffer press does
enum class BufferFire : i32 { Nothing = 0, UseItem, StandingOrder, CastNow };

// R59 timed ailments set by spells
enum class SpellAilmentKind : i32 { Levitate = 0, SightVeil, SeeInvisible, Protection };

// R5-R9 facts the cast gate reads from the square and encounter
struct CastSituation {
    bool monstersPresent = false;  // fn0EEF() > 0 (R6); CastGate flag (R8) is supplied by the caller
    bool engaged = false;          // Foes(0).Engaged (R6)
    bool antiMagicSquare = false;  // ANTIMAGIC or EXTINGUISHER at position (R6, R49.6)
};

// R10 / engine item combat round: which item is casting
struct ItemCastRequest {
    i32 memberIndex = 0;   // party member casting the item
    i32 itemSlot = -1;     // pack slot of the item
    i32 itemIndex = -1;    // item index used in ItemOrders
    i32 itemLevel = 0;     // item spell level; 0 means a level-0 item (R18, R20)
    bool fromBuffer = false;
};

// R49 monster spell pool bounds
struct MonsterSpellBand {
    i32 low = 1;
    i32 top = 1;
};

// R50 breath attack result
struct BreathResult {
    BreathOutcome outcome = BreathOutcome::NoBreath;
    i32 damage = 0;
};

// ---- Rule routines ------------------------------------------------------

// R1 spell use class, derived from raw spell fields each time
SpellUse spellUse(const SpellDef& spell);

// R3 unfit gate applies only to buffers and item spells with item level 0
bool unfitGateApplies(bool fromBufferOrItem, i32 itemLevel);

// R4 unfit gate: Known, SpellLevel, Stat in order; returns None if the caster passes
CastRefusal unfitGateCheck(const Ctx& ctx, const Character& who, const SpellDef& spell);

// R5-R6 refusal gate: SpellPoints, NonCombat, AntiMagic, CombatOnly, TooLongPeaced.
// R14 is part of this gate: an engaged NonCombat spell is refused (no separate routine).
// UNKNOWN: R5 4a "Combat spell" is read as use == Combat (Either passes with no monsters).
CastRefusal checkCastGate(const Character& caster, i32 costSp, SpellUse use, i32 spellLevel,
                          const CastSituation& situation);

// R5 whether a refusal clears the item slot (-1)
bool refusalClearsItemSlot(CastRefusal refusal);

// R7 peaced: monsters on the square but not engaged
bool isPeaced(const CastSituation& situation);

// R8 CastGate monsters flag: (combat not over) or (waiting count > 0)
// UNKNOWN: whether this matches the original test.
bool castGateMonsters(bool combatOver, i32 waitingCount);

// R9 full port check: unfit gate (if it applies), then refusal gate. Cost from castCostSp.
CastRefusal checkCastFull(const Ctx& ctx, const Character& caster, const SpellDef& spell,
                          i32 itemLevel, bool fromBufferOrItem, const CastSituation& situation);

// R10-R11 combat round recheck before any draw. A refusal sets the order to Fight,
// spends no SP and draws no RNG. Item refusals leave the item uncharged (R10).
CastRefusal recheckCastForRound(const Ctx& ctx, Character& caster, CombatOrder& order,
                                const SpellDef& spell, i32 itemLevel, bool fromBufferOrItem,
                                const CastSituation& situation);

// R12 cast roll: one draw after a passing check, before any effect; also drawn outside combat
i32 drawCastRoll(Ctx& ctx);

// R13 BecomesOrder: monsters present and (fightOn or spell is not NonCombat) makes it the standing order
CastDisposition castDisposition(bool monstersPresent, bool fightOn, SpellUse use);

// R15 standing spell order whose caster lacks SP becomes Fight, keeping its target.
// Cost 0 is always affordable. UNKNOWN: a spell order is identified by its spellId.
void enforceStandingOrderSp(CombatOrder& order, i32 curSpellPoints, i32 cost);

// R16 points are taken only after target selection succeeds; a death by teleport spends nothing.
// R17 (raise refusals still cost points) is handled by the caller.
bool spendsCastPoints(bool targetChosen, bool killedByTeleport);

// R16 pay the cast cost and set the cast flag that item charges depend on
void payCastCost(Character& caster, i32 cost);

// R18 cast cost: 0 when the item level is non-zero, otherwise the engine cost (R28)
i32 castCostSp(const Ctx& ctx, const Character& caster, const SpellDef& spell, i32 itemLevel);

// R19 spend one item charge on a completed cast; returns true if a charge was spent
bool spendItemCharge(Item& item, bool castCompleted);

// R10 engine item combat round: on pass, sets party engaged, order, and ItemOrders[member].
// UNKNOWN: the Sub0BDE() call is not described in the chunk and is not reproduced.
CastRefusal beginItemSpellOrder(const Ctx& ctx, Party& party, const ItemCastRequest& request,
                                const SpellDef& spell, const CastSituation& situation);

// R20 spell level for effect resolution.
// CONFLICT: text step 3 (level-0 heal uses guild spell level, min 1) vs pseudo-code (free → 1).
// Implementation follows the text.
i32 effectSpellLevel(const Ctx& ctx, const Character& caster, const SpellDef& spell,
                     i32 itemLevel, bool isHeal, bool free);

// R21 spell level from guild level: clamp(floor(guildLevel / 2 + 0.5), 1, 255)
i32 guildSpellLevel(i32 guildLevel);

// R22 caster's spell level for a spell: highest SL over joined guilds with rate > 0 and ceiling >= level
i32 casterSpellLevel(const Ctx& ctx, const Character& caster, const SpellDef& spell);

// R23 per-guild reach test: spellLevel * rate <= guildLevel / 2 + 0.5 (exact comparison)
bool guildReachesSpell(i32 spellLevel, f32 rate, i32 guildLevel);

// R23 a spell is within reach when some joined guild (Level > 0) has rate > 0, a ceiling
// reaching the level, and passes guildReachesSpell
bool spellWithinReach(const Ctx& ctx, const Character& caster, const SpellDef& spell);

// R23 ids of all spells within reach for this character
Vec<i32> listReachableSpellIds(const Ctx& ctx, const Character& caster);

// R24 announce spells that became reachable between two states, once, under a header line
void announceNewlyReachable(Ctx& ctx, const Character& before, const Character& after);

// R25 spell level shown in the list: floor(spellLevel * rate), exact product
i32 shownSpellLevel(i32 spellLevel, f32 rate);

// R26 teaching guild index: reaching guild with the highest spell level, first wins ties; 0 if none
i32 teachingGuild(const Ctx& ctx, const Character& caster, const SpellDef& spell);

// R28 rated spell level: round_half_even(level * r), r = rate == 0 ? 1 : rate
i32 ratedSpellLevel(i32 spellLevel, f32 rate);

// R28 per-guild price: Int((rated / SL) * costRatio * r * 1.9), minus 1 if the exact product is a positive whole number
i32 spellPriceForGuild(i32 spellLevel, f32 rate, i32 costRatio, i32 guildLevel);

// R28 engine cost: cheapest accepted guild sets the cost, clamped up to the smallest accepted rated level.
// Ratio-0 spells give 0 (fn1485).
// CONFLICT: published test page gives -1 for ratio 0 (see spellCostPublished).
i32 spellCostEngine(const Ctx& ctx, const Character& caster, const SpellDef& spell);

// R29 published test-page cost: Int(rated / SL * ratio * rate * 1.9 - 0.0001). Not used by the port.
i32 spellCostPublished(i32 spellLevel, i32 costRatio, f32 rate, i32 guildLevel);

// R31 archive spell cost: floor(start * ratio * baseMod^2 * 1.9 / SL - 0.0001). Not engine-verified.
i32 archiveSpellCost(i32 startSpellLevel, f32 spellPointRatio, f32 baseMod, i32 sl);

// R31 archive minimum cost: startSpellLevel * baseMod. Not engine-verified.
i32 archiveMinimumCost(i32 startSpellLevel, f32 baseMod);

// R31/R32 archive initial cost: floor(ratio * baseMod * 1.9 - 0.0001). Assumes SL = start * baseMod.
i32 archiveInitialCost(f32 spellPointRatio, f32 baseMod);

// R31 spell level needed for a desired cost: ceil(start * ratio * baseMod^2 * 1.9 / (desired + 1))
i32 archiveLevelForDesiredCost(i32 startSpellLevel, f32 spellPointRatio, f32 baseMod, i32 desiredCost);

// R31 guild level equivalent of a spell level: spellLevel * 2 - 1
i32 archiveGuildLevelEquivalent(i32 spellLevel);

// R33 spreadsheet common core D (initial cost, not scaled by SL): floor(Q * R * 1.9 - 0.0001)
i32 sheetCommonInitialCost(f32 guildMod, f32 scRatio);

// R33 spreadsheet common core E: max(floor(C * Q * R * 1.9 / SL - 0.0001), C).
// Set Sanctuary is hardcoded in the sheets and does not use this formula (R33 quirk).
i32 sheetCommonCost(i32 initialLevel, f32 guildMod, f32 scRatio, i32 sl);

// R33 spreadsheet common core K (minimum cost at guild level): ceil(C * Q * R * 1.9 / (C + 1)) * 2 - 1
i32 sheetCommonMinCost(i32 initialLevel, f32 guildMod, f32 scRatio);

// R34 Sorceror I: G + G * ln(C2) / ln(5) + INT((C2 / 10) * (H - 1)). INT truncates toward zero.
f64 sorcererFormulaI(i32 base, i32 power, f64 c2);

// R34 Sorceror J: as I with ln(10). J grows more slowly than I for C2 > 1.
f64 sorcererFormulaJ(i32 base, i32 power, f64 c2);

// R35 Charm of Opening sheet cost: raw = floor(E4 * SL_init^2 * 1.9 / C2 - 0.0001); SL_init if raw < SL_init
i32 charmOpeningSheetCost(i32 slInit, f32 scRatio, f64 c2);

// R35 Charm of Opening initial cost: floor(E4 * SL_init * 1.9 - 0.0001)
i32 charmOpeningInitialCost(i32 slInit, f32 scRatio);

// R35 Charm of Opening min cost: ceil(E4 * SL_init^2 * 1.9 / (SL_init + 1)) * 2 - 1
// Uses only SL_init, not the current SL.
i32 charmOpeningMinCost(i32 slInit, f32 scRatio);

// R36 archive availability gate: in a guild's spellbook, SL reaches learning level, stats met.
// UNKNOWN: learning levels are not reconciled with R23 (R38).
bool archiveSpellAvailable(const Ctx& ctx, const Character& caster, const SpellDef& spell);

// R39 effectiveness uses the teaching guild's spell level and the lowest cost among guilds.
// Covered by casterSpellLevel (R22) and spellCostEngine (R28); no separate routine.

// R40 archive COMBAT/NON-COMBAT restriction. Not used; the cast gate (R5/R6) applies.
// CONFLICT: archive says "peaceful" is undefined and compares against R6.

// R42 archive item spell level: itemLevel if positive, else highest guild spell level across all guilds.
// CONFLICT: R20 governs; this is the archive variant.
i32 archiveItemSpellLevel(const Character& who, i32 itemLevel);

// R45 spell points fully restored on return to town
void restoreSpellPointsInTown(Character& who);

// R46 natural max SP: ((INT * 10) / 2) + ((WIS * 10) / 2), natural unmodified stats
i32 naturalMaxSpellPoints(const Abilities& abilities);

// R47 recalculate max SP; refill sets cur = max, otherwise cur = min(cur, max)
void recalcSpellPoints(Character& who, bool refill);

// R49.2 spell level band for monster pool: low = max(1, VbInt(level - level/3.0)), top = level + 1
// UNKNOWN: the c23 form low = max(1, B(2*floor/3)) uses an undefined B.
MonsterSpellBand monsterSpellBand(i32 levelFound);

// R49.6 antimagic and extinguisher squares suppress hostile casting
bool hostileCastingSuppressed(const CastSituation& situation);

// R49.3-4 monster skip rolls: flat 10% check, then 50% for non-mage classes. True means cast nothing.
bool monsterSpellSkipped(Ctx& ctx, bool isMageClass);

// R49.1 sample a school from the spell flag bits until one is present.
// Returns kNoSchool when the 1001-attempt cap is hit.
// UNKNOWN: what follows the cap.
i32 pickMonsterSpellSchool(Ctx& ctx, u32 spellFlagMask);

// R49.2 pool of spell ids in a school whose level lies in [band.low, band.top]
Vec<i32> monsterSpellPool(const Ctx& ctx, i32 school, MonsterSpellBand band);

// R49.5 draw one record from the pool. Returns its spell id, or -1 if rejected
// (requires Damage2 > 0 and SpellEffect > 0).
// Original quirk: the pick sound uses the pool index, not the spell id.
i32 drawMonsterSpell(Ctx& ctx, const Vec<i32>& pool);

// R49.7 a selected monster spell replaces the accumulated damage, hits up to AffectGroup + 1
// leading party slots, and ends the loop even at zero damage. Returns the spell damage applied.
// UNKNOWN: "companion slots" are read as party member slots.
i32 applyMonsterSpellHits(Ctx& ctx, Party& party, const SpellDef& spell, i32 accumulatedDamage);

// R50.1 breath chance percent: 50, raised to 80 if top > monLvl * 1.25, lowered to 20 if top < monLvl * 0.75
i32 breathChancePercent(i32 topGuildLevel, i32 monsterLevel);

// R50.2-3 breath range: l = VbInt(monLvl * ln(monLvl + 1));
// range = Int(((ln(l) + l / 15.0) / ln(1.08) - casterGuildLevel / 20.0) + 1).
// Returns false when ln(0) would be raised (EndGroupTurn). Caller handles level 0.
bool breathDamageRange(i32 monsterLevel, i32 casterGuildLevel, i32& outRange);

// R50 breath attack: chance roll, then damage. Monster level 0 ends the whole turn.
// UNKNOWN: the chance roll form (percent check) is not shown in the chunk.
BreathResult breathAttack(Ctx& ctx, i32 monsterLevel, i32 topGuildLevel, i32 casterGuildLevel);

// R77 charm works on any monster whose damage would have killed it
bool charmWouldKill(i32 damage, i32 targetHits);

// R51.1 a charm that kills with no free companion slot is refused before casting
bool charmRefusedNoSlot(bool wouldKill, i32 freeCompanionSlots);

// R51.3 charm test: effect in 75..99 with index effect - 75 matching the target's monster class
bool charmEffectMatches(i32 effect, i32 monsterClass);

// R51.2 companion carrying the monster's hits before the spell, its max hits, and the spell level as binding level
Companion makeCharmCompanion(const Creature& target, i32 spellLevel);

// R51.4 charm fills the last slot: charmed creature leaves, next creature felled,
// the one after stands at nought (Hits 0, not unrolled). UNKNOWN: exact member indexing.
void charmLastSlotFallout(Encounter& encounter, i32 groupIndex);

// R52 charm difficulty: Round((30 + levelFound) * ln((guildLevel + 5) / 10.0 + 1)); if !p036E, Round(d * 1.5)
// UNKNOWN: meaning of the p036E flag.
i32 charmDifficulty(i32 levelFound, i32 guildLevel, bool p036E);

// R51.5 / R52 charm identify level: gap = Round(identifySkill - difficulty);
// gap >= 0 → 0, gap <= -15 → 3, gap <= -5 → 2, else 1.
// identifySkillValue is the result of identifySkill(topGuildLevel, Int, Wis) from guilds1.h.
// Known-threshold m0088 is never set, so it is 0.
i32 charmIdLevel(i32 levelFound, i32 guildLevel, bool p036E, i32 identifySkillValue);

// R53 bind level: round_half_even(spellLevel + ln((10 + casterSpellLevel) / 5.0))
i32 bindLevel(i32 spellLevel, i32 casterSpellLevel);

// R53 bind has no effect when the companion's binding level is >= level. Points are still spent.
bool bindHasNoEffect(const Companion& comp, i32 level);

// R54 protection level on cast: Int(casterSpellLevel / 2) + 1
i32 protectLevelOnCast(i32 casterSpellLevel);

// R54 protection level on recompute: Int(guildLevel / 2) + 1
i32 protectLevelOnRecompute(i32 guildLevel);

// R54 protection defence: Log(p) * (Log(p) * 3), p = slot value
f64 protectionDefence(f64 slotValue);

// R54 cast Protection: sets Ailments.protectLevel from the caster spell level
// Original quirk: a later recompute overwrites it (R54 carried deliberately).
void castProtection(Character& who, i32 casterSpellLevel);

// R54 recompute protection level from the active guild level. Runs on equip/unequip,
// antimagic squares (MAZEMOD.BAS:1749) and the Blackout chest trap (MONSMOD.BAS:3609).
void recomputeProtection(Character& who);

// R55 wrap a coordinate on the map: v < 0 → size - 1; v >= size → 0; else v
i32 wrapMapCoord(i32 v, i32 size);

// R55 move one square in the facing direction, wrapping at the map edge.
// UNKNOWN: axis and sign per facing are not stated in the chunk.
void stepFacing(Location& where, Facing facing, i32 size);

// R55 Ethereal Portal and Passwall. If the target is solid rock, the party dies (port: caster only, see
// UNKNOWN in source). Otherwise the spell is spent, the caster moves one square and arrives via
// ArriveBySpell (party.arrivedBySpell). Returns true if the move happened.
bool movePassThrough(Ctx& ctx, Party& party, i32 casterIndex, bool targetIsRock, i32 size);

// R56 Detect Rock: marks the rock bit ahead unless the party is lost. If lost, marks kBitNotRock.
// UNKNOWN: square index "ahead" and rock bit index are not given in the chunk.
void detectRockAhead(FloorState& floor, const Location& where, bool lost);

// R57 Depth Perception: does not clear LostLevel or LostFacing before sub0848 and LookAround
void applyDepthPerception(Ctx& ctx, Party& party);

// R57 Find Direction: does not clear LostLevel or LostFacing before sub0848 and LookAround
void applyFindDirection(Ctx& ctx, Party& party);

// R59 Levitate, Sight Veil, See Invisible, Protection set Ailments(n) and the bit.
// UNKNOWN: which ailment bit Sight Veil sets.
void setSpellAilment(Ailments& ailments, SpellAilmentKind kind, i32 level);

// R60 Teleport and Cure Paralysis with monsters present go to the combat branch in the original
// (the port casts on the spot). Teleport and Cure Paralysis are exactly the Either class (R2).
bool routesToCombatBranch(bool monstersPresent, SpellUse use);

// R61 buffer fire decision. Item buffer: packSlot > 0 with an empty or out-of-range slot does nothing.
// Spell buffer: standing order if monsters are in front, else cast now.
// CONFLICT: whether buffer casts pass the cast checks (audit source vs c1, c34).
BufferFire bufferFireDecision(bool isSpellBuffer, i32 packSlot, bool itemSlotEmptyOrOutOfRange,
                              bool monstersInFront);

// R62 best party chance over all members, alive or dead: Rules.BestAbilityChance(m, 5, level, 0).
// UNKNOWN: meaning of the abilityIndex, level and extra arguments.
f32 bestPartyAbilityChance(const Party& party, i32 abilityIndex, i32 level, i32 extra);

// R62 an identified square is written to the automap; already-mapped squares say nothing
void perceiveSquare(Ctx& ctx, SquareState& square);

// R62 look-ahead runs after every redraw unless NoPerceptionMessages is set or the character is lost
bool lookAheadEnabled(bool noPerceptionMessages, bool lost);

// R63 random seer fee in whole gold: (fee + Rnd * fee / 4) / 2 with fee = 132567.52 * ln(level + 1) * level
i64 seerFee(Ctx& ctx, i32 level);

// R63 fixed person-locate fee in whole gold: (long)(132567.52 * ln(level + 1) * level)
i64 seerLocateCharacterFee(i32 level);

// R64 whole-party variant: effect >= 200
bool isWholePartyEffect(i32 rawEffect);

// R64 real effect id: effect - 200 for whole-party variants, else the effect
i32 realSpellEffect(i32 rawEffect);

// R64 / R65 binding spell: effects 40..42 or 150 (Life Domination sets ForLife)


// R64 charm effect: effects 75..99
bool isCharmEffect(i32 effect);

// R64 resist effect: effects 50..60, indexed by effect - 50
bool isResistEffect(i32 effect);

// R65 group spell: AffectGroup >= 6
bool isGroupSpell(i32 affectGroup);

// R65 offensive exclusion by category name: Healing, Resist, Movement, Sight, Opening, Control, Charm.
// UNKNOWN: category index to name mapping is not in the chunk.
bool isOffensiveCategory(const Ctx& ctx, i32 category);

// R66 consumable item types: Potion, Scroll, Tome, Dust, Crystal, Rod, Sphere, Cube
bool isConsumableItem(ItemType type);

// R66 a consumable is removed when empty (charges == 0)
bool itemRemovedWhenEmpty(const Item& item);

// R67 spells at level 50 are item-only
bool isItemOnlySpellLevel(i32 spellLevel);

// R67 highest spell ceiling for a category over guilds with HasSpellTable
i32 spellCeiling(const Ctx& ctx, i32 category);

// R67 a spell is unreachable when its level exceeds the category ceiling
bool spellUnreachable(const Ctx& ctx, const SpellDef& spell);

// R71 cast-target choices: party member indices only (0 .. size - 1), never companions
Vec<i32> castTargetMembers(const Party& party);

// R73 Fate: rolls an unused Int(Rnd * 3), rerolls hits and SP, names the first party character.
// SP is taken after the effect. UNKNOWN: whose hits and SP are rerolled.
void applyFate(Ctx& ctx, Party& party, i32 casterIndex);

// R78 wand strength comes from the item level, not the user's level
i32 wandStrength(i32 itemLevel);

// R86 wiki cost: (int)(level / characterSpellLevel * baseCost * guildMod * 1.9 - 0.0001).
// guildMod 0 becomes 1; characterSpellLevel 0 becomes spellLevel.
// UNKNOWN: operand types (integer division would truncate); not confirmed to match the port.
i32 wikiSpellCost(i32 spellLevel, i32 characterSpellLevel, i32 baseCost, f32 guildMod);

// R86 wiki initial cost: wikiSpellCost with characterSpellLevel = 0
i32 wikiInitialCost(i32 spellLevel, i32 baseCost, f32 guildMod);

// Quirks and deviations R14, R17, R44, R58, R68, R69, R70, R72, R74-R76, R79-R85 are
// covered by the routines above or are UI and data notes; no separate routines are declared.

}  // namespace mordor
