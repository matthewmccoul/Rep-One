#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/state_party.h"
#include "mordor/casting1.h"  // FIXED: SpellUse, CastRefusal, CastSituation and MonsterSpellBand are declared in casting1.h, which was not included.

namespace mordor {


struct SpellChargeResult {           // c21 item charge spend result
    bool removed = false;            // consumable removed when empty (R66)
    i32 chargesLeft = 0;
};

struct PortalStep {                  // c44 Ethereal Portal planned step
    i32 x = 0;
    i32 y = 0;
    bool intoRock = false;
};

// ---- Rule routines ----

// c1 Spell use class from raw effect, Damage2 and category
SpellUse spellUseClass(i32 rawEffect, i32 damage2, i32 category);

// c1 Level-0 gate a: spell known by some guild (Level > 0, rate > 0, SpellMax >= spell level)
// FIXED: GuildSpellStanding does not exist; using closest existing type GuildDef.
bool spellKnownByAnyGuild(const Vec<GuildDef>& guilds, i32 spellLevel);

// c1 Level-0 gate b: round_even(spellLevel * rate) <= floor(GL/2 + 0.5) for some guild
// FIXED: GuildSpellStanding does not exist; using closest existing type GuildDef.
bool spellLevelOkForAnyGuild(const Vec<GuildDef>& guilds, i32 spellLevel);

// c1 Level-0 gate c: StatWithMods(i) >= Required[i] for all i (UNKNOWN: manual says unclear; engine treats as minimums)
bool meetsRequiredStats(const Character& who, const Arr<i32, kStatCount>& required);

// c1 Level-0 item spell gate in order a, b, c; returns None if all pass
// FIXED: SpellRefusalKind does not exist; using closest existing type CastRefusal.
// FIXED: GuildSpellStanding does not exist; using closest existing type GuildDef.
CastRefusal levelZeroItemGate(const Character& who,
                              const Vec<GuildDef>& guilds,
                              i32 spellLevel,
                              const Arr<i32, kStatCount>& required);

// c1 Cast refusal order: SpellPoints, NonCombat, AntiMagic, CombatOnly, TooLongPeaced
// FIXED: SpellRefusalKind does not exist; using closest existing type CastRefusal.
// FIXED: CastCheckInputs does not exist; using closest existing type CastSituation.
CastRefusal firstCastRefusal(const CastSituation& in);


// c46 NoSpellsHere = Has(ANTIMAGIC) OR Has(EXTINGUISHER)
bool castBlockedBySquare(bool antimagic, bool extinguisher);

// c46 AntiMagicHere = Has(ANTIMAGIC)
bool antiMagicHere(bool antimagic);

// c58 Casting from the character window in town is refused
bool castBlockedInTown(bool inTown);

// c106 SL = clamp(vbInt(GL / 2 + 0.5), 1, 255)
i32 spellLevelFromGuildLevel(i32 guildLevel);

// c18 Rated level = roundHalfEven(level * rate)
i32 ratedSpellLevel(i32 spellLevel, double rate);

// c21 Reach: rate > 0 and Level * rate <= GL / 2 + 0.5
// CONFLICT: c1 gate uses roundHalfEven(level*rate); c21 uses exact product. Port uses exact; preferred c21.
bool levelReachesSpell(i32 guildLevel, double rate, i32 spellLevel);

// c21 Teaching guild: reaching guild with highest spell level; guild 0 if none
// UNKNOWN: tie-break between equal spell levels (first in list assumed)
// FIXED: GuildReach does not exist; using closest existing type GuildDef.
i32 teachingGuildIndex(const Vec<GuildDef>& reaches);

// c62 Spell unreachable if spellLevel > ceiling
bool spellUnreachable(i32 spellLevel, i32 ceiling);

// c62 Ceiling = max SpellMax over guilds with HasSpellTable
// FIXED: GuildSpellStanding does not exist; using closest existing type GuildDef.
i32 spellCeilingAcrossGuilds(const Vec<GuildDef>& guilds);

// c49 Within reach: some joined guild can learn the spell
// FIXED: GuildSpellStanding does not exist; using closest existing type GuildDef.
bool spellWithinReachOfAnyGuild(const Vec<GuildDef>& guilds, i32 spellLevel);

// c49 Announce newly within-reach spells on level change
void announceReachableSpells(Ctx& ctx, const Character& who, const Vec<i32>& spellIds);

// c18 Pricing rate: 0 is treated as 1
double pricingRate(double rate);

// c18 Engine cost: Int((rated / SL) * u4 * r * 1.9), minus 1 if exact product is a whole positive number
// UNKNOWN: u4 and rate are Single in the original; double used here.
i32 engineSpellCost(i32 ratedLevel, i32 spellLevel, double u4, double rate);

// c61 Published cost: Int(rated/SL * ratio * rate * 1.9 - 0.0001)
// CONFLICT: c61 epsilon form differs from c18 engine form at exact integers (e.g. 209 vs 208). Preferred: engine (c18).
i32 publishedSpellCost(i32 ratedLevel, i32 spellLevel, double ratio, double rate);

// c18 Cheapest accepted guild wins
// UNKNOWN: tie-break between equal costs (first in list assumed)
// FIXED: SpellCostPick does not exist; the routine yields a guild index, so using i32.
// FIXED: GuildCostCandidate does not exist; using closest existing type GuildDef.
i32 pickCheapestAcceptedGuild(const Vec<GuildDef>& candidates);

// c43 Item spells with non-zero item level cost 0 SP
i32 itemSpellCostPoints(i32 itemLevel, i32 engineCost);

// c21 Castable: charges > 0 and SpellIndex in 1..spellTableCount
bool itemIsCastable(const Item& item, i32 spellTableCount);

// c21 Spell slot in the spell table: SpellIndex - 1
i32 itemSpellSlot(const Item& item);

// c21 R66 consumable item types: Potion, Scroll, Tome, Dust, Crystal, Rod, Sphere, Cube
bool itemIsConsumable(ItemType type);

// c21 Spend one charge; item removed when empty; called only on a completed cast
SpellChargeResult spendItemCharge(Ctx& ctx, Item& item);

// c14 Monster spell pool: top = LevelFound + 1; low = max(1, vbInt(LevelFound - LevelFound / 3.0))
// CONFLICT: c23 gives low = max(1, B(2*floor/3)); preferred c14.
// FIXED: MonsterSpellPool does not exist; the routine computes a top/low range, so using the existing MonsterSpellBand.
MonsterSpellBand monsterSpellPool(i32 levelFound);

// c14 Accept a monster spell when Damage2 > 0 and SpellEffect > 0
bool monsterSpellAccepted(i32 damage2, i32 spellEffect);

// c14 Monster no-cast rolls: flat 10%, then 50% if not a mage
// UNKNOWN: order of the two rolls in the original
bool rollMonsterNoCast(Ctx& ctx, bool casterIsMage);

// c14 Breath chance: 50 base; 80 if top > monLvl*1.25; 20 if top < monLvl*0.75
// Quirk: Log(0) errors in the original; this routine assumes monsterLevel > 0.
i32 breathChancePercent(i32 top, i32 monsterLevel);

// c14 Breath roll using breathChancePercent
bool rollBreath(Ctx& ctx, i32 top, i32 monsterLevel);

// c27 Protection set by cast: Int(SL / 2) + 1
i32 protectionFromCast(i32 spellLevel);

// c27 Protection recompute: Int(GuildLevel / 2) + 1
i32 protectionRecompute(i32 guildLevel);

// c27 Apply protection at cast time (the later recompute overwrites it, carried deliberately)
void setProtectionOnCast(Ctx& ctx, Character& who, i32 spellLevel);

// c27 Recompute protection from the active guild level
void recomputeProtection(Ctx& ctx, Character& who);

// c21 Max SP = (natural INT + natural WIS) * 5
i32 maxSpellPoints(i32 naturalInt, i32 naturalWis);

// c20 Seer fee: 132567.52 * Log(level + 1) * level (natural log)
// UNKNOWN: whether result is in Currency units (1/10000 gold) or whole gold; rounding mode not stated.
Currency seerFee(i32 level);

// c44 Coordinate wrap for the 30 x 30 grid
// UNKNOWN: exact semantics of Wrap(v, size); positive modulo assumed
i32 wrapCoordinate(i32 v, i32 size);

// c44 Plan one Ethereal Portal step; caller supplies whether the target square is rock
PortalStep planEtherealPortalStep(i32 x, i32 y, i32 dx, i32 dy, bool targetIsRock);

// c44 Portal into rock
// CONFLICT: c44 original kills the whole party; c34 port kills only the caster. Preferred: whole party.
// UNKNOWN: the original's repeated-death loop for casterSlot >= 2 is not reproduced.
void etherealPortalIntoRock(Ctx& ctx, Party& party, i32 casterSlot);

// c13 Party-wide effect: Effect >= 200
bool isPartyWide(i32 effect);

// c13 Real effect of a party-wide variant: Effect - 200
i32 realEffect(i32 effect);

// c13 ResistIndex for effects 50..60 (effect - 50); -1 for none
// UNKNOWN: index for mass resistances beyond the list.
i32 resistIndexForEffect(i32 effect);

// c21 Binding effects: 40..42 or 150
bool isBindingEffect(i32 effect);

// c21 Apply binding effect to a companion (150 sets binding to 100 and ForLife)
// UNKNOWN: increment size for effects 40..42.


// c56 Charm effects: 75..99
bool isCharmEffect(i32 effect);

// c56 Charm index = Effect - 75
i32 charmIndex(i32 effect);

// c56 Charm matches target monster class
// UNKNOWN: mapping from charm index to monster class id not in this chunk.
bool charmMatchesMonsterClass(i32 effect, i32 monsterClass);

// c21 Group spell: AffectGroup >= 6
bool isGroupSpell(i32 affectGroup);

// c56 Charm applies when damage would have killed the target
// UNKNOWN: >= vs > at equality.
bool damageWouldKill(i32 damage, i32 curHits);

// c6 Charm identify level from gap: gap >= 0 -> 0; gap <= -15 -> 3; gap <= -5 -> 2; else 1
i32 charmIdentifyLevel(i32 gap);

// c34 Cast on Who? prompt for effects 0 and 18
// CONFLICT: original asks only when companion count > 0 (M5B33); port always asks. Preferred: original.
// UNKNOWN: M5B33 assumed to be the companion count.
bool needsWhoPrompt(i32 effect, bool hasCompanions);

// c58 Spell cast RNG draw, made even outside combat (kept on purpose)
double drawSpellCastRoll(Ctx& ctx);

// c65 Standing order out of SP: order becomes Fight
void dropOrderOutOfSp(Ctx& ctx, Character& who);

// c29 Spend spell points after the effect is applied; clamped at zero
void chargeSpellPoints(Ctx& ctx, Character& who, i32 cost);

// c1 Successful cast at level <= 7 against peaced monsters sets Foes(0).Engaged
void engageAfterPeacedCast(Ctx& ctx, Party& party, i32 spellLevel, bool castSucceeded);

// T-2/T-3/T-4 spreadsheet Initial SC (D)
// UNKNOWN: D formula not stated in this chunk; inferred floor(Q * R * 1.9 - 0.0001) from table rows.
// CONFLICT: table gives Restrain 95, Charm Dragon 95, Dominate Demon 114; inferred formula gives 94, 94, 113.
i64 sheetInitialSC(double q, double r);

// c80 Spreadsheet Spell Cost (E) = MAX(ROUNDDOWN(C*Q*R*1.9/SL - 0.0001, 0), C)
// Note: Charm sheet E uses SL_init squared [c71]; not covered here.
i64 sheetSpellCost(double c, double q, double r, double yourSL);

// c80 Spreadsheet Min Cost (K) = ROUNDUP(C*Q*R*1.9/(C+1), 0) * 2 - 1
// CONFLICT: c80 says Seeker only uses (C+1); common core uses (C+1) on all sheets. Preferred: common core.
i64 sheetMinCost(double c, double q, double r);

// c106 Archive cost = floor(StartSpellLevel * SpellPointRatio * BaseMod^2 * 1.9 / SL - 0.0001)
// CONFLICT: archive min cost (StartSpellLevel * BaseMod) conflicts with spreadsheet Initial-SL form.
i64 archiveSpellCost(i32 startSpellLevel, double spellPointRatio, double baseMod, i32 sl);

}  // namespace mordor
