// Module dungeon2b: chutes, pits, teleporters, lost state, fog, perception,
// rescue and trap-disarm helpers. Rule numbers (Rxx) refer to the c15p2 notes.
//
// UNKNOWN (module-wide): the SRD excerpt gives quirks and conflicts but not the
// full pseudo-code. Floor-data field names (FloorChute::depth) and the automap,
// square-flag and fog bit numbers are not in the excerpt. They are assumed here
// and marked UNKNOWN where used. Verify against game_data.h and the decompile
// before relying on them.
//
// CONFLICT (module-wide): isLimitedBuild, rescueChance(i32,bool),
// deadBodyFacing and trapDisarmLogTerm are also declared in other headers
// (dungeon3.h, dungeon1b.h) with the same signatures. They are defined only here.

#include <cmath>

#include "mordor/dungeon1b.h"  // ChuteTakeOutcome must be visible before dungeon2b.h
#include "mordor/combat2a.h"   // resolveChestTrap
#include "mordor/dungeon2b.h"

namespace mordor {

namespace {

// UNKNOWN: SquareState::trapFlags bit for anti-magic (R24).
constexpr i32 kAntiMagicFlag = 0x1;
// UNKNOWN: SquareState::trapFlags bit for quicksand (R23/c17).
constexpr i32 kQuicksandFlag = 0x2;
// UNKNOWN: SquareState::bits mask for walls and doors removed by fog (c27).
constexpr u32 kWallDoorMask = 0x0Fu;
// UNKNOWN: SquareState::bits mask for secret doors (c13).
constexpr u32 kSecretDoorMask = 0x100u;
// UNKNOWN: automap bit numbers (c16 notes the numbering was lost).
constexpr i32 kAutomapTeleporterBit = 3;  // MAZEMOD 631
constexpr i32 kAutomapBumpBit = 5;        // c42/c44
constexpr i32 kAutomapLostBit = 6;        // c9
// UNKNOWN: safety cap on teleport redraws (the original has no stated cap).
constexpr i32 kTeleportDrawTries = 100000;
// R31 (c17): search ends at this many days (2,000,000-day cap).
constexpr i64 kRescueDayCap = 2000000;
// Take outcome codes (numeric; ChuteTakeOutcome names are not in the excerpt).
constexpr i32 kTakeOk = 0;
constexpr i32 kTakeRefused = 1;

// UNKNOWN: ChuteTakeOutcome value names are not in the SRD excerpt.
ChuteTakeOutcome outcomeFromCode(i32 code) {
    return static_cast<ChuteTakeOutcome>(code);
}

bool validIndex(const FloorState& f, i32 idx) {
    return idx >= 0 && idx < static_cast<i32>(f.squares.size());
}

// UNKNOWN: the floor is assumed to be a square grid, side = floor(sqrt(count)).
i32 gridSide(const FloorState& f) {
    const i32 n = static_cast<i32>(f.squares.size());
    i32 s = 0;
    while ((s + 1) * (s + 1) <= n) {
        ++s;
    }
    return s;
}

i32 squareIndexOf(const FloorState& f, i32 x, i32 y) {
    const i32 s = gridSide(f);
    if (s <= 0 || x < 0 || y < 0 || x >= s || y >= s) {
        return -1;
    }
    return y * s + x;
}

// UNKNOWN: N is y-1, E is x+1, S is y+1, W is x-1.
i32 squareAhead(const FloorState& f, i32 x, i32 y, Facing facing) {
    switch (facing) {
        case Facing::N:
            return squareIndexOf(f, x, y - 1);
        case Facing::E:
            return squareIndexOf(f, x + 1, y);
        case Facing::S:
            return squareIndexOf(f, x, y + 1);
        case Facing::W:
            return squareIndexOf(f, x - 1, y);
    }
    return -1;
}

}  // namespace

// ---------------------------------------------------------------------------
// Chutes
// ---------------------------------------------------------------------------

// R2: per-member float check, evaluated as written: Rnd * 100 <= kChuteFloatPercent.
// CONFLICT/UNKNOWN: the direction of the comparison is unresolved (the literal
// reading gives about 7%, the test reports about 88%). Implemented literally.
// R1 (preferred engine): levitation is not checked here; the roll is per member.
bool chuteFloatRoll(Ctx& ctx, const Character& member, i32 dungeonLevel) {
    (void)member;
    return ctx.rng.nextDouble() * 100.0 <= static_cast<double>(chuteFloatPercent(dungeonLevel));
}

// R2/R3: the stated threshold is used for every level.
// UNKNOWN: threshold for levels other than 8 (R3).
i32 chuteFloatPercent(i32 dungeonLevel) {
    (void)dungeonLevel;
    return kChuteFloatPercent;
}

// R4 / c30: landing level is the from level plus the depth. Depth 0 is no change.
i32 chuteLandingLevel(i32 fromLevel, i32 depth) {
    return fromLevel + depth;
}

// R5 (preferred engine): depth comes from the chute record.
// UNKNOWN: FloorChute field name 'depth' is assumed; verify against game_data.h.
i32 chuteDepth(const FloorChute& chute) {
    return static_cast<i32>(chute.depth);
}

// R7 (preferred c44): Log(size + 2), rounded to an integer.
// UNKNOWN: natural log assumed; Integer versus Round_half_even is unresolved,
// so roundHalfEven (named by the c66 test) is used.
i32 pitDamage(i32 pitSize) {
    if (pitSize + 2 <= 0) {
        return 0;  // UNKNOWN: log of a non-positive value; treated as no damage
    }
    return static_cast<i32>(roundHalfEven(std::log(static_cast<double>(pitSize) + 2.0)));
}

// R8 / c30 / c44: the original rolls each member; the current port applies the
// roll per fallen member. UNKNOWN: which members fall. Simplest choice: every
// living member falls and takes pitDamage. Companions are not hit here
// (c30: list update and sub0930 handle companions).
PitFallResult applyPitFall(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 pitSize) {
    (void)companions;
    PitFallResult out;
    for (i32 i = 0; i < party.size; ++i) {
        Character& m = party.members[static_cast<u32>(i)];
        if (m.isDead()) {
            continue;
        }
        out.fallen += 1;
        const i32 dmg = pitDamage(pitSize);
        m.vitals.curHits -= dmg;
        if (m.vitals.curHits <= 0) {
            // UNKNOWN: death threshold at 0 hits assumed.
            m.vitals.curHits = 0;
            m.deathState = DeathState::Dead;
            out.killed += 1;
        }
        ctx.say("dungeon2b.pit.fall", "You fall into the pit.",
                {static_cast<i64>(dmg)}, {m.name});
    }
    return out;
}

// c30: the party list update removes fallen (dead) members, keeping order.
// Companions of a removed member are not released here (sub0930's job).
void pruneFallenMembers(Ctx& ctx, Party& party) {
    i32 w = 0;
    for (i32 r = 0; r < party.size; ++r) {
        if (party.members[static_cast<u32>(r)].isDead()) {
            ctx.say("dungeon2b.party.prune", "A member is removed from the party.", {}, {});
            continue;
        }
        if (w != r) {
            party.members[static_cast<u32>(w)] = party.members[static_cast<u32>(r)];
        }
        ++w;
    }
    for (i32 r = w; r < party.size; ++r) {
        party.members[static_cast<u32>(r)] = Character{};
    }
    party.size = w;
    party.wiped = (w == 0);
}

// c66: the original writes the area of the landed-on square on the lower floor.
i32 chuteLandingArea(const FloorState& lowerFloor, i32 squareIndex) {
    if (!validIndex(lowerFloor, squareIndex)) {
        return -1;
    }
    return lowerFloor.squares[static_cast<u32>(squareIndex)].areaId;
}

// TAKE (c30, c45): drops everyone; no box and no sound. The per-member float
// roll (R2) applies to walked chutes, not to TAKE.
// c45: a TAKE on the limited build's dead-end level is refused.
// c30: depth 0 means no floor change.
// c30: no party-list prune after a taken chute.
// UNKNOWN: chute damage on TAKE is not applied (R6 expression unknown).
ChuteTakeOutcome takeChute(Ctx& ctx, Party& party, Vec<Companion>& companions,
                           i32 depth, bool deadEndLevel) {
    (void)companions;
    if (deadEndLevel) {
        ctx.say("dungeon2b.chute.take.refused", "The chute will not take you.", {}, {});
        return outcomeFromCode(kTakeRefused);
    }
    if (depth == 0) {
        return outcomeFromCode(kTakeOk);
    }
    for (i32 i = 0; i < party.size; ++i) {
        Character& m = party.members[static_cast<u32>(i)];
        m.location.dungeonLevel = chuteLandingLevel(m.location.dungeonLevel, depth);
    }
    party.dungeonLevel = chuteLandingLevel(party.dungeonLevel, depth);
    ctx.say("dungeon2b.chute.take", "You drop down the chute.",
            {static_cast<i64>(depth)}, {});
    return outcomeFromCode(kTakeOk);
}

// c47: a chute under the caster skips SettleArea, because the chute fall
// settles the area it ends in.
bool settleAreaAfterFall(bool chuteUnderCaster) {
    return !chuteUnderCaster;
}

// ---------------------------------------------------------------------------
// Teleporters and automap
// ---------------------------------------------------------------------------

// MAZEMOD 631: a teleporter is always noticed.
// R11 (preferred engine, c28/c63): the teleporter bit is written to the automap.
void noticeTeleporter(Ctx& ctx, SquareState& sq) {
    writeAutomapBit(ctx, sq, kAutomapTeleporterBit);
}

// sub087C; MAZEMOD 634-637. UNKNOWN: bit numbering (c16).
void writeAutomapBit(Ctx& ctx, SquareState& sq, i32 bit) {
    (void)ctx;
    if (bit < 0 || bit > 30) {
        return;
    }
    sq.automap |= (1 << bit);
}

// R12 (c22): the draw rejects rock and teleporter squares.
// UNKNOWN: rock is taken as rockRecorded.
bool teleportDrawAccepts(const FloorState& floor, i32 squareIndex) {
    if (!validIndex(floor, squareIndex)) {
        return false;
    }
    const SquareState& sq = floor.squares[static_cast<u32>(squareIndex)];
    return !sq.rockRecorded && !sq.teleporterPresent;
}

// Redraws until a square is accepted. Returns -1 if the cap is reached or the
// floor is empty. The original has no stated cap; the cap is a safety bound.
i32 drawTeleportDestination(Ctx& ctx, const FloorState& floor) {
    const i32 n = static_cast<i32>(floor.squares.size());
    if (n <= 0) {
        return -1;
    }
    for (i32 tries = 0; tries < kTeleportDrawTries; ++tries) {
        const i32 idx = ctx.rng.below(n);
        if (teleportDrawAccepts(floor, idx)) {
            return idx;
        }
    }
    return -1;
}

// CONFLICT R13: the original re-runs the square routine; the port settles and
// describes only. Implemented as the port: describe only, no re-run and no
// loop guard (chains are not followed here).
void rerunSquare(Ctx& ctx, Party& party, Vec<Companion>& companions,
                 FloorState& floor, i32 squareIndex) {
    (void)party;
    (void)companions;
    if (!validIndex(floor, squareIndex)) {
        return;
    }
    ctx.say("dungeon2b.rerun.describe", "You find yourself in a new square.",
            {static_cast<i64>(squareIndex)}, {});
}

// R14 (c56): exactly three setters: the teleporter square, the Teleport spell,
// and a lone chest trap.
bool forcedFlagSetBy(ForcedFlagSetter who) {
    switch (who) {
        case ForcedFlagSetter::TeleporterSquare:
        case ForcedFlagSetter::TeleportSpell:
        case ForcedFlagSetter::LoneChestTrap:
            return true;
    }
    return false;
}

// R15 (c46): the chest teleporter moves only the opener, and only in a party.
// Alone, the room is settled at once and nothing moves.
// UNKNOWN: the square index is written to location.x with y = 0, because the
// mapping from square index to x,y is not in the excerpt.
void applyChestTeleporter(Ctx& ctx, Party& party, FloorState& floor, i32 openerIndex) {
    if (openerIndex < 0 || openerIndex >= party.size) {
        return;
    }
    if (party.size <= 1) {
        ctx.say("dungeon2b.chest.teleporter.alone", "The room is settled.", {}, {});
        return;
    }
    const i32 dest = drawTeleportDestination(ctx, floor);
    if (dest < 0) {
        return;
    }
    Character& opener = party.members[static_cast<u32>(openerIndex)];
    opener.location.x = dest;
    opener.location.y = 0;
    ctx.say("dungeon2b.chest.teleporter", "The opener is teleported.",
            {static_cast<i64>(dest)}, {opener.name});
}

// R16 UNKNOWN: whether the first draw uses the old level or the leader's new
// level is not stated. The party's current level is used.
i32 teleporterDrawLevel(const Party& party) {
    return party.dungeonLevel;
}

// R17 (c35): the limited spell clamps the level to kLimitedBuildTeleportCap
// before the fall and rock tests.
i32 limitedSpellLevel(bool limitedBuild, i32 level) {
    if (limitedBuild && level > kLimitedBuildTeleportCap) {
        return kLimitedBuildTeleportCap;
    }
    return level;
}

// R17/R22 (c44, c63): the limited build denies destinations beyond its last
// level. Level 0 is not checked on the teleporter-square path (c22, c44).
bool teleporterSquareDenied(bool limitedBuild, i32 destLevel, i32 lastLevel) {
    return limitedBuild && destLevel > lastLevel;
}

// R18 (c22): rock if level > MaxDungeonLevel, or level > dungeon count.
bool teleportLevelIsRock(i32 level, i32 maxDungeonLevel, i32 dungeonLevelCount) {
    return level > maxDungeonLevel || level > dungeonLevelCount;
}


// R23 (preferred c44): effects in kSquareEffectOrder order.
// UNKNOWN: pit, chute, stairs, rotator, extinguisher and water flags come from
// floor data that is not in the excerpt. Those effects are not applied here.
void resolveSquareEffects(Ctx& ctx, Party& party, Vec<Companion>& companions,
                          FloorState& floor, i32 squareIndex) {
    (void)party;
    (void)companions;
    if (!validIndex(floor, squareIndex)) {
        return;
    }
    SquareState& sq = floor.squares[static_cast<u32>(squareIndex)];
    for (i32 k = 0; k < 9; ++k) {
        switch (kSquareEffectOrder[static_cast<u32>(k)]) {
            case 0:  // WaterOrQuicksand: UNKNOWN flags; not applied
                break;
            case 1:  // AntiMagicAndStudPerception: R24 refused casts
                if (squareRefusesCasts(sq)) {
                    ctx.say("dungeon2b.square.antimagic", "Magic fails here.", {}, {});
                }
                break;
            case 2:  // Extinguisher: UNKNOWN (R26); not applied
                break;
            case 3:  // Facing: UNKNOWN; not applied
                break;
            case 4:  // Rotator: see loseBearingByRotator; flag not in excerpt
                break;
            case 5:  // Pit: applyPitFall needs pit size from floor data; not applied
                break;
            case 6:  // StairsCaption: UNKNOWN text; not applied
                break;
            case 7:  // Teleporter: always noticed (MAZEMOD 631)
                if (sq.teleporterPresent) {
                    noticeTeleporter(ctx, sq);
                    ctx.say("dungeon2b.square.teleporter", "You feel a teleporter.", {}, {});
                }
                break;
            case 8:  // Chute: UNKNOWN chute data; not applied
                break;
            default:
                break;
        }
    }
}

// R24 (preferred c28): the anti-magic square refuses casts.
// UNKNOWN: flag bit; whether the extinguisher also refuses (c36) is unconfirmed.
bool squareRefusesCasts(const SquareState& sq) {
    return (sq.trapFlags & kAntiMagicFlag) != 0;
}

// R25 (preferred engine, c9): step one square forward, then test the square
// stepped onto. Rock there means death. Off-map is treated as no kill (UNKNOWN).
bool etherealPortalKills(const FloorState& floor, i32 x, i32 y, Facing facing) {
    const i32 idx = squareAhead(floor, x, y, facing);
    if (!validIndex(floor, idx)) {
        return false;
    }
    return floor.squares[static_cast<u32>(idx)].rockRecorded;
}

// R27 (preferred c30): the same-area path turns timCombat off. Modelled here
// by zeroing the countdown. UNKNOWN: countdown 0 taken as the clock off. The
// clock is not restarted until the next monShowMonsters. CONFLICT with c44.
void suspendCombatClockSameArea(Ctx& ctx, Encounter& encounter) {
    encounter.countdown = 0;
    ctx.say("dungeon2b.combatClock.suspend", "The combat clock is paused.", {}, {});
}

// R28 (engine, c20): a lair is the monster's home area.
bool isLairOf(const Creature& monster, i32 areaId) {
    return monster.homeArea == areaId;
}

// R30 (preferred engine, c39): a rescued body arrives at the town dead screen.
bool rescueArrivesInTown(const Character& body) {
    (void)body;
    return true;
}

// R31 (c17): chance = max(16 - L, 0.1). The easy-find addition (16 - L) is
// applied after the floor, so levels above 16 give a negative chance.
// CONFLICT: also declared in dungeon1b.h with the same signature.
// UNKNOWN: units; the chance is taken as a percentage per day (ceiling 100).
double rescueChance(i32 level, bool easyFind) {
    // R-rescue [c17]: chance = max(16 - L, 0.1). Wiki [c97] uses 17; CONFLICT, c17 preferred.
    // The SRD values are pinned here. They must agree with kRescueBaseLevel and kRescueMinChance in game_data.
    constexpr i32 kRescueBaseSrd = 16;
    constexpr double kRescueFloorSrd = 0.1;
    const i32 base = kRescueBaseSrd - level;
    double chance = base > 0 ? static_cast<double>(base) : kRescueFloorSrd;
    if (easyFind) {
        // R-rescue [c17]: easy-find adds 16 - L, applied after the 0.1 floor and not clamped.
        // For levels above 16 the chance goes negative, so the body is never found (c17).
        chance += static_cast<double>(base);
    }
    return chance;
}

// R31 (c17): one day at a time until found or the day cap (2,000,000) is hit.
// A negative chance is never found, so the cap ends the loop.
RescueResult searchForBody(Ctx& ctx, i32 level, bool easyFind) {
    RescueResult r;
    const double chance = rescueChance(level, easyFind);
    while (r.daysSearched < kRescueDayCap) {
        r.daysSearched += 1;
        if (ctx.rng.nextDouble() * 100.0 < chance) {
            r.found = true;
            break;
        }
    }
    return r;
}

// R32 (engine, c11): only Resurrect undoes death from rock.
// CONFLICT: the wiki's Rock Retrieve (c91) is not modelled.
bool rockUndoneBy(RaiseKind kind) {
    return kind == RaiseKind::Resurrect;
}



// R34 (c24, c97): disarm stat = WIS/4 + DEX/2.
double trapDisarmStat(i32 wis, i32 dex) {
    return static_cast<double>(wis) / 4.0 + static_cast<double>(dex) / 2.0;
}

// ---------------------------------------------------------------------------
// Lost state
// ---------------------------------------------------------------------------

// c13: a rotator loss sets position and facing only. The floor flag is untouched.
void loseBearingByRotator(Ctx& ctx, Party& party) {
    party.lostPosition = true;
    party.lostFacing = true;
    party.lost = true;
    ctx.say("dungeon2b.rotator.lost", "You have lost your bearings.", {}, {});
}

// c13: FoundYourself resolves all lost state. The position roll is not guarded
// by LostPosition (c13). UNKNOWN: the roll itself is not in the excerpt; the
// flags are cleared directly.
void foundYourself(Ctx& ctx, Party& party) {
    party.lostPosition = false;
    party.lostFacing = false;
    party.lostLevel = false;
    party.lost = false;
    for (i32 i = 0; i < party.size; ++i) {
        party.members[static_cast<u32>(i)].lost = LostState{};
    }
    ctx.say("dungeon2b.found", "You know where you are again.", {}, {});
}

// c13: walking onto mapped ground ends being lost outright.
void stepOntoMappedGround(Ctx& ctx, Party& party) {
    foundYourself(ctx, party);
}

// c9: the step on which you realise you are lost is recorded on the automap.
void realiseLost(Ctx& ctx, Party& party, FloorState& floor, i32 squareIndex) {
    party.lost = true;
    if (validIndex(floor, squareIndex)) {
        writeAutomapBit(ctx, floor.squares[static_cast<u32>(squareIndex)], kAutomapLostBit);
    }
    ctx.say("dungeon2b.lost.realised", "You realise you are lost.", {}, {});
}

// ---------------------------------------------------------------------------
// Fog and mapping
// ---------------------------------------------------------------------------

// c13: fog on the stepped-onto square gates all neighbour writes.
void markNeighbours(Ctx& ctx, FloorState& floor, i32 x, i32 y, bool steppedOntoFogged) {
    if (steppedOntoFogged) {
        return;
    }
    const i32 dxs[4] = {0, 1, 0, -1};
    const i32 dys[4] = {-1, 0, 1, 0};
    for (i32 d = 0; d < 4; ++d) {
        const i32 idx = squareIndexOf(floor, x + dxs[d], y + dys[d]);
        if (validIndex(floor, idx)) {
            setMappedBit(ctx, floor.squares[static_cast<u32>(idx)]);
        }
    }
}

// c27: fog removes walls and doors, not the square.
// UNKNOWN: the wall and door mask is assumed.
void applyFog(Ctx& ctx, SquareState& sq) {
    (void)ctx;
    sq.bits &= ~kWallDoorMask;
}

// c27/c28: the mapped bit is set whether or not the square is fogged.
// UNKNOWN: field used is 'explored'.
void setMappedBit(Ctx& ctx, SquareState& sq) {
    (void)ctx;
    sq.explored = true;
}

// c42/c44: a wall bump in fog is recorded.
void recordBumpInFog(Ctx& ctx, SquareState& sq) {
    writeAutomapBit(ctx, sq, kAutomapBumpBit);
}

// c13: secret doors found before a backfill are forgotten.
void forgetSecretDoorsBeforeBackfill(Ctx& ctx, FloorState& floor) {
    (void)ctx;
    const i32 n = static_cast<i32>(floor.squares.size());
    for (i32 i = 0; i < n; ++i) {
        floor.squares[static_cast<u32>(i)].bits &= ~kSecretDoorMask;
    }
}

// ---------------------------------------------------------------------------
// Perception
// ---------------------------------------------------------------------------

// c17/c31 (MAINMOD2 1962): WIS + StatMods/4.
i32 perceptionAheadRating(const Character& leader) {
    const u32 w = static_cast<u32>(StatIndex::Wis);
    return leader.abilities.stats[w] + leader.abilities.statMods[w] / 4;
}

// c17/c28: announces each feature on every turn facing it, with no automap
// check. A teleporter is always noticed (MAZEMOD 631).
// UNKNOWN: other features need floor data not in the excerpt; not announced.
// The floor is const here, so the automap bit is written by noticeTeleporter
// elsewhere; this function only announces.
void perceiveAhead(Ctx& ctx, const Party& party, const FloorState& floor) {
    if (party.size <= 0) {
        return;
    }
    // RESOLVED-BY-REFERENCE: PerceiveAhead.cs Look: "Each feature gets its own Rnd * 100 < perception"
    // and the party's best is "already capped at 95 by Best". Only the teleporter is gated here;
    // the other features' data is not in this excerpt, so they are not emitted.
    i32 rating = perceptionAheadRating(party.leader());
    if (rating > 95) {
        rating = 95;
    }
    const i32 idx = squareAhead(floor, party.x, party.y, party.facing);
    if (!validIndex(floor, idx)) {
        return;
    }
    const SquareState& ahead = floor.squares[static_cast<u32>(idx)];
    if (ahead.teleporterPresent) {
        // ERRATUM ch15 (h): SRD says "always noticed"; reference rolls per feature, TELEPORTER included.
        if (ctx.rng.nextDouble() * 100.0 < static_cast<double>(rating)) {
            ctx.say("dungeon2b.perceive.teleporter", "You sense a teleporter ahead.", {}, {});
        }
    }
}

// c17: certainty is taken from (int)perception. The c17 comment "eight bits
// always pass" does not match a plain count of exclusions, so it is not used.
// UNKNOWN: the exact exclusion effect; certainty = perception - exclusions.
bool squarePerceptionPasses(Ctx& ctx, i32 perception, i32 exclusions) {
    const i32 certainty = perception - exclusions;
    if (certainty >= 100) {
        return true;
    }
    if (certainty <= 0) {
        return false;
    }
    return ctx.rng.nextDouble() * 100.0 < static_cast<double>(certainty);
}

// c17/c28: quicksand suppresses perception only while standing in quicksand,
// and the square is still mapped.
bool quicksandSuppressesPerception(const Party& party, const SquareState& standingOn) {
    (void)party;
    return (standingOn.trapFlags & kQuicksandFlag) != 0;
}

// c46: a chest trap in a party leaves the chest untrapped and full; loot, gold
// and the trap exit are skipped. Alone, the trap is resolved and "You realize"
// is shown (sub08E1).
void resolveChestTrapInParty(Ctx& ctx, Party& party, SquareState& sq) {
    if (party.size > 1) {
        sq.chestTrap = ChestTrap::NotTrapped;
        ctx.say("dungeon2b.chest.partyTrap", "The chest is not trapped while others are here.",
                {}, {});
        return;
    }
    ctx.say("dungeon2b.chest.realize", "You realize the chest is trapped.", {}, {});
    resolveChestTrap(ctx, party, sq.chestTrap);
}

// c45: SyncAreas writes area -1 at level 0. A body in rock is written as area 0
// and is dropped from the party immediately after.
void syncAreas(Ctx& ctx, Party& party, FloorState& floor) {
    const i32 idx = squareIndexOf(floor, party.x, party.y);
    const bool onMap = validIndex(floor, idx);
    const i32 areaHere = onMap ? floor.squares[static_cast<u32>(idx)].areaId : -1;
    bool rockBody = false;
    for (i32 i = 0; i < party.size; ++i) {
        Character& m = party.members[static_cast<u32>(i)];
        if (party.dungeonLevel == 0) {
            m.location.area = -1;
            continue;
        }
        if (m.isDead() && onMap && floor.squares[static_cast<u32>(idx)].rockRecorded) {
            m.location.area = 0;
            rockBody = true;
            continue;
        }
        m.location.area = areaHere;
    }
    if (rockBody) {
        pruneFallenMembers(ctx, party);
    }
}

// c45: a companion put down is released from its slot, and the body is not
// left lying in its slot.
void putDownCompanion(Ctx& ctx, Companion& companion) {
    companion.occupied = false;
    companion.released = true;
    ctx.say("dungeon2b.companion.putDown", "You put the companion down.",
            {static_cast<i64>(companion.slot)}, {companion.name});
}

}  // namespace mordor
