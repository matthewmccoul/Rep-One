#pragma once
// SRD part c11p1: Monsters, Member Step-Up through Quick Reference.
// Declarations only. Bodies are implemented in src/monsters1b*.cpp.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_character.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_item.h"
#include "mordor/state_party.h"

namespace mordor {

struct GameState;  // defined by the game state module (see API INDEX clock.h / progression2.h)

// ---- constants ----
// R38 monster attack resolution
inline constexpr i32 kMeleeAttemptsMax = 5;
// R42 joining
inline constexpr i32 kJoinBindingLevel = -10;
// R43 friendly join
inline constexpr i32 kFriendlyJoinRollRange = 3000;
inline constexpr i32 kJoinGroupCap = 4;
// R46 Asmodeus
inline constexpr i32 kAsmodeusMonsterId = 277;
// R49 room occupancy
inline constexpr i32 kStaleWindowFactor = 2;
inline constexpr i64 kMsPerSecond = 1000;
// R50 respawn delay
inline constexpr i32 kRespawnBaseSeconds = 1200;
inline constexpr i32 kRespawnPerLevelSeconds = 150;
inline constexpr i32 kRespawnPerAreaSeconds = 5;
inline constexpr i32 kRespawnMinSeconds = 120;
// R53 quest pick
inline constexpr i32 kQuestClassHeadStartPercent = 40;
inline constexpr i32 kQuestGapHighPercent = 90;
inline constexpr i32 kQuestGapMidPercent = 50;
inline constexpr i32 kQuestGapLowPercent = 10;
// W7 / W9 paralysis stat shift
inline constexpr i32 kGateFreeAdShift = -32;
inline constexpr i32 kParalysisFullPart = 32;
// W13 library knowledge
inline constexpr i32 kKnowledgeScale = 10000;
inline constexpr u32 kItemDeathsSentinel = 0xFFFFu;
// W17 floor bitmask (hypothesis, UNVERIFIED)
inline constexpr u32 kFloorMaskAllEleven = 2047u;

// ---- enums ----
// W2 companion spawn mode. UNKNOWN: numeric codes; Companion.spawnMode is i32 and its mapping is not given.
enum class CompanionSpawnMode : i32 {
    Always,
    PercentageChance,
    OnlyIfSpacePermits
};

// R37 Companion mishap message
// A fatal mishap is announced only by the death. Returns true when a separate mishap message is shown.
bool mishapAnnouncedSeparately(bool fatal);

// R38 Monster attack resolution (wiki/archive)
// CONFLICT: the relation of this order to engine Procedure R27 is Unknown; this is the archive order.
// Pre-attack: 1) breath, 2) acid.
void monsterBreath(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex);
void monsterAcid(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex);
void monsterPreAttack(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex);
// Attack: melee, up to kMeleeAttemptsMax attempts. Returns hits landed.
i32 monsterMeleeAttempts(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex);
// Post-attack step 1: electrocute. Also the target of a cast spell (skips melee).
void electrocute(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex);
// UNKNOWN: post-attack step 7 (destroy item) and step 8 (steal) bodies are not in this part.
void monsterDestroyItem(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex);
void monsterSteal(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex);
// Post-attack, in order: 1) electrocute, 2) age, 3) drain, 4) stone, 5) poison, 6) disease, 7) destroy item, 8) steal.
// Age, drain, stone, poison and disease reuse the status and progression routines in the API INDEX.
void monsterPostAttack(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex);
// Full resolution. A cast spell goes directly from the pre-attack step to electrocute and skips melee.
void resolveMonsterAttack(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex, bool castSpell);

// R39 Breath and acid, archive "real" formulas
// Pure: LevelMod = Int(MonGuildLvl * LN(MonGuildLvl + 1)).
// UNKNOWN: LN(0) when LevelMod is 0 (MonGuildLvl 0); the page does not say.
i32 realLevelMod(i32 monGuildLvl);
// Pure: first block (breath). Modifier1 = Int(((((LN(LevelMod) + LevelMod/15) / LN(1.08)) - CharGuildLvl/20) + 1).
i32 realBreathModifier(i32 monGuildLvl, i32 charGuildLvl);
// Pure: second block (acid and electrocution). Modifier1 = Int(((((LN(MonGuildLvl+1) + (MonGuildLvl+1)/15) / LN(1.08)) - CharGuildLvl/10) + 1).
i32 realAcidModifier(i32 monGuildLvl, i32 charGuildLvl);
// Rnd-driven base damage, first block. UNKNOWN: Rnd distribution; the page says these differ from simplified versions.
i32 breathBaseDamage(Ctx& ctx, i32 monGuildLvl, i32 charGuildLvl);
// Rnd-driven base damage, second block.
i32 acidBaseDamage(Ctx& ctx, i32 monGuildLvl, i32 charGuildLvl);
// Same second block as acid. Legend identical.
i32 electrocutionBaseDamage(Ctx& ctx, i32 monGuildLvl, i32 charGuildLvl);

// R40 Step-up
// When the front member falls, the next member steps forward. Record is loaded with a sign draw, then an amount draw (R28).
// Max is set to the loaded hits.
void stepUpFrontMember(Ctx& ctx, FoeGroup& group);
// Pure: true if the aimed group has no members left. AimedRecord does not move off a gone group (quirk).
bool aimedGroupGone(const SquareState& sq, const CombatOrder& order);
// A thief's sub0D08 reads the aimed group's record. If that group is empty, it retargets (clock.h retarget).
void thiefCheckAimedGroup(Ctx& ctx, GameState& game, Character& thief, const SquareState& sq);

// R41 Fear (Trap 9)
// Pure: a companion is released only if its owner has Hits > 0 and is not ForLife, and the companion is occupied.
// CONFLICT-free reading: "character" is taken as the owner Character, not Companion.forLife.
bool fearReleases(const Character& owner, const Companion& comp);
// Releases each qualifying companion (releaseCompanion), resets carry if CarryingCompanion < kCompanionSlots,
// and says "Your companions were scared away!" if any fled. Returns the number fled.
i32 fearTrap(Ctx& ctx, Party& party, Arr<Companion, kCompanionSlots>& slots);

// R42 Joining (engine/fidelity)
// Pure: true if the front group's JoinOffer is > 0. UNKNOWN: what "friendly" adds to this test.
bool joinOfferOpen(const FoeGroup& front);
// Takes joiners from Foes(0) only, up to front.count, using fn1163 semantics with binding level kJoinBindingLevel.
// Each joiner gets the front's hits and rolled max. Returns the number joined.
// If no free slot: say "Join - No room for more Companions!" and the offer stands. Silent on success.
i32 recruitJoiners(Ctx& ctx, i32 ownerIndex, FoeGroup& front, Arr<Companion, kCompanionSlots>& slots);
// Key handling is out of scope. This is the AcceptJoin rule: recruit, then clear the group if emptied.
bool acceptJoin(Ctx& ctx, i32 ownerIndex, SquareState& sq, FloorState& floor, Arr<Companion, kCompanionSlots>& slots, i64 nowMs);
// ClearedByJoining runs when joining empties the group.
// UNKNOWN: its body is not in this part; assumed to run markCleared (R48).
void clearedByJoining(Ctx& ctx, SquareState& sq, FloorState& floor, i64 nowMs);

// R43 Friendly join (fidelity)
// Pure: threshold = CHA*10 - (LevelFound-1)*10, using natural CHA.
i32 friendlyJoinThreshold(i32 cha, i32 levelFound);
// Join roll: Int(Rnd*3000)+1 < threshold.
// CONFLICT: audit form uses "<"; wiki form uses "<=" (rand(1,3000) <= CHA*10 - (mLvl-1)*10). Engine "<" is used.
bool friendlyJoinRoll(Ctx& ctx, i32 cha, i32 levelFound);
// Offered count: Int(Rnd*min(groupSize, kJoinGroupCap)) + 1. Also the offer size in R44.
i32 friendlyJoinCount(Ctx& ctx, i32 groupSize);

// R44 Charm and join (wiki/archive)
// Pure: charm forces a join if spell damage exceeds the monster's remaining HP.
bool charmForcesJoin(i32 spellDamage, i32 monsterHitsRemaining);
// Pure: offer eligibility.
// CONFLICT: wiki says "only group in the room and character level exceeds monster level".
// Archive says "character's current Guild Level >= monster's Guild Level". Archive is used here.
bool monsterOffersJoin(const Character& ch, i32 monsterGuildLevel);
// Pure: Good monsters join only Good characters. Neutral monsters can join any character. Evil monsters never join.

// Hostility: Good monsters are hostile to Neutral/Evil characters. Neutral monsters are hostile 60% of the time.
// Evil monsters are always hostile.
// CONFLICT: overlaps hostileBase / hostileRoll in clock.h (engagement). Wiki rule is declared here for the join path only.
bool alignmentHostileRoll(Ctx& ctx, CharAlign monsterAlign, Align charAlign);
// Pure: charm percentage = max(0, (CHA - mLvl + 1) / 3). Table shows decimals, so returns f64.
// UNKNOWN: whether the table keeps fractional percent.
f64 charmJoinPercent(i32 cha, i32 mLvl);

// R45 Charm and join roll
// No routine here: the roll is given in R43 (friendlyJoinRoll).

// R46 Asmodeus
// Pure: if monsterId == kAsmodeusMonsterId and count > 0, returns max(1, count - 1). Otherwise count.
i32 asmodeusAdjustedCount(i32 monsterId, i32 count);
// Display of invisible monsters is out of scope (display only).
// Quirk: a charmed creature leaves alive. It raises a kill but no damage, and the caster is paid XP on those hits.
void creditCharmedCreature(Ctx& ctx, Character& caster, Creature& creature, i64 hitsInflicted);
// Pure: a kill by a companion is on nobody's Kills.
bool killCreditable(bool byCompanion);
// Pure: a record left by a stone that is no longer a fight monster is nobody's damage.
bool damageCreditable(bool fightMonster);

// R47 Clearing
// Pure: true if no group in the square remains standing (no group with count > 0).
bool noGroupRemains(const SquareState& sq);
// Gold: paid immediately if there is no chest; otherwise it waits in the chest.
// UNKNOWN: pending source is party.fightGold; sink is sq.chestGold when present.
void settleFightGold(Ctx& ctx, Party& party, SquareState& sq);
// Fight won and no group remains: Asmodeus question first (original), then markCleared (R48), then gold.
void clearFight(Ctx& ctx, Party& party, SquareState& sq, FloorState& floor, i64 nowMs);
// Standing groups older than 2t are removed on refill (R49 stale window).
void pruneStaleGroups(Ctx& ctx, SquareState& sq, i64 nowMs, i32 respawnSec);

// R48 Clearing and refilling
// MarkCleared: if Foes[0].Ref == -1 return. Else all counts = 0, Ref = -1, Stamp = now (seconds), JoinOffer = 0, tally - 1.
void markCleared(Ctx& ctx, SquareState& sq, FloorState& floor, i64 nowMs);
// NoteRefilled: tally + 1.
void noteRefilled(Ctx& ctx, FloorState& floor);

// R49 Room occupancy
// Pure: seconds since midnight from ms. Quirk: no date, so absolute difference is used.
i64 secondsSinceMidnight(i64 nowMs);
// Pure: true if any group still has count > 0.
bool anyStanding(const SquareState& sq);
// Pure: stale if |nowSec - stampSec| > 2t.
bool groupStale(i64 nowSec, i64 stampSec, i32 respawnSec);
// Pure: RefillDue. Empty if no standing groups, or stale, and then |now - stamp| > t (or stamp == 0).
bool refillDue(const SquareState& sq, i64 nowMs, i32 respawnSec);
// Pure: IsOccupied = areaId > 0 and (RefillDue or AnyStanding).
bool isOccupied(const SquareState& sq, i64 nowMs, i32 respawnSec);

// R50 Respawn delay
// t = 1200 + 150*level, then minus 5*(areaCount - tally), with minimum 120. Engine and test forms share this formula.
// CONFLICT: the operand is headerAreas (engine) or total (test); the test meaning of the second argument is Unknown.
i32 respawnDelaySeconds(i32 level, i32 areaCount, i32 tally);

// R51 Refill waiting
// Covered by refillDue (strict >) and groupStale (stale after twice the respawn time).

// R52 Gold and chests
// Covered by settleFightGold (R47.2) and markCleared (chest stays).

// R53 Quest "Defeat One" monster pick
// Loop repeated until a valid pick exists. Returns table index, or -1 when there are no classes.
// Tables are the caller's view of monster classes (classFirst[cls], classLast[cls]) and monsters (levelFound, chance).
// Quirk: no cap in the original. VB And is not short-circuit, so the Rnd draw happens for every candidate.
// Rare is chance < kRareMonsterWeight. Turned is Rnd*100 > kRareRejectPercent.

// Pure: level-gap gate for certain specials. Returns 90, 50 or 10.
i32 questLevelGapPercent(i32 charLvl, i32 monLvl);

// W1 Level population (generator)
// No routine declared: step order is Unknown.

// W2 Companion spawn (generator)
// Pure: companion spawn mode applies to companionID.
bool companionSpawns(Ctx& ctx, CompanionSpawnMode mode, i32 percent, bool spaceAvailable);

// W3 to W6 Winning a fight, room occupancy, refill, respawn time
// See R47 to R51. No new declarations.

// W7 Paralysis applied
// Pure: Part = StatsADNow + StoredADShift. Paralyse and unparalyse are in the API INDEX.
i32 paralysisPart(const Character& c);

// W8 Paralysis cured and W9 freed at city gate
// See unparalyse and clearParalysisAtCityGate in the API INDEX.

// W10 Monster attack resolution: see R38.
// W11 Charm and join: see R44.
// W12 Spreadsheet note: display only, out of scope.

// W13 Library knowledge: constants kKnowledgeScale and kItemDeathsSentinel above.

// W14 Bonus HP at early guild levels: see ConHitsBonus in the API INDEX (guilds3.h).

// W15 Dice notation for monster attributes (wiki)
// Pure: average of NdM+K, i.e. n*(sides+1)/2 + plus.
// UNKNOWN: whether the printed number is always this average.
f64 diceAverage(i32 n, i32 sides, i32 plus);
// Rolls NdM+K.
i32 rollDice(Ctx& ctx, i32 n, i32 sides, i32 plus);

// W16 Bit tests (generic, wiki)
// Pure: (flags & bit) != 0. UNKNOWN: bit values.
bool hasFlagBit(u32 flags, u32 bit);

// W17 Floor bitmask (hypothesis only, UNVERIFIED)
// Pure: floor n present if (floorMask >> (n-1)) & 1 == 1.
bool floorBitPresent(u32 floorMask, i32 n);

// W18 Spawn counts and laired placement: hypothesis only, no routine declared.

// Quick Reference
// Every item refers to a rule above or to a rule in another module. No new declarations.

}  // namespace mordor
