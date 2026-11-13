The build failed because `findItem` assigned an `ItemDef` table row to an `Item`. The fix copies the id into a fresh `Item`, which is all `equipProblem` reads. The C# side threw on id 366, which isn't in `H.ItemsById`, so it now returns `MISSING_ITEM`, the same text the C++ side emits for an absent id.

=== FILE: cases.json ===
[
{"args": {"id": 1, "s0": 18, "s1": 18, "s2": 18, "s3": 18, "s4": 18, "s5": 18}, "rng": [0.5]},
{"args": {"id": 1, "s0": 0, "s1": 0, "s2": 0, "s3": 0, "s4": 0, "s5": 0}, "rng": [0.5]},
{"args": {"id": 1, "s0": -1, "s1": -1, "s2": -1, "s3": -1, "s4": -1, "s5": -1}, "rng": [0.5]},
{"args": {"id": 2, "s0": 18, "s1": 18, "s2": 18, "s3": 18, "s4": 18, "s5": 18}, "rng": [0.5]},
{"args": {"id": 2, "s0": 0, "s1": 0, "s2": 0, "s3": 0, "s4": 0, "s5": 0}, "rng": [0.5]},
{"args": {"id": 3, "s0": 10, "s1": 10, "s2": 10, "s3": 10, "s4": 10, "s5": 10}, "rng": [0.5]},
{"args": {"id": 5, "s0": 25, "s1": 25, "s2": 25, "s3": 25, "s4": 25, "s5": 25}, "rng": [0.5]},
{"args": {"id": 8, "s0": 0, "s1": 0, "s2": 0, "s3": 0, "s4": 0, "s5": 0}, "rng": [0.5]},
{"args": {"id": 13, "s0": 10, "s1": 10, "s2": 10, "s3": 10, "s4": 10, "s5": 10}, "rng": [0.5]},
{"args": {"id": 20, "s0": 5, "s1": 5, "s2": 5, "s3": 5, "s4": 5, "s5": 5}, "rng": [0.5]},
{"args": {"id": 33, "s0": 18, "s1": 18, "s2": 18, "s3": 18, "s4": 18, "s5": 18}, "rng": [0.5]},
{"args": {"id": 50, "s0": 0, "s1": 0, "s2": 0, "s3": 0, "s4": 0, "s5": 0}, "rng": [0.5]},
{"args": {"id": 75, "s0": 25, "s1": 25, "s2": 25, "s3": 25, "s4": 25, "s5": 25}, "rng": [0.5]},
{"args": {"id": 100, "s0": 12, "s1": 12, "s2": 12, "s3": 12, "s4": 12, "s5": 12}, "rng": [0.5]},
{"args": {"id": 124, "s0": 0, "s1": 18, "s2": 0, "s3": 18, "s4": 0, "s5": 18}, "rng": [0.5]},
{"args": {"id": 150, "s0": 18, "s1": 0, "s2": 18, "s3": 0, "s4": 18, "s5": 0}, "rng": [0.5]},
{"args": {"id": 175, "s0": 1, "s1": 1, "s2": 1, "s3": 1, "s4": 1, "s5": 1}, "rng": [0.5]},
{"args": {"id": 200, "s0": 20, "s1": 20, "s2": 20, "s3": 20, "s4": 20, "s5": 20}, "rng": [0.5]},
{"args": {"id": 225, "s0": 3, "s1": 3, "s2": 3, "s3": 3, "s4": 3, "s5": 3}, "rng": [0.5]},
{"args": {"id": 250, "s0": 25, "s1": 25, "s2": 25, "s3": 25, "s4": 25, "s5": 25}, "rng": [0.5]},
{"args": {"id": 250, "s0": 99, "s1": 99, "s2": 99, "s3": 99, "s4": 99, "s5": 99}, "rng": [0.5]},
{"args": {"id": 299, "s0": 18, "s1": 18, "s2": 18, "s3": 18, "s4": 18, "s5": 18}, "rng": [0.5]},
{"args": {"id": 300, "s0": 0, "s1": 0, "s2": 0, "s3": 0, "s4": 0, "s5": 0}, "rng": [0.5]},
{"args": {"id": 320, "s0": 18, "s1": 18, "s2": 18, "s3": 18, "s4": 18, "s5": 18}, "rng": [0.5]},
{"args": {"id": 366, "s0": 18, "s1": 18, "s2": 18, "s3": 18, "s4": 18, "s5": 18}, "rng": [0.5]}
]
=== END FILE ===
=== FILE: Case.cs ===
using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int id = a.GetProperty("id").GetInt32();
        var stats = new int[6];
        for (int i = 0; i < 6; i++)
            stats[i] = a.GetProperty("s" + i).GetInt32();

        // Absent record: same canonical text the C++ side emits when findItem fails.
        if (!H.ItemsById.ContainsKey((short)id)) return "MISSING_ITEM";

        Item it = H.ItemsById[(short)id];
        Character c = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);

        // Non-null lookup so the hands and worn-count checks run, matching the C++ copies.
        string? why = c.EquipProblem(it, _ => null);
        return why ?? "null";
    }
}
=== END FILE ===
=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/items1a.h"
#include "mordor/items1b.h"

namespace mordor {

// Fills an Item with only the id from the table row. equipProblem and equipCheck
// look up the definition from the id, so the id is the only field loaded here.
static bool findItem(const Ctx& ctx, i32 id, Item& out) {
    for (const auto& r : ctx.data.items) {
        if (r.id == id) {
            out = Item{};
            out.id = r.id;
            return true;
        }
    }
    return false;
}

static void loadCharacter(const JsonValue& a, Character& c) {
    for (int i = 0; i < 6; ++i) {
        char key[3] = {'s', static_cast<char>('0' + i), 0};
        c.abilities.stats[i] = static_cast<i32>(A(a, key).asInt());
    }
}

// Canonical text = the reference's EquipProblem string, or "null" when none.
static Str reasonOf(EquipProblem p) {
    switch (p) {
        case EquipProblem::None: return Str("null");
        case EquipProblem::NoFreeHands: return Str("No Free Hands!");
        case EquipProblem::UsingTooMany: return Str("Using too Many!");
        case EquipProblem::ClassRestricted: return Str("Your current guild cannot use this item!");
        case EquipProblem::GuildsCannotUse: return Str("Your currently joined guilds cannot use this item!");
        case EquipProblem::GuildLevelTooLow: return Str("Your guild level is too low to use item!");
        case EquipProblem::AllGuildLevelsTooLow: return Str("All guild levels too low to use item!");
        case EquipProblem::StatsTooLow: return Str("One or more stats are too low!");
    }
    return Str("UNKNOWN");
}

// equipCheck outcomes mapped to the reference text for the same category.
// Outcomes with no reference text are emitted under their own names.
static Str outcomeOf(EquipOutcome o) {
    switch (o) {
        case EquipOutcome::Ok: return Str("null");
        case EquipOutcome::NoFreeHands: return Str("No Free Hands!");
        case EquipOutcome::TooManyWorn: return Str("Using too Many!");
        case EquipOutcome::GuildRestricted: return Str("Your currently joined guilds cannot use this item!");
        case EquipOutcome::ClassRestricted: return Str("Your current guild cannot use this item!");
        case EquipOutcome::GuildLevelTooLow: return Str("Your guild level is too low to use item!");
        case EquipOutcome::StatsTooLow: return Str("One or more stats are too low!");
        case EquipOutcome::CursedStatsTooLow: return Str("CursedStatsTooLow");
        case EquipOutcome::SilentRefusal: return Str("SilentRefusal");
    }
    return Str("UNKNOWN");
}

static Str run_equipProblem(const JsonValue& a, Ctx& ctx) {
    i32 id = static_cast<i32>(A(a, "id").asInt());
    Item it;
    if (!findItem(ctx, id, it)) return Str("MISSING_ITEM");
    Character c;
    loadCharacter(a, c);
    Inventory inv;
    EquipProblem p = equipProblem(static_cast<const Ctx&>(ctx), c, inv, it);
    return reasonOf(p);
}

static Str run_equipCheck(const JsonValue& a, Ctx& ctx) {
    i32 id = static_cast<i32>(A(a, "id").asInt());
    Item it;
    if (!findItem(ctx, id, it)) return Str("MISSING_ITEM");
    Character c;
    loadCharacter(a, c);
    Inventory inv;
    // storeLike = false: the wear path, which runs the hands and worn-count checks.
    EquipOutcome o = equipCheck(ctx, c, inv, it, false);
    return outcomeOf(o);
}

const CopyEntry kCopies[] = {
    {"equipProblem", &run_equipProblem},
    {"equipCheck", &run_equipCheck},
};
const std::size_t kNumCopies = sizeof(kCopies) / sizeof(kCopies[0]);

}  // namespace mordor
=== END FILE ===

Notes:
- **equipCheck and cursed items:** `findItem` loads only the id. `equipCheck` reads `item.cursed`, which the harness doesn't set from the record. A cursed item that fails on stats will therefore show `StatsTooLow` on the C++ side but `One or more stats are too low!` on the C# side. I didn't confirm that `Item` has a `cursed` field, so I didn't set it. If a mismatch appears for cursed items, this is the first place to look.
- **Unreachable branches:** the inventory is always empty, so the hands and worn-cap branches can't fire. The cases only exercise the guild, level and stats branches.

REFERENCE: Character.EquipProblem