// death1a.cpp: death recording, party wipe and prune, rescue, morgue and
// pending-raise rules from SRD chapter 18 part c18p1 (rules R3 to R11).
#include "mordor/death1a.h"

#include <cmath>

#include "mordor/math_util.h"
#include "mordor/shared_helpers.h"

namespace mordor {

static_assert(static_cast<i32>(DeathState::Rock) == kDeathCodeRock, "R3: rock death code is 8");
static_assert(static_cast<i32>(DeathState::Stoned) == kDeathCodeStoned, "R3: stoned death code is 2");

namespace {


// R5 step 5 / R6: remove the dead while more than one body is loaded.
void shiftCounted(Arr<bool, kPartyMaxSize>& counted, i32 index) {
    for (i32 j = index; j + 1 < kPartyMaxSize; ++j) {
        counted[uidx(j)] = counted[uidx(j + 1)];
    }
    counted[uidx(kPartyMaxSize - 1)] = false;
}

// R5 step 5 and R6 pseudo-code: prune members at nought (hits <= 0).
// A member already counted in this wipe is not counted again.
void pruneTheDead(Ctx& ctx, Party& party, Arr<bool, kPartyMaxSize>& countedDead) {
    i32 i = 0;
    while (i < party.size) {
        Character& c = party.members[uidx(i)];
        if (party.size > 1 && isUpdateCasualty(c)) {
            if (!countedDead[uidx(i)]) {
                applyPruneDeath(ctx, c);
                countedDead[uidx(i)] = true;
            }
            if (removePartyMember(party, i)) {
                shiftCounted(countedDead, i);
                continue;
            }
        }
        ++i;
    }
}

}  // namespace

// R3 step 4 / R6 step 2: guild log line. Uses the R6 wording, DiedSomewhere
// when the level is lost and DiedOnLevel otherwise.
// UNKNOWN: exact guild log text is not given in c18p1.
void logDeath(Ctx& ctx, const Character& who) {
    if (who.lost.level) {
        ctx.say("death.guildlog.died_somewhere", "died somewhere", {}, {who.name});
    } else {
        ctx.say("death.guildlog.died_on_level", "died on level N", {who.location.dungeonLevel}, {who.name});
    }
}

// R4 step 4: DeadScreenOwed = IsDead and party.Size == 1.
bool deadScreenOwed(const Character& who, const Party& party) {
    return who.isDead() && party.size == 1;
}

// R5 step 2: a member with hits at or below 0, or who is stoned.
bool isWipeCasualty(const Character& who) {
    return who.vitals.curHits <= 0 || who.deathState == DeathState::Stoned;
}

// R5 (PartyWiped): mark casualties, log the wipe, count newly dead once,
// prune if more than one body remains, and flag the dead screen for a lone body.
void onPartyWiped(Ctx& ctx, Party& party, Arr<bool, kPartyMaxSize>& countedDead) {
    party.wiped = true;

    // R5 step 1: StoreRoom. UNKNOWN: room storage is outside this module.

    Arr<bool, kPartyMaxSize> newlyDead{};
    bool anyRock = false;
    for (i32 i = 0; i < party.size; ++i) {
        Character& c = party.members[uidx(i)];
        if (!isWipeCasualty(c)) {
            continue;
        }
        // R5 step 2: IsDead = true (a stoned member keeps its Stoned state).
        if (c.deathState == DeathState::Alive) {
            c.deathState = DeathState::Dead;
        }
        newlyDead[uidx(i)] = true;
        if (c.deathState == DeathState::Rock) {
            anyRock = true;
        }
    }

    // R5 step 3: log the wipe once.
    if (anyRock) {
        ctx.say("death.wipe.dead_in_rock", "dead in rock", {}, {});
    } else {
        ctx.say("death.wipe.dead_in_dungeon", "dead in the dungeon", {}, {});
    }

    // R5 step 4: each newly dead, not yet counted: +1 Deaths and a log line.
    for (i32 i = 0; i < party.size; ++i) {
        if (newlyDead[uidx(i)] && !countedDead[uidx(i)]) {
            Character& c = party.members[uidx(i)];
            c.deaths += 1;
            logDeath(ctx, c);
            countedDead[uidx(i)] = true;
        }
    }

    // R5 step 5: if more than one body remains, prune the dead first.
    if (party.size > 1) {
        pruneTheDead(ctx, party, countedDead);
    }

    // R5 step 6: exactly one left, the dead screen (Sub0930) runs.
    // UNKNOWN: Sub0930 is not declared here; the event asks the caller to run it.
    if (party.size == 1 && deadScreenOwed(party.leader(), party)) {
        ctx.say("death.wipe.dead_screen", "dead screen due", {}, {party.leader().name});
    }
}

// R6 step 1 pseudo-code test (CurHits == 0) and the quirk "Hits <= False".
// CONFLICT: the pseudo-code says == 0 but the prose says zero or below; the
// original quirk compares with <= 0. Chosen: <= 0.
bool isUpdateCasualty(const Character& who) {
    return who.vitals.curHits <= 0;
}

// R6 step 2: ordinary death counted and guild-logged. Saving the body is the
// caller's job (no file IO in the core).
void applyPruneDeath(Ctx& ctx, Character& who) {
    who.vitals.curHits = 0;
    // R28 / R49: Sub0930Head = ReleaseFollowers; EndEveryAilment (R45).
    // Unparalyse (R14): ParalysisLevel = 0 and the paralysed flag clears.
    who.ailments.paralysed = false;
    who.ailments.paralysisLevel = 0;
    // CurePoison, CureDisease
    who.ailments.poisonLevel = 0;
    who.ailments.diseaseLevel = 0;
    // R45 EndEveryAilment: SpellResist[all] = 0
    for (u32 i = 0; i < static_cast<u32>(kResistKinds); ++i) {
        who.resistances.resist[i] = 0;
    }
    // ReleaseFollowers and the carry link: the carrier is cleared.
    who.carryingCharacterName = Str();
    // R45 note: DeathState 2 (Stoned) is not one of Ailments 0-6, so it survives sub048C (ch08 terms).
    if (who.deathState != DeathState::Stoned) {
        who.deathState = DeathState::Dead;
    }
    who.deaths += 1;
    logDeath(ctx, who);
}

// R6 step 3: the last remaining member is never removed. Removes the member at
// index by shifting the rest down.
// UNKNOWN: parallel per-member arrays (combat and item orders) are not shifted here.
bool removePartyMember(Party& party, i32 index) {
    if (party.size <= 1) {
        return false;
    }
    if (index < 0 || index >= party.size) {
        return false;
    }
    for (i32 j = index; j + 1 < party.size; ++j) {
        party.members[uidx(j)] = party.members[uidx(j + 1)];
    }
    party.size -= 1;
    party.members[uidx(party.size)] = Character{};
    return true;
}

}  // namespace mordor
