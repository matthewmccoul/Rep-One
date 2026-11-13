The previous Case.cs and case.cpp built their characters differently: C# used `Character.Create` with zeroed stats, while C++ used a default-constructed `Character`. Both sides now use the same race, the Nomad starting guild, and the same six stats, which come from args. I also added `shim_status1a.cpp`, which the previous files omitted even though the status1a copy is file-local. `createCharacter`'s stats parameter type isn't shown, so the C++ side passes a braced list.

=== FILE: cases.json ===
[
  {"args": {"level": 5, "prior_level": 0, "prior_adshift": 0, "stat_str": 10, "stat_int": 10, "stat_wis": 10, "stat_con": 10, "stat_cha": 10, "stat_dex": 10}, "rng": [0.5]},
  {"args": {"level": 1, "prior_level": 0, "prior_adshift": 0, "stat_str": 3, "stat_int": 3, "stat_wis": 3, "stat_con": 3, "stat_cha": 3, "stat_dex": 3}, "rng": [0.5]},
  {"args": {"level": 0, "prior_level": 0, "prior_adshift": 0, "stat_str": 18, "stat_int": 18, "stat_wis": 18, "stat_con": 18, "stat_cha": 18, "stat_dex": 18}, "rng": [0.5]},
  {"args": {"level": -1, "prior_level": 0, "prior_adshift": 0, "stat_str": 16, "stat_int": 8, "stat_wis": 12, "stat_con": 14, "stat_cha": 10, "stat_dex": 15}, "rng": [0.5]},
  {"args": {"level": -5, "prior_level": 0, "prior_adshift": 3, "stat_str": 12, "stat_int": 14, "stat_wis": 9, "stat_con": 11, "stat_cha": 13, "stat_dex": 7}, "rng": [0.5]},
  {"args": {"level": 2147483647, "prior_level": 0, "prior_adshift": 0, "stat_str": 5, "stat_int": 17, "stat_wis": 6, "stat_con": 18, "stat_cha": 4, "stat_dex": 12}, "rng": [0.5]},
  {"args": {"level": -2147483648, "prior_level": 0, "prior_adshift": 0, "stat_str": 15, "stat_int": 13, "stat_wis": 17, "stat_con": 9, "stat_cha": 8, "stat_dex": 11}, "rng": [0.5]},
  {"args": {"level": 255, "prior_level": 0, "prior_adshift": 0, "stat_str": 9, "stat_int": 11, "stat_wis": 7, "stat_con": 13, "stat_cha": 16, "stat_dex": 14}, "rng": [0.5]},
  {"args": {"level": 256, "prior_level": 0, "prior_adshift": 0, "stat_str": 18, "stat_int": 3, "stat_wis": 18, "stat_con": 3, "stat_cha": 18, "stat_dex": 3}, "rng": [0.5]},
  {"args": {"level": 7, "prior_level": 3, "prior_adshift": 4, "stat_str": 4, "stat_int": 6, "stat_wis": 8, "stat_con": 10, "stat_cha": 12, "stat_dex": 14}, "rng": [0.5]},
  {"args": {"level": 0, "prior_level": 3, "prior_adshift": 4, "stat_str": 13, "stat_int": 12, "stat_wis": 11, "stat_con": 10, "stat_cha": 9, "stat_dex": 8}, "rng": [0.5]},
  {"args": {"level": 9, "prior_level": -1, "prior_adshift": 4, "stat_str": 17, "stat_int": 15, "stat_wis": 13, "stat_con": 11, "stat_cha": 9, "stat_dex": 7}, "rng": [0.5]},
  {"args": {"level": 1, "prior_level": 1, "prior_adshift": 0, "stat_str": 10, "stat_int": 10, "stat_wis": 10, "stat_con": 10, "stat_cha": 10, "stat_dex": 10}, "rng": [0.5]},
  {"args": {"level": 12, "prior_level": 12, "prior_adshift": -7, "stat_str": 3, "stat_int": 3, "stat_wis": 3, "stat_con": 3, "stat_cha": 3, "stat_dex": 3}, "rng": [0.5]},
  {"args": {"level": -5, "prior_level": -5, "prior_adshift": 2, "stat_str": 18, "stat_int": 18, "stat_wis": 18, "stat_con": 18, "stat_cha": 18, "stat_dex": 18}, "rng": [0.5]},
  {"args": {"level": 4, "prior_level": 2, "prior_adshift": 12, "stat_str": 16, "stat_int": 8, "stat_wis": 12, "stat_con": 14, "stat_cha": 10, "stat_dex": 15}, "rng": [0.5]},
  {"args": {"level": 4, "prior_level": 0, "prior_adshift": 12, "stat_str": 12, "stat_int": 14, "stat_wis": 9, "stat_con": 11, "stat_cha": 13, "stat_dex": 7}, "rng": [0.5]},
  {"args": {"level": 10, "prior_level": 0, "prior_adshift": -5, "stat_str": 5, "stat_int": 17, "stat_wis": 6, "stat_con": 18, "stat_cha": 4, "stat_dex": 12}, "rng": [0.5]},
  {"args": {"level": 6, "prior_level": 0, "prior_adshift": 2147483647, "stat_str": 15, "stat_int": 13, "stat_wis": 17, "stat_con": 9, "stat_cha": 8, "stat_dex": 11}, "rng": [0.5]},
  {"args": {"level": 2, "prior_level": 0, "prior_adshift": -2147483648, "stat_str": 9, "stat_int": 11, "stat_wis": 7, "stat_con": 13, "stat_cha": 16, "stat_dex": 14}, "rng": [0.5]},
  {"args": {"level": 15, "prior_level": 0, "prior_adshift": 7, "stat_str": 18, "stat_int": 3, "stat_wis": 18, "stat_con": 3, "stat_cha": 18, "stat_dex": 3}, "rng": [0.5]},
  {"args": {"level": 8, "prior_level": 0, "prior_adshift": 0, "stat_str": 4, "stat_int": 6, "stat_wis": 8, "stat_con": 10, "stat_cha": 12, "stat_dex": 14}, "rng": [0.0]},
  {"args": {"level": 8, "prior_level": 0, "prior_adshift": 0, "stat_str": 13, "stat_int": 12, "stat_wis": 11, "stat_con": 10, "stat_cha": 9, "stat_dex": 8}, "rng": [0.999]},
  {"args": {"level": 0, "prior_level": 0, "prior_adshift": 9, "stat_str": 17, "stat_int": 15, "stat_wis": 13, "stat_con": 11, "stat_cha": 9, "stat_dex": 7}, "rng": [0.5]}
]

=== END FILE ===
=== FILE: Case.cs ===
using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    // paralyse consumes no random draws (the reference takes no Random); rng is unused.
    public static string Run(JsonElement a, Random rng)
    {
        int level = a.GetProperty("level").GetInt32();
        int priorLevel = a.GetProperty("prior_level").GetInt32();
        int priorAd = a.GetProperty("prior_adshift").GetInt32();

        // Stats in STR, INT, WIS, CON, CHA, DEX order, matching the C++ side.
        int[] stats = new int[]
        {
            a.GetProperty("stat_str").GetInt32(),
            a.GetProperty("stat_int").GetInt32(),
            a.GetProperty("stat_wis").GetInt32(),
            a.GetProperty("stat_con").GetInt32(),
            a.GetProperty("stat_cha").GetInt32(),
            a.GetProperty("stat_dex").GetInt32(),
        };

        // Same race, alignment, sex and starting guild as the C++ createCharacter call.
        var c = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats);
        c.ParalysisLevel = priorLevel;
        c.ADShift = priorAd;

        // No item lookup: all items absent on both sides.
        c.Paralyse(level, _ => null);

        // Paralyse returns void; state fields changed by the rule: ParalysisLevel, ADShift.
        return $"{c.ParalysisLevel}|{c.ADShift}";
    }
}

=== END FILE ===
=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/state_character.h"
#include "mordor/attributes.h"

namespace mordor {

// status2.cpp copy is not file-local: harness renames it to paralyse__status2.
void paralyse__status2(Ctx& ctx, Character& c, i32 level);

// status1a.cpp copy is file-local and same-name: reached through shim_status1a.cpp.
void paralyse_via_status1a(Ctx& ctx, Character& c, i32 level);

// Same construction as the C# side: Human, Nomad starting guild, same six stats (STR,INT,WIS,CON,CHA,DEX).
static void build(const JsonValue& a, Ctx& ctx, Character& c) {
    createCharacter(ctx, c, Race::Human, Guild::Nomad,
        {static_cast<i32>(A(a, "stat_str").asInt()),
         static_cast<i32>(A(a, "stat_int").asInt()),
         static_cast<i32>(A(a, "stat_wis").asInt()),
         static_cast<i32>(A(a, "stat_con").asInt()),
         static_cast<i32>(A(a, "stat_cha").asInt()),
         static_cast<i32>(A(a, "stat_dex").asInt())});
}

// Case-specific state set after construction, as on the C# side.
static void setup(const JsonValue& a, Character& c) {
    c.ailments.paralysisLevel = static_cast<i32>(A(a, "prior_level").asInt());
    c.combat.adShift = static_cast<i32>(A(a, "prior_adshift").asInt());
}

static Str canon(const Character& c) {
    return I(c.ailments.paralysisLevel) + "|" + I(c.combat.adShift);
}

static Str run_status1a(const JsonValue& a, Ctx& ctx) {
    Character c;
    build(a, ctx, c);
    setup(a, c);
    paralyse_via_status1a(ctx, c, static_cast<i32>(A(a, "level").asInt()));
    return canon(c);
}

static Str run_status2(const JsonValue& a, Ctx& ctx) {
    Character c;
    build(a, ctx, c);
    setup(a, c);
    paralyse__status2(ctx, c, static_cast<i32>(A(a, "level").asInt()));
    return canon(c);
}

const CopyEntry kCopies[] = {
    {"paralyse@status1a", &run_status1a},
    {"paralyse@status2", &run_status2},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor

=== END FILE ===
=== FILE: shim_status1a.cpp ===
namespace mordor {

// Export wrapper for the file-local status1a copy (renamed to paralyse__status1a by the harness).
void paralyse_via_status1a(Ctx& ctx, Character& c, i32 level) {
    paralyse__status1a(ctx, c, level);
}

}  // namespace mordor

=== END FILE ===

REFERENCE: Character.Paralyse