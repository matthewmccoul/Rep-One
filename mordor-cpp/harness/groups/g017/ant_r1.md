I couldn't build the full differential test. The reference's hand, worn-cap, and guild checks depend on inventory and guild state that the shown API does not expose, so some branches can't be reached on both sides. The gaps are listed after the files.

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

static bool findItem(const Ctx& ctx, i32 id, Item& out) {
    for (const auto& r : ctx.data.items) {
        if (r.id == id) { out = r; return true; }
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
    EquipOutcome o = equipCheck(ctx, c, inv, it, false);
    return outcomeOf(o);
}

const CopyEntry kCopies[] = {
    {"equipProblem", &run_equipProblem},
    {"equipCheck", &run_equipCheck},
};
const std::size_t kNumCopies = 2;

} // namespace mordor
=== END FILE ===

Gaps and assumptions:
- **Unreachable on both sides:** NoFreeHands and UsingTooMany need worn or carried items, and the shown API doesn't let me set those up identically in C# and C++. Both are therefore untested.
- **Null-lookup path:** C# `EquipProblem(it, null)` skips the hands and worn checks. The C++ copies always run those checks, so this path is not tested.
- **Guild outcomes:** Guild membership isn't settable through the shown API, so the guild outcomes depend on the table's `Guilds` and `ClassRestricted` values for each item.
- **equipCheck mapping:** Its outcomes are mapped to reference text by category. SilentRefusal and CursedStatsTooLow have no reference equivalent and print under their own names, so they will mismatch by design. The copy's always-`GuildLevelTooLow` outcome will mismatch the reference's flag-based wording for unrestricted items.
- **Unverified ids:** The item ids are chosen by guess. I did not check that each id exists in the shareware table. A missing id makes the C# side throw and the C++ side print `MISSING_ITEM`.
- **Assumed C++ APIs:** The C++ side assumes `ctx.data.items` holds `Item` records, that `Character` has `abilities.stats`, and that the headers are at `mordor/items1a.h` and `mordor/items1b.h`. `equipProblem` has two overloads, so the call casts `ctx` to `const Ctx&` to pick the one in `items1b.h`.
- Stat-check cases use fixed values (0, 1, 3, 5, 10, 12, 18, 20, 25, 99, -1) and do not target each item's `StatsRequired` threshold exactly. The `s0`–`s5` keys avoid assuming a JSON array API in C++.
- The rule draws no random numbers, so every case uses `rng: [0.5]`.

REFERENCE: Character.EquipProblem