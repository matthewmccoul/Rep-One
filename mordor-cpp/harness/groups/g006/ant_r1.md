I'm writing the three files now. Several details can't be confirmed from the excerpts, so I list those assumptions after the files.

=== FILE: cases.json ===
[
{"args":{"trap":1,"level":1,"opener":0,"size":1,"packId":0,"h0":20,"mh0":20,"p0":5,"mp0":5},"rng":[0.5]},
{"args":{"trap":1,"level":5,"opener":1,"size":4,"packId":0,"h0":10,"mh0":10,"p0":2,"mp0":2,"h1":12,"mh1":12,"p1":3,"mp1":3,"h2":8,"mh2":9,"p2":1,"mp2":4,"h3":15,"mh3":15,"p3":0,"mp3":0},"rng":[0.1,0.9,0.3,0.7]},
{"args":{"trap":1,"level":0,"opener":0,"size":1,"packId":0,"h0":1,"mh0":1,"p0":0,"mp0":0},"rng":[0.0]},
{"args":{"trap":2,"level":3,"opener":0,"size":2,"packId":0,"h0":7,"mh0":7,"p0":0,"mp0":0,"h1":9,"mh1":9,"p1":1,"mp1":1},"rng":[0.0]},
{"args":{"trap":2,"level":15,"opener":2,"size":3,"packId":0,"h0":5,"mh0":5,"p0":2,"mp0":2,"h1":6,"mh1":6,"p1":2,"mp1":2,"h2":4,"mh2":4,"p2":1,"mp2":1},"rng":[0.99]},
{"args":{"trap":1,"level":9,"opener":3,"size":4,"packId":0,"h0":20,"mh0":20,"p0":4,"mp0":4,"h1":20,"mh1":20,"p1":4,"mp1":4,"h2":20,"mh2":20,"p2":4,"mp2":4,"h3":20,"mh3":20,"p3":4,"mp3":4},"rng":[0.25,0.5,0.75,0.999]},
{"args":{"trap":3,"level":1,"opener":0,"size":1,"packId":0,"h0":20,"mh0":20,"p0":0,"mp0":0},"rng":[0.0]},
{"args":{"trap":3,"level":1,"opener":0,"size":1,"packId":0,"h0":20,"mh0":20,"p0":0,"mp0":0},"rng":[0.999]},
{"args":{"trap":3,"level":10,"opener":1,"size":3,"packId":0,"h0":30,"mh0":30,"p0":0,"mp0":0,"h1":25,"mh1":25,"p1":0,"mp1":0,"h2":40,"mh2":40,"p2":0,"mp2":0},"rng":[0.5,0.25,0.75]},
{"args":{"trap":3,"level":0,"opener":0,"size":1,"packId":0,"h0":1,"mh0":1,"p0":0,"mp0":0},"rng":[0.5]},
{"args":{"trap":3,"level":4,"opener":0,"size":1,"packId":0,"h0":3,"mh0":3,"p0":0,"mp0":0},"rng":[0.9999]},
{"args":{"trap":4,"level":4,"opener":1,"size":2,"packId":0,"h0":15,"mh0":15,"p0":0,"mp0":0,"h1":16,"mh1":16,"p1":0,"mp1":0},"rng":[0.6]},
{"args":{"trap":4,"level":15,"opener":0,"size":4,"packId":0,"h0":50,"mh0":50,"p0":0,"mp0":0,"h1":40,"mh1":40,"p1":0,"mp1":0,"h2":30,"mh2":30,"p2":0,"mp2":0,"h3":60,"mh3":60,"p3":0,"mp3":0},"rng":[0.3,0.8,0.1,0.95]},
{"args":{"trap":4,"level":0,"opener":0,"size":1,"packId":0,"h0":1,"mh0":1,"p0":0,"mp0":0},"rng":[0.5]},
{"args":{"trap":5,"level":1,"opener":0,"size":1,"packId":0,"h0":10,"mh0":10,"p0":0,"mp0":0},"rng":[0.0]},
{"args":{"trap":5,"level":1,"opener":0,"size":1,"packId":1,"h0":10,"mh0":10,"p0":0,"mp0":0},"rng":[0.0]},
{"args":{"trap":5,"level":10,"opener":0,"size":1,"packId":1,"h0":10,"mh0":10,"p0":0,"mp0":0},"rng":[0.69]},
{"args":{"trap":5,"level":10,"opener":0,"size":1,"packId":1,"h0":10,"mh0":10,"p0":0,"mp0":0},"rng":[0.68]},
{"args":{"trap":5,"level":0,"opener":0,"size":1,"packId":1,"h0":10,"mh0":10,"p0":0,"mp0":0},"rng":[0.49]},
{"args":{"trap":5,"level":0,"opener":0,"size":1,"packId":1,"h0":10,"mh0":10,"p0":0,"mp0":0},"rng":[0.48]},
{"args":{"trap":5,"level":5,"opener":1,"size":2,"packId":1,"h0":10,"mh0":10,"p0":0,"mp0":0,"h1":10,"mh1":10,"p1":0,"mp1":0},"rng":[0.0]},
{"args":{"trap":6,"level":1,"opener":0,"size":1,"packId":0,"h0":2,"mh0":10,"p0":1,"mp0":6},"rng":[0.5,0.5]},
{"args":{"trap":6,"level":1,"opener":0,"size":1,"packId":0,"h0":0,"mh0":0,"p0":0,"mp0":0},"rng":[0.999]},
{"args":{"trap":6,"level":3,"opener":0,"size":1,"packId":0,"h0":-4,"mh0":20,"p0":9,"mp0":9},"rng":[0.0,0.9999]},
{"args":{"trap":6,"level":7,"opener":1,"size":2,"packId":0,"h0":5,"mh0":5,"p0":30,"mp0":30,"h1":9,"mh1":9,"p1":12,"mp1":12},"rng":[0.6]},
{"args":{"trap":1,"level":2,"opener":0,"size":1,"packId":0,"h0":1,"mh0":1,"p0":1,"mp0":1},"rng":[0.0]}
]
=== END FILE ===

=== FILE: Case.cs ===
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    static int I32(JsonElement a, string k) =>
        a.TryGetProperty(k, out var v) ? v.GetInt32() : 0;

    public static string Run(JsonElement a, Random rng)
    {
        int trap = I32(a, "trap");
        int level = I32(a, "level");
        int openerIdx = I32(a, "opener");
        int size = I32(a, "size");
        int packId = I32(a, "packId");

        var party = new List<Character>();
        for (int i = 0; i < size; i++)
        {
            var c = Character.Create("M" + i, RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
            c.MaxHits = I32(a, "mh" + i);
            c.CurHits = I32(a, "h" + i);
            c.MaxSpellPoints = I32(a, "mp" + i);
            c.CurSpellPoints = I32(a, "p" + i);
            party.Add(c);
        }

        var opener = party[openerIdx];
        if (packId > 0)
        {
            for (int s = 1; s <= ChestTraps.PackSlots - 1; s++)
                opener.Pack[s] = H.ItemsById[(short)packId];
        }

        Func<short, Item?> item = id => H.ItemsById.ContainsKey(id) ? H.ItemsById[id] : null;

        // Return value is not part of the canonical form: the C++ void copy has none.
        _ = ChestTraps.Spring(trap, party, opener, level, rng, item, (x, y) => 0);

        var parts = new List<string>();
        foreach (var c in party)
        {
            parts.Add(c.CurHits.ToString());
            parts.Add(c.CurSpellPoints.ToString());
        }
        parts.Add(opener.Pack.Count(x => x != null && x.ItemId > 0).ToString());
        parts.Add(H.F(rng.NextDouble()));
        return string.Join("|", parts);
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/combat2a.h"
#include "mordor/loot1a.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"
#include "mordor/ctx.h"

namespace mordor {

// The two copies share a name; the harness renames each source file's function.
ChestTrapResult resolveChestTrap__loot1a(Ctx& ctx, Party& party, i32 openerIndex, ChestTrap trap);
void resolveChestTrap__combat2a(Ctx& ctx, Party& party, ChestTrap trap);

static i32 num(const JsonValue& a, const char* k) {
    return static_cast<i32>(A(a, k).asInt());
}

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

static void build(const JsonValue& a, Party& party, i32& opener) {
    static const char* hK[] = {"h0", "h1", "h2", "h3"};
    static const char* mhK[] = {"mh0", "mh1", "mh2", "mh3"};
    static const char* pK[] = {"p0", "p1", "p2", "p3"};
    static const char* mpK[] = {"mp0", "mp1", "mp2", "mp3"};

    const i32 size = num(a, "size");
    const i32 level = num(a, "level");
    opener = num(a, "opener");
    party.size = size;
    party.opener = opener;
    party.dungeonLevel = level;

    for (i32 i = 0; i < size; ++i) {
        Character& m = party.members[static_cast<u32>(i)];
        m.location.dungeonLevel = level;
        m.vitals.maxHits = num(a, mhK[i]);
        m.vitals.curHits = num(a, hK[i]);
        m.vitals.maxSpellPoints = num(a, mpK[i]);
        m.vitals.curSpellPoints = num(a, pK[i]);
    }

    const i32 packId = num(a, "packId");
    if (packId > 0) {
        for (i32 s = 1; s <= kPackSlots - 1; ++s) {
            Item it{};
            it.id = static_cast<decltype(it.id)>(packId);
            party.inventories[static_cast<u32>(opener)].pack[static_cast<u32>(s)] = it;
        }
    }
}

static Str canon(Party& party, Ctx& ctx, i32 opener) {
    Str out;
    for (i32 i = 0; i < party.size; ++i) {
        const Character& m = party.members[static_cast<u32>(i)];
        out += I(m.vitals.curHits) + "|" + I(m.vitals.curSpellPoints) + "|";
    }
    i32 n = 0;
    const auto& pack = party.inventories[static_cast<u32>(opener)].pack;
    for (i32 s = 0; s < kPackSlots; ++s) {
        if (pack[static_cast<u32>(s)].id > 0) ++n;
    }
    out += I(n) + "|" + F(ctx.rng.nextDouble());
    return out;
}

static Str run_loot1a(const JsonValue& a, Ctx& ctx) {
    Party party{};
    i32 opener = 0;
    build(a, party, opener);
    (void)resolveChestTrap__loot1a(ctx, party, opener, trapOf(num(a, "trap")));
    return canon(party, ctx, opener);
}

static Str run_combat2a(const JsonValue& a, Ctx& ctx) {
    Party party{};
    i32 opener = 0;
    build(a, party, opener);
    resolveChestTrap__combat2a(ctx, party, trapOf(num(a, "trap")));
    return canon(party, ctx, opener);
}

const CopyEntry kCopies[] = {
    {"resolveChestTrap@loot1a", &run_loot1a},
    {"resolveChestTrap@combat2a", &run_combat2a},
};
const std::size_t kNumCopies = 2;

} // namespace mordor
=== END FILE ===

Assumptions to verify before running:
- **Reference state is partial.** Poison and disease flags aren't readable through the C++ API shown (the `ailments` fields aren't in the excerpt), so they're left out of the canonical string on every side. The trailing next-draw value does reveal RNG consumption, so trap 1 and 2 are checked through draw counts only. The loot1a `ChestTrapResult` return and fire damage are also not in the canonical string, because the combat2a copy is void and the output must have the same shape on both sides.
- **Shared pack and table access.** The C# side assumes `Pack` is an assignable array, `H.ItemsById` is a dictionary keyed by item id, and `ChestTraps.PackSlots` is public. The C++ side assumes `party.inventories[i].pack[s].id` and `party.opener` exist, as the copies use them, though the excerpt doesn't show them.
- **Item id 1 must be a real shareware record.** Cases 16 to 21 fill the whole pack with it. I haven't checked that it's destructible (not in special type 500..511), so cases 16 to 21 cover the destroy branch only if it is. No guild-item case is included, because I don't know a guild quest item's id.
- **Traps 7 to 9 are not covered.** The excerpts of both copies cut off before them, and the reference is cut off too.

REFERENCE: ChestTraps.Spring