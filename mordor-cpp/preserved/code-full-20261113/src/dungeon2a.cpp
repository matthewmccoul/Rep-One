// dungeon2a.cpp: Chapter 15 dungeon exploration, part c15p2
// (exit, close, entry hooks, limited build, Seer).
// Pure helpers live in an anonymous namespace; they hold no state.

#include "mordor/dungeon2a.h"
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/events.h"
#include "mordor/state_common.h"
#include "mordor/state_creature.h"
#include "mordor/state_character.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_party.h"

namespace mordor {
namespace {

// Number of Area records in FloorState::areas (Arr<Area, 201>).
// UNKNOWN: index base not stated; scanned as 0..200.
constexpr i32 kAreaSlots = 201;

// FloorFlag bit positions from the Field.Flags table (c13).
constexpr u32 kBitStairsUp = 12u;
constexpr u32 kBitStairsDown = 13u;
constexpr u32 kBitChute = 21u;

char lowerAscii(char c) {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c - 'A' + 'a');
    }
    return c;
}

bool equalNoCase(const Str& a, const Str& b) {
    const i32 n = static_cast<i32>(a.size());
    if (n != static_cast<i32>(b.size())) {
        return false;
    }
    for (i32 i = 0; i < n; ++i) {
        if (lowerAscii(a[i]) != lowerAscii(b[i])) {
            return false;
        }
    }
    return true;
}

// Blur on one axis (R137): Rnd 50% picks the sign, then Int(Rnd*spread).
i32 blurAxis(Ctx& ctx, i32 v, i32 spread, i32 limit) {
    const bool minus = ctx.rng.percent(50);
    // UNKNOWN: spread <= 0 is not stated; no offset is drawn in that case.
    const i32 shift = spread > 0 ? static_cast<i32>(ctx.rng.below(spread)) : 0;
    const i32 moved = minus ? v - shift : v + shift;
    return clampv(moved, 1, limit);
}

}  // namespace

// ---------------------------------------------------------------------------
// Action hooks (R123)
// ---------------------------------------------------------------------------

ActionRunStart beginActionRun(Ctx& ctx, ActionRunState& run, ActionTrigger trigger,
                              const Vec<ModHookSlot>& mods) {
    // R123: an active run refuses and logs the new request.
    if (run.active) {
        ctx.say("dungeon.action.refused",
                "An action run is already active; the new request is refused.",
                {static_cast<i32>(trigger)}, {});
        return ActionRunStart::Refused;
    }
    // R123: with no hooked mod the task completes at once; the context
    // factory is never called, so the run never becomes active.
    const i32 n = static_cast<i32>(mods.size());
    for (i32 i = 0; i < n; ++i) {
        if (mods[i].live && mods[i].hasHook && !mods[i].disabled) {
            run.active = true;
            return ActionRunStart::RunHooks;
        }
    }
    return ActionRunStart::NoHooks;
}

i32 nextHookedMod(const Vec<ModHookSlot>& mods, i32 afterIndex) {
    // R123: hooks run in load order (vector order).
    const i32 n = static_cast<i32>(mods.size());
    const i32 start = maxv(afterIndex + 1, 0);
    for (i32 i = start; i < n; ++i) {
        if (mods[i].live && mods[i].hasHook && !mods[i].disabled) {
            return i;
        }
    }
    return -1;
}

void disableModAfterHookFailure(Ctx& ctx, ModHookSlot& mod) {
    // R123: a hook that throws disables its mod.
    mod.disabled = true;
    ctx.say("dungeon.action.hookDisabled",
            "A mod hook failed and has been disabled.",
            {static_cast<i32>(mod.loadOrder)}, {});
}

void finishActionRun(ActionRunState& run) {
    // R123: the context is closed and the run ends.
    run.active = false;
}

// ---------------------------------------------------------------------------
// Stairs up and exit (R124, R125, R126, R127)
// ---------------------------------------------------------------------------

void climbToTownAlive(Ctx& ctx, Party& party) {
    // R124: the leader's position is placed at the party's square and the
    // party exits to town alive.
    // RESOLVED-BY-REFERENCE: town is dungeon level 0. Rescue.cs CarryHome: "carried to town -
    // DungeonLevel = 0". The town level is therefore 0, not the party's level.
    Location& at = party.members[0].location;
    at.x = party.x;
    at.y = party.y;
    at.dungeonLevel = 0;
    at.facing = static_cast<i32>(party.facing);
    party.inDungeon = false;
    party.onStairs = false;
    ctx.say("dungeon.climb.town", "You climb the stairs to town.", {}, {});
}

bool stairsClimbStillValid(const Party& party) {
    // R124: the climb goes on only if still on the stairs and in the dungeon.
    return party.onStairs && party.inDungeon;
}

bool monstersInArea(const Party& party, const FloorState& floor) {
    // R125 "monsters are in the area".
    // UNKNOWN: exact predicate not stated. Monsters count as present if a
    // living party foe exists, a living foe sits in the leader's area, or the
    // leader's area has a standing group.
    const i32 foeCount = static_cast<i32>(party.foes.size());
    for (i32 i = 0; i < foeCount; ++i) {
        if (party.foes[i].alive) {
            return true;
        }
    }
    const i32 areaIndex = party.members[0].location.area;
    if (areaIndex >= 0 && areaIndex < kAreaSlots) {
        const Area& area = floor.areas[areaIndex];
        for (i32 j = 0; j < 4; ++j) {
            if (area.foes[j].alive) {
                return true;
            }
        }
        if (static_cast<i32>(area.standingGroup.size()) > 0) {
            return true;
        }
    }
    return false;
}

ExitMenuOutcome requestExitToMenu(Ctx& ctx, const Party& party, const FloorState& floor) {
    // R125: refused with a beep when monsters are present and the character is alive.
    // UNKNOWN: "the character" taken as the party leader. The beep is carried by the event code.
    if (monstersInArea(party, floor) && !party.members[0].isDead()) {
        ctx.say("dungeon.exit.blockedBeep",
                "Cannot exit while monsters are in the area!", {}, {});
        return ExitMenuOutcome::BlockedByMonsters;
    }
    ctx.say("dungeon.exit.prompt", "Exit to Main Menu?", {}, {});
    return ExitMenuOutcome::ConfirmPrompt;
}

void confirmExitToMenu(Ctx& ctx, Party& party) {
    // R125 Yes: save, unload party, return to title. The character stays where
    // it stood; there is no town arrival.
    const i32 n = static_cast<i32>(party.size);
    for (i32 i = 0; i < n && i < kPartyMaxSize; ++i) {
        party.members[i].saved = true;
    }
    party.loaded = false;
    party.inDungeon = false;
    ctx.say("dungeon.exit.confirmed", "Saving and returning to the title screen.", {}, {});
}

void refuseWindowClose(Ctx& ctx) {
    // R126: user-initiated window close is refused.
    ctx.say("dungeon.window.closeRefused",
            "Please use the Mordor EXIT buttons to leave the game.", {}, {});
}

SessionCloseOutcome requestSessionEndClose(Ctx& ctx, const Party& party, const FloorState& floor) {
    // R127 (FRMMAIN.BAS:148-191): refused when monsters are present, the
    // character is alive and the level is above 0.
    if (monstersInArea(party, floor) && !party.members[0].isDead() && party.dungeonLevel > 0) {
        ctx.say("dungeon.close.blocked",
                "Cannot exit while monsters are in the area!", {}, {});
        return SessionCloseOutcome::RefusedMonsters;
    }
    ctx.say("dungeon.close.prompt", "Exit Mordor?", {}, {});
    return SessionCloseOutcome::ConfirmExit;
}

TakeTarget takeButtonTarget(const FloorState& floor, const Party& party) {
    // Original quirk: the Take button uses stairs or chute, not Objects & Spells (c33).
    // UNKNOWN: the square flags are read from SquareState::bits using the Field.Flags
    // bit numbers. The square index is the 1-based header index minus one.
    // UNKNOWN: stairs are checked before chute; the order is not stated.
    const i32 idx = squareIndexOf(party.x, party.y) - 1;
    if (idx < 0 || idx >= static_cast<i32>(floor.squares.size())) {
        return TakeTarget::None;
    }
    const u32 bits = floor.squares[idx].bits;
    const bool stairs = (bits & (1u << kBitStairsUp)) != 0u || (bits & (1u << kBitStairsDown)) != 0u;
    if (stairs) {
        return TakeTarget::Stairs;
    }
    if ((bits & (1u << kBitChute)) != 0u) {
        return TakeTarget::Chute;
    }
    return TakeTarget::None;
}

// ---------------------------------------------------------------------------
// Limited (shareware) build (R128 to R131)
// ---------------------------------------------------------------------------

i32 clampLevelRequest(Ctx& ctx, ReleaseKind release, i32 requestedLevel) {
    // R128: every level change goes through this clamp. Limit is 15 (full) or 3 (limited).
    const i32 limit = buildLevelLimit(release);
    // RESOLVED-BY-REFERENCE: LimitedVersion.cs Clamp: "if (!On || level < 0 || level > DeepestLevel
    // || level <= LastLevel) return (level, null);" so requests below 0 or above DeepestLevel pass
    // through unchanged with no message. Requests in (LastLevel, DeepestLevel] are denied with
    // LevelDenied and set to LastLevel. DeepestLevel is mc015C (GameFiles.FullLevels), 15 in the
    // full build.
    constexpr i32 kDeepestLevel = 15; // mc015C
    if (requestedLevel < 0 || requestedLevel > kDeepestLevel) {
        return requestedLevel;
    }
    if (requestedLevel > limit) {
        // R128: denial message; level is set to the limit.
        ctx.say("dungeon.levelDenied",
                "LIMITED VERSION: Request to go to Level n is Denied.",
                {requestedLevel}, {});
        return limit;
    }
    return requestedLevel;
}

ReleaseKind classifyRelease(i32 levelsWord, bool someMonsterLevelAtOrBelowThreshold,
                            bool someItemFloorAtOrBelowThreshold) {
    // R129: Full only if levels >= 15, some monster LevelFound <= 15 and some item Floor <= 15.
    if (levelsWord >= kReleaseLevelThreshold &&
        someMonsterLevelAtOrBelowThreshold &&
        someItemFloorAtOrBelowThreshold) {
        return ReleaseKind::Full;
    }
    return ReleaseKind::Limited;
}

LimitedMessageKind reportLimitedMessage(Ctx& ctx, LimitedAction action, i32 level, i32 lastLevel) {
    // R130. The m0622 and m02A6 message flags are not modelled: the limited
    // build's messages are taken as on (original quirk, c33/c35).
    // For TeleporterDestination, `level` is the destination Z2.
    switch (action) {
        case LimitedAction::TakeChute:
            // m0622 and DungeonLevel = last level
            if (level == lastLevel) {
                ctx.say("dungeon.limited.chuteTakeDenied",
                        "You cannot take this Chute in the Limited Version.",
                        {level, lastLevel}, {});
                return LimitedMessageKind::ChuteTakeDenied;
            }
            return LimitedMessageKind::None;
        case LimitedAction::WalkOnChute:
            // m02A6 and DungeonLevel = last level
            if (level == lastLevel) {
                ctx.say("dungeon.limited.chuteWalkOn",
                        "There is a chute here, but it doesn't do anything in the Limited Version.",
                        {level, lastLevel}, {});
                return LimitedMessageKind::ChuteWalkOnNoEffect;
            }
            return LimitedMessageKind::None;
        case LimitedAction::TeleporterDestination:
            // m02A6 and Z2 > last level
            if (level > lastLevel) {
                ctx.say("dungeon.limited.teleporterDenied",
                        "..but it doesn't go anywhere in the limited version...",
                        {level, lastLevel}, {});
                return LimitedMessageKind::TeleporterDenied;
            }
            return LimitedMessageKind::None;
        default:
            return LimitedMessageKind::None;
    }
}

bool isDestinationDenied(i32 destinationLevel, ReleaseKind release) {
    // R131: teleporter and chute destinations past the limit are denied.
    return destinationLevel > buildLevelLimit(release);
}

// ---------------------------------------------------------------------------
// Seer (R132 to R139)
// ---------------------------------------------------------------------------

bool seerCanReach(i32 dungeonLevel) {
    // R132: nothing below dungeon level 12 (DeepestHeCanReach).
    return dungeonLevel <= kSeerDeepestReach;
}

SeerEarlyResult rollSeerEarlyFailure(Ctx& ctx, i32 level) {
    // R133: fail if Int(Rnd*100)+1 < level*5. On failure a second roll names
    // the level if Rnd*100 >= level*5, otherwise it says nothing.
    SeerEarlyResult result;
    const i32 threshold = level * kSeerEarlyFactor;
    const i32 roll = static_cast<i32>(ctx.rng.below(100)) + 1;
    if (roll < threshold) {
        result.failed = true;
        const double second = ctx.rng.nextDouble() * 100.0;
        result.namesLevel = second >= static_cast<double>(threshold);
    }
    return result;
}

bool isSeerNamePrefix(const Str& typed, const Str& name) {
    // R134: case-insensitive prefix; the name must be at least as long as the typed text.
    const i32 typedLen = static_cast<i32>(typed.size());
    const i32 nameLen = static_cast<i32>(name.size());
    if (nameLen < typedLen) {
        return false;
    }
    for (i32 i = 0; i < typedLen; ++i) {
        if (lowerAscii(typed[i]) != lowerAscii(name[i])) {
            return false;
        }
    }
    return true;
}

SeerNameMatch matchSeerNames(const Str& typed, const Vec<Str>& names) {
    // R134: an exact match wins at once. Count 0 none, 1 one, 2 or more several.
    SeerNameMatch match;
    const i32 n = static_cast<i32>(names.size());
    for (i32 i = 0; i < n; ++i) {
        if (equalNoCase(typed, names[i])) {
            match.count = 1;
            match.index = i;
            match.exact = true;
            return match;
        }
        if (isSeerNamePrefix(typed, names[i])) {
            match.count += 1;
            match.index = i;
        }
    }
    if (match.count != 1) {
        match.index = -1;
    }
    return match;
}

i32 seerFirstRecord(SeerTargetKind kind) {
    // R134 (FRMSEEK.BAS:66-86): monsters from record 1, items from record 0.
    // UNKNOWN: character record start not stated; 0 is used.
    switch (kind) {
        case SeerTargetKind::Monster:
            return kSeerMonsterFirstRecord;
        case SeerTargetKind::Item:
            return kSeerItemFirstRecord;
        case SeerTargetKind::Character:
        default:
            return 0;
    }
}

Str monsterNameFor(const GameData& data, i32 monsterId, i32 nameForm, IdLevel idLevel) {
    // R134: the plural branch is monsterNameFor(Me, 2, record) with IdLevel 0.
    // UNKNOWN: MonsterDef name fields were not supplied to this module. Assumed:
    // data.monsters indexed by monster id, with .name and .pluralName.
    // idLevel is accepted for the call shape; the R134 plural call passes 0 and it is not used here.
    (void)idLevel;
    if (monsterId < 0 || monsterId >= static_cast<i32>(data.monsters.size())) {
        return Str();
    }
    const MonsterDef& def = data.monsters[monsterId];
    if (nameForm == kSeerPluralNameForm) {
        // FIXED: MonsterDef has no pluralName member; the plural form is not
        // available, so the singular name is returned.
        return def.name;
    }
    return def.name;
}

SeerMonsterPick locateMonster(Ctx& ctx, const FloorState& floor, i32 monsterId, i32 monsterType) {
    // R135: a lair is returned 75% of the time when reached (Rnd*100 < 75).
    // Otherwise a random candidate area whose spawn mask admits the type is returned.
    SeerMonsterPick pick;

    i32 lairArea = -1;
    for (i32 i = 0; i < kAreaSlots; ++i) {
        if (floor.areas[i].lairId == monsterId) {
            lairArea = i;
            break;
        }
    }
    if (lairArea >= 0 && ctx.rng.nextDouble() * 100.0 < static_cast<double>(kSeerLairPercent)) {
        pick.lairResult = true;
        pick.areaIndex = lairArea;
        return pick;
    }

    // UNKNOWN: monsterType outside 0..31 cannot be a spawn bit; no candidate is found.
    if (monsterType < 0 || monsterType > 31) {
        pick.areaIndex = -1;
        return pick;
    }
    const u32 typeBit = 1u << static_cast<u32>(monsterType);
    i32 candidates = 0;
    for (i32 i = 0; i < kAreaSlots; ++i) {
        if ((static_cast<u32>(floor.areas[i].spawnMask) & typeBit) != 0u) {
            candidates += 1;
        }
    }
    if (candidates == 0) {
        pick.areaIndex = -1;
        return pick;
    }
    const i32 chosen = static_cast<i32>(ctx.rng.below(candidates));
    i32 seen = 0;
    for (i32 i = 0; i < kAreaSlots; ++i) {
        if ((static_cast<u32>(floor.areas[i].spawnMask) & typeBit) != 0u) {
            if (seen == chosen) {
                pick.areaIndex = i;
                return pick;
            }
            seen += 1;
        }
    }
    pick.areaIndex = -1;
    return pick;
}

i32 countSpawnClasses(i32 spawnMask) {
    // R135 classes = popcount of SpawnMask.
    u32 bits = static_cast<u32>(spawnMask);
    i32 count = 0;
    while (bits != 0u) {
        count += static_cast<i32>(bits & 1u);
        bits >>= 1u;
    }
    return count;
}

SeerCommonality seerCommonality(i32 classCount) {
    // Seer commonality table (c20).
    if (classCount <= 2) {
        return SeerCommonality::VeryCommon;
    }
    if (classCount <= 4) {
        return SeerCommonality::FairlyCommon;
    }
    if (classCount <= 7) {
        return SeerCommonality::Uncommon;
    }
    if (classCount <= 11) {
        return SeerCommonality::Rare;
    }
    return SeerCommonality::AlmostUnheard;
}

SeerFrequency seerFrequency(i32 chancePercent) {
    // Seer frequency table (c20).
    if (chancePercent <= 2) {
        return SeerFrequency::AlmostNever;
    }
    if (chancePercent <= 8) {
        return SeerFrequency::VeryRarely;
    }
    if (chancePercent <= 15) {
        return SeerFrequency::Rarely;
    }
    if (chancePercent <= 35) {
        return SeerFrequency::Uncommonly;
    }
    if (chancePercent <= 50) {
        return SeerFrequency::Commonly;
    }
    return SeerFrequency::VeryCommonly;
}

SeerCharacterReport locateCharacter(Ctx& ctx, const Character& subject, i32 searchLevel,
                                    bool subjectInCity) {
    // R136: a character in the city cannot be located.
    SeerCharacterReport report;
    if (subjectInCity) {
        report.outcome = SeerCharacterOutcome::CityRefused;
        return report;
    }
    // Reference Seer.cs:122. need = level*3; roll = Int(Rnd*100)+1.
    // UNKNOWN: "level" in R136 is taken as searchLevel (see R133 legend).
    const i32 need = searchLevel * kSeerNeedFactor;
    const i32 roll = static_cast<i32>(ctx.rng.below(100)) + 1;
    if (roll < need) {
        if (roll + kSeerLevelOnlySlack >= need) {
            // UNKNOWN: "level only" has no SeerCharacterOutcome value. It is reported as
            // Located with the level set and the coordinates left at 0.
            report.outcome = SeerCharacterOutcome::Located;
            report.level = subject.location.dungeonLevel;
            return report;
        }
        report.outcome = SeerCharacterOutcome::Failed;
        return report;
    }
    report.outcome = SeerCharacterOutcome::Located;
    report.x = subject.location.x;
    report.y = subject.location.y;
    report.level = subject.location.dungeonLevel;
    report.amongDead = subject.isDead();
    return report;
}

BlurredPoint blurPoint(Ctx& ctx, i32 x, i32 y, i32 spread) {
    // R137: each coordinate is shifted by up to `spread` either way and clamped
    // to the floor. Monsters use spread 4; characters and items spread is UNKNOWN (R139).
    BlurredPoint point;
    point.x = blurAxis(ctx, x, spread, kFloorWidth);
    point.y = blurAxis(ctx, y, spread, kFloorHeight);
    return point;
}

Currency chargeSeerFee(Ctx& ctx, Purse& wallet, Currency fee) {
    // R138: the fee is taken as charPay takes it. The bank makes up a short purse.
    // UNKNOWN: charPay's internals are not in this module. Policy chosen: on-hand gold
    // first, then the bank covers the shortfall.
    if (fee <= 0) {
        return 0;
    }
    const Currency fromHand = fee < wallet.goldOnHand ? fee : wallet.goldOnHand;
    wallet.goldOnHand -= fromHand;
    const Currency shortfall = fee - fromHand;
    const Currency fromBank = shortfall < wallet.goldInBank ? shortfall : wallet.goldInBank;
    wallet.goldInBank -= fromBank;
    const Currency charged = fromHand + fromBank;
    if (charged < fee) {
        ctx.say("dungeon.seer.feeShort",
                "The Seer's fee could not be fully paid.",
                {static_cast<i32>(fee), static_cast<i32>(charged)}, {});
    }
    return charged;
}

// ---------------------------------------------------------------------------
// Preserved depth formulas
// ---------------------------------------------------------------------------

bool noteDepth(i32 level, i32& deepestReached) {
    // NoteDepth(level): if level <= DeepestReached: return false; DeepestReached = level; return true.
    if (level <= deepestReached) {
        return false;
    }
    deepestReached = level;
    return true;
}

i32 effectiveDeepest(i32 dungeonDeepest, i32 mapDeepest) {
    // deepest = Max(DungeonState.DeepestReached, map.DeepestVisited); if deepest == 0: deepest = 1.
    i32 deepest = maxv(dungeonDeepest, mapDeepest);
    if (deepest == 0) {
        deepest = 1;
    }
    return deepest;
}

}  // namespace mordor
