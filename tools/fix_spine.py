"""Brain integration of the ant-written state headers: shared enums/constants into state_common.h, removal of duplicates, party pruning."""
import re, os
R = "/tmp/claude-0/-home-user-Rep-One/17a752bb-9e88-5884-a5bf-3b7fc4ab8c87/scratchpad/mordor/code/include/mordor/"
def rd(n): return open(R + n).read()
def wr(n, t): open(R + n, "w").write(t)
def drop_block(t, header_regex):
    m = re.search(header_regex + r".*?\n\};\n", t, re.S)
    return t[:m.start()] + t[m.end():] if m else t
def add_include(t, inc):
    if inc in t: return t
    return re.sub(r'(#include "mordor/types.h"\n)', r'\1' + inc + "\n", t, count=1)
wr("state_common.h", """// Shared constants and enums used by several state headers (single definition site).
#pragma once
#include "mordor/types.h"

namespace mordor {

inline constexpr i32 kGuildCount = 12;       // guilds in table order
inline constexpr i32 kPackSlots = 40;        // pack array 0..39; index 0 is Hands
inline constexpr i32 kCompanionSlots = 4;    // companion slots 0..3 per character
inline constexpr i32 kPartyMaxSize = 4;      // party maximum

// Death states (SRD ch.18 Table 1): 0 = alive or ordinary death handled by flags, 1..8 named states.
enum class DeathState : i32 { Alive = 0, Dead = 1, Stoned = 2, Destroyed = 3, Immolated = 4, Frozen = 5, Annihilated = 6, Melted = 7, Rock = 8 };

// How well an item is identified (SRD ch.13).
enum IdLevel : i32 { Unknown = 0, Barely = 1, Partial = 2, Mostly = 3, All = 4 };

// Chest trap kinds 0..10 (SRD ch.7 and ch.14): 1-4 hit every member, 5-10 only the opener; 3 and 4 do damage.
enum class ChestTrap : i32 { NotTrapped = 0, PoisonGas = 1, DiseaseSpores = 2, Explosion = 3, Fire = 4, ItemDestruction = 5, Fate = 6, Trap7 = 7, Blackout = 8, Trap9 = 9, Trap10 = 10 };

}  // namespace mordor
""")
t = rd("types.h"); t = t.replace("using u64 = std::uint64_t;", "using u64 = std::uint64_t;\nusing f32 = float;\nusing f64 = double;"); wr("types.h", t)
# item
t = rd("state_item.h"); t = drop_block(t, r"enum IdLevel : i32 \{"); t = drop_block(t, r"enum ChestTrap : i32 \{"); t = add_include(t, '#include "mordor/state_common.h"')
t = t.replace("ChestTrap chestTrap = NotTrapped;", "ChestTrap chestTrap = ChestTrap::NotTrapped;"); wr("state_item.h", t)
# dungeon
t = rd("state_dungeon.h"); t = drop_block(t, r"enum class ChestTrap : i32 \{"); t = add_include(t, '#include "mordor/state_common.h"')
t = t.replace("ChestTrap::NOT_TRAPPED", "ChestTrap::NotTrapped"); wr("state_dungeon.h", t)
# character
t = rd("state_character.h"); t = drop_block(t, r"enum class DeathState : i32 \{")
for c in ("kGuildCount", "kPackSlots", "kCompanionSlots"): t = re.sub(r"inline constexpr i32 " + c + r" = \d+;[^\n]*\n", "", t)
t = add_include(t, '#include "mordor/state_common.h"'); wr("state_character.h", t)
# companion
t = rd("state_companion.h")
t = t.replace("enum class Alignment : i32", "enum class CompanionAlignment : i32").replace("Alignment alignment = Alignment::Good", "CompanionAlignment alignment = CompanionAlignment::Good")
t = t.replace("enum class IdLevel : i32", "enum class OwnerIdLevel : i32").replace("IdLevel::", "OwnerIdLevel::").replace("OwnerOwnerIdLevel", "OwnerIdLevel")
t = t.replace("enum class DeathState : i32", "enum class CompanionDeathState : i32")
t = re.sub(r"\bDeathState\b(?!\w)", "CompanionDeathState", t).replace("CompanionCompanionDeathState", "CompanionDeathState")
t = re.sub(r"(?<![A-Za-z])IdLevel (\w+)", r"OwnerIdLevel \1", t)
wr("state_companion.h", t)
# game
t = rd("state_game.h"); t = re.sub(r"constexpr i32 kGuildCount = 12;[^\n]*\n", "", t)
t = t.replace("enum class Alignment {", "enum class StoreAlignment {").replace("Alignment storeAlignChoice = Alignment::Unaligned;", "StoreAlignment storeAlignChoice = StoreAlignment::Unaligned;")
t = add_include(t, '#include "mordor/state_common.h"'); wr("state_game.h", t)
# party
t = rd("state_party.h")
for pat in [r"\n[^\n]*Arr<Companion, kCompanionSlots> companions[^\n]*", r"\n[^\n]*freeCompanionSlots[^\n]*", r"\n[^\n]*Currency gold = 0;[^\n]*(?:\n\s+//[^\n]*)*", r"\n[^\n]*Currency bank = 0;[^\n]*(?:\n\s+//[^\n]*)*",
            r"\n[^\n]*topGuildLevel[^\n]*", r"\n[^\n]*Arr<Slot, kPackSlots> pack[^\n]*", r"\n[^\n]*Vec<Slot> vault[^\n]*", r"\n[^\n]*Vec<i32> library[^\n]*",
            r"\nconstexpr i32 kPartyMaxSize[^\n]*", r"\nconstexpr i32 kCompanionSlots[^\n]*", r"\nconstexpr i32 kPackSlots[^\n]*",
            r"// UNKNOWN: Companion, CombatOrder, ItemOrder, Creature and Slot are assumed[^\n]*\n// state_character.h or a shared header[^\n]*\n"]:
    t = re.sub(pat, "", t)
t = t.replace('#include "mordor/state_character.h"', '#include "mordor/state_character.h"\n#include "mordor/state_creature.h"')
t = t.replace("struct Party {", "struct CombatOrder { i32 order = 0; i32 targetGroup = -1; };            // per-character standing order (see OrderKind) and target group index\nstruct ItemOrder { i32 slot = -1; i32 item = -1; bool cast = false; };  // per-member item order\n\nstruct Party {", 1)
wr("state_party.h", t)
print("spine fixed")
