// casting1_b.cpp : second half of casting1 (monster spells, breath, charms,
// binding, protection, map/utility effects, seer fees, spell-data predicates,
// wiki cost). Implements only the functions listed for this part.
//
// UNKNOWN (global): field names of SpellDef, GuildDef, Item, etc. are taken
// from the SRD / data file spelling (Damage2, SpellEffect, AffectGroup, Level,
// Category, HasSpellTable, SpellMax). Verify against game_data.h.

#include <cmath>

#include "mordor/casting1.h"
#include "mordor/math_util.h"

namespace mordor {

// R62 identified squares are written to the automap. Already-mapped squares say nothing.
// UNKNOWN: "already mapped" tested via automap != 0.
void perceiveSquare(Ctx& ctx, SquareState& square) {
    if (!square.identified) {
        return;
    }
    if (square.automap != 0) {
        return;  // already mapped: say nothing
    }
    square.automap = 1;
    ctx.say("perceive.mapped", "Square mapped.", {}, {});
}

// R67 ceiling = max over guilds with HasSpellTable of SpellMax[category].
// UNKNOWN: GuildDef field names assumed (hasSpellTable, spellMax).
// FIXED: HasSpellTable/SpellMax -> hasSpellTable/spellMax (names in GuildDef).
i32 spellCeiling(const Ctx& ctx, i32 category) {
    i32 ceiling = 0;
    for (const GuildDef& g : ctx.data.guilds) {
        if (g.hasSpellTable && g.spellMax[static_cast<u32>(category)] > ceiling) {
            ceiling = g.spellMax[static_cast<u32>(category)];
        }
    }
    return ceiling;
}

// R67 a spell is unreachable if s.Level > ceiling.
bool spellUnreachable(const Ctx& ctx, const SpellDef& spell) {
    return spell.level > spellCeiling(ctx, spell.category);
}

}  // namespace mordor
