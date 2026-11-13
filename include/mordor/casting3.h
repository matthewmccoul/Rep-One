#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/attributes.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- constants and tables (SRD c09p3)

// R7 constant 1.9 in every cost formula (all sources agree)
inline constexpr double kCostFactor = 1.9;
// R33 engine MaxSP = (INT + WIS) * 5
inline constexpr i32 kMaxSpellPointMultiplier = 5;
// U16 stated spell level cap
inline constexpr i32 kSpellLevelCap = 255;
// R13 engine low-level divisor: VbInt(LevelFound - LevelFound / 3.0)
inline constexpr double kLowLevelDivisor = 3.0;
// R15 peaced casting limit: Combat spells allowed with monsters present but not engaged at level <= 7
inline constexpr i32 kPeacedCastLevelLimit = 7;
// R36 Set Sanctuary is spell level 1 in all guilds
inline constexpr i32 kSetSanctuarySpellLevel = 1;
// R18 protection base for Sorcerer and Wizard
// UNKNOWN: manual and archive say 3; Wizard sheet C = 15 (c79) disagrees; 3 used.
inline constexpr i32 kProtectionBaseSorWiz = 3;
// R29 engine: Life Domination effect value 150 sets ForLife
inline constexpr i32 kBindLifeDomination = 150;
// R29 archive: effect value 150 sets binding to 100
// CONFLICT: archive wording differs from engine; kept for reference, engine preferred.
inline constexpr i32 kBindArchiveBinding = 100;
// R20 Firebolt base level, engine MDATA2 value
// CONFLICT: Help and manual say 2; engine preferred.
inline constexpr i32 kFireboltBaseLevel = 1;
// R21 Blue Flame base level, engine MDATA2 value
// CONFLICT: Help and archive say 4; engine preferred.
inline constexpr i32 kBlueFlameBaseLevel = 3;
// R23 Cause Wounds level, engine MDATA2 value
// CONFLICT: Help and archive say 2; engine preferred.
inline constexpr i32 kCauseWoundsBaseLevel = 1;

// R14/R15 cast scope classification of a spell
enum class CastScope : i32 {
    Combat = 0,
    NonCombat = 1,
    Either = 2
};

// ---- cost formulas (R7 to R11)

// R7 engine cost product before rounding: rated * u4 * rate * 1.9 / SL
// UNKNOWN: rated, u4 and rate are undefined in this part; the text lists rate twice,
//          the quick reference lists it once. Quick reference form implemented.
double costProductEngine(i64 rated, i64 u4, i64 rate, i32 spellLevel);

// R7 spreadsheet cost product: C * Q * R * 1.9 / SL
// CONFLICT: spreadsheets differ from engine and archive; not used by the port.
double costProductSheet(i64 initialSpellLevel, i64 guildMod, i64 ratio, i32 spellLevel);

// R7 archive cost product: StartSpellLevel * SpellPointRatio * BaseMod^2 * 1.9 / SL
double costProductArchive(i64 startSpellLevel, i64 spellPointRatio, i64 baseMod, i32 spellLevel);

// R7 C# cost: spell.Level / characterSpellLevel * spell.Cost * guildMod * 1.9
// UNKNOWN: operand types (integer division or not), U18. Returned as double.
double costProductCSharp(i32 spellLevel, i32 characterSpellLevel, i64 spellCost, i64 guildMod);

// R10 published whole-number rule: Int(product - 0.0001)
i64 costFromProductPublished(double product);

// R10 engine whole-number rule: if product is whole, subtract 1.
// CONFLICT: a zero product would give -1 with no guard; zero ratio costs 0 per c23, c27, c61.
// UNKNOWN: whether Rules.cs guards the zero case (U10). Zero is returned for a zero product.
i64 costFromProductEngine(double product);

// R8 cost minimum by engine rule: clamp up to the smallest accepted rated level
// UNKNOWN: the smallest accepted rated level is not given in this part; caller supplies it.
i64 costAtLeastMinimum(i64 cost, i64 minimum);

// R8 archive cost minimum: minimum = StartSpellLevel * BaseMod
i64 archiveCostMinimum(i64 startSpellLevel, i64 baseMod);

// R11 Set Sanctuary cost: engine ratio-0 cost is 0 (sheet overrides D = E = K = 1 and R = 0 not used)
i64 setSanctuaryCost();

// R9 guild level equivalent of a spell level: Spell level * 2 - 1
// UNKNOWN: whether spreadsheet K is a cost or a level (CONFLICT); this maps a level.
i32 guildLevelEquivalent(i32 spellLevel);

// ---- spell level derivation (R5, R6, R37)

// R37 spell level from guild level: ceil(L / 2). Three formulas agree; ceil form used.
i32 spellLevelFromGuildLevel(i32 guildLevel);

// R6 item spell level for a negative item level: caster's current guild spell level.
// CONFLICT: archive c107 uses the highest spell level across guilds unless the item specifies one; engine preferred.
// UNKNOWN: scope of this rule for non-negative item levels (U32).
i32 itemSpellLevel(i32 itemLevel, i32 casterGuildSpellLevel);

// R5 heal level for a level-0 item, damage route per c6: free ? 1 : Spellbook.SpellLevelFor(actor, s)
// UNKNOWN: whether this route applies to heals (c44 says heals use the port route).
i32 zeroLevelItemSpellLevel(bool free, i32 spellBookSpellLevel);

// R5 port heal level for a level-0 item: SpellLevel(guild.Level), minimum 1
i32 portZeroLevelHealLevel(i32 guildLevel);

// ---- casting gates and charges (R1 to R4, R14 to R16, R25, R31, R32)

// R1 order left after a refused cast.
// CONFLICT: c31 says None (or Fight/Defend on antimagic); c29, c6, c61 say Fight. Fight returned.
CombatOrderKind refusedCastOrder(bool antimagicSquare, bool hadSpell);

// R2 whether the level-0 item gates apply to buffered spells.
// CONFLICT: engine (c1) applies them; audit (c34) skips them. Engine preferred.
bool castGatesApplyToBuffer(i32 itemLevel);

// R3 Ethereal Portal into rock, port behaviour: only the caster dies (ordinary death) and SP is charged.
// CONFLICT: original kills the whole party with rock death and marks the automap (c34); port (c44) preferred.
void etherealPortalIntoRock(Ctx& ctx, Party& party, i32 casterIndex);

// R4 charge for a level-0 item cast: if itemLevel == 0, who.curSpellPoints -= cost.
// CONFLICT: prose says no SP is charged; formula and c43 preferred, so the charge applies.
void chargeLevelZeroItemCast(Ctx& ctx, Character& who, i32 itemLevel, i64 cost);

// R14, R15 combat-state cast restriction.
// Combat spells: allowed with monsters present but not engaged at base level <= 7 (c1, engine).
// NonCombat spells: refused only when engaged (c1, engine).
// CONFLICT: archive c107 restricts Combat to combat and NonCombat to no or peaceful monsters; engine preferred.
bool castAllowedByCombatState(CastScope scope, i32 baseLevel, bool monstersPresent, bool engaged);

// R16 monsters-present definition, engine form: fn0EEF() > 0
// UNKNOWN: whether equivalent to the port form below (U-R16).
bool monstersPresentEngine(i32 monsterCount);

// R16 monsters-present definition, port form: (combat not over) or (waiting count > 0)
bool monstersPresentPort(bool combatOver, i32 waitingCount);

// R25 item spell stat gate: StatWithMods(i) >= Required[i] for every stat.
// UNKNOWN: whether this applies to general spells (U29); engine item gate used.
bool meetsStatMinimums(const Character& c, const Arr<i32, kStatCount>& required);

// R31 whether the cast asks "Who?". Port always asks.
// CONFLICT: original asks only for effects 0 and 18 (c34); port behaviour is a stated deviation.
bool castAsksWho(i32 effectCode);

// R32 cast gate rounding: half-to-even. The list gate uses the exact product (not rounded).
i64 castGateRound(double product);

// R30 charm with no free companion slot: refused before casting when it kills the monster.
// UNKNOWN: whether the last slot can be filled without refusal (c56 disagrees).
bool charmRefusedNoSlot(i32 freeCompanionSlots, bool killsMonster);

// ---- protection and binding (R17, R18, R29)

// R17 protection set at cast time. Port overwrites immediately; original sets Ailments only.
// CONFLICT: a later recompute overwrites in the original (c27); port deviation to verify (c34).
void applyProtectionOnCast(Ctx& ctx, Character& who, i32 protectLevel);

// R18 protection base for a guild. Sorcerer and Wizard use kProtectionBaseSorWiz.
// UNKNOWN: meaning of sheet C (CONFLICT, c74 and c79); other guilds return 0.
i32 protectionBaseForGuild(Guild guild);

// R29 bind effect. Effect value 150 sets ForLife (engine).
// CONFLICT: archive sets binding to kBindArchiveBinding; engine preferred.
void applyBindEffect(Ctx& ctx, Companion& target, i32 effectValue);

// ---- max SP (R33)

// R33 maximum spell points: (INT + WIS) * 5, using statWithMods.
// UNKNOWN: archive gives no formula (c107); engine formula used.
i32 maxSpellPoints(const Character& c);

// ---- monster low level (R13)

// R13 engine low-level value: VbInt(LevelFound - LevelFound / 3.0)
// UNKNOWN: audit form B(2 * floor / 3) (c23) has an undefined B (U4); not declared.
i32 monsterLowLevelEngine(i32 levelFound);

}  // namespace mordor
