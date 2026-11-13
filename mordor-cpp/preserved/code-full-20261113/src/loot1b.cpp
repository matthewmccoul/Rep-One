// loot1b.cpp: chest generation, chest traps, gold splitting and gold transfers.
// Implements the functions declared in include/mordor/loot1b.h.
#include "mordor/loot1b.h"
#include "mordor/combat2a.h"

#include <cmath>
#include <string>

namespace mordor {

namespace {

constexpr i64 kCurrencyUnit = 10000;          // Currency is 1/10000 gold
constexpr double kCurrencyUnitD = 10000.0;

inline u32 slot(i32 i) {
    return static_cast<u32>(i);
}

// Currency -> whole gold as a double (fractions kept).
inline double wholeGold(Currency c) {
    return static_cast<double>(c) / kCurrencyUnitD;
}

// Reduce hit points, floor at zero.
// UNKNOWN: floor at zero assumed; SRD notes give no clamp.
void applyDamage(Character& c, i32 amount) {
    c.vitals.curHits -= amount;
    if (c.vitals.curHits < 0) {
        c.vitals.curHits = 0;
    }
}

// Class modifier row; classes outside 0..3 (ASMODEUS_CHEST = 4) use row 0.
// UNKNOWN: class 4 has no modifiers in the SRD tables.
const ChestClassMod& modOf(ChestClass cls) {
    i32 idx = static_cast<i32>(cls);
    if (idx < 0 || idx >= kChestClassCount) {
        idx = 0;
    }
    return kChestClassMods[slot(idx)];
}

}  // namespace

// ---------------------------------------------------------------------------
// Treasure class (T1, Q2, Q3)
// ---------------------------------------------------------------------------

// T1: bonus is 25 if the monster is a lair monster and BoxChance[2] > 0.
// CONFLICT-2: bonus target ambiguous; the bonus is applied to both thresholds as written.
i32 treasureClassBonus(bool lairMonster, i32 boxChance2) {
    return (lairMonster && boxChance2 > 0) ? 25 : 0;
}

// T1 / Q3: one roll r; class 2 if r < BoxChance[1]+bonus; class 3 if r < BoxChance[2]+bonus
// (overwrites, no break). Class 1 is never produced.
// UNKNOWN: scale of r; implemented as Rnd*100.
ChestClass rollTreasureClass(Ctx& ctx, BoxChanceThresholds boxChance, bool lairMonster) {
    const i32 bonus = treasureClassBonus(lairMonster, boxChance.chance2);
    const double r = ctx.rng.nextDouble() * 100.0;
    i32 k = 0;                                       // class 0: no chest
    if (r < static_cast<double>(boxChance.chance1 + bonus)) {
        k = 2;                                       // BOX
    }
    if (r < static_cast<double>(boxChance.chance2 + bonus)) {
        k = 3;                                       // CHEST overwrites (Q3)
    }
    return static_cast<ChestClass>(k);
}

// T2 gold factor: 0.5, 0.5, 1.0, 1.5.
double chestGoldFactor(ChestClass cls) {
    return modOf(cls).goldFactor;
}

// T2 item roll adjust: none, +15, none, -25.
i32 chestItemRollAdjust(ChestClass cls) {
    return modOf(cls).rollAdjust;
}

// T2 item band adjust: none, -1, none, none.
i32 chestItemBandAdjust(ChestClass cls) {
    return modOf(cls).bandAdjust;
}

// T2 trap chance percent: 0, 25, 75, 90.
i32 chestTrapChancePercent(ChestClass cls) {
    return modOf(cls).trapChancePercent;
}

// T2 / Q2: trapped if Rnd*100 < trap chance.
// UNKNOWN: exact roll form; the notes give only the percentage.
bool rollChestTrapped(Ctx& ctx, ChestClass cls) {
    const double pct = static_cast<double>(chestTrapChancePercent(cls));
    return ctx.rng.nextDouble() * 100.0 < pct;
}

// Q4: the chest trap is cleared on every generation pass. The lock is never cleared
// here, so a lock rolled on a discarded pass still stands.
// UNKNOWN: chestTeleporterTrap cleared with the trap.
void beginChestGenerationPass(Ctx& ctx, SquareState& chest) {
    (void)ctx;
    chest.chestTrap = ChestTrap::NotTrapped;
    chest.trapFlags = 0;
    chest.chestTeleporterTrap = false;
}

// Q1: roll increase from a magic lock is Log(level+10)*5 (VB Log = natural log).
double magicLockRollIncrease(i32 level) {
    const double arg = static_cast<double>(level) + 10.0;
    if (arg <= 0.0) {
        return 0.0;  // UNKNOWN: Log of a non-positive value; no increase assumed
    }
    return std::log(arg) * 5.0;
}

// T4: item band low end by drop level, table for levels 1..16.
// UNKNOWN: levels above 16 use round_half_even(level/2) clamped to >= 1 (T4 note).
i32 itemBandLowEnd(i32 dropLevel) {
    if (dropLevel >= 1 && dropLevel <= 16) {
        return kItemBandLowEnd[slot(dropLevel - 1)];
    }
    const i64 v = roundHalfEven(static_cast<double>(dropLevel) / 2.0);
    return static_cast<i32>(v < 1 ? 1 : v);
}

// T5 / Q5: Log((LevelFound+1) / Log(2)), division inside the log, reproduced as written.
// UNKNOWN: for LevelFound <= -1 the log argument is not positive; NaN/-inf is returned.
double goldDepthTermAsWritten(i32 levelFound) {
    return std::log((static_cast<double>(levelFound) + 1.0) / std::log(2.0));
}

// Q6: group gold under 500 is replaced by Int(Rnd*500)+2 (range 2..501).
// UNKNOWN: unit of the 500 threshold; treated as whole gold (Currency is 1/10000).
Currency applyChestGoldFloor(Ctx& ctx, Currency groupGold) {
    if (groupGold < 500 * kCurrencyUnit) {
        const i64 whole = static_cast<i64>(ctx.rng.below(500)) + 2;
        return whole * kCurrencyUnit;
    }
    return groupGold;
}

// ---------------------------------------------------------------------------
// Opening (T6, CONFLICT-22)
// ---------------------------------------------------------------------------

// T6: sample counts fit count = 10 - 2*Log(disarm - 10), rounded half to even, minimum 2.
// Reproduces 11->10, 12->9, 20->5, 60->2, 99->2 (raw 1.023 raised to 2).
// UNKNOWN: formula fitted to the T6 sample values; the notes give only the table.
// Disarm 10 (Log(0)) and below: no count set, returns 0.
i32 openingRoundsForDisarm(i32 disarm) {
    const i32 d = disarm - 10;
    if (d <= 0) {
        return 0;
    }
    const double raw = 10.0 - 2.0 * std::log(static_cast<double>(d));
    i64 n = roundHalfEven(raw);
    if (n < 2) {
        n = 2;  // T6 minimum
    }
    return static_cast<i32>(n);
}

// CONFLICT-22: a count of 2 opens on the third turn, so turn = count + 1.
// A count of 0 (no count set) gives turn 0.
i32 openingTurnFromCount(i32 count) {
    if (count <= 0) {
        return 0;
    }
    return count + 1;
}

// Q27: display cap of 98 on disarm, applied after confidence (the caller uses the uncapped value).
i32 capDisarmForDisplay(i32 disarmUncapped) {
    return disarmUncapped > 98 ? 98 : disarmUncapped;
}

// Q27: shown-trap test uses the uncapped confidence (may exceed 98).
// UNKNOWN: exact form of the test; implemented as Rnd*100 < confidence.
bool chestTrapSeen(Ctx& ctx, i32 confidenceUncapped) {
    return ctx.rng.nextDouble() * 100.0 < static_cast<double>(confidenceUncapped);
}

// Q20 / CONFLICT: the original tests only DeathState (MAINMOD2.BAS:4064);
// the port's CurHits <= 0 refusal is not reproduced.
bool canOpenChest(const Character& opener) {
    return !opener.isDead();
}

// Lock is read from the square (area) record, as the original does (Q19).
ChestLock chestLockOf(const SquareState& chest) {
    return chest.chestLock;
}

// Q29: gv1D7E is never assigned, so the test reduces to a comparison with zero.
// UNKNOWN: taken as lock != UNLOCKED (CHARM_OPENED = -1 also counts).
// R21, R10 and the preserved formula `locked = lockState > 0`.
// Only MAGIC_LOCKED (1) is a magic lock. CHARM_OPENED (-1) is a separate state
// that the SRD tests as lockState == -1 (the dungeon-level item-roll term).
bool isMagicLockedChest(const SquareState& chest) {
    return static_cast<i32>(chest.chestLock) > 0;
}

// Q43: spot test. The viewer's own DEX (base stat, no gear) against group 0's monster DEX.
// A DEX of zero on either side is logged as an error and leaves the panel as it was.
// UNKNOWN: exact spot formula not given in the notes; implemented as strict DEX comparison.
bool chestTrapPanelShown(Ctx& ctx, const Character& viewer, i32 group0MonsterDex, bool previouslyShown) {
    const i32 viewerDex = viewer.abilities.stats[slot(static_cast<i32>(StatIndex::Dex))];
    if (viewerDex == 0 || group0MonsterDex == 0) {
        ctx.say("loot.spotDexZero", "Trap spot test: a DEX of zero was read; panel unchanged.", {}, {});
        return previouslyShown;
    }
    return viewerDex > group0MonsterDex;
}

// Q43: the Open key asks whether the trap label is showing, so a hidden panel blocks it.
bool openKeyEnabled(bool trapLabelShowing) {
    return trapLabelShowing;
}

// Q41: a disarm (of a trapped chest) awards experience; an untrapped chest gives none.
bool disarmAwardsXp(bool chestWasTrapped) {
    return chestWasTrapped;
}

// Q7: gold is not paid when the chest is not opened or the trap kills its opener.
// CONFLICT-8 / UNKNOWN-16: an opener killed (not the leader) is treated the same way.
bool chestGoldPayable(bool opened, bool openerKilledByTrap) {
    return opened && !openerKilledByTrap;
}

// Q47: the original pays the fight gold at victory only when there is no chest.
bool fightGoldPaidAtVictory(bool chestPresent) {
    return !chestPresent;
}

// Q17 / Q13 / Q14: after an explosion.
// A leader left at nought gets the opener killed. The opener is set dead once; a dead
// opener is not counted again (Q13). If the opener is not in slot 0 and one character
// remains, nobody is set dead (Q14; the original would loop).
// UNKNOWN: Q17 wording is mixed on whether the opener is declared dead; the death is recorded here.
void settleExplosionDeaths(Ctx& ctx, Party& party, i32 opener) {
    if (party.size <= 0 || opener < 0 || opener >= party.size) {
        return;
    }
    if (party.members[0].vitals.curHits > 0) {
        return;  // only a leader at nought triggers this path (Q17)
    }
    i32 alive = 0;
    for (i32 i = 0; i < party.size; ++i) {
        if (!party.members[slot(i)].isDead()) {
            ++alive;
        }
    }
    if (opener != 0 && alive == 1) {
        return;  // Q14: sets nobody dead instead of the original's endless loop
    }
    Character& o = party.members[slot(opener)];
    if (o.isDead()) {
        return;  // Q13: no second death count for an already-dead opener
    }
    o.deathState = DeathState::Dead;
    o.deaths += 1;
    ctx.say("loot.explosionDeath", "The guild log records the death of the opener.", {}, {o.name});
}

// Q37 / CONFLICT-9: receiver index = CInt(Rnd * (n-1)), both ends half weight as in the original.
// The notes say the port draws uniformly; the original's rule is reproduced here.
i32 pickRandomReceiver(Ctx& ctx, i32 memberCount) {
    if (memberCount <= 1) {
        return 0;
    }
    return static_cast<i32>(roundHalfEven(ctx.rng.nextDouble() * static_cast<double>(memberCount - 1)));
}

// T3: bit index = trap value (bit 0 = Not Trapped ... bit 10 = Withering).
bool chestTrapFlagSet(i32 trapFlags, ChestTrap trap) {
    const i32 bit = static_cast<i32>(trap);
    if (bit < 0 || bit > 31) {
        return false;
    }
    return ((static_cast<u32>(trapFlags) >> bit) & 1u) != 0u;
}

// Q28: random false name drawn from the 11 names (indices 0..10); bit 11 is never shown.
ChestTrap pickFalseTrapName(Ctx& ctx) {
    return static_cast<ChestTrap>(ctx.rng.below(kChestTrapBitCount));
}

// T3 fire: damage reduced by Fire resistance/2 percent (Resistances(0), Q12).
// CONFLICT-7: original rounds by integer assignment (MONSMOD.BAS:3530-3533); rounding used here.
// UNKNOWN: negative result (resistance above 200) clamped to 0.
i32 fireTrapDamageAfterResist(i32 baseDamage, i32 fireResist) {
    const double pct = static_cast<double>(fireResist) / 2.0;
    const double reduced = static_cast<double>(baseDamage) * (1.0 - pct / 100.0);
    i64 v = roundHalfEven(reduced);
    if (v < 0) {
        v = 0;
    }
    return static_cast<i32>(v);
}

// T3: chest trap damage with parameter k (explosion k=4, fire k=6).
// UNKNOWN: damage formula not given in the notes; implemented as Int(Rnd*k)+1.
i32 chestTrapDamageRoll(Ctx& ctx, i32 k) {
    if (k <= 0) {
        return 0;
    }
    return ctx.rng.below(k) + 1;
}

// Q11 / CONFLICT-6: poisoned if Rnd*100 < Resist/2, as written in T3 (higher resistance is caught more often).
bool poisonGasCatches(Ctx& ctx, i32 poisonResist) {
    return ctx.rng.nextDouble() * 100.0 < static_cast<double>(poisonResist) / 2.0;
}

// T3 disease: test roll < Resistances[4]/2 (c64).
// UNKNOWN-5: exact test not confirmed; same form as the poison test is used.
bool diseaseCatches(Ctx& ctx, i32 diseaseResist) {
    return ctx.rng.nextDouble() * 100.0 < static_cast<double>(diseaseResist) / 2.0;
}

// T3 poison gas: each member tested with Resist(5); poison level = DungeonLevel (Ailments(0)).
// UNKNOWN: dead members are skipped.
void trapPoisonGas(Ctx& ctx, Party& party) {
    chestAfflictParty(ctx, party, DamageKind::Poison);
}

// T3 disease spores: each member tested with Resistances[4]; disease at floor strength.
// UNKNOWN: dead members are skipped.
void trapDiseaseSpores(Ctx& ctx, Party& party) {
    chestAfflictParty(ctx, party, DamageKind::Disease);
}

// T3 exploding box (k=4): damages all in the group (manual, c84).
// UNKNOWN: a separate roll per living member; dead members are skipped.
void trapExplodingBox(Ctx& ctx, Party& party) {
    for (i32 i = 0; i < party.size; ++i) {
        Character& c = party.members[slot(i)];
        if (c.isDead()) {
            continue;
        }
        applyDamage(c, chestTrapDamageRoll(ctx, 4));
    }
}

// T3 fire trap (k=6): fire ball on opener and nearby (manual, c84).
// UNKNOWN: nearby set not given; every living member is hit, with per-member Fire resistance (Resistances(0)).
// The opener index is accepted for the signature; the scope is not narrowed by it.
void trapFireBall(Ctx& ctx, Party& party, i32 opener) {
    (void)opener;
    for (i32 i = 0; i < party.size; ++i) {
        Character& c = party.members[slot(i)];
        if (c.isDead()) {
            continue;
        }
        const i32 base = chestTrapDamageRoll(ctx, 6);
        applyDamage(c, fireTrapDamageAfterResist(base, c.resistances.resist[0]));
    }
}

// T3 slime (ItemDestruction): may destroy one item from pack slots 1..39.
// UNKNOWN: Character holds no item list (Inventory is separate state and is not passed in),
// so no item can be selected here. Returns false (nothing destroyed, no message).
bool trapSlime(Ctx& ctx, Character& opener) {
    (void)ctx;
    (void)opener;
    return false;
}

// T3 fate: rerolls the opener's HP and SP.
// UNKNOWN: reroll ranges not given; implemented as HP 1..maxHits and SP 0..maxSpellPoints.
void trapFate(Ctx& ctx, Character& opener) {
    if (opener.vitals.maxHits > 0) {
        opener.vitals.curHits = ctx.rng.below(opener.vitals.maxHits) + 1;
    }
    if (opener.vitals.maxSpellPoints > 0) {
        opener.vitals.curSpellPoints = ctx.rng.below(opener.vitals.maxSpellPoints + 1);
    }
}

// T3 teleporter (idx 7): the opener is moved to a random square on the same level, and the
// chest becomes untrapped (manual, c84).
// UNKNOWN: destination x/y not derivable from SquareState (no width); only the area is set.
// UNKNOWN: spent trap flags cleared along with the trap (Q40).
void trapTeleportOpener(Ctx& ctx, Character& opener, const FloorState& floor, SquareState& chest) {
    chest.chestTrap = ChestTrap::NotTrapped;
    chest.trapFlags = 0;
    const i32 n = static_cast<i32>(floor.squares.size());
    if (n <= 0) {
        return;
    }
    const i32 s = ctx.rng.below(n);
    opener.location.area = floor.squares[slot(s)].areaId;
}

// T3 blackout (idx 8), c24: removes spell-granted effects.
// CONFLICT-26: the manual says all resistances and modifiers go; the SRD c24 wording is followed.
// UNKNOWN: which ailment flags count as spell-granted.
void trapBlackout(Ctx& ctx, Character& opener) {
    (void)ctx;
    opener.ailments.protectLevel = 0;
    opener.ailments.levitating = false;
    opener.ailments.invisible = false;
    opener.ailments.seeInvisible = false;
}

// T3 fear (idx 9), c24: removes living companions with binding below 100 (kBindingMax).
// Q15: the message "Your companions were scared away!" is output on each call.
// UNKNOWN: "removed" implemented as occupied = false and present = false.
i32 trapFearCompanions(Ctx& ctx, Vec<Companion>& companions) {
    i32 removed = 0;
    for (u32 i = 0; i < companions.size(); ++i) {
        Companion& c = companions[i];
        if (c.occupied && !c.isDead && c.bindingLevel < kBindingMax) {
            c.occupied = false;
            c.present = false;
            ++removed;
        }
    }
    ctx.say("loot.companionsScared", "Your companions were scared away!", {}, {});
    return removed;
}

// ---------------------------------------------------------------------------
// Gold splitting, formatting and transfers (T7, T8, T9, Q32-Q34, Q42, Q44)
// ---------------------------------------------------------------------------

// T8 / Q8: paid share = round_half_even(total / shares) in whole gold, returned as Currency.
Currency splitSharePaid(Currency total, i32 shares) {
    if (shares <= 0) {
        return 0;
    }
    const double whole = wholeGold(total) / static_cast<double>(shares);
    return roundHalfEven(whole) * kCurrencyUnit;
}

// T7 / T8 / Q8: quoted share = round half up of total / shares in whole gold (2.5 -> 3, 10/3 -> 3).
Currency splitShareQuoted(Currency total, i32 shares) {
    if (shares <= 0) {
        return 0;
    }
    const double whole = wholeGold(total) / static_cast<double>(shares);
    return vbInt(whole + 0.5) * kCurrencyUnit;
}

// T7: display in whole gold, half up (10.5 -> "11"). Q44 / CONFLICT-12: a negative value that
// rounds to nothing keeps its sign (-0.4 -> "-").
Str formatWholeGold(Currency amount) {
    const double v = wholeGold(amount);
    const i64 mag = vbInt(std::fabs(v) + 0.5);
    if (v < 0.0 && mag == 0) {
        return Str("-");
    }
    const std::string body = std::to_string(mag);
    return Str(v < 0.0 ? ("-" + body) : body);
}

// Q32 / T7: thief takes CLng-rounded (half to even) gold of the lesser of roll and purse.
// The purse is then set to zero if it went negative (the -0.5 case).
Currency thiefTakeGold(Ctx& ctx, Purse& purse, Currency rolled) {
    (void)ctx;
    if (rolled <= 0) {
        return 0;
    }
    const Currency basis = (rolled > purse.goldOnHand) ? purse.goldOnHand : rolled;
    if (basis <= 0) {
        return 0;
    }
    const i64 whole = roundHalfEven(wholeGold(basis));
    const Currency taken = whole * kCurrencyUnit;
    purse.goldOnHand -= taken;
    if (purse.goldOnHand < 0) {
        purse.goldOnHand = 0;
    }
    return taken;
}

// Q33: give takes CLng (half to even) of the amount in whole gold; the fraction stays with the giver.
// UNKNOWN: no sufficiency check; the original has none in the notes.
Currency giveGold(Ctx& ctx, Purse& giver, Purse& receiver, Currency amount) {
    (void)ctx;
    const i64 whole = roundHalfEven(wholeGold(amount));
    const Currency given = whole * kCurrencyUnit;
    giver.goldOnHand -= given;
    receiver.goldOnHand += given;
    return given;
}

// T9 / Q34: exchange pays a whole amount. Gold on hand is used first; any shortfall comes
// exactly from the bank, which may go negative.
// Example: hand 10.5, bank 5, amount 12 -> hand 0, bank 3.5.
void exchangeGoldInTown(Ctx& ctx, Purse& wallet, Currency amount) {
    (void)ctx;
    const Currency due = roundHalfEven(wholeGold(amount)) * kCurrencyUnit;
    if (wallet.goldOnHand >= due) {
        wallet.goldOnHand -= due;
        return;
    }
    const Currency shortfall = due - wallet.goldOnHand;
    wallet.goldOnHand = 0;
    wallet.goldInBank -= shortfall;
}

// T9: unchecked withdrawal; hand += amount, bank -= amount. Negative amounts are not refused.
// Example: hand 0.25, withdraw -5 -> hand -4.75.
void withdrawGoldUnchecked(Ctx& ctx, Purse& wallet, Currency amount) {
    (void)ctx;
    wallet.goldOnHand += amount;
    wallet.goldInBank -= amount;
}

// Q42 / CONFLICT-18: the code (not the comment) is followed: total purse at dungeon level 0,
// gold on hand otherwise.
Currency shownGold(const Character& ch, i32 dungeonLevel) {
    return (dungeonLevel == 0) ? ch.purse() : ch.wallet.goldOnHand;
}

// Q8 / Q47: only two wordings are reachable.
void announceGoldShare(Ctx& ctx, Currency share, Currency recovered, bool recoveredAny) {
    if (recoveredAny) {
        ctx.say("loot.shareRecovered",
                "Your share is " + formatWholeGold(share) + " and you recovered "
                    + formatWholeGold(recovered) + " Gold!",
                {}, {});
    } else {
        ctx.say("loot.share", "Your share is " + formatWholeGold(share) + " Gold!", {}, {});
    }
}

// ---------------------------------------------------------------------------
// Cursed items and charms (T10, T13, T15, Q47)
// ---------------------------------------------------------------------------

// Q47 / T10: only curse kind 2 attaches itself; kind 1 stays in the pack.
bool curseAttachesItself(const Item& item) {
    return item.cursed == CurseKind2;
}

// Q47: chest items attach under the same curse-kind-2 rule.
bool chestItemAttaches(const Item& item) {
    return curseAttachesItself(item);
}

// T13: the seven charm items are identified by id, which the notes do not give, and Character
// holds no item list. UNKNOWN: returns false.
bool holdsCharmOfOpening(const Ctx& ctx, const Character& ch) {
    (void)ctx;
    (void)ch;
    return false;
}

// T13 / T15: charm identification by id is not given in the notes.
// UNKNOWN: returns false.
bool charmOpensChest(const Ctx& ctx, const Item& item) {
    (void)ctx;
    (void)item;
    return false;
}

}  // namespace mordor
