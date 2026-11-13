Here are all three files. The Case.cs `Pack` assignment is gone, so the compile error no longer applies. Notes on gaps and assumptions follow the files.

=== FILE: cases.json ===
[
{"args":{"trap":1,"level":3,"opener":0,"count":1,"h0":10,"mh0":10,"s0":5,"ms0":5},"rng":[0.0]},
{"args":{"trap":1,"level":3,"opener":0,"count":1,"h0":10,"mh0":10,"s0":5,"ms0":5},"rng":[0.999]},
{"args":{"trap":1,"level":3,"opener":1,"count":3,"h0":10,"mh0":10,"s0":5,"ms0":5,"h1":10,"mh1":10,"s1":5,"ms1":5,"h2":10,"mh2":10,"s2":5,"ms2":5},"rng":[0.0,0.999,0.0]},
{"args":{"trap":1,"level":3,"opener":1,"count":3,"h0":10,"mh0":10,"s0":5,"ms0":5,"h1":10,"mh1":10,"s1":5,"ms1":5,"h2":10,"mh2":10,"s2":5,"ms2":5},"rng":[0.999,0.0,0.999]},
{"args":{"trap":1,"level":0,"opener":0,"count":1,"h0":10,"mh0":10,"s0":5,"ms0":5},"rng":[0.5]},
{"args":{"trap":1,"level":15,"opener":3,"count":4,"h0":20,"mh0":20,"s0":0,"ms0":0,"h1":20,"mh1":20,"s1":0,"ms1":0,"h2":20,"mh2":20,"s2":0,"ms2":0,"h3":20,"mh3":20,"s3":0,"ms3":0},"rng":[0.25]},
{"args":{"trap":1,"level":1,"opener":0,"count":1,"h0":0,"mh0":0,"s0":0,"ms0":0},"rng":[0.0]},
{"args":{"trap":2,"level":3,"opener":0,"count":1,"h0":10,"mh0":10,"s0":5,"ms0":5},"rng":[0.0]},
{"args":{"trap":2,"level":3,"opener":0,"count":1,"h0":10,"mh0":10,"s0":5,"ms0":5},"rng":[0.999]},
{"args":{"trap":2,"level":5,"opener":2,"count":3,"h0":10,"mh0":10,"s0":5,"ms0":5,"h1":10,"mh1":10,"s1":5,"ms1":5,"h2":10,"mh2":10,"s2":5,"ms2":5},"rng":[0.999,0.999,0.0]},
{"args":{"trap":2,"level":0,"opener":0,"count":2,"h0":10,"mh0":10,"s0":5,"ms0":5,"h1":10,"mh1":10,"s1":5,"ms1":5},"rng":[0.5]},
{"args":{"trap":3,"level":0,"opener":0,"count":1,"h0":10,"mh0":10,"s0":0,"ms0":0},"rng":[0.9]},
{"args":{"trap":3,"level":1,"opener":0,"count":1,"h0":10,"mh0":10,"s0":0,"ms0":0},"rng":[0.99]},
{"args":{"trap":3,"level":15,"opener":0,"count":2,"h0":100,"mh0":100,"s0":0,"ms0":0,"h1":100,"mh1":100,"s1":0,"ms1":0},"rng":[0.5]},
{"args":{"trap":3,"level":15,"opener":0,"count":1,"h0":1,"mh0":1,"s0":0,"ms0":0},"rng":[0.99]},
{"args":{"trap":3,"level":5,"opener":2,"count":3,"h0":50,"mh0":50,"s0":0,"ms0":0,"h1":50,"mh1":50,"s1":0,"ms1":0,"h2":50,"mh2":50,"s2":0,"ms2":0},"rng":[0.0,0.5,0.99]},
{"args":{"trap":4,"level":1,"opener":0,"count":1,"h0":10,"mh0":10,"s0":0,"ms0":0},"rng":[0.99]},
{"args":{"trap":4,"level":15,"opener":1,"count":2,"h0":100,"mh0":100,"s0":0,"ms0":0,"h1":100,"mh1":100,"s1":0,"ms1":0},"rng":[0.25,0.75]},
{"args":{"trap":4,"level":0,"opener":0,"count":1,"h0":3,"mh0":3,"s0":0,"ms0":0},"rng":[0.0]},
{"args":{"trap":4,"level":8,"opener":0,"count":1,"h0":0,"mh0":0,"s0":0,"ms0":0},"rng":[0.6]},
{"args":{"trap":5,"level":0,"opener":0,"count":1,"h0":10,"mh0":10,"s0":5,"ms0":5},"rng":[0.25]},
{"args":{"trap":5,"level":0,"opener":0,"count":1,"h0":10,"mh0":10,"s0":5,"ms0":5},"rng":[0.5]},
{"args":{"trap":5,"level":15,"opener":0,"count":1,"h0":10,"mh0":10,"s0":5,"ms0":5},"rng":[0.75]},
{"args":{"trap":5,"level":15,"opener":0,"count":1,"h0":10,"mh0":10,"s0":5,"ms0":5},"rng":[0.875]},
{"args":{"trap":5,"level":15,"opener":0,"count":1,"h0":10,"mh0":10,"s0":5,"ms0":5},"rng":[0.99]},
{"args":{"trap":6,"level":1,"opener":0,"count":1,"h0":10,"mh0":20,"s0":3,"ms0":8},"rng":[0.0]},
{"args":{"trap":6,"level":1,"opener":0,"count":1,"h0":10,"mh0":20,"s0":3,"ms0":8},"rng":[0.999]},
{"args":{"trap":6,"level":1,"opener":0,"count":1,"h0":5,"mh0":0,"s0":5,"ms0":0},"rng":[0.5]},
{"args":{"trap":6,"level":1,"opener":1,"count":2,"h0":5,"mh0":8,"s0":2,"ms0":6,"h1":5,"mh1":8,"s1":2,"ms1":6},"rng":[0.5,0.25,0.75,0.5]}
]
=== END FILE ===

=== FILE: Case.cs ===
using System;
using System.Collections.Generic;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int trap = a.GetProperty("trap").GetInt32();
        int level = a.GetProperty("level").GetInt32();
        int opener = a.GetProperty("opener").GetInt32();
        int count = a.GetProperty("count").GetInt32();

        var party = new List<Character>();
        for (int i = 0; i < count; i++)
        {
            var c = Character.Create("M" + i, RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
            c.MaxHits = a.GetProperty("mh" + i).GetInt32();
            c.CurHits = a.GetProperty("h" + i).GetInt32();
            c.MaxSpellPoints = a.GetProperty("ms" + i).GetInt32();
            c.CurSpellPoints = a.GetProperty("s" + i).GetInt32();
            party.Add(c);
        }

        var who = party[opener];
        var r = ChestTraps.Spring(trap, party, who, level, rng, _ => null, (x, y) => 0);

        // Canonical: poisoned|diseased|itemsDestroyed|hits per member|spell points per member.
        // The reference TrapResult has no damage field, so damage is compared through hits.
        var fields = new List<string>
        {
            who.Poisoned ? "1" : "0",
            who.Diseased ? "1" : "0",
            r.Lines.Count > 0 ? "1" : "0",
        };
        foreach (var c in party) fields.Add(c.CurHits.ToString());
        foreach (var c in party) fields.Add(c.CurSpellPoints.ToString());
        return string.Join("|", fields);
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"
#include "mordor/loot1a.h"
#include "mordor/combat2a.h"

namespace mordor {

// Renamed same-name copies. The harness renames each definition to these names.
ChestTrapResult resolveChestTrap__loot1a(Ctx& ctx, Party& party, i32 openerIndex, ChestTrap trap);
void resolveChestTrap__combat2a(Ctx& ctx, Party& party, ChestTrap trap);

static ChestTrap trapOf(i32 n) {
    switch (n) {
        case 1: return ChestTrap::PoisonGas;
        case 2: return ChestTrap::DiseaseSpores;
        case 3: return ChestTrap::Explosion;
        case 4: return ChestTrap::Fire;
        case 5: return ChestTrap::ItemDestruction;
        default: return ChestTrap::Fate;
    }
}

static Str keyOf(const char* prefix, i32 i) { return Str(prefix) + I(i); }

static i32 N(const JsonValue& a, const char* k) { return static_cast<i32>(A(a, k).asInt()); }

// Builds the party from the same keys the C# side reads.
static void buildParty(const JsonValue& a, Party& p) {
    const i32 count = N(a, "count");
    const i32 level = N(a, "level");
    p.size = count;
    p.dungeonLevel = level;
    p.opener = N(a, "opener");
    for (i32 i = 0; i < count && i < kPartyMaxSize; ++i) {
        Character& c = p.members[static_cast<u32>(i)];
        c.vitals.curHits = N(a, keyOf("h", i).c_str());
        c.vitals.maxHits = N(a, keyOf("mh", i).c_str());
        c.vitals.curSpellPoints = N(a, keyOf("s", i).c_str());
        c.vitals.maxSpellPoints = N(a, keyOf("ms", i).c_str());
        c.location.dungeonLevel = level;
    }
}

// Canonical: poisoned|diseased|destroyed|hits per member|spell points per member.
static Str finish(const Party& p, Str poisoned, Str diseased, Str destroyed) {
    Str s = poisoned + "|" + diseased + "|" + destroyed;
    for (i32 i = 0; i < p.size; ++i) s += "|" + I(p.members[static_cast<u32>(i)].vitals.curHits);
    for (i32 i = 0; i < p.size; ++i) s += "|" + I(p.members[static_cast<u32>(i)].vitals.curSpellPoints);
    return s;
}

static Str run_loot1a(const JsonValue& a, Ctx& ctx) {
    Party p;
    buildParty(a, p);
    ChestTrapResult r = resolveChestTrap__loot1a(ctx, p, N(a, "opener"), trapOf(N(a, "trap")));
    return finish(p, I(r.poisoned ? 1 : 0), I(r.diseased ? 1 : 0), I(r.itemsDestroyed ? 1 : 0));
}

// combat2a returns void and its ailment and item results are not observable through the shown
// fields, so those three flags print NA. This mismatch is reported, not hidden.
static Str run_combat2a(const JsonValue& a, Ctx& ctx) {
    Party p;
    buildParty(a, p);
    resolveChestTrap__combat2a(ctx, p, trapOf(N(a, "trap")));
    return finish(p, Str("NA"), Str("NA"), Str("NA"));
}

const CopyEntry kCopies[] = {
    {"resolveChestTrap@loot1a", &run_loot1a},
    {"resolveChestTrap@combat2a", &run_combat2a},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
=== END FILE ===

Notes on what this covers and where it is weak:

- **Compile not verified.** I could not build any of these. Each uses only the members shown in the outlines and the copy source.
- **Slime item branch not exercised.** I removed the `Pack` writes because `InventorySlot` has no shown constructor or setter. Trap 5 cases therefore cover the roll gate (below, at, and above the threshold for levels 0 and 15) and the empty-slot path. The branch that destroys an actual item is not covered.
- **Damage not compared directly.** The reference `TrapResult` has no damage field, so `r.damage` from the C++ copy is not printed. Hit points per member are printed instead, which captures the damage.
- **combat2a flags print NA.** That copy returns void, so its poisoned, diseased, and destroyed results are not observable. It will mismatch the reference on those three fields for every case.
- **Resistance is uncontrolled.** Poison, disease, and fire damage depend on resistance I can't set from shown fields. Draws of 0.0 and 0.999 are meant to hit the pass and fail sides, assuming resistance is strictly between 0 and 200 percent. Fire rounding may not be hit exactly.
- **RNG mapping assumed.** Both sides are assumed to map `below(n)` and `Random.Next(n)` to `floor(u*n)` on the scripted value, and the harness is assumed to provide a `Random` subclass that returns the scripted doubles.
- **Stats assumed equal.** Both sides use zero stats. C++ stats are not shown, so this is an assumption.
- **Traps 7 and up are not covered.** The reference text for those cases is truncated.

REFERENCE: ChestTraps.Spring