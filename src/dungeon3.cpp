#include <cmath>

#include "mordor/dungeon3.h"
#include "mordor/ctx.h"
#include "mordor/events.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/state_character.h"
#include "mordor/state_common.h"
#include "mordor/state_creature.h"
#include "mordor/state_dungeon.h"
#include "mordor/types.h"

namespace mordor {

namespace {

// UNKNOWN: map side length. Engine addressing and MDATA11 give 30 (R51).
constexpr i32 kMapSide = 30;

// R37: notice roll is rng.Next(100) + 1 <= P. Next(100) assumed 0-99.
constexpr i32 kNoticeRange = 100;

// R53 / c18: stud spread draw is Rnd*75 < 50.
constexpr double kStudDrawRange = 75.0;
constexpr double kStudDrawLimit = 50.0;
constexpr i32 kStudSpreadOn = 1;

// Shareware / limited build threshold (R33 via c33, c35).
constexpr i32 kFullDungeonLevelCount = 15;

// R42: facings are 0-based in this port (&3). CONFLICT, see R42.
constexpr i32 kFacingCount = 4;

// Rescue chance constants (c17).
constexpr double kRescueBaseLevel = 16.0;
constexpr double kRescueMinChance = 0.1;
constexpr double kRescueCeiling = 100.0;
constexpr i32 kCeilingLevel = 1;
constexpr i32 kCeilingMaxArea = 18;

// Teleporter draw exclusion range (c22, MAZEMOD 1798-1808).
constexpr i32 kMazeModExcludeLow = 1798;
constexpr i32 kMazeModExcludeHigh = 1808;

// Wrong-guess bits (R39): literal reading "eight bits always pass".
constexpr u32 kWrongGuessBits = 0xFFu;

// Facing arrow base offset (R43 / c63).
constexpr i32 kFacingArrowOffset = 22;

// Body facing offset (R42 / c48).
constexpr i32 kBodyFacingOffset = 3;

// Forward step per facing for the ethereal portal (R42 mapping UNKNOWN).
// 0 = north (y-1), 1 = east (x+1), 2 = south (y+1), 3 = west (x-1).
constexpr i32 kStepDx[4] = {0, 1, 0, -1};
constexpr i32 kStepDy[4] = {-1, 0, 1, 0};

i32 squareIndexOf(i32 x, i32 y) {
    return y * kMapSide + x;
}

}  // namespace

// R49 / c13 style pure helper: returns true if a teleporter draw must reject the mazeMod.
bool mazeModExcluded(i32 mazeMod) {
    // UNKNOWN: the SRD says the teleporter draw rejects rock and teleporter squares at
    // MAZEMOD 1798-1808 [c22]. The range is taken as given; the meaning of mazeMod is not stated.
    return mazeMod >= kMazeModExcludeLow && mazeMod <= kMazeModExcludeHigh;
}

// Shareware / limited build detection (c33, c35).
bool isLimitedBuild(i32 dungeonLevelCount) {
    // CONFLICT: c33 lists three tests (fewer than 15 levels, monsters or items stopping short).
    // Preferred: the dungeon level count is the only reliable test, since shipped monsters and
    // items reach 16 [c35].
    return dungeonLevelCount < kFullDungeonLevelCount;
}

// Rescue chance (c17, preferred over the wiki formula c97).
double rescueChance(i32 dungeonLevel, bool easyFind, i32 area) {
    const double level = static_cast<double>(dungeonLevel);
    double chance = maxv(kRescueBaseLevel - level, kRescueMinChance);
    if (easyFind) {
        // c17: easy-find adds 16 - L.
        chance += kRescueBaseLevel - level;
    }
    // UNKNOWN: the ceiling of 100 applies on level 1 areas 18 and lower [c17]. Applied as a cap
    // only in that case; the exact semantics of the ceiling are not stated.
    if (dungeonLevel == kCeilingLevel && area <= kCeilingMaxArea) {
        chance = minv(chance, kRescueCeiling);
    }
    return chance;
}

// Rescue roll. UNKNOWN: whether chance is a percentage. Treated as a percent of a Rnd draw.
bool rollRescue(Ctx& ctx, double chance) {
    return ctx.rng.nextDouble() * 100.0 < chance;
}

// Trap disarm log term (c24 preferred over c97).
double trapDisarmLogTerm(double m) {
    // CONFLICT: c24 uses ln(max(2, M)); c97 uses ln(mLvl) with no max. c24 followed.
    return std::log(maxv(2.0, m));
}

// Disarm stat (c24, c97): WIS/4 + DEX/2.
double disarmStat(i32 wis, i32 dex) {
    // UNKNOWN: rounding of the divisions is not stated; real division used.
    // CONFLICT: c31 thieving reads WIS and DEX with gear in full; not reconciled.
    return static_cast<double>(wis) / 4.0 + static_cast<double>(dex) / 2.0;
}

// Notice roll (R37).
bool noticePasses(Ctx& ctx, i32 perception) {
    // CONFLICT: at P = 100 this always passes [c17]. c63 says perception 100 does not name a
    // rotator. Unresolved; the formula is applied as written.
    return ctx.rng.below(kNoticeRange) + 1 <= perception;
}

// Identification flag on fog meeting (R38).
void clearIdentificationOnFogMeeting(Ctx& ctx, Creature& monster, bool fogPresent) {
    (void)ctx;
    // R38: text says only a meeting in fog clears identification. Code sets M4EEE = 1 when fog
    // is present [c5]. UNKNOWN: meaning of M4EEE; mapped to monster.identified = false.
    if (fogPresent) {
        monster.identified = false;
    }
}

// Wrong-guess bit set (R39).
u32 setWrongGuessBits(Ctx& ctx, u32 bits) {
    // RESOLVED-BY-REFERENCE: Perception.cs WrongGuess draws Next(23) and rejects bits 0-5, PIT,
    // STAIRS_UP, STAIRS_DOWN, WATER, QUICKSAND, ROCK, FOG, CHUTE and TELEPORTER. The eight
    // invisible kinds that always pass are 6-10, 17, 18 and 22, so the loop always ends.
    // UNKNOWN: the reference also rejects the real answer (realBit). This signature has no
    // realBit parameter, so that exclusion cannot be applied here.
    for (;;) {
        const i32 bit = static_cast<i32>(ctx.rng.below(23));
        const bool invisible = (bit >= 6 && bit <= 10) || bit == 17 || bit == 18 || bit == 22;
        if (invisible) {
            return bits | (static_cast<u32>(1) << static_cast<u32>(bit));
        }
    }
}

// Seer charge (R40). The fee is passed in; the fee amount is UNKNOWN.
bool seerCharge(Ctx& ctx, Character& payer, Currency fee) {
    (void)ctx;
    // R40 / c30: charPay mechanics. Implemented locally because ChargeResult fields are not in
    // the API index. Hand gold is spent first, then bank gold.
    if (fee <= 0) {
        return true;
    }
    if (payer.purse() < fee) {
        return false;
    }
    const Currency fromHand = minv(payer.wallet.goldOnHand, fee);
    payer.wallet.goldOnHand -= fromHand;
    const Currency fromBank = fee - fromHand;
    payer.wallet.goldInBank -= fromBank;
    return true;
}

// Blur spread (R41).
i32 blurSpreadSquares(bool isMonster) {
    // R41 / c30: the monster uses four squares. UNKNOWN for characters and items; zero assumed.
    return isMonster ? 4 : 0;
}

// Dead body facing (R42 / c48).
i32 deadBodyFacing(i32 leadFacing) {
    // UNKNOWN: whether Facing is stored 1-based. Result base unresolved (R42).
    return (leadFacing + kBodyFacingOffset) & 3;
}

// Facing arrow cell (R43 / c63).
i32 facingArrowCell(i32 facing) {
    // CONFLICT: with facing 1 this gives cell 23, which is also the RESCUE floor bit. Unresolved.
    return facing + kFacingArrowOffset;
}

// Chute lookup square (R45 / c45).
Location chuteLookupSquare(i32 x, i32 y) {
    // UNKNOWN: the +1 is applied here as in c45. Probably belongs at the API boundary (R45).
    Location loc{};
    loc.x = x + 1;
    loc.y = y + 1;
    return loc;
}

// Found yourself (R46, R47).
void foundYourself(Ctx& ctx, Character& who) {
    // R46 / R47: clears lost position and lost level. UNKNOWN: whether lost facing is also
    // cleared; left unchanged. Order within a step not stated.
    if (!who.lost.position && !who.lost.level) {
        return;
    }
    who.lost.position = false;
    who.lost.level = false;
    // RESOLVED-BY-REFERENCE: Lost.cs TryToWorkItOut emits "You've figured out where you are!"
    ctx.say("dungeon.found_yourself", "You've figured out where you are!", {}, {});
}

// Try to work it out (R47). Roll is the notice roll against perception (R37).
bool tryToWorkItOut(Ctx& ctx, Character& who) {
    // RESOLVED-BY-REFERENCE: Lost.cs TryToWorkItOut. Three independent rolls on
    // weight = Log(perception)^2 with divisors 15 (level), 20 (facing), 25 (position);
    // each passes when weight / n > Rnd * 100. noticePasses is no longer used here.
    // UNKNOWN: the reference takes perception from Rules.BestAbilityChance over the party;
    // that function is not in this signature, so who.perception is used.
    if (!who.isLost()) {
        return true;
    }
    const double perception = static_cast<double>(who.perception);
    const double weight = perception <= 0.0 ? 0.0 : std::log(perception) * std::log(perception);

    if (who.lost.level && weight / 15.0 > ctx.rng.nextDouble() * 100.0) {
        who.lost.level = false;
        ctx.say("dungeon.figured_level", "You've figured out what level you're on...", {}, {});
    }
    if (who.lost.facing && weight / 20.0 > ctx.rng.nextDouble() * 100.0) {
        who.lost.facing = false;
        ctx.say("dungeon.figured_facing", "You've figured out what direction you're facing...", {}, {});
    }
    if (weight / 25.0 > ctx.rng.nextDouble() * 100.0) {
        foundYourself(ctx, who);
    }
    return !who.isLost();
}

// Square effects on arrival (R47, c44 order preferred).
void applySquareEffects(Ctx& ctx, Arr<Character, kPartyMaxSize>& members, i32 partySize, FloorState& floor, Location& loc) {
    (void)members;
    (void)partySize;
    const i32 index = squareIndexOf(loc.x, loc.y);
    if (index < 0 || index >= static_cast<i32>(floor.squares.size())) {
        return;
    }
    SquareState& square = floor.squares[static_cast<u32>(index)];

    // UNKNOWN: order of automap recording versus square effects is not stated (R47).
    // Automap recording is placed first here.
    mapSquareOnAutomap(ctx, square, false, false);

    // c44 order (preferred), steps not specified in this SRD part:
    //   1. water / quicksand (WADE)            UNKNOWN: handled by the water module
    //   2. anti-magic and stud perception       UNKNOWN: square fields not given
    //   3. extinguisher, facing, rotator        UNKNOWN: square fields not given
    //   4. pit                                  handled by dungeon2b
    //   5. stairs caption, teleporter, chute    handled by dungeon2b / dungeon1b
    // Only automap recording is implemented in this part.
}

// Automap mapping (R48, engine preferred).
void mapSquareOnAutomap(Ctx& ctx, SquareState& square, bool fogEdge, bool wallBump) {
    (void)ctx;
    // R48 / c42 / c44: fog edges are not mapped in the grey view. A wall bump is recorded in fog.
    if (fogEdge && !wallBump) {
        return;
    }
    square.explored = true;
    // UNKNOWN: encoding of the wall-bump record in square fields is not given. Not stored here.
    (void)wallBump;
}

// Rotator loss (R49, R42).
void applyRotatorLoss(Ctx& ctx, Character& who) {
    // R49 / c13: the floor is kept, so the floor is not touched.
    // c44: the port rotator uses rng.Next(4), which is 0-based (R42).
    // UNKNOWN: the ability-5 chance of the other overload is not part of this signature.
    who.location.facing = static_cast<i32>(ctx.rng.below(kFacingCount));
    who.lost.facing = true;
}

// Stairs up destination (R52, engine preferred).
i32 stairsUpDestination(i32 dungeonLevel) {
    // R52 / c39 / c45: stairs up from level 1 reach town (level 0). Otherwise one level up.
    return maxv(dungeonLevel - 1, 0);
}

// Stud spread draw (R53 / c18).
i32 rollStudSpread(Ctx& ctx) {
    // CONFLICT: c24 leaves the replacement value unspecified. c18 gives spread = 1 when
    // Rnd*75 < 50. Otherwise 0 is used. Original unconfirmed.
    return ctx.rng.nextDouble() * kStudDrawRange < kStudDrawLimit ? kStudSpreadOn : 0;
}

// Ethereal portal (R47, c9 engine preferred over c89).
void applyEtherealPortal(Ctx& ctx, Arr<Character, kPartyMaxSize>& members, i32 partySize, const FloorState& floor, Location& loc) {
    // CONFLICT: engine (c9): step one forward, then test the square stepped onto.
    // Wiki (c89) tests the square behind; not used.
    // UNKNOWN: forward direction per facing (R42). Mapping used: see kStepDx / kStepDy.
    const i32 facing = loc.facing & 3;
    loc.x += kStepDx[facing];
    loc.y += kStepDy[facing];

    // UNKNOWN: rock test. SquareState has no direct rock flag in this SRD part; rockRecorded is
    // used as the closest field.
    const i32 index = squareIndexOf(loc.x, loc.y);
    bool isRock = false;
    if (index >= 0 && index < static_cast<i32>(floor.squares.size())) {
        isRock = floor.squares[static_cast<u32>(index)].rockRecorded;
    }

    const i32 count = minv(partySize, kPartyMaxSize);
    for (i32 i = 0; i < count; ++i) {
        Character& member = members[static_cast<u32>(i)];
        member.location = loc;
        if (isRock && !member.isDead()) {
            // Rock = death (c9). UNKNOWN: whether DeathState::Rock is the correct state.
            member.deathState = DeathState::Rock;
            // UNKNOWN: message text is not in the SRD; placeholder wording.
            ctx.say("dungeon.ethereal_rock", "The portal sends you into solid rock.", {}, {});
        }
    }
}

}  // namespace mordor
