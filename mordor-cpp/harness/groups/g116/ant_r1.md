=== FILE: cases.json ===
[
{"args": {"lo": 0, "hi": 0}, "rng": [0.5]},
{"args": {"lo": 1, "hi": 0}, "rng": [0.5]},
{"args": {"lo": 2, "hi": 0}, "rng": [0.5]},
{"args": {"lo": 7, "hi": 0}, "rng": [0.5]},
{"args": {"lo": 524287, "hi": 0}, "rng": [0.5]},
{"args": {"lo": 1048575, "hi": 0}, "rng": [0.5]},
{"args": {"lo": 1048575, "hi": 524287}, "rng": [0.5]},
{"args": {"lo": 1048575, "hi": 1048575}, "rng": [0.5]},
{"args": {"lo": 1048575, "hi": 262143}, "rng": [0.5]},
{"args": {"lo": 0, "hi": 524288}, "rng": [0.5]},
{"args": {"lo": 1, "hi": 524288}, "rng": [0.5]},
{"args": {"lo": 524288, "hi": 0}, "rng": [0.5]},
{"args": {"lo": 0, "hi": 1}, "rng": [0.5]},
{"args": {"lo": 524287, "hi": 1048575}, "rng": [0.5]},
{"args": {"lo": 1048574, "hi": 1048575}, "rng": [0.5]},
{"args": {"lo": 349525, "hi": 349525}, "rng": [0.5]},
{"args": {"lo": 1048575, "hi": 1023}, "rng": [0.5]},
{"args": {"lo": 1048575, "hi": 1047551}, "rng": [0.5]},
{"args": {"lo": 1020, "hi": 0}, "rng": [0.5]},
{"args": {"lo": 64, "hi": 0}, "rng": [0.5]},
{"args": {"lo": 63, "hi": 0}, "rng": [0.5]},
{"args": {"lo": 1015808, "hi": 1048575}, "rng": [0.5]},
{"args": {"lo": 1015807, "hi": 1048575}, "rng": [0.5]},
{"args": {"lo": 0, "hi": 262144}, "rng": [0.5]},
{"args": {"lo": 0, "hi": 393216}, "rng": [0.5]},
{"args": {"lo": 3, "hi": 786432}, "rng": [0.5]},
{"args": {"lo": 1048575, "hi": 917503}, "rng": [0.5]},
{"args": {"lo": 240, "hi": 0}, "rng": [0.5]},
{"args": {"lo": 1, "hi": 0}, "rng": [0.0]},
{"args": {"lo": 1048575, "hi": 1048575}, "rng": [0.99]},
{"args": {"lo": 0, "hi": 0}, "rng": [0.3, 0.7]}
]
=== END FILE ===
=== FILE: Case.cs ===
using System;
using System.Text.Json;
using System.Globalization;
using Mordor.Core;

public static class Case
{
    // Pack occupancy is given as two bitmasks: bit k of "lo" = pack slot k+1 (slots 1..20),
    // bit k of "hi" = pack slot k+21 (slots 21..40). Slot 0 (Hands) is never stored.
    // FirstFreeSlot consumes no random draws, so rng is unused.
    public static string Run(JsonElement a, System.Random rng)
    {
        var c = Character.Create("Test", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);

        long lo = a.GetProperty("lo").GetInt64();
        long hi = a.GetProperty("hi").GetInt64();
        for (int k = 0; k < 20; k++)
        {
            if (((lo >> k) & 1) != 0) c.PutInSlot(k + 1, new InventorySlot { ItemId = 1 });
            if (((hi >> k) & 1) != 0) c.PutInSlot(k + 21, new InventorySlot { ItemId = 1 });
        }

        int r = c.FirstFreeSlot();
        return r.ToString(CultureInfo.InvariantCulture);
    }
}
=== END FILE ===
=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/state_character.h"

namespace mordor {

i32 exp_freeStorageSlot_items2b(const Inventory& inv);
i32 exp_freeSlot_items2b_2(const Inventory& inv);

// Pack occupancy: bit k of "lo" = pack slot k+1 (1..20), bit k of "hi" = pack slot k+21 (21..40).
static Inventory buildInv(const JsonValue& a) {
    Inventory inv{};
    long long lo = A(a, "lo").asInt();
    long long hi = A(a, "hi").asInt();
    for (int k = 0; k < 20; ++k) {
        if ((lo >> k) & 1) inv.pack[k + 1].id = 1;
        if ((hi >> k) & 1) inv.pack[k + 21].id = 1;
    }
    return inv;
}

static Str run_freeStorageSlot(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    Inventory inv = buildInv(a);
    return I(exp_freeStorageSlot_items2b(inv));
}

static Str run_freeSlot(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    Inventory inv = buildInv(a);
    return I(exp_freeSlot_items2b_2(inv));
}

const CopyEntry kCopies[] = {
    {"freeStorageSlot", &run_freeStorageSlot},
    {"freeSlot", &run_freeSlot},
};
const std::size_t kNumCopies = 2;

} // namespace mordor
=== END FILE ===
=== FILE: shim_items2b.cpp ===
namespace mordor {
i32 exp_freeStorageSlot_items2b(const Inventory& inv) { return freeStorageSlot(inv); }
}
=== END FILE ===
=== FILE: shim_items2b_2.cpp ===
namespace mordor {
i32 exp_freeSlot_items2b_2(const Inventory& inv) { return freeSlot(inv); }
}
=== END FILE ===

Assumptions to check when the harness runs:
- `Character.PackSize` is not in the outline. The test assumes it is 41, so `FirstFreeSlot` scans slots 1..40, matching `kLastPackIndex` = 40.
- Occupied slots are filled with `ItemId = 1` (C#) and `inv.pack[i].id = 1` (C++). This assumes item id 1 exists in the shareware tables and that `Item` has an `id` field.
- Pack slot 0 (Hands) is not written by any case, because the reference's behaviour for slot 0 is not shown.

REFERENCE: Character.FirstFreeSlot