#pragma once
// companions1b.h: SRD part c12p1, companion rules R27 to R52 (spell or blow, drift and leaving,
// turning, death and carry, Kick Out and right-click Attack, morgue raise, load normalisation,
// slots and names, monster hostility, bind and charm).
// Declarations only. Bodies live in src/companions1b_*.cpp.
// Text screens (R3-style UI prompts and right-click menus) are out of scope; only the rule
// decisions are declared here. Core never prints: all output goes through ctx.say.

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/attributes.h"
#include "mordor/status2.h"
#include "mordor/guilds1.h"
#include "mordor/races.h"

namespace mordor {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

// R27 spell or blow
inline constexpr i32 kCompanionCastLevelDivisor = 6;   // R27.6 companion casts at guild level / 6
inline constexpr f64 kSpellMishapFactor = 1.5;         // R27.9 spell odds x 1.5
inline constexpr f64 kSpellMishapFloor = 1.0;          // R27.9 clamp floor (1)
inline constexpr f64 kSpellMishapCeiling = 40.0;       // R27.9 clamp ceiling (40)
inline constexpr i32 kCasterFizzlePercent = 50;        // R27.5 non-Mage caster gives up half the time
inline constexpr i32 kCasterFailPercent = 50;          // R27.5 companion caster failure 50%

// R28 blow
inline constexpr i32 kBlowFlagCrit = 10;               // R28 Bit(flags,10): crit flag
inline constexpr i32 kBlowFlagBackstab = 11;           // R28 Bit(flags,11): backstab flag
inline constexpr i32 kBlowFlagElectrocute = 6;         // R31 bit 6: electrocute
inline constexpr i32 kBlowFlagStone = 8;               // R30 bit 8: stone
inline constexpr i32 kBlowCritBonus = 4;               // R28 damMod += 4 on crit
inline constexpr i32 kBlowBackstabBonus = 1;           // R28 damMod += 1 on backstab
// CONFLICT: R28 prose says a monster's crit against a character is +2 (c25); the formula gives +4.
// Preferred: the formula (+4) is used for blows.

// R29 / R30 / R31 resistance and stone
inline constexpr f64 kStoneLogBase = 2.5;              // R30 chance uses Log(Max(1,con)) / Log(2.5)
inline constexpr i32 kResistHalfDivisor = 2;           // R29 resist removes half its percentage

// R34 drift and round
inline constexpr i32 kDriftRollRange = 200;            // R34 rng()*200 < Drift
inline constexpr f64 kDriftContentFloor = 0.5;         // R34 content drift minimum
inline constexpr f64 kDriftContentScale = 15.0;        // R34 content drift scale
inline constexpr f64 kDriftBoundFloor = 1.0;           // R34 bound drift minimum
inline constexpr i32 kLeaveRollRange = 200;            // R34 leave test rng()*200 < ChanceToLeave

// R35 chance to leave
inline constexpr f64 kBoundScale = 4.5;                // R35 round_half_even(|binding| * 4.5)
inline constexpr f64 kMonsterLeaveScale = 6.0;         // R35 mon = ln(monsterGL + 1) * 6
inline constexpr f64 kMonsterLeaveWeight = 4.0;        // R35 x = mon * 4
inline constexpr f64 kBoundWeight = 2.0;               // R35 bound * 2
inline constexpr f64 kTopGuildLnDivisor = 10.0;        // R35 ln(topGuildLevel / 10 + 1)
inline constexpr f64 kTopGuildLnWeight = 4.0;          // R35 ln(...) * 4
inline constexpr f64 kChaDivisor = 2.0;                // R35 x = x - charisma / 2
inline constexpr f64 kLeaveNormDivisor = 200.0;        // R35 x = x / 200 * 100
inline constexpr f64 kLeaveNormScale = 100.0;          // R35 x = x / 200 * 100
inline constexpr f64 kHitsLeaveScale = 15.0;           // R35 x += (maxHits - hits) / maxHits * 15
inline constexpr f64 kLeaveFinalDivisor = 2.0;         // R35 x = x / 2
// Describe thresholds (R35 table): 0 is "Completely"; then <=3, <=10, <=15, <=20, else.
inline constexpr f64 kLeaveDescribeLimit[4] = {3.0, 10.0, 15.0, 20.0};

// R36 turning
inline constexpr i32 kTurnEvilPercent = 75;            // R36 Evil creature turns if rnd1*100 < 75
inline constexpr i32 kTurnNeutralPercent = 15;         // R36 Neutral creature turns if rnd2*100 < 15
// CONFLICT: docs c27 give Evil 3 in 4 and Neutral about 1 in 7; engine c8 uses 75 and 15.
// Preferred: engine values above.

// R42 morgue raise
inline constexpr i32 kMorgueRaiseBase = 85;            // R42 raise chance = min(98, 85 + CON)
inline constexpr i32 kMorgueRaiseCap = 98;
inline constexpr i32 kCompanionConIndex = 3;           // R42 Stats[3] is CON

// R47 monster hostility
inline constexpr i32 kNeutralMonsterHostilePercent = 60; // R47 Neutral monsters hostile 60% of the time

// R49 bind spells (index by BindSpell)
inline constexpr i32 kControlBaseLevel = 3;            // R49 Control base 3
inline constexpr i32 kBindBaseLevel = 6;               // R49 Bind base 6
inline constexpr i32 kDominationBaseLevel = 12;        // R49 Domination base 12
inline constexpr i32 kBindReqInt[3] = {12, 14, 18};    // R49 Control, Bind, Domination
inline constexpr i32 kBindReqWis[3] = {12, 14, 18};
inline constexpr i32 kBindReqCon[3] = {0, 0, 18};
inline constexpr i32 kBindReqCha[3] = {10, 16, 18};

// ---------------------------------------------------------------------------
// Enums and small result structs
// ---------------------------------------------------------------------------

enum class CompanionAction : i32 {
    Cast = 0,
    Blow = 1
};

enum class HealRoute : i32 {
    Party = 0,              // heal reaches the party
    MonstersAsDamage = 1,   // heal passed a slot, thrown at monsters as damage
    Nobody = 2              // M5B1D is 0: reaches nobody
};

enum class MishapCaption : i32 {
    HitFor = 0,             // "You were accidently hit for"
    ElectrocutedFor = 1     // "You were accidently electrocuted for"
};

enum class KickOutRefusal : i32 {
    None = 0,
    Dead = 1,               // "drop it first"
    City = 2                // party is in the city
};

struct KickOutCheck {
    bool allowed = false;
    KickOutRefusal refusal = KickOutRefusal::None;
    bool warnBound = false; // binding > 0 earns a warning
};

enum class RightClickOutcome : i32 {
    Ignored = 0,            // Hits <= 0: silent
    Refused = 1,            // both Good
    LetGo = 2               // call letCompanionGo forced
};

enum class TurnOutcome : i32 {
    Joins = 0,              // companion turns and joins the fight
    Holds = 1,              // does not turn
    RefusedByFoe = 2,       // "Your companion refuses to join the current monsters!"
    NoGroupSlot = 3         // no free group slot (of 4)
};

enum class MorgueRaiseOutcome : i32 {
    NoDeadSlot = 0,         // no slot with Hits == 0
    Raised = 1,             // Hits = MaxHits
    Failed = 2              // slot released and left empty
};

enum class DragSwapOutcome : i32 {
    Swapped = 0,
    RefusedDead = 1         // "No use re-arranging a dead companion!"
};

enum class BindSpell : i32 {
    Control = 0,            // increases Bind level
    Bind = 1,               // renews a fading Bind
    Domination = 2          // COMBAT charm; renews Bind level
};

// Live monster set for R41: combat monsters if a fight exists, else the waiting group.
// Both are borrowed; the routine may append a joining companion's body to the live set.
struct LiveSetRef {
    Vec<Creature>& combatMonsters;
    Vec<Creature>& waitingGroup;
    bool fightExists;
    bool groupSlotFree;     // R36.3 a free group slot exists (of 4)
};

// Blow inputs for R28 to R31, taken from the attacking companion's monster body (def).
struct BlowParams {
    u32 specialFlags = 0;   // def flags (crit bit 10, backstab bit 11, stone bit 8, electrocute bit 6)
    i32 damageMod = 0;      // def.DamageMod
    i32 guildLevel = 0;     // def.GuildLevel
    i32 levelFound = 0;     // LevelFound (whose is Unknown, see R30)
    i32 att = 0;            // def.Att
    i32 stat0 = 0;          // Stats[0] (meaning Unknown, see R28)
    i32 stonerCon = 0;      // stoner's CON (R30)
};

// ---------------------------------------------------------------------------
// R27 Spell or blow
// ---------------------------------------------------------------------------
// R27.1 and R27.4 cast allowed only if no companion has cast for this owner this round and anti-magic absent.
bool companionMayCast(bool ownerCastThisRound, bool antiMagicPresent);
// R27.2 take one charge from the owner's item in the owner's pack slot; refuse with out-of-charges line if none.
bool companionTakeCastCharge(Ctx& ctx, Inventory& ownerInv, i32 packSlot);
// R27.3 owner's order after a companion cast: Defend for Att 0 or 1, Fight for Att 2.
CombatOrderKind orderAfterCompanionCast(i32 att);
// R27.5 non-Mage companion caster gives up half the time (fn0FF5).
bool companionCastFizzles(Ctx& ctx, bool casterIsMage);
// R27.5 companion caster failure: 50%.
bool companionCastFails(Ctx& ctx);
// R27.6 companion cast level is guild level / 6.
i32 companionCastLevel(i32 guildLevel);
// R27.6 heal routing; heals reach nobody when the M5B1D flag is 0.
HealRoute companionHealRoute(bool passedToSlot, i32 healTargetFlag);
// R27.7 magic immunity of the companion's body (tested on the companion itself, 2556).
bool companionMagicImmune(const Creature& body);
// R27.9 spell mishap odds: spell odds x 1.5, clamped to 1..40.
f64 companionSpellMishapChance(f64 spellOdds);
// R27.9 spell mishap roll; draws the unused Int(Rnd*100) first, then tests the clamped chance.
bool companionSpellMishapRoll(Ctx& ctx, f64 spellOdds);
// R27 top level: decide and prepare a companion's action for this round (charge, order, fizzle).
CompanionAction companionSpellOrBlow(Ctx& ctx, Companion& caster, Character& owner, Inventory& ownerInv,
                                     i32 packSlot, bool antiMagicPresent, bool casterIsMage);

// ---------------------------------------------------------------------------
// R28 Blow
// ---------------------------------------------------------------------------
// helper: Bit(flags, n) as used throughout R28 to R31.
bool specialFlagBit(u32 flags, i32 bit);
// R28 MonsterCritChance(GuildLevel, LevelFound, targetLevel, backstab) as a percent.
// UNKNOWN: the formula is not given in this part; defined with MonsterCritChance elsewhere.
f64 monsterCritChance(i32 guildLevel, i32 levelFound, i32 targetLevel, bool backstab);
// R28 damMod after crit (flag 10 and flag 11 rolls) and the unseen halving.
// UNKNOWN: VB And is not short-circuit, so both crit rolls are assumed always drawn.
i32 blowDamageMod(Ctx& ctx, const BlowParams& p, bool targetUnseen, i32 targetLevel);
// R28 damage roll: Rules.Fn0DEE(rng, def.Att, targetDef, def.GuildLevel, Stats[0], damMod).
// UNKNOWN: Fn0DEE is not given in this part.
i32 blowDamageRoll(Ctx& ctx, const BlowParams& p, i32 targetDef, i32 damMod);
// R28 whole blow: damMod (crit, backstab, unseen) then damage. No to-hit roll, no minimum 1, no size term.
i32 blowDamage(Ctx& ctx, const BlowParams& p, i32 targetDef, i32 targetLevel, bool targetUnseen);

// ---------------------------------------------------------------------------
// R29 Resistance on breath, acid spit, electrocution and stone
// ---------------------------------------------------------------------------
// after = damage - damage * (resist / 2) / 100
i32 afterResistance(i32 damage, i32 resistPercent);

// ---------------------------------------------------------------------------
// R30 Stone (bit 8)
// ---------------------------------------------------------------------------
// chance = con * (Log(Max(1,con)) / Log(2.5)) - LevelFound, capped at 95 (kStoneChanceCap).
// UNKNOWN: which LevelFound is used (see R60 in source).
f64 stoneChance(i32 con, i32 levelFound);
// R30 full stone test: res roll (rnd(100)+1 > res) then rnd()*100 > chance means stoned.
bool stoneLands(Ctx& ctx, i32 stoneRes, i32 stonerCon, i32 levelFound);
// R30 stone and electrocution follow-ups test Foes(M547A).Hits > 0; returns next live foe index or -1.
// UNKNOWN: line 1215 behaviour in the original; index semantics assumed.
i32 nextLiveFoe(const Vec<Creature>& foes, i32 startIndex);

// ---------------------------------------------------------------------------
// R31 Electrocution (bit 6)
// ---------------------------------------------------------------------------
// Raw(Electrocute, def.GuildLevel, targetLevel). UNKNOWN: raw formula not given in this part.
i32 rawElectrocuteDamage(i32 guildLevel, i32 targetLevel);
// zap = AfterResistance(Raw(...), Res(target, Electrical)); electrical index is breathResistIndex(BreathType::Electrical).
i32 electrocutionDamage(i32 guildLevel, i32 targetLevel, i32 electricalResistPercent);
// R28 to R31: stone (bit 8) and electrocute (bit 6) on a landed blow. Returns electrocution damage to add.
i32 applyLandedBlowEffects(Ctx& ctx, const BlowParams& p, Creature& target, i32 targetLevel,
                           i32 stoneRes, i32 electricalResistPercent);

// ---------------------------------------------------------------------------
// R32 Mishap sounds and captions
// ---------------------------------------------------------------------------
// R32.1-2 death cry DI-UNDEA, once, before the mishap line. suppressed = victim's No Party Sound (spell)
// or owner's No Party Sound (blow).
void mishapDeathCry(Ctx& ctx, bool suppressed);
// R32.3 melee companion hits a party member without killing: attack cry DRAGON once; only the owner hears it.
void mishapAttackCry(Ctx& ctx);
// R32.4-5 mishap captions. HitFor prints only when the on-screen character is hit; ElectrocutedFor always prints.
// andCompanions appends " and his companions" (R32.6).
void mishapCaption(Ctx& ctx, MishapCaption kind, bool watchedCharacterHit, Str victimName, bool andCompanions);
// R32.6 suffix names only the last slot visited (the suffix flag is reset on each pass).
bool mishapSuffixApplies(i32 slotVisited, i32 lastSlotVisited);

// ---------------------------------------------------------------------------
// R33 Kills
// ---------------------------------------------------------------------------
// Companion kill: not added to the character's Kills; reported to the mods as a Companion-kind kill for the owner.
// CONFLICT: docs c36 say companion kills count toward the owner's Kills. Preferred: engine c5 (report only).
void reportCompanionKill(Ctx& ctx, const Character& owner, i32 monsterId);

// ---------------------------------------------------------------------------
// R34 Drift and leaving (once per round per non-ForLife companion, slots 0 to 3)
// ---------------------------------------------------------------------------
// Drift content (Binding < 0): max(0.5, (MaxHits - Hits) / MaxHits * 15); 0 if MaxHits <= 0.
f64 companionDriftContent(i32 hits, i32 maxHits);
// Drift bound (Binding > 0): max(1, GuildLevel / ln(3) * 2 - TopGuildLevel / ln(12) * |Binding / 2|).
// UNKNOWN: whether |Binding / 2| is real or integer division; real division assumed.
f64 companionDriftBound(i32 guildLevel, i32 topGuildLevel, i32 bindingLevel);
// R34.1 CompanionMood.Round: possible drift one notch toward 0. ForLife (100) is never touched.
// UNKNOWN: Drift for Binding == 0 is not stated; no drift assumed.
void companionMoodRound(Ctx& ctx, Companion& c, i32 topGuildLevel);
// R34 whole round pass over slots 0 to 3. Per companion: mood, leave test, turn check (R36),
// then turn message and join, or leave line and clear the slot. Left line is not printed when it turns.
// creatureAligns is resolved by the caller from each body's MDATA5 alignment.
void companionRoundPass(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, const Character& owner,
                        const Arr<CharAlign, kCompanionSlots>& creatureAligns, LiveSetRef& live);

// ---------------------------------------------------------------------------
// R35 Chance to leave
// ---------------------------------------------------------------------------
// bound = round_half_even(|bindingLevel| * 4.5)
i32 boundFromBinding(i32 bindingLevel);
// Full ChanceToLeave in percent, clamped 0 to 90 (kChanceToLeaveMax). bindingLevel == 100 gives 0.
// monsterGuildLevel is the companion's guildLevel field.
f64 companionChanceToLeave(const Companion& c, i32 topGuildLevel, i32 charisma);
// R35 describe text for a chance (table in R35).
Str companionLeaveDescribe(f64 chance);
// R34/R35 leave test: rng()*200 < chance, or Binding == 0.
// UNKNOWN: VB Or evaluates both sides, so the roll is assumed always drawn.
bool companionLeaveRoll(Ctx& ctx, const Companion& c, f64 chance);

// ---------------------------------------------------------------------------
// R36 Turning
// ---------------------------------------------------------------------------
// turns = |creature.Align - owner.Align| > 1; Evil creature 75%; Neutral creature 15%; forced turns always.
// Good creature never turns unless forced. Binding <= 0 never turns unless forced. Refused if a fight is
// alive and |Foes[0].Align - creature.Align| > 1. Needs a free group slot.
// UNKNOWN: "fight alive" taken as foes non-empty. Both rnd1 and rnd2 are always drawn.
TurnOutcome companionTurnCheck(Ctx& ctx, const Companion& c, Align ownerAlign, CharAlign creatureAlign,
                               bool forced, const Vec<Creature>& foes, bool groupSlotFree);

// ---------------------------------------------------------------------------
// R37 Death of a companion and carry choice
// ---------------------------------------------------------------------------
// R37.1 owner chooses a body only while carrying nothing or a companion (CarryingCompanion < 4).
bool carryChoiceOpen(i32 carryIndex);
// R37.2 candidates are occupied slots 0 to 3 at or below 0 Hits. First taken; later replaces only with
// strictly greater MaxHits. Returns slot or -1. Ties go to the earlier slot.
i32 pickCarryCandidate(const Arr<Companion, kCompanionSlots>& slots, const Arr<i32, kCompanionSlots>& bodySizes);
// R37.3 pickup test: Monsters(Ref).Size - Race.Size <= 1.
bool carryPickTest(i32 bodySize, i32 carrierSize);
// R37.2-3 full carry choice: candidate, "dropped" message, pickup test or "too big to carry" release, sweep.
// Quirks kept: "dropped" prints before the size test; with nothing carried a heavier second candidate
// prints "dropped" with companion slot -1 and ends the sub (UNKNOWN: port-only quirk, reproduced by callers).
void runCarryChoice(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, Inventory& inv,
                    const Arr<i32, kCompanionSlots>& bodySizes, i32 carrierSize);
// R37.4 every fallen companion not carried is released silently.
void sweepFallenCompanions(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, i32 keptSlot);
// R37.5 death lets go of every companion and anything carried (sub0930).
void companionsLetGoOnDeath(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, Inventory& inv);
// R37 BaseValue of a fallen body. UNKNOWN: formula not in SRD; counts only positive resistances; a creature of 1 hit is worth 1.
i64 creatureBaseValue(const Creature& c);
// R38 trigger: the engine runs the carry choice when a companion's Hits reach zero.
bool companionDeathTrigger(const Companion& c);

// ---------------------------------------------------------------------------
// R39 Kick Out
// ---------------------------------------------------------------------------
// Refused if dead or the party is in the city. A bound companion (binding > 0) earns a warning.
KickOutCheck kickOutCheck(const Companion& c, bool inCity);
// R39.2 Kick Out after Yes/No prompt answer. On confirmed, calls letCompanionGo(forced: false).
// Returns true if the companion was let go (or left). Refusals leave the selection untouched.
bool kickOutCompanion(Ctx& ctx, Companion& c, const Character& owner, CharAlign creatureAlign,
                      bool inCity, bool confirmed, LiveSetRef& live);

// ---------------------------------------------------------------------------
// R40 Right-click Attack
// ---------------------------------------------------------------------------
// R40.1-2: Ignored if Hits <= 0; Refused if character and companion are both Good.
// compareAlign is the character's alignment; port passes Align::Good (m02BE never assigned).
// CONFLICT: R40 prose compares the character's real alignment; port compares a constant Good (c43, c55). Port behaviour kept.
RightClickOutcome rightClickAttackCheck(const Companion& c, Align compareAlign);
// R40.3 right-click Attack: calls letCompanionGo(forced: true) when the check passes. Returns true if it was called.
bool rightClickAttack(Ctx& ctx, Companion& c, const Character& owner, CharAlign creatureAlign,
                      Align compareAlign, LiveSetRef& live);

// ---------------------------------------------------------------------------
// R41 LetCompanionGo
// ---------------------------------------------------------------------------
// Live set is combat monsters if a fight exists, else the waiting group. Turn check (R36); a turn drops the
// companion into the fight. A non-forced failed turn clears the slot silently.
// UNKNOWN: whether naming refuses during a fight (source comment says so; port does not check).
bool letCompanionGo(Ctx& ctx, Companion& c, const Character& owner, CharAlign creatureAlign,
                    LiveSetRef& live, bool forced);

// ---------------------------------------------------------------------------
// R42 Morgue raise
// ---------------------------------------------------------------------------
// raise chance = min(98, 85 + CON); fail if rng.Next(100) > chance.
i32 morgueRaiseChance(i32 companionCon);
// R42.1 first slot with Hits == 0, or -1.
i32 morgueOfferSlot(const Arr<Companion, kCompanionSlots>& slots);
// R42.1-3 raise at the morgue. Gold (cost) is taken from goldOnHand first and not refunded on failure.
// UNKNOWN: cost amount and purse source are not stated in this part; caller supplies cost.
// Failure releases the slot. Afterwards carry index and CarryingCharacter are cleared unconditionally.
MorgueRaiseOutcome raiseCompanionAtMorgue(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, Character& owner,
                                          Inventory& ownerInv, Currency cost);

// ---------------------------------------------------------------------------
// R43 Load-time normalisation
// ---------------------------------------------------------------------------
// R43.2 old Id-keyed records settled by Id. Id 307 uses alignment.
// UNKNOWN: what settling does for other Ids is not stated in this part.
void settleLegacyCompanionId(Ctx& ctx, Companion& c, i32 legacyId);
// R43 port: an Unaligned companion saved by older code is given its creature's alignment on load.
void fillUnalignedCompanionAlignment(Companion& c, CharAlign creatureAlign);
// R43.3 a carry index >= 4 is a carried body and is put down on load.
void putDownCarriedBodyOnLoad(Ctx& ctx, Inventory& inv);
// R43.4 the carry choice (R37/R38) runs when a member joins or loads (docs) via runCarryChoice.
// R43.1 Ref resolution (R3) is outside this part.

// ---------------------------------------------------------------------------
// R44 Slots and names
// ---------------------------------------------------------------------------
// First empty slot (Hits or occupied false), or -1.
i32 firstCompanionHole(const Arr<Companion, kCompanionSlots>& slots);
// keepSlotName (fn1163): the name left in the slot is kept by the new occupant.
Str keepSlotName(const Companion& previousOccupant);
// R44 new companion fills the first hole and keeps its name. Returns slot or -1 if full.
i32 placeNewCompanion(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, Companion incoming);
// R45 a released slot keeps its last occupant's record. Name blanking is screen behaviour (out of scope).

// ---------------------------------------------------------------------------
// R46 Drag swap
// ---------------------------------------------------------------------------
// Refused if either end is dead, with "No use re-arranging a dead companion!".
DragSwapOutcome dragSwapCompanions(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, i32 fromSlot, i32 toSlot);

// ---------------------------------------------------------------------------
// R47 Monster hostility to characters
// ---------------------------------------------------------------------------
// Good monsters join Good characters only. Evil monsters never join (simplest reading).
// UNKNOWN: which characters Evil monsters may join; "level exceeds their level" is ambiguous (see R60).
bool monsterMayJoin(CharAlign monsterAlign, Align charAlign);
// Good monster: never hostile to Good, always to Neutral and Evil. Neutral monster: hostile 60% (rolled).
// Evil monster: always hostile. UNKNOWN: when the Neutral roll is made.
bool monsterHostileToCharacter(Ctx& ctx, CharAlign monsterAlign, Align charAlign);

// ---------------------------------------------------------------------------
// R48 Charm makes a companion
// ---------------------------------------------------------------------------
// Charm spells make a creature a companion of the caster. Eligibility via canCharm. Places into first hole.
// UNKNOWN: companion field values set by charm are not stated in this part.
bool charmCreatureAsCompanion(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, const Character& caster,
                              i32 monsterId);

// ---------------------------------------------------------------------------
// R49 Bind spells
// ---------------------------------------------------------------------------
// Caster stat requirements from the spell table (Int, Wis, Con, Cha per BindSpell).
bool bindSpellMeetsRequirements(const Character& c, BindSpell spell);
// Bind spells do not work on companions that joined of their own free will.
bool bindSpellApplies(const Companion& c);
// Raises the Bind level of an existing companion, clamped to kBindingMax (100).
// UNKNOWN: the SRD gives no amount; the spell base level is used as the amount (simplest sensible choice).
void castBindSpell(Ctx& ctx, Companion& target, BindSpell spell);

// ---------------------------------------------------------------------------
// R50 Control Dragon
// ---------------------------------------------------------------------------
// A failed Control Dragon: the target responds adversely. UNKNOWN: response not stated; simplest choice is hostile.
void controlDragonFailure(Ctx& ctx, Creature& target);

// ---------------------------------------------------------------------------
// R51 Dominate Demon
// ---------------------------------------------------------------------------
// Binds a demon for an unknown period. UNKNOWN: duration. Sets the companion bound.
void dominateDemonBind(Ctx& ctx, Companion& demon);

// ---------------------------------------------------------------------------
// R52 Bind terms
// ---------------------------------------------------------------------------
// "Bind level" in spell wording is BindingLevel (Companion::bindingLevel). No separate routine.

}  // namespace mordor
