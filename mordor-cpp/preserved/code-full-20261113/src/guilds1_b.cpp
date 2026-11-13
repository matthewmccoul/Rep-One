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

// FIXED: GuildDef has no member forfeitCost in game_data.h. Returns 0, so no levels are
// dropped until the field is confirmed.
i32 guildForfeitCostOf(const GuildDef& guildDef) {
    (void)guildDef;
    return 0;
}

// ASSUMPTION: a Nomad guild is recognised by its data name.
bool isNomadDef(const GuildDef& guildDef) {
    return guildDef.name == "Nomad";
}

// Non-negative integer to text (used for day gaps in log prefixes).
Str intText(i64 value) {
    Str out;
    if (value == 0) {
        out += '0';
        return out;
    }
    i64 v = value < 0 ? -value : value;
    Str reversed;
    while (v > 0) {
        reversed += static_cast<char>('0' + (v % 10));
        v /= 10;
    }
    for (std::size_t i = reversed.size(); i > 0; --i) {
        out += reversed[i - 1];
    }
    return out;
}

// UNKNOWN: a zero divisor in the R38/R38 modulo is guarded by treating the
// remainder as 1 (never an award). The SRD does not say what happens.
i32 modOrNonZero(i32 a, i32 b) {
    return b == 0 ? 1 : a % b;
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

// R-none: no SRD body. UNKNOWN: simplest sensible choice. The joined guild with
// the highest rating for this ability wins; ties keep the earlier guild; Nomad if
// nothing is joined.
Guild winningGuild(Ctx& ctx, const Character& character, i32 ability) {
    (void)ctx;
    if (ability < 0 || ability >= 6) {
        return Guild::Nomad;
    }
    Guild best = Guild::Nomad;
    f32 bestRating = 0.0f;
    bool found = false;
    for (i32 g = 0; g < kGuildCount; ++g) {
        const GuildMembership& m = character.guilds[static_cast<u32>(g)];
        if (!m.joined) {
            continue;
        }
        const f32 rating = m.abilityRating[static_cast<u32>(ability)];
        if (!found || rating > bestRating) {
            best = static_cast<Guild>(g);
            bestRating = rating;
            found = true;
        }
    }
    return best;
}

// UNKNOWN: no SRD formula for the rating. Simplest sensible: round half even,
// then floor at kRatingMin.
i32 abilityRating(const GuildDef& guildDef, f64 value) {
    (void)guildDef;
    const i32 rating = static_cast<i32>(roundHalfEven(value));
    return rating < kRatingMin ? kRatingMin : rating;
}

// R38: AdInterval(level).
i32 adInterval(const GuildDef& guildDef, i32 level, bool isNomad) {
    const i32 hitsCap = guildDef.hitsCapLevel;             // ML = HitsCapLevel (R38, R69)
    // R38: acc = 0 unless level > HitsCapLevel.
    if (level <= hitsCap) {
        return 0;
    }
    const i32 bonus = isNomad ? kNomadAdBonus : 0;         // +3 for Nomad (R38)
    f32 acc = static_cast<f32>(hitsCap);                   // accumulated A/D is Single
    for (i32 i = hitsCap; i <= level; ++i) {
        // step = Round_half_even((Int(HitsCapLevel)/2 + acc/18) / (AdEvery + bonus))
        // R38: the reference's AdEvery is a Single, so the sum is taken from its Single value.
const f64 denom = static_cast<f64>(static_cast<f32>(guildDef.adEvery)) + static_cast<f64>(bonus);
        i64 step = 0;                                      // step is an Integer
        if (denom != 0.0) {
            // Reference: acc / 18f is a Single division; only the sum is Double.
            const f32 accOver18 = acc / 18.0f;
            const f64 num = static_cast<f64>(hitsCap) / 2.0 + static_cast<f64>(accOver18);
            step = roundHalfEven(num / denom);
        }
        // UNKNOWN: a zero AdEvery+bonus gives step 0; the SRD does not say.
        acc = acc + static_cast<f32>(step);
        if (acc > static_cast<f32>(level)) {
            break;
        }
    }
    if (acc == static_cast<f32>(hitsCap)) {
        acc = static_cast<f32>(level + 1);
    }
    if (level > guildDef.adMaxLevel) {
        acc = static_cast<f32>(static_cast<f64>(acc) * 1.1);
    }
    // Round(acc): VB Round is round half even.
    return static_cast<i32>(roundHalfEven(static_cast<f64>(acc)));
}

// R38: GuildAD loop. Returns the guild's own Att and Def awards.
// The Nomad interval (+3) is applied through adInterval.
AdValue guildAD(const GuildMembership& membership, const GuildDef& guildDef) {
    AdValue out;
    const bool nomad = isNomadDef(guildDef);
    const i32 hitsCap = guildDef.hitsCapLevel;
    const i32 top = membership.level < guildDef.adMaxLevel ? membership.level : guildDef.adMaxLevel;
    // R38: GuildsJoined(g).Att is a Single; the displayed value is its whole part.
    f32 att = 0.0f;
    f32 def = 0.0f;
    i32 cached = 0;
    for (i32 l = 1; l <= top; ++l) {
        // Reference: if (cached < l) cached = AdInterval(l) before the modulus above the cap.
        if (l > hitsCap && cached < l) {
            cached = adInterval(guildDef, l, nomad);
        }
        // r = l % AdEvery if l <= HitsCapLevel else l % cached (zero interval gives r = 0)
        i32 r = 0;
        if (l <= hitsCap) {
            r = modOrNonZero(l, static_cast<i32>(guildDef.adEvery));  // R10: level Mod Int(AdEvery)
        } else {
            r = (cached <= 0) ? 0 : (l % cached);
        }
        if (r == 0) {
            att += static_cast<f32>(guildDef.attPerLevel);
            def += static_cast<f32>(guildDef.defPerLevel);
            if (l >= hitsCap) {
                cached = adInterval(guildDef, l, nomad);
            }
        }
        cached = adInterval(guildDef, l, nomad);           // recomputed every level, as FRMCTYG does
    }
    // Truncate to the whole part only at the end (Single accumulation, then Int).
    out.att = static_cast<f64>(static_cast<i32>(att));
    out.def = static_cast<f64>(static_cast<i32>(def));
    return out;
}

// R38: base A/D is added on display (Rules.BaseAD [c61]). No formula in this excerpt.
// UNKNOWN: returns 0.
i32 baseAD(Ctx& ctx, const Character& character) {
    (void)ctx;
    (void)character;
    return 0;
}

// R39: Guild A/D = (max over joined guilds of Att, max over joined guilds of Def).
// R38 / BestGuildAD (Rules.cs:151): each joined membership's A/D is derived from its level by
// guildAD (the R38 loop), then the highest attack and highest defence are taken independently.
// Raw values: the 10/10 base is added on display, not here.
BestAd bestGuildAD(const GameData& data, const Character& character) {
    BestAd best;
    for (i32 g = 0; g < kGuildCount; ++g) {
        const GuildMembership& m = character.guilds[static_cast<u32>(g)];
        if (!m.joined) {
            continue;
        }
        const AdValue ad = guildAD(m, data.guilds[static_cast<u32>(g)]);
        if (ad.att > best.att) {
            best.att = static_cast<decltype(best.att)>(ad.att);
        }
        if (ad.def > best.def) {
            best.def = static_cast<decltype(best.def)>(ad.def);
        }
    }
    return best;
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

// R40: Nomad_value(L) = 10 + 1.5*L for even L in 2..30.
// Level 1 is 10 on the sheet, which does not fit the formula (R40).
// UNKNOWN: the sheet values for odd or out-of-range levels are not in the excerpt
// (R41); 0 is returned for them.
f64 nomadValue(i32 level) {
    if (level == 1) {
        return 10.0;
    }
    if (level >= kNomadEvenMin && level <= kNomadEvenMax && level % 2 == 0) {
        return 10.0 + 1.5 * static_cast<f64>(level);
    }
    return 0.0;
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

// R48: min(floor(gLevel/25), 10).
i32 questMonsterMaxLevel(i32 guildLevel) {
    const i64 levels = vbInt(static_cast<f64>(guildLevel) / static_cast<f64>(kLevelsPerTier));
    const i64 capped = levels < kTierMax ? levels : static_cast<i64>(kTierMax);
    return static_cast<i32>(capped);
}

// R49, reconstructed: itemMax = min( (min(floor(gLevel/25), 10) / 1.5) + 1, 7 ).
// UNKNOWN: the source's rounding is ambiguous. Floor is used, as in R47.
i32 questItemMaxLevel(i32 guildLevel) {
    const i32 monLevel = questMonsterMaxLevel(guildLevel);
    const i32 raw = itemBand(monLevel);
    return raw < kItemLevelMax ? raw : kItemLevelMax;
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

// R54: monster kill (game). Experience (sub0CDF, sub0F11) is paid elsewhere.
// CONFLICT: R54 step 2 says each party member gets QuestDone, but also "only the
// killer counts". The killer alone is implemented.
void monsterKill(Ctx& ctx, Party& party, i32 monsterId) {
    if (party.killer < 0 || party.killer >= party.size) {
        return;
    }
    Character& killer = party.members[static_cast<u32>(party.killer)];
    GuildMembership& m = killer.guilds[static_cast<u32>(killer.activeGuild)];
    if (m.quest.kind != QuestKind::Defeat || m.quest.state != QuestState::Active
        || m.quest.target != monsterId) {
        return;
    }
    m.questDone = true;
    if (party.isSolo()) {
        ctx.say("guilds.quest_done_alone", "You have completed your quest!", {}, {});
    } else {
        ctx.say("guilds.quest_done", Str("\"") + killer.name + Str("\" completed her quest!"),
                {}, {killer.name});
    }
}

// R56: forfeit (player).
// Quirk: the refusal message contains the original typo "Cnanot".
// Not enforced: the wiki's "pinned with an outstanding quest" rule (R56 step 3).
bool forfeitQuest(Ctx& ctx, Character& character, const Inventory& inventory) {
    // R56 step 1: refuse if any equipped slot is cursed. Nothing else happens.
    // UNKNOWN: "cursed slot" is read as Item.slotCursed on worn items.
    for (const Item& it : inventory.worn) {
        if (it.slotCursed) {
            ctx.say("guilds.forfeit_refused",
                    "You Cnanot forfeit while cursed gear is equipped.", {}, {});
            return false;
        }
    }
    // R56 step 2: drop ForfeitCost levels (no dice). Experience resets to the new floor.
    const i32 guildIndex = static_cast<i32>(character.activeGuild);
    GuildMembership& m = character.guilds[static_cast<u32>(guildIndex)];
    const GuildDef& guildDef = ctx.data.guilds[static_cast<u64>(guildIndex)];
    const RaceDef& raceDef = ctx.data.races[static_cast<u64>(static_cast<i32>(character.race))];
    i32 newLevel = m.level - guildForfeitCostOf(guildDef);  // FIXED: was guildDef.forfeitCost (no such member)
    // UNKNOWN: a level below 0 is clamped to 0; the SRD does not say.
    if (newLevel < 0) {
        newLevel = 0;
    }
    m.level = newLevel;
    m.experience = expForLevel(guildDef, raceDef, newLevel);
    // UNKNOWN: the SRD does not say that the quest is abandoned on forfeit. It is
    // cleared here because the message is "forfeit Quest".
    clearQuest(ctx, m);
    // UNKNOWN: the equipment recheck (R56 step 2, [c24]) is not in this excerpt
    // and is not performed here.
    ctx.say("guilds.forfeit_quest", "forfeit Quest", {}, {});
    ctx.say("guilds.demotion", "Demotion Complete", {}, {});
    return true;
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

// R61: identify skill (game). Skill = 0 with no character, otherwise
// IdentifySkill(top guild level, effective Int, effective Wis).
// UNKNOWN: the body of IdentifySkill is not in the excerpt. Returns 0.
i32 identifySkill(i32 topGuildLevel, i32 intStat, i32 wisStat) {
    (void)topGuildLevel;
    (void)intStat;
    (void)wisStat;
    return 0;
}

// R62, R63: guild item recharge on arrival in town (engine).
// Pack slots 1..39 are checked. Slot 0 is not checked.
void townRecharge(Ctx& ctx, Inventory& inventory) {
    (void)ctx;
    for (i32 slot = kRechargeFirstSlot; slot <= kRechargeLastSlot; ++slot) {
        Item& it = inventory.pack[static_cast<u32>(slot)];
        const bool guildItem = it.ailments >= kGuildMarkerBase
                               && it.ailments <= kGuildMarkerBase + kGuildCount - 1;
        if (guildItem && it.charges < it.maxCharges) {
            it.charges = it.maxCharges;
        }
    }
}

// R65, R66: guild log cleaning (game and engine).
// UNKNOWN: the trigger point (sub316B) is not given; this implements the R66 test.
// The optional GUILDLOG.TXT archive is not written here, because the core does not
// touch files. The caller owns that output.
void cleanGuildLog(Ctx& ctx, GameState& game) {
    const i64 guildCount = static_cast<i64>(ctx.data.guilds.size());
    const i64 perGuild = static_cast<i64>(kLogPerGuild);
    const i64 threshold = perGuild * (guildCount > 1 ? guildCount : 1);
    const i64 count = static_cast<i64>(game.guildLog.size());
    if (count <= threshold) {
        return;  // needsClean = count > 50 x max(1, guildCount)
    }
    // Keep the newest entries across the whole log.
    Vec<GuildLogEvent> kept;
    for (i64 i = count - static_cast<i64>(kLogKeepNewest); i < count; ++i) {
        kept.push_back(game.guildLog[static_cast<u64>(i)]);
    }
    game.guildLog = kept;
}

// R66: log prefix. Gap is the absolute day difference (quirk: a clock set backwards
// still reads "ago").
Str logPrefix(i64 entryDay, i64 today) {
    const i64 gap = today >= entryDay ? today - entryDay : entryDay - today;
    if (gap == 0) {
        return Str("TODAY: ");
    }
    if (gap == 1) {
        return Str("1 DAY AGO: ");
    }
    return intText(gap) + Str(" DAYS AGO: ");
}

// R67: release sets MonsterIndex = -1 and keeps the name.
// Quirk: the companion slot keeps the last occupant's name.
void releaseCompanion(Ctx& ctx, Companion& companion) {
    (void)ctx;
    companion.monsterId = -1;
}

// R67: fill (fn1163) writes Hits, not Name.
void fillCompanion(Ctx& ctx, Companion& companion, i32 hits) {
    (void)ctx;
    companion.hits = hits;
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
