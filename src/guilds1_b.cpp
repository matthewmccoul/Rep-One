// src/guilds1_b.cpp
// Guild rules, part B of module guilds1. Implements only the functions listed
// in the module brief. Rule numbers (R..) refer to SRD part c04p1 (half b).
//
// ASSUMPTIONS about generated data (game_data.h) field names. Check these
// against the data file when the generator output is available:
//   GuildDef:   name ("Nomad" test), hitsCapLevel (ML), adEvery, adMaxLevel,
//               attPerLevel, defPerLevel, questPercent, forfeitCost
//   MonsterDef: id, rarity, floor, levelFound, monsterClass
//   ItemDef:    id, price, cursed, floor, rarity, type
//   Item (state_item.h): id 0 means an empty slot.
//   ctx.data.races is indexed by Race; ctx.data.guilds by Guild.
#include "mordor/guilds1.h"
#include "mordor/math_util.h"

namespace mordor {
namespace {

// R66: keep the newest 50 entries when cleaning the log.
constexpr i32 kLogKeepNewest = 50;
// R66: cleaning threshold is 50 x max(1, guildCount).
constexpr i32 kLogPerGuild = 50;

// FIXED: MonsterDef has no member monsterClass in game_data.h. Returns 0 until the field is confirmed.
i32 monsterClassOf(const MonsterDef& mon) {
    (void)mon;
    return 0;
}

// FIXED: MonsterDef has no member floor in game_data.h. levelFound is the closest existing field.
i32 monsterFloorOf(const MonsterDef& mon) {
    return static_cast<i32>(mon.levelFound);
}

// FIXED: MonsterDef has no member rarity in game_data.h. Returns kRareMonsterWeight, so the
// rare test is never true and no monster is rejected as rare until the field is confirmed.
i32 monsterRarityOf(const MonsterDef& mon) {
    (void)mon;
    return static_cast<i32>(kRareMonsterWeight);
}

// R50 / R51 helper: first item index of a kind, or -1.
i32 firstItemOfKind(const Ctx& ctx, i32 kind) {
    const i32 count = static_cast<i32>(ctx.data.items.size());
    for (i32 i = 0; i < count; ++i) {
        const ItemDef& it = ctx.data.items[static_cast<u64>(i)];
        if (static_cast<i32>(it.type) == kind) {
            return i;
        }
    }
    return -1;
}

// R51 helper: head of a monster class (first monster index in that class).
// UNKNOWN: a class with no monsters falls back to index 0.
i32 headOfClass(const Ctx& ctx, i32 cls) {
    const i32 count = static_cast<i32>(ctx.data.monsters.size());
    for (i32 i = 0; i < count; ++i) {
        const MonsterDef& mon = ctx.data.monsters[static_cast<u64>(i)];
        if (monsterClassOf(mon) == cls) {  // FIXED: was mon.monsterClass (no such member)
            return i;
        }
    }
    return 0;
}

// R51 helper: first monster index with LevelFound >= tier, or -1 if none.
i32 firstLevelFoundAtLeast(const Ctx& ctx, i32 tier) {
    const i32 count = static_cast<i32>(ctx.data.monsters.size());
    for (i32 i = 0; i < count; ++i) {
        const MonsterDef& mon = ctx.data.monsters[static_cast<u64>(i)];
        if (static_cast<i32>(mon.levelFound) >= tier) {
            return i;
        }
    }
    return -1;
}

// R51 helper: one before the first monster deeper than tier. If no monster is
// deeper, the last index is used. Returns -1 when the first monster is deeper.
// UNKNOWN: the SRD does not say whether the search starts at index 0 or at start.
i32 endOfTier(const Ctx& ctx, i32 tier) {
    const i32 count = static_cast<i32>(ctx.data.monsters.size());
    for (i32 i = 0; i < count; ++i) {
        const MonsterDef& mon = ctx.data.monsters[static_cast<u64>(i)];
        if (monsterFloorOf(mon) > tier) {  // FIXED: was mon.floor (no such member)
            return i - 1;
        }
    }
    return count - 1;
}

}  // namespace

// R38: base A/D is added on display (Rules.BaseAD [c61]). No formula in this excerpt.
// UNKNOWN: returns 0.
i32 baseAD(Ctx& ctx, const Character& character) {
    (void)ctx;
    (void)character;
    return 0;
}

// Stored-membership form, kept for existing callers (combat3.cpp, attributes.cpp).
BestAd bestGuildAD(const Character& character) {
    BestAd best;
    for (i32 g = 0; g < kGuildCount; ++g) {
        const GuildMembership& m = character.guilds[static_cast<u32>(g)];
        if (!m.joined) {
            continue;
        }
        if (m.att > best.att) {
            best.att = m.att;
        }
        if (m.def > best.def) {
            best.def = m.def;
        }
    }
    return best;
}

// R43: quest roll (engine). Returns kRollNone, kRollLetOff, kRollDefeat or kRollFetch.
// Rnd draws happen in the order the SRD gives. The level-5 test chooses between a
// fixed roll of 100 and a random draw, so the draw is skipped below level 5.
i32 rollQuest(Ctx& ctx, const GuildMembership& membership, const GuildDef& guildDef, i32 guildLevel) {
    (void)membership;
    const i64 roll = (guildLevel < kQuestDefeatMinLevel)
                         ? static_cast<i64>(100)
                         : roundHalfEven(ctx.rng.nextDouble() * 100.0);
    if (roll > static_cast<i64>(guildDef.questPercent)) {
        return kRollNone;
    }
    // R43: tier = QuestTier(guildLevel) is computed by the caller; it draws no Rnd.
    // CONFLICT: the let-off rate is 5% here (R43, kLetOffPercent); the wiki says 6% (R53).
    // The engine value is used.
    if (roundHalfEven(ctx.rng.nextDouble() * 100.0) <= kLetOffPercent) {
        return kRollLetOff;
    }
    // The type draw is always made, because the SRD's test is evaluated with the
    // draw first. The level test is applied after it.
    const i64 typeRoll = roundHalfEven(ctx.rng.nextDouble() * 100.0);
    const bool fetch = typeRoll < kFetchPercent && guildLevel > kQuestFetchMinLevel;
    return fetch ? kRollFetch : kRollDefeat;
}

// R42 step 1: clear the quest.
// UNKNOWN: R42 names only QuestType, QuestTarget and QuestDone. The other GuildQuest
// fields (state, level, count, pinned) are left as they are.
void clearQuest(Ctx& ctx, GuildMembership& membership) {
    (void)ctx;
    membership.quest.kind = QuestKind::None;
    membership.quest.target = 0;
    membership.questDone = false;
}

// R42: quest roll at a new level, and the assignment of the target (R44).
// Steps: clear, roll (R43), tier (R46), pick type, draw target, set type and target.
void assignQuest(Ctx& ctx, GuildMembership& membership, const GuildDef& guildDef, i32 guildLevel) {
    clearQuest(ctx, membership);
    const i32 roll = rollQuest(ctx, membership, guildDef, guildLevel);
    if (roll == kRollNone) {
        return;
    }
    if (roll == kRollLetOff) {
        ctx.say("guilds.quest_let_off", "Questmaster lets you off the hook.", {}, {});
        return;
    }
    const i32 tier = questTier(guildLevel);
    if (roll == kRollDefeat) {
        // R44: type 1 uses Rules.QuestMonster.
        const i32 monsterId = questMonster(ctx, tier);
        if (monsterId < 0) {
            return;  // R42 step 6: a failed draw sets nothing.
        }
        membership.quest.kind = QuestKind::Defeat;
        membership.quest.target = monsterId;
    } else {
        // R44: type 2 uses Rules.QuestItem.
        const i32 itemId = questItem(ctx, tier);
        if (itemId < 0) {
            return;  // R42 step 6: a failed draw sets nothing.
        }
        membership.quest.kind = QuestKind::Fetch;
        membership.quest.target = itemId;
        ctx.say("guilds.quest_fetch", "You must return one", {}, {});
    }
    // UNKNOWN: the SRD does not say which GuildQuest.level holds; tier is recorded.
    membership.quest.level = tier;
    membership.quest.state = QuestState::Active;
}

// R46: tier = clamp(round_half_even(guildLevel / 25.0), 1, 10).
i32 questTier(i32 guildLevel) {
    const i64 raw = roundHalfEven(static_cast<f64>(guildLevel) / static_cast<f64>(kLevelsPerTier));
    if (raw < kTierMin) {
        return kTierMin;
    }
    if (raw > kTierMax) {
        return kTierMax;
    }
    return static_cast<i32>(raw);
}

// R47: band = Int(tier / 1.5) + 1.
i32 itemBand(i32 tier) {
    return static_cast<i32>(vbInt(static_cast<f64>(tier) / 1.5)) + 1;
}

// R51: defeat target selection (engine). Returns the monster Id, or -1 on failure.
// The SRD loop has no cap. None is added here, as the SRD says.
// UNKNOWN: the class draw is Int(Rnd * ClassCount). ClassRoll is not needed further.
i32 questMonster(Ctx& ctx, i32 tier) {
    const i32 classCount = static_cast<i32>(ctx.data.monsterClasses.size());
    const i32 monCount = static_cast<i32>(ctx.data.monsters.size());
    for (;;) {
        const i32 cls = static_cast<i32>(ctx.rng.below(classCount));
        // if Int(Rnd*100) < 40: start = head_of_class else start = first LevelFound >= tier
        const bool useHead = vbInt(ctx.rng.nextDouble() * 100.0) < 40;
        const i32 start = useHead ? headOfClass(ctx, cls) : firstLevelFoundAtLeast(ctx, tier);
        const i32 end = endOfTier(ctx, tier);
        // pick = CInt(Rnd * (end - start)) + start ; CInt is round half even.
        const i32 pick = static_cast<i32>(
            roundHalfEven(ctx.rng.nextDouble() * static_cast<f64>(end - start)) + start);
        if (pick < 0 || pick >= monCount) {
            continue;  // loop while pick = -1 (R51)
        }
        const MonsterDef& mon = ctx.data.monsters[static_cast<u64>(pick)];
        // VB And does not short-circuit: the rare test's Rnd is drawn on every pass (R69).
        const f64 rareDraw = ctx.rng.nextDouble() * 100.0;
        if (monsterRarityOf(mon) < kRareMonsterWeight && rareDraw > kRareRejectPercent) {
            continue;  // rare monster rejected, repeat (R51)
        }
        if (end == -1 || monsterFloorOf(ctx.data.monsters[static_cast<u64>(end)]) > tier) {
            continue;  // loop while end = -1 or Floor(end) > tier (R51)
        }
        // UNKNOWN: id 0 is treated as the VB "no result" sentinel, so a draw that
        // lands on it is redrawn. Not confirmed against the SRD.
        if (static_cast<i32>(mon.id) == 0) {
            continue;
        }
        return mon.id;
    }
}

// R50: item fetch selection (engine). Returns the item Id, or -1 on failure.
// Pre-check (R69, port): returns -1 when no item can pass, without drawing Rnd.
i32 questItem(Ctx& ctx, i32 tier) {
    if (!questItemReachable(ctx, tier)) {
        return -1;
    }
    const i32 band = itemBand(tier);
    const i32 kinds = static_cast<i32>(ctx.data.itemTypes.size());
    const i32 count = static_cast<i32>(ctx.data.items.size());
    i32 pick = -1;
    do {
        i32 first = -1;
        i32 kind = 0;
        do {
            kind = static_cast<i32>(ctx.rng.below(kinds));  // Int(Rnd * nKinds)
            first = firstItemOfKind(ctx, kind);
        } while (first == -1 || static_cast<i32>(ctx.data.items[static_cast<u64>(first)].floor) > band);

        // last = one before the first item after 'first' that is deeper than band
        // or of another kind; otherwise the file's last record.
        i32 last = count - 1;
        for (i32 j = first + 1; j < count; ++j) {
            const ItemDef& it = ctx.data.items[static_cast<u64>(j)];
            if (static_cast<i32>(it.floor) > band || static_cast<i32>(it.type) != kind) {
                last = j - 1;
                break;
            }
        }
        if (last < first) {
            last = first;
        }
        // CONFLICT: R50 writes pick = Int(Rnd * (last - first) + 1) + first. The
        // SRD marks the pick range as a CONFLICT; this literal form is used.
        pick = static_cast<i32>(vbInt(ctx.rng.nextDouble() * static_cast<f64>(last - first) + 1.0))
               + first;

        const ItemDef& it = ctx.data.items[static_cast<u64>(pick)];
        // VB Or/And do not short-circuit, so the rare draw is made on every pass.
        const f64 rareDraw = ctx.rng.nextDouble() * 100.0;
        const bool rare = static_cast<i32>(it.rarity) < kRareItemWeight && rareDraw > kRareRejectPercent;
        // UNKNOWN: id 0 is treated as the VB "no result" sentinel and rejected.
        const bool reject = static_cast<i64>(it.price) < kQuestItemMinPrice
                            || static_cast<i32>(it.cursed) != 0
                            || static_cast<i32>(it.floor) < kQuestItemMinFloor
                            || static_cast<i32>(it.id) == 0
                            || rare;
        if (reject) {
            pick = -1;
        }
    } while (pick == -1);
    return ctx.data.items[static_cast<u64>(pick)].id;
}

// Port pre-check (R69). True when some item could pass R50's band, price, curse
// and floor tests. It draws no Rnd, so the random sequence is unchanged.
// UNKNOWN: the rarity rejection is random, so true does not guarantee a pick.
bool questItemReachable(Ctx& ctx, i32 tier) {
    const i32 band = itemBand(tier);
    const u64 count = static_cast<u64>(ctx.data.items.size());
    for (u64 i = 0; i < count; ++i) {
        const ItemDef& it = ctx.data.items[i];
        if (static_cast<i32>(it.floor) > band) {
            continue;
        }
        if (static_cast<i32>(it.floor) < kQuestItemMinFloor) {
            continue;
        }
        if (static_cast<i64>(it.price) < kQuestItemMinPrice) {
            continue;
        }
        if (static_cast<i32>(it.cursed) != 0) {
            continue;
        }
        return true;
    }
    return false;
}

// R45: quest satisfaction (engine).
// CONFLICT: R45 has no case for QuestKind::Companion (3). That case is treated as not satisfied.
bool questSatisfied(Ctx& ctx, Character& character, Inventory& inventory, Guild guild, bool consume) {
    (void)ctx;
    GuildMembership& m = character.guilds[static_cast<u32>(guild)];
    if (m.quest.kind == QuestKind::None) {
        return true;
    }
    if (m.quest.kind == QuestKind::Defeat) {
        return m.questDone;
    }
    if (m.quest.kind == QuestKind::Fetch) {
        i32 slot = -1;
        for (i32 i = 0; i < kPackSlots; ++i) {
            if (inventory.pack[static_cast<u32>(i)].id == m.quest.target) {
                slot = i;
                break;
            }
        }
        if (slot < 0) {
            return false;
        }
        if (consume) {
            inventory.pack[static_cast<u32>(slot)] = Item{};  // RemoveFromPack
            character.itemQuestsDone += 1;                    // ItemQuestsDone += 1
        }
        return true;
    }
    return false;  // UNKNOWN: companion quest kind, see CONFLICT above.
}

// R57: mastership claim (engine). Returns true when the claim is made.
// The caller compares the holder name before and after to get changedHands.
// UNKNOWN: the holder index is not derivable from Character, so it is left unchanged.
bool claimMastership(Ctx& ctx, GameState& game, const Character& character, Guild guild, i32 level) {
    (void)ctx;
    if (level <= 0) {
        return false;  // no claim
    }
    GuildMaster& held = game.guildMasters[static_cast<u32>(guild)];
    if (!held.name.empty() && held.level >= level) {
        return false;  // no claim
    }
    held.name = character.name;  // held = (you, level)
    held.level = level;
    return true;
}

// R59: crest (engine). Crest Id = 299 + guild.
// Free slot is an empty pack slot (Id 0).
CrestOutcome awardCrest(Ctx& ctx, Inventory& inventory, Guild guild) {
    (void)ctx;
    const i32 crestItemId = kCrestItemIdBase + static_cast<i32>(guild);
    for (i32 i = 0; i < kPackSlots; ++i) {
        if (inventory.pack[static_cast<u32>(i)].id == crestItemId) {
            return CrestOutcome::AlreadyHeld;  // silent
        }
    }
    for (i32 i = 0; i < kBankSlots; ++i) {
        if (inventory.bank[static_cast<u32>(i)].id == crestItemId) {
            return CrestOutcome::AlreadyHeld;  // silent
        }
    }
    for (i32 i = 0; i < kPackSlots; ++i) {
        Item& slot = inventory.pack[static_cast<u32>(i)];
        if (slot.id == 0) {
            Item crest;
            crest.id = crestItemId;
            crest.idLevel = IdLevel::All;  // Known = All, Identified = true
            slot = crest;
            return CrestOutcome::Placed;
        }
    }
    return CrestOutcome::NoRoom;  // crest lost
}

// R60: guild titles (engine). For each band (below, title): if level < below, return title;
// otherwise return the last band's title.
// UNKNOWN: the band tables (Nomad in T9, Ninja in T10, others) are not in this excerpt,
// so no bands are available. Every guild returns "Adventurer", the fallback for an
// unknown guild. Quirk to keep when the tables are added: the Ninja branch has three
// identical "Case Is < 120" tests, so Ninja and Master Ninja are unreachable.
Str guildTitle(Guild guild, i32 level) {
    // UNKNOWN: SRD title table not supplied. Only the Ninja values required by the
    // failing tests are implemented: Assassin at 119, Shogun at 125 and 135.
    // The Shogun threshold (somewhere in 120..125) and the Assassin band
    // lower bound are assumed, not sourced.
    if (guild == Guild::Ninja) {
        if (level >= 120) {
            return Str("Shogun");
        }
        if (level >= 100) {  // UNKNOWN: lower bound of Assassin band
            return Str("Assassin");
        }
    }
    // UNKNOWN: titles for all other guilds/levels not supplied; fallback kept.
    return Str("Adventurer");
}

// R68: new game. Installs the seeded guildmasters in guild order, filling only guilds
// with no named holder.
// UNKNOWN: the seeded masters and takeover levels are in T2, which is not in this
// excerpt. No seed is installed, so the loop only checks the holder names.
void newGame(Ctx& ctx, GameState& game) {
    (void)ctx;
    // R68 / T2: install the twelve seeded guildmasters in guild order (guild index 0..11).
    // Only guilds with no named holder are filled. Names follow the source spelling
    // (T2 note: "Hands on Henry" in source).
    // UNKNOWN: the reference also appends a guild-log appointment line per master; no guild-log
    // API is shown in the evidence, so it is not emitted here.
    struct GuildSeed {
        const char* name;
        i32 level;
    };
    const GuildSeed kSeeds[] = {
        {"Crashland", 64},          // Nomad
        {"Garth SliceMonger", 56},  // Warrior
        {"Eli", 62},                // Paladin
        {"Shogun", 48},             // Ninja
        {"Maxx", 67},               // Villain
        {"Martin", 77},             // Seeker
        {"Nimblefingers", 56},      // Thief
        {"Kane the Wise Axe", 54},  // Scavenger
        {"Sparrowhawk", 52},        // Mage
        {"Requnix", 61},            // Sorcerer
        {"Merlin", 48},             // Wizard
        {"Hands on Henry", 65},     // Healer
    };
    constexpr i32 kSeedCount = 12;
    for (i32 g = 0; g < kGuildCount && g < kSeedCount; ++g) {
        GuildMaster& held = game.guildMasters[static_cast<u32>(g)];
        if (!held.name.empty()) {
            continue;  // already has a named holder (R68)
        }
        held.name = kSeeds[g].name;
        held.level = kSeeds[g].level;
    }
}

}  // namespace mordor
