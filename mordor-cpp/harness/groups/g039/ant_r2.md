The C++ build failed because the previous `case.cpp` declared a different symbol (`mordor_export_partyb_prune`) and never declared the renamed `pruneTheDead__*` functions. The copies share a name, so the harness renames each one and they can be called directly, with no shim. The new `case.cpp` declares each renamed function once with its exact signature. `cases.json` and `Case.cs` are unchanged.

The `NullReferenceException` is on the C# side and isn't addressed here. It may come from `Character.Create` receiving `default` for `stats`, or from `RaceData.ByName("Human")` returning null. I can't confirm either from the information provided.

=== FILE: cases.json ===
[
{"args":{"n":1,"sel":0,"h0":0,"l0":1,"x0":2,"y0":3},"rng":[0.5]},
{"args":{"n":1,"sel":9,"h0":5,"l0":1,"x0":2,"y0":3},"rng":[0.5]},
{"args":{"n":2,"sel":0,"h0":5,"l0":1,"x0":1,"y0":1,"h1":3,"l1":1,"x1":2,"y1":2},"rng":[0.5]},
{"args":{"n":2,"sel":0,"h0":5,"l0":1,"x0":1,"y0":1,"h1":0,"l1":2,"x1":4,"y1":4},"rng":[0.5]},
{"args":{"n":2,"sel":0,"h0":0,"l0":1,"x0":1,"y0":1,"h1":4,"l1":3,"x1":9,"y1":9},"rng":[0.5]},
{"args":{"n":2,"sel":1,"h0":0,"l0":1,"x0":1,"y0":1,"h1":4,"l1":1,"x1":5,"y1":5},"rng":[0.5]},
{"args":{"n":2,"sel":0,"h0":0,"l0":1,"x0":1,"y0":1,"h1":0,"l1":2,"x1":2,"y1":2},"rng":[0.5]},
{"args":{"n":2,"sel":0,"h0":-3,"l0":1,"x0":1,"y0":1,"h1":2,"l1":1,"x1":2,"y1":2},"rng":[0.5]},
{"args":{"n":2,"sel":0,"h0":1,"l0":1,"x0":1,"y0":1,"h1":1,"l1":1,"x1":2,"y1":2},"rng":[0.5]},
{"args":{"n":2,"sel":0,"h0":1,"l0":1,"x0":1,"y0":1,"h1":0,"l1":1,"x1":2,"y1":2},"rng":[0.5]},
{"args":{"n":2,"sel":0,"h0":0,"l0":0,"x0":0,"y0":0,"h1":3,"l1":-1,"x1":5,"y1":-2},"rng":[0.5]},
{"args":{"n":2,"sel":0,"h0":0,"l0":1,"x0":1,"y0":1,"h1":2,"l1":1,"x1":1,"y1":1},"rng":[0.5]},
{"args":{"n":2,"sel":-1,"h0":0,"l0":1,"x0":1,"y0":1,"h1":1,"l1":1,"x1":2,"y1":2},"rng":[0.5]},
{"args":{"n":2,"sel":5,"h0":0,"l0":1,"x0":1,"y0":1,"h1":1,"l1":1,"x1":2,"y1":2},"rng":[0.5]},
{"args":{"n":2,"sel":1,"h0":2,"l0":1,"x0":3,"y0":3,"h1":4,"l1":1,"x1":7,"y1":7},"rng":[0.5]},
{"args":{"n":2,"sel":0,"h0":0,"l0":0,"x0":0,"y0":0,"h1":0,"l1":0,"x1":0,"y1":0},"rng":[0.5]},
{"args":{"n":3,"sel":2,"h0":5,"l0":1,"x0":1,"y0":1,"h1":0,"l1":1,"x1":2,"y1":2,"h2":6,"l2":1,"x2":3,"y2":3},"rng":[0.5]},
{"args":{"n":3,"sel":0,"h0":0,"l0":1,"x0":1,"y0":1,"h1":0,"l1":1,"x1":2,"y1":2,"h2":6,"l2":1,"x2":3,"y2":3},"rng":[0.5]},
{"args":{"n":3,"sel":1,"h0":0,"l0":1,"x0":1,"y0":1,"h1":4,"l1":1,"x1":2,"y1":2,"h2":0,"l2":1,"x2":3,"y2":3},"rng":[0.5]},
{"args":{"n":3,"sel":0,"h0":5,"l0":1,"x0":1,"y0":1,"h1":0,"l1":2,"x1":2,"y1":2,"h2":0,"l2":1,"x2":3,"y2":3},"rng":[0.5]},
{"args":{"n":3,"sel":2,"h0":5,"l0":1,"x0":1,"y0":1,"h1":5,"l1":1,"x1":2,"y1":2,"h2":0,"l2":1,"x2":3,"y2":3},"rng":[0.5]},
{"args":{"n":3,"sel":7,"h0":5,"l0":1,"x0":1,"y0":1,"h1":5,"l1":1,"x1":2,"y1":2,"h2":5,"l2":1,"x2":3,"y2":3},"rng":[0.5]},
{"args":{"n":3,"sel":0,"h0":0,"l0":1,"x0":1,"y0":1,"h1":0,"l1":1,"x1":2,"y1":2,"h2":0,"l2":1,"x2":3,"y2":3},"rng":[0.5]},
{"args":{"n":4,"sel":3,"h0":0,"l0":1,"x0":1,"y0":1,"h1":0,"l1":1,"x1":2,"y1":2,"h2":0,"l2":1,"x2":3,"y2":3,"h3":0,"l3":1,"x3":4,"y3":4},"rng":[0.5]},
{"args":{"n":4,"sel":3,"h0":5,"l0":1,"x0":1,"y0":1,"h1":0,"l1":2,"x1":2,"y1":2,"h2":0,"l2":1,"x2":3,"y2":3,"h3":2,"l3":4,"x3":4,"y3":4},"rng":[0.5]},
{"args":{"n":4,"sel":1,"h0":5,"l0":1,"x0":1,"y0":1,"h1":0,"l1":1,"x1":2,"y1":2,"h2":5,"l2":1,"x2":3,"y2":3,"h3":5,"l3":1,"x3":4,"y3":4},"rng":[0.5]},
{"args":{"n":4,"sel":3,"h0":5,"l0":1,"x0":1,"y0":1,"h1":0,"l1":1,"x1":2,"y1":2,"h2":5,"l2":1,"x2":3,"y2":3,"h3":5,"l3":1,"x3":4,"y3":4},"rng":[0.5]},
{"args":{"n":4,"sel":0,"h0":0,"l0":1,"x0":1,"y0":1,"h1":5,"l1":1,"x1":2,"y1":2,"h2":0,"l2":1,"x2":3,"y2":3,"h3":5,"l3":1,"x3":4,"y3":4},"rng":[0.5]},
{"args":{"n":4,"sel":2,"h0":5,"l0":1,"x0":1,"y0":1,"h1":5,"l1":1,"x1":2,"y1":2,"h2":5,"l2":1,"x2":3,"y2":3,"h3":5,"l3":1,"x3":4,"y3":4},"rng":[0.5]}
]
=== END FILE ===
=== FILE: Case.cs ===
using System;
using System.Collections.Generic;
using System.Reflection;
using System.Text;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int n = Get(a, "n");
        var list = new List<Character>();
        for (int i = 0; i < n; i++)
        {
            var c = Character.Create(((char)('A' + i)).ToString(), RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, default);
            c.CurHits = Get(a, "h" + i);
            c.DungeonLevel = Get(a, "l" + i);
            c.X = Get(a, "x" + i);
            c.Y = Get(a, "y" + i);
            list.Add(c);
        }
        var party = new Party(list.ToArray());
        // SelectedIndex has a private setter; set it through the existing setter.
        typeof(Party).GetProperty("SelectedIndex")!.GetSetMethod(true)!.Invoke(party, new object[] { Get(a, "sel") });

        party.RemoveTheDead(); // returned messages and rng are not part of the canonical string

        return Dump(party);
    }

    static int Get(JsonElement a, string key) => a.TryGetProperty(key, out var v) ? v.GetInt32() : 0;

    static string Dump(Party p)
    {
        var sb = new StringBuilder();
        sb.Append(p.Size).Append('|').Append(p.SelectedIndex);
        foreach (var c in p.Members)
        {
            sb.Append('|').Append(c.Name).Append('|').Append(c.CurHits)
              .Append('|').Append(c.IsDead ? 1 : 0)
              .Append('|').Append(c.Stoned ? 1 : 0)
              .Append('|').Append(c.DungeonLevel)
              .Append('|').Append(c.X).Append('|').Append(c.Y);
        }
        return sb.ToString();
    }
}
=== END FILE ===
=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"

namespace mordor {

// Same-name copies: the harness renames each source's pruneTheDead, so these
// are callable directly. Each signature matches its original exactly.
void pruneTheDead__partyb(Ctx& ctx, Party& party);
void pruneTheDead__death1a(Ctx& ctx, Party& party, Arr<bool, kPartyMaxSize>& countedDead);

static i32 slot(const JsonValue& a, const char* prefix, i32 i) {
    Str k = Str(prefix) + I(i);
    return static_cast<i32>(A(a, k.c_str()).asInt());
}

static void fillParty(Party& p, const JsonValue& a) {
    const i32 n = static_cast<i32>(A(a, "n").asInt());
    for (i32 i = 0; i < n; ++i) {
        Character& m = p.members[i];
        m.name = Str(1, static_cast<char>('A' + i));
        m.vitals.curHits = slot(a, "h", i);
        m.location.dungeonLevel = slot(a, "l", i);
        m.location.x = slot(a, "x", i);
        m.location.y = slot(a, "y", i);
    }
    p.size = n;
    p.selectedIndex = static_cast<i32>(A(a, "sel").asInt());
}

static Str dump(Party& p) {
    Str s = I(p.size) + "|" + I(p.selectedIndex);
    for (i32 i = 0; i < p.size; ++i) {
        Character& m = p.members[i];
        const long long dead = m.isDead() ? 1 : 0;
        const long long stoned = (m.deathState == DeathState::Stoned) ? 1 : 0;
        s += "|" + m.name + "|" + I(m.vitals.curHits) + "|" + I(dead) + "|" + I(stoned)
           + "|" + I(m.location.dungeonLevel) + "|" + I(m.location.x) + "|" + I(m.location.y);
    }
    return s;
}

static Str run_partyb(const JsonValue& a, Ctx& ctx) {
    Party p;
    fillParty(p, a);
    pruneTheDead__partyb(ctx, p);
    return dump(p);
}

static Str run_death1a(const JsonValue& a, Ctx& ctx) {
    Party p;
    fillParty(p, a);
    Arr<bool, kPartyMaxSize> counted{};
    pruneTheDead__death1a(ctx, p, counted);
    return dump(p);
}

const CopyEntry kCopies[] = {
    {"pruneTheDead@partyb", &run_partyb},
    {"pruneTheDead@death1a", &run_death1a},
};
const std::size_t kNumCopies = 2;

} // namespace mordor
=== END FILE ===

REFERENCE: Party.RemoveTheDead