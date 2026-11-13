// casting1_b.cpp : second half of casting1 (monster spells, breath, charms,
// binding, protection, map/utility effects, seer fees, spell-data predicates,
// wiki cost). Implements only the functions listed for this part.
//
// UNKNOWN (global): field names of SpellDef, GuildDef, Item, etc. are taken
// from the SRD / data file spelling (Damage2, SpellEffect, AffectGroup, Level,
// Category, HasSpellTable, SpellMax). Verify against game_data.h.

#include <cmath>

#include "mordor/casting1.h"
#include "mordor/math_util.h"

namespace mordor {

// R49.5 draw one record from the pool; reject unless Damage2 > 0 and SpellEffect > 0.
// Returns the spell id, or -1 when the pool is empty or the record is rejected.
i32 drawMonsterSpell(Ctx& ctx, const Vec<i32>& pool) {
    const i32 n = static_cast<i32>(pool.size());
    if (n <= 0) {
        return -1;
    }
    const i32 pick = ctx.rng.below(n);
    // R49 quirk: the pick sound uses the pool index, not the spell chosen.
    ctx.say("monster.spellPick", "Monster spell pick sound (pool index, not spell id).", {}, {});
    const i32 id = pool[static_cast<u32>(pick)];
    if (id < 0 || id >= static_cast<i32>(ctx.data.spells.size())) {
        return -1;
    }
    const SpellDef& s = ctx.data.spells[static_cast<u32>(id)];
    // R49.5 + R49 quirk: positive-power filter.
    // FIXED: SpellEffect -> killEffect (closest existing SpellDef member).
    if (s.damage2 > 0 && s.killEffect > 0) {
        return id;
    }
    return -1;
}

// R49.7 a selected spell replaces the accumulated character damage, hits up to
// AffectGroup+1 leading party slots, and the loop ends even at zero damage
// (the caller ends the loop). Returns the replacement damage.
// UNKNOWN: "companion slots" interpreted as the leading party members.
i32 applyMonsterSpellHits(Ctx& ctx, Party& party, const SpellDef& spell, i32 accumulatedDamage) {
    (void)accumulatedDamage;  // replaced by the spell's damage (R49.7)
    const i32 damage = static_cast<i32>(spell.damage2);
    const i32 reach = spell.affectGroup + 1;
    i32 n = reach < party.size ? reach : party.size;
    if (n > kPartyMaxSize) {
        n = kPartyMaxSize;
    }
    for (i32 i = 0; i < n; ++i) {
        party.members[static_cast<u32>(i)].vitals.curHits -= damage;
    }
    ctx.say("monster.spellHits", "A monster spell strikes the front of the party.", {}, {});
    return damage;
}

// R50.1 chance: 50; 80 if top > monLvl + monLvl/4.0; 20 if top < monLvl - monLvl/4.0.
// The second test runs after the first, as in the SRD.
i32 breathChancePercent(i32 topGuildLevel, i32 monsterLevel) {
    i32 chance = kBreathBaseChance;
    const f64 top = static_cast<f64>(topGuildLevel);
    const f64 mon = static_cast<f64>(monsterLevel);
    if (top > mon + mon / 4.0) {
        chance = kBreathHighChance;
    }
    if (top < mon - mon / 4.0) {
        chance = kBreathLowChance;
    }
    return chance;
}

// R50.2 range formula. Returns false where the original's Log(...) would raise
// (Log(0) and the other non-positive arguments).
bool breathDamageRange(i32 monsterLevel, i32 casterGuildLevel, i32& outRange) {
    const f64 mon = static_cast<f64>(monsterLevel);
    if (mon + 1.0 <= 0.0) {
        return false;  // Log of non-positive value
    }
    // l046C = VbInt(monLvl * Log(monLvl+1))
    const i64 l046C = vbInt(mon * std::log(mon + 1.0));
    if (l046C <= 0) {
        return false;  // Log(0) raised (R50.3 quirk)
    }
    const f64 l = static_cast<f64>(l046C);
    // range = Int(((Log(l046C) + l046C/15.0)/Log(1.08) - c.Guild.Level/20.0) + 1)
    const f64 v = (std::log(l) + l / 15.0) / std::log(1.08)
                  - static_cast<f64>(casterGuildLevel) / 20.0 + 1.0;
    outRange = static_cast<i32>(vbInt(v));
    return true;
}

// R50 breath attack. Order: chance roll, level-0 end, range (may Log(0) -> end
// of group turn), then one damage roll.
// UNKNOWN: whether a level-0 monster ends the turn only on a successful chance
// roll; implemented as chance first, then EndTurn on a hit.
BreathResult breathAttack(Ctx& ctx, i32 monsterLevel, i32 topGuildLevel, i32 casterGuildLevel) {
    BreathResult r;
    const i32 chance = breathChancePercent(topGuildLevel, monsterLevel);
    if (!ctx.rng.percent(chance)) {
        r.outcome = BreathOutcome::NoBreath;  // chance roll missed
        return r;
    }
    if (monsterLevel <= 0) {
        r.outcome = BreathOutcome::EndTurn;   // R50.2 level 0 ends the whole turn
        return r;
    }
    i32 range = 0;
    if (!breathDamageRange(monsterLevel, casterGuildLevel, range)) {
        r.outcome = BreathOutcome::EndGroupTurn;  // R50.3 Log(0) raised, resume at End Sub
        return r;
    }
    // dmg = VbInt(((Int((Rnd*range+1)/2) + range/4.0) * 0.7) / 2)
    const f64 rnd = ctx.rng.nextDouble();
    const i64 inner = vbInt((rnd * static_cast<f64>(range) + 1.0) / 2.0);
    const f64 dmg = ((static_cast<f64>(inner) + static_cast<f64>(range) / 4.0) * 0.7) / 2.0;
    r.damage = static_cast<i32>(vbInt(dmg));
    r.outcome = BreathOutcome::Breathed;
    ctx.say("monster.breath", "The monster breathes.", {}, {});
    // CONFLICT-free note: resistance (fire/cold) is applied by the caller.
    return r;
}

// R51.1 / R77 a charm kills when its damage would have killed the target.
bool charmWouldKill(i32 damage, i32 targetHits) {
    return damage >= targetHits;
}

// R51.3 charm test: effect in 75..99 and index (effect - 75) equals monster class.
bool charmEffectMatches(i32 effect, i32 monsterClass) {
    if (effect < kEffectCharmFirst || effect > kEffectCharmLast) {
        return false;
    }
    return (effect - kEffectCharmFirst) == monsterClass;
}

// R51.2 a successful charm creates a companion carrying the monster's hits before
// the spell, its MaxHits, and the spell level as its binding level.
// UNKNOWN: fields other than hits/maxHits/bindingLevel are copied as sensible defaults.
Companion makeCharmCompanion(const Creature& target, i32 spellLevel) {
    Companion c;
    c.ref = target.ref;
    c.occupied = true;
    c.monsterId = target.id;
    c.hits = target.hits;          // R51.2 hits before the spell
    c.maxHits = target.maxHits;
    c.bindingLevel = spellLevel;   // R51.2
    c.level = target.levelFound;
    c.specialPropertyFlags = target.specialPropertyFlags;
    switch (target.align) {
        case CharAlign::Good:
            c.alignment = CompanionAlignment::Good;
            break;
        case CharAlign::Neutral:
            c.alignment = CompanionAlignment::Neutral;
            break;
        case CharAlign::Evil:
            c.alignment = CompanionAlignment::Evil;
            break;
    }
    return c;
}

// R51.4 when the charm fills the last companion slot: the charmed creature leaves
// the fight, the next creature of its group is felled, and the one after it stands
// at nought (Hits 0, not unrolled). Quirk R56/c56: only the third record stands at nought.
// UNKNOWN: group-to-monsters layout. Members of group g are assumed to be stored
// contiguously in encounter.monsters, starting after the counts of earlier groups.
void charmLastSlotFallout(Encounter& encounter, i32 groupIndex) {
    if (groupIndex < 0 || groupIndex >= kMaxGroups) {
        return;
    }
    i32 base = 0;
    for (i32 g = 0; g < groupIndex; ++g) {
        base += encounter.groups[static_cast<u32>(g)].count;
    }
    const i32 n = static_cast<i32>(encounter.monsters.size());
    if (base >= n) {
        return;
    }
    Creature& charmed = encounter.monsters[static_cast<u32>(base)];
    charmed.charmed = true;
    charmed.alive = false;   // leaves the fight
    charmed.gone = true;
    if (base + 1 < n) {
        Creature& felled = encounter.monsters[static_cast<u32>(base + 1)];
        felled.hits = 0;     // felled
        felled.alive = false;
    }
    if (base + 2 < n) {
        Creature& third = encounter.monsters[static_cast<u32>(base + 2)];
        third.hits = 0;      // stands at nought
        third.unrolled = false;
    }
}

// R52 difficulty:
// difficulty = Round((30 + LevelFound) * Log((GuildLevel+5)/10.0 + 1))
// if not p036E: difficulty = Round(difficulty * 1.5)
i32 charmDifficulty(i32 levelFound, i32 guildLevel, bool p036E) {
    const f64 arg = (static_cast<f64>(guildLevel) + 5.0) / 10.0 + 1.0;
    f64 difficulty = 0.0;
    if (arg > 0.0) {
        difficulty = static_cast<f64>(
            roundHalfEven(static_cast<f64>(30 + levelFound) * std::log(arg)));
    }
    if (!p036E) {
        difficulty = static_cast<f64>(roundHalfEven(difficulty * 1.5));
    }
    return static_cast<i32>(difficulty);
}

// R53 BindLevel = round_even(spellLevel + ln((10 + casterSpellLevel) / 5)).
// UNKNOWN: "/5" taken as floating division; non-positive log argument returns spellLevel.
i32 bindLevel(i32 spellLevel, i32 casterSpellLevel) {
    const f64 arg = (10.0 + static_cast<f64>(casterSpellLevel)) / 5.0;
    if (arg <= 0.0) {
        return spellLevel;
    }
    return static_cast<i32>(roundHalfEven(static_cast<f64>(spellLevel) + std::log(arg)));
}

// R53 "if comp.BindLevel >= level: had no effect" (points are still spent by the caller).
bool bindHasNoEffect(const Companion& comp, i32 level) {
    return comp.bindingLevel >= level;
}

// R54 cast: Ailments(Protect) = Int(casterSpellLevel / 2) + 1
i32 protectLevelOnCast(i32 casterSpellLevel) {
    return static_cast<i32>(vbInt(static_cast<f64>(casterSpellLevel) / 2.0)) + 1;
}

// R54 recompute: Ailments(Protect) = Int(GuildLevel / 2) + 1
i32 protectLevelOnRecompute(i32 guildLevel) {
    return static_cast<i32>(vbInt(static_cast<f64>(guildLevel) / 2.0)) + 1;
}

// R54 defence = Log(p) * (Log(p) * 3)
// UNKNOWN: Log of a non-positive slot value raises in the original; returns 0 here.
f64 protectionDefence(f64 slotValue) {
    if (slotValue <= 0.0) {
        return 0.0;
    }
    const f64 l = std::log(slotValue);
    return l * (l * 3.0);
}

// R54 / R59 cast sets Protect from the caster's spell level. The original does not
// recompute here (the port's cast-time Recompute is a deviation and is not followed).
void castProtection(Character& who, i32 casterSpellLevel) {
    setSpellAilment(who.ailments, SpellAilmentKind::Protection,
                    protectLevelOnCast(casterSpellLevel));
}

// R55 Wrap(v, size) = v<0 ? size-1 : v>=size ? 0 : v
i32 wrapMapCoord(i32 v, i32 size) {
    if (v < 0) {
        return size - 1;
    }
    if (v >= size) {
        return 0;
    }
    return v;
}

// R55 move one square in the facing direction, wrapping at the map edge.
// UNKNOWN: axis convention: N = y-1, E = x+1, S = y+1, W = x-1.
void stepFacing(Location& where, Facing facing, i32 size) {
    switch (facing) {
        case Facing::N:
            where.y = wrapMapCoord(where.y - 1, size);
            break;
        case Facing::E:
            where.x = wrapMapCoord(where.x + 1, size);
            break;
        case Facing::S:
            where.y = wrapMapCoord(where.y + 1, size);
            break;
        case Facing::W:
            where.x = wrapMapCoord(where.x - 1, size);
            break;
    }
}

// R55 Ethereal Portal / Passwall. Target rock: the caster dies and the spell is not
// spent (R58 original: no SP charge when it kills).
// CONFLICT: original kills the whole party (and hangs for slot >= 2, c44); the port
// kills only the caster (c34). Implemented: port behaviour, to avoid the hang.
// Otherwise the party moves one square and arrives by spell; the spell is spent.
bool movePassThrough(Ctx& ctx, Party& party, i32 casterIndex, bool targetIsRock, i32 size) {
    if (casterIndex < 0 || casterIndex >= party.size) {
        return false;
    }
    Character& caster = party.members[static_cast<u32>(casterIndex)];
    if (targetIsRock) {
        caster.deathState = DeathState::Dead;
        caster.vitals.curHits = 0;
        ctx.say("spell.passThroughRock", "The caster is caught in rock.", {}, {});
        return false;
    }
    Location loc;
    loc.x = party.x;
    loc.y = party.y;
    loc.dungeonLevel = party.dungeonLevel;
    stepFacing(loc, party.facing, size);
    party.x = loc.x;
    party.y = loc.y;
    party.arrivedBySpell = true;  // R55 ArriveBySpell
    return true;
}

// R56 Detect Rock: unless lost, marks the rock record of the square ahead;
// if lost, marks bit 23 (not rock) on that square.
// UNKNOWN: which SquareState field holds the "rock bit" (rockRecorded used here).
void detectRockAhead(FloorState& floorState, const Location& where, bool lost) {
    Location ahead = where;
    const i32 fi = ((where.facing % 4) + 4) % 4;
    Facing f = Facing::W;
    if (fi == 0) {
        f = Facing::N;
    } else if (fi == 1) {
        f = Facing::E;
    } else if (fi == 2) {
        f = Facing::S;
    }
    stepFacing(ahead, f, kMapSize);
    const i32 idx = ahead.y * kMapSize + ahead.x;
    if (idx < 0 || idx >= static_cast<i32>(floorState.squares.size())) {
        return;
    }
    SquareState& sq = floorState.squares[static_cast<u32>(idx)];
    if (!lost) {
        sq.rockRecorded = true;
    } else {
        sq.bits |= 1u << static_cast<u32>(kBitNotRock);
    }
}

// R57 original: Depth Perception does not clear LostLevel or LostFacing before
// sub0848 and LookAround run, so the lost flags are left untouched here.
// UNKNOWN: the effects of sub0848 / LookAround are not in this SRD chunk.
void applyDepthPerception(Ctx& ctx, Party& party) {
    (void)party;
    ctx.say("spell.depthPerception", "Depth perception.", {}, {});
}

// R57 original: Find Direction does not clear LostLevel or LostFacing before the
// look-around runs. UNKNOWN: effect body not in this SRD chunk.
void applyFindDirection(Ctx& ctx, Party& party) {
    (void)party;
    ctx.say("spell.findDirection", "Find direction.", {}, {});
}

// R59 Levitate, Sight Veil, See Invisible and Protection set Ailments(n).
// UNKNOWN: boolean flags are set when level > 0; Protection stores the level.
void setSpellAilment(Ailments& ailments, SpellAilmentKind kind, i32 level) {
    switch (kind) {
        case SpellAilmentKind::Levitate:
            ailments.levitating = level > 0;
            break;
        case SpellAilmentKind::SightVeil:
            ailments.invisible = level > 0;
            break;
        case SpellAilmentKind::SeeInvisible:
            ailments.seeInvisible = level > 0;
            break;
        case SpellAilmentKind::Protection:
            ailments.protectLevel = level;
            break;
    }
}

// R60 with monsters present, Teleport and Cure Paralysis go to the combat branch
// in the original (MAINMOD2.BAS:856). CONFLICT: the port casts on the spot.
// UNKNOWN: the SRD gives only SpellUse here, so every non-NonCombat spell is routed.
bool routesToCombatBranch(bool monstersPresent, SpellUse use) {
    return monstersPresent && use != SpellUse::NonCombat;
}

// R62 best party WIS chance is taken over all members, alive or dead.
// UNKNOWN: per-member chance is read from the active guild's abilityRating[abilityIndex];
// level and extra are not used by the SRD chunk.
f32 bestPartyAbilityChance(const Party& party, i32 abilityIndex, i32 level, i32 extra) {
    (void)level;
    (void)extra;
    f32 best = 0.0f;
    if (abilityIndex < 0 || abilityIndex >= 6) {
        return best;
    }
    for (i32 i = 0; i < party.size && i < kPartyMaxSize; ++i) {
        const Character& m = party.members[static_cast<u32>(i)];
        const f32 v = m.guilds[static_cast<u32>(m.activeGuild)]
                          .abilityRating[static_cast<u32>(abilityIndex)];
        if (v > best) {
            best = v;
        }
    }
    return best;
}

// R62 identified squares are written to the automap. Already-mapped squares say nothing.
// UNKNOWN: "already mapped" tested via automap != 0.
void perceiveSquare(Ctx& ctx, SquareState& square) {
    if (!square.identified) {
        return;
    }
    if (square.automap != 0) {
        return;  // already mapped: say nothing
    }
    square.automap = 1;
    ctx.say("perceive.mapped", "Square mapped.", {}, {});
}

// R62 look-ahead runs unless NoPerceptionMessages is set or the character is lost.
bool lookAheadEnabled(bool noPerceptionMessages, bool lost) {
    return !noPerceptionMessages && !lost;
}



// R64 effect >= 200 is the whole-party variant.
bool isWholePartyEffect(i32 rawEffect) {
    return rawEffect >= kEffectWholePartyOffset;
}

// R64 real effect = Effect - 200 for the whole-party variant.
i32 realSpellEffect(i32 rawEffect) {
    return rawEffect >= kEffectWholePartyOffset ? rawEffect - kEffectWholePartyOffset : rawEffect;
}

// R64/R65 charm effects 75..99.
bool isCharmEffect(i32 effect) {
    return effect >= kEffectCharmFirst && effect <= kEffectCharmLast;
}

// R64 "Resists" effects 50..60.
bool isResistEffect(i32 effect) {
    return effect >= kEffectResistFirst && effect <= kEffectResistLast;
}

// R65 group spell: AffectGroup >= 6.
bool isGroupSpell(i32 affectGroup) {
    return affectGroup >= kGroupAffectMin;
}

// R65 offensive exclusions are by category name (Healing, Resist, Movement, Sight,
// Opening, Control, Charm).
// UNKNOWN: category names are not in GameData; only the ids known from the header
// (Charm = 7, Heal = 9) are excluded here. Other excluded categories are not mapped.
bool isOffensiveCategory(const Ctx& ctx, i32 category) {
    (void)ctx;
    if (category == kCategoryCharm || category == kCategoryHeal) {
        return false;
    }
    return true;
}

// R66 consumables: Potion, Scroll, Tome, Dust, Crystal, Rod, Sphere, Cube.
bool isConsumableItem(ItemType type) {
    return type >= Potion && type <= Cube;
}

// R66 an item is removed when empty (consumables only).
bool itemRemovedWhenEmpty(const Item& item) {
    return isConsumableItem(item.type);
}

// R67 spells at level 50 are item-only.
bool isItemOnlySpellLevel(i32 spellLevel) {
    return spellLevel == kItemOnlySpellLevel;
}

// R67 ceiling = max over guilds with HasSpellTable of SpellMax[category].
// UNKNOWN: GuildDef field names assumed (hasSpellTable, spellMax).
// FIXED: HasSpellTable/SpellMax -> hasSpellTable/spellMax (names in GuildDef).
i32 spellCeiling(const Ctx& ctx, i32 category) {
    i32 ceiling = 0;
    for (const GuildDef& g : ctx.data.guilds) {
        if (g.hasSpellTable && g.spellMax[static_cast<u32>(category)] > ceiling) {
            ceiling = g.spellMax[static_cast<u32>(category)];
        }
    }
    return ceiling;
}

// R67 a spell is unreachable if s.Level > ceiling.
bool spellUnreachable(const Ctx& ctx, const SpellDef& spell) {
    return spell.level > spellCeiling(ctx, spell.category);
}

// R71 cast-target box lists party members only, never companions.
Vec<i32> castTargetMembers(const Party& party) {
    Vec<i32> out;
    for (i32 i = 0; i < party.size && i < kPartyMaxSize; ++i) {
        out.push_back(i);
    }
    return out;
}

// R78 wand strength comes from the item's level, not the user's.
i32 wandStrength(i32 itemLevel) {
    return itemLevel;
}

// R86 wiki cost:
// guildMod = guild.SpellGuildMods[cat]; if 0 then 1
// if characterSpellLevel == 0 then characterSpellLevel = spell.Level
// cost = (int)(spell.Level / characterSpellLevel * spell.Cost * guildMod * 1.9 - 0.0001)
// UNKNOWN: operand types not stated; integer division of the level ratio assumed
// (C# int operands). A zero divisor returns 0.
i32 wikiSpellCost(i32 spellLevel, i32 characterSpellLevel, i32 baseCost, f32 guildMod) {
    f32 gm = guildMod;
    if (gm == 0.0f) {
        gm = 1.0f;
    }
    i32 csl = characterSpellLevel;
    if (csl == 0) {
        csl = spellLevel;
    }
    if (csl == 0) {
        return 0;
    }
    const i32 ratio = spellLevel / csl;
    const f64 v = static_cast<f64>(ratio) * static_cast<f64>(baseCost)
                  * static_cast<f64>(gm) * 1.9 - 0.0001;
    return static_cast<i32>(v);
}

// R86 GetInitialCost(spell, guild) = cost with characterSpellLevel = 0.
i32 wikiInitialCost(i32 spellLevel, i32 baseCost, f32 guildMod) {
    return wikiSpellCost(spellLevel, 0, baseCost, guildMod);
}

}  // namespace mordor
