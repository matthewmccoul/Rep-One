// Implementations of guilds1.h functions whose rules are in the SRD part c04p1.
// Functions not listed here are defined elsewhere.

#include <cmath>
#include <string>

#include "mordor/guilds1.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"

namespace mordor {


// R32: hits gained on a level.
// UNKNOWN: the per-guild AH (average hits), ML (hits-cap level) and MH (hits at or
//          after ML) values come from the spelltool sheet (R9), which the SRD does
//          not give. The GuildDef field names below are ASSUMED; verify them against
//          game_data.h.
// FIXED: GuildDef has no member hitsAtCap. MH is read from avgHits, the closest
//        existing field. This is a guess, not the sheet value: verify against game_data.h.
// ASSUMED: "no hits for a level another guild already reached" means the gain is
//          zero unless membership.level > character.topGuildLevel (topGuildLevel is
//          the highest level reached before this level).
// ASSUMED: the 2/3 chance gates only the Int(Rnd*(AH/5)+1) term; the CON bonus
//          (1..CON-16, uniform) applies whenever CON > 16.
// ASSUMED: the 999 cap limits the resulting maximum hits (vitals.maxHits).
// CONFLICT: R33 gives a Nomad-specific rule (dossier B: 5 hits plus Con bonus at or
//           below level 30, 1 hit above). Dossier A (R32) applies the CON bonus to any
//           guild below ML. This function has no guild identity, so the R32 rule is
//           used for all guilds.
i32 hitsGain(Ctx& ctx, const Character& character, const GuildMembership& membership,
             const GuildDef& guildDef, i32 con) {
    if (membership.level <= character.topGuildLevel) {
        return 0;  // R32: no hits for a level another guild already reached.
    }

    i32 gain = 0;
    if (membership.level >= guildDef.hitsCapLevel) {  // UNKNOWN field name (ML)
        gain = guildDef.avgHits;                      // FIXED: was hitsAtCap (MH); see above
    } else {
        const i32 ah = guildDef.avgHits;              // UNKNOWN field name (AH)
        gain = ah;
        if (ctx.rng.nextDouble() < 2.0 / 3.0) {
            // Int(Rnd*(AH/5)+1), with Int = truncation (vbInt).
            gain += static_cast<i32>(vbInt(ctx.rng.nextDouble() * (static_cast<f64>(ah) / 5.0) + 1.0));
        }
        if (con > kHitsConThreshold) {  // R32: CON > 16 gives a bonus
            gain += ctx.rng.below(con - kHitsConThreshold) + 1;  // 1..CON-16
        }
    }

    // R32: maximum hits is 999.
    const i32 room = kMaxHits - character.vitals.maxHits;
    if (room <= 0) {
        return 0;
    }
    if (gain > room) {
        gain = room;
    }
    return gain;
}

// R7: the guildmaster is the highest-level member; holding the seat also holds the crest.
// The claim itself is performed by claimMastership (defined elsewhere); the crest is
// awarded by awardCrest (defined elsewhere) once the claim succeeds.
// UNKNOWN: the exact messages are not in the SRD; the English below is ASSUMED.
void mastershipProcedure(Ctx& ctx, GameState& game, const Character& character,
                         Inventory& inventory, Guild guild) {
    const GuildMembership& membership = character.guilds[static_cast<u32>(guild)];
    if (!membership.joined) {
        return;
    }
    if (!claimMastership(ctx, game, character, guild, membership.level)) {
        return;
    }
    const CrestOutcome outcome = awardCrest(ctx, inventory, guild);
    switch (outcome) {
        case CrestOutcome::Placed:
            ctx.say("guilds1.crest.placed", "The guild crest has been placed in your pack.",
                    {crestId(guild)}, {});
            break;
        case CrestOutcome::NoRoom:
            ctx.say("guilds1.crest.noRoom", "There is no room in your pack for the guild crest.",
                    {crestId(guild)}, {});
            break;
        case CrestOutcome::AlreadyHeld:
            ctx.say("guilds1.crest.held", "You already hold this guild's crest.",
                    {crestId(guild)}, {});
            break;
    }
}



}  // namespace mordor
