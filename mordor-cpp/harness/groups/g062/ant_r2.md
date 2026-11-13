=== FILE: cases.json ===
[
{"args":{"race":0,"STR":14,"INT":12,"WIS":10,"CON":15,"CHA":8,"DEX":13},"rng":[0.5]},
{"args":{"race":0,"STR":1,"INT":1,"WIS":1,"CON":1,"CHA":1,"DEX":1},"rng":[0.5]},
{"args":{"race":0,"STR":18,"INT":18,"WIS":18,"CON":18,"CHA":18,"DEX":18},"rng":[0.5]},
{"args":{"race":0,"STR":0,"INT":0,"WIS":0,"CON":0,"CHA":0,"DEX":0},"rng":[0.5]},
{"args":{"race":0,"STR":25,"INT":12,"WIS":10,"CON":30,"CHA":8,"DEX":13},"rng":[0.5]},
{"args":{"race":0,"STR":-1,"INT":12,"WIS":10,"CON":15,"CHA":8,"DEX":13},"rng":[0.5]},
{"args":{"race":0,"STR":2147483647,"INT":12,"WIS":10,"CON":15,"CHA":8,"DEX":13},"rng":[0.5]},
{"args":{"race":0,"STR":14,"INT":12,"WIS":10,"CON":15,"CHA":8,"DEX":13},"rng":[0.0]},
{"args":{"race":0,"STR":14,"INT":12,"WIS":10,"CON":15,"CHA":8,"DEX":13},"rng":[0.999999]},
{"args":{"race":0,"STR":1,"INT":2,"WIS":3,"CON":4,"CHA":5,"DEX":6},"rng":[0.25,0.75]},
{"args":{"race":1,"STR":10,"INT":10,"WIS":10,"CON":10,"CHA":10,"DEX":10},"rng":[0.5]},
{"args":{"race":1,"STR":3,"INT":18,"WIS":7,"CON":12,"CHA":9,"DEX":16},"rng":[0.5]},
{"args":{"race":1,"STR":1,"INT":1,"WIS":1,"CON":1,"CHA":1,"DEX":1},"rng":[0.5]},
{"args":{"race":1,"STR":0,"INT":0,"WIS":0,"CON":0,"CHA":0,"DEX":0},"rng":[0.5]},
{"args":{"race":1,"STR":0,"INT":0,"WIS":0,"CON":0,"CHA":0,"DEX":18},"rng":[0.5]},
{"args":{"race":1,"STR":10,"INT":10,"WIS":10,"CON":10,"CHA":10,"DEX":10},"rng":[0.1,0.9]},
{"args":{"race":2,"STR":15,"INT":15,"WIS":15,"CON":15,"CHA":15,"DEX":15},"rng":[0.5]},
{"args":{"race":2,"STR":18,"INT":3,"WIS":12,"CON":9,"CHA":14,"DEX":11},"rng":[0.5]},
{"args":{"race":2,"STR":99,"INT":99,"WIS":99,"CON":99,"CHA":99,"DEX":99},"rng":[0.5]},
{"args":{"race":2,"STR":0,"INT":0,"WIS":0,"CON":0,"CHA":0,"DEX":0},"rng":[0.5]},
{"args":{"race":2,"STR":10,"INT":10,"WIS":10,"CON":0,"CHA":10,"DEX":10},"rng":[0.999999]},
{"args":{"race":3,"STR":12,"INT":14,"WIS":11,"CON":13,"CHA":10,"DEX":9},"rng":[0.5]},
{"args":{"race":3,"STR":1,"INT":0,"WIS":0,"CON":0,"CHA":0,"DEX":0},"rng":[0.5]},
{"args":{"race":3,"STR":-2147483648,"INT":12,"WIS":10,"CON":15,"CHA":8,"DEX":13},"rng":[0.5]},
{"args":{"race":3,"STR":7,"INT":7,"WIS":7,"CON":7,"CHA":7,"DEX":7},"rng":[0.3]}
]
=== END FILE ===
=== FILE: Case.cs ===
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        // Race is selected by its index in RaceData.All (assumed to match the C++ Race enum value).
        var race = RaceData.All[a.GetProperty("race").GetInt32()];
        int[] stats =
        {
            a.GetProperty("STR").GetInt32(),
            a.GetProperty("INT").GetInt32(),
            a.GetProperty("WIS").GetInt32(),
            a.GetProperty("CON").GetInt32(),
            a.GetProperty("CHA").GetInt32(),
            a.GetProperty("DEX").GetInt32(),
        };

        // Reference R5 creation. Name, sex and alignment are not part of the compared R5 state.
        var c = Character.Create("Test", race, Alignment.Neutral, Sex.Male, stats);

        // Character.Race holds the race name; recover its index in RaceData.All.
        int raceNum = -1;
        for (int i = 0; i < RaceData.All.Count; i++)
        {
            if (RaceData.All[i].Name == c.Race)
            {
                raceNum = i;
                break;
            }
        }

        // GoldOnHand is a VbCurrency (no implicit conversion to double), so go through its text form.
        // NumberStyles.Any tolerates thousands separators and currency symbols.
        double gold = double.Parse(c.GoldOnHand.ToString(), NumberStyles.Any, CultureInfo.InvariantCulture);

        var p = new List<string>();
        p.Add(raceNum.ToString());
        foreach (var s in c.Stats) p.Add(s.ToString());
        foreach (var s in c.StatMax) p.Add(s.ToString());
        p.Add(c.MaxHits.ToString());
        p.Add(c.CurHits.ToString());
        p.Add(H.F(gold));
        p.Add(c.Area.ToString());
        p.Add(c.Guilds.Count.ToString());
        p.Add(c.Guilds[0].Level.ToString());
        p.Add(c.Guilds[0].Experience.ToString());
        return string.Join("|", p);
    }
}
=== END FILE ===
=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/state_character.h"

namespace mordor {

// Same-name copy from src/attributes.cpp, renamed by the harness to createCharacter__attributes.
void createCharacter__attributes(Ctx& ctx, Character& c, Race race, Guild startGuild,
                                 const Arr<i32, kStatCount>& stats);

static Str joinBar(const Vec<Str>& p) {
    Str out;
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (i != 0) out += "|";
        out += p[i];
    }
    return out;
}

static Str run_attributes(const JsonValue& a, Ctx& ctx) {
    const i32 raceIdx = static_cast<i32>(A(a, "race").asInt());

    // Stats in STR, INT, WIS, CON, CHA, DEX order (same as the reference).
    Arr<i32, kStatCount> st{};
    st[0] = static_cast<i32>(A(a, "STR").asInt());
    st[1] = static_cast<i32>(A(a, "INT").asInt());
    st[2] = static_cast<i32>(A(a, "WIS").asInt());
    st[3] = static_cast<i32>(A(a, "CON").asInt());
    st[4] = static_cast<i32>(A(a, "CHA").asInt());
    st[5] = static_cast<i32>(A(a, "DEX").asInt());

    Character c;
    createCharacter__attributes(ctx, c, static_cast<Race>(raceIdx), Guild::Nomad, st);

    Vec<Str> p;
    p.push_back(I(static_cast<long long>(c.race)));
    for (u32 i = 0; i < static_cast<u32>(kStatCount); ++i) p.push_back(I(c.abilities.stats[i]));
    for (u32 i = 0; i < static_cast<u32>(kStatCount); ++i) p.push_back(I(c.abilities.statMax[i]));
    p.push_back(I(c.vitals.maxHits));
    p.push_back(I(c.vitals.curHits));
    // Reference stores whole gold; C++ stores 1/10000 gold.
    p.push_back(F(static_cast<double>(c.wallet.goldOnHand) / 10000.0));
    p.push_back(I(c.location.area));

    i32 joined = 0;
    for (u32 i = 0; i < static_cast<u32>(kGuildCount); ++i) {
        if (c.guilds[i].joined) ++joined;
    }
    const GuildMembership& m = c.guilds[static_cast<u32>(Guild::Nomad)];
    p.push_back(I(joined));
    p.push_back(I(m.level));
    p.push_back(I(m.experience));

    return joinBar(p);
}

const CopyEntry kCopies[] = {
    {"createCharacter@attributes", &run_attributes},
};
const std::size_t kNumCopies = 1;

}  // namespace mordor
=== END FILE ===

REFERENCE: Character.Create