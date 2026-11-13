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

// R35: ability chance for one membership. The caller takes the best across guilds.
// ASSUMED: "effective" stats are character.abilities.stats (statMods not added).
// ASSUMED: the result is converted to i32 with round-half-even, matching the VB
//          Integer assignment noted in R27/R31.
i32 abilityChance(const GuildMembership& membership, i32 ability, const Character& character,
                  i32 dungeonLevel, i32 monsterGuildLevel) {
    const f64 rating = static_cast<f64>(membership.abilityRating[static_cast<u32>(ability)]);
    const f64 lvl = static_cast<f64>(membership.level);
    const f64 dl = static_cast<f64>(dungeonLevel);
    const f64 wis = static_cast<f64>(character.abilities.stats[static_cast<u32>(StatIndex::Wis)]);
    const f64 dex = static_cast<f64>(character.abilities.stats[static_cast<u32>(StatIndex::Dex)]);

    f64 r = 0.0;
    if (ability == kAbilityThieving) {
        // R35 Thieving (0): monster guild level is floored at kMonsterGuildLevelFloor (2).
        const i32 mgl = monsterGuildLevel < kMonsterGuildLevelFloor
                            ? kMonsterGuildLevelFloor
                            : monsterGuildLevel;
        const f64 stat = wis / 4.0 + dex / 2.0;
        const f64 against = (std::log(static_cast<f64>(mgl)) * 4.0)
                            * (std::log(dl + 1.0) * 1.25) * 1.25;
        r = (stat + (4.5 + std::exp(std::log(lvl + 1.0) / std::log(15.0)))
                        * (rating / 10.0 * 14.0))
            - against * 1.5;
        if (dl > 0.0) {  // R35: clamp to 5..98 only when DL > 0
            const f64 lo = static_cast<f64>(kThievingMin);
            const f64 hi = static_cast<f64>(kThievingMax);
            if (r < lo) r = lo;
            if (r > hi) r = hi;
        }
    } else if (ability == kAbilityFighting) {
        // R35 Fighting (4); same formula as R36 with the FightingBase term.
        r = kFightingBase
            + ((std::log(100.0 + lvl) / 1.75 - 2.3) * (rating * rating)) / 2.0;
    } else if (ability == kAbilityPerception) {
        // R35 Perception (5): r = (rating+climb)*(rating*0.3) + wis; clamp(r - DL*2, 5, 90).
        const f64 climb = std::log(lvl) / std::log(1.25);
        r = (rating + climb) * (rating * 0.3) + wis;
        r -= dl * 2.0;
        const f64 lo = static_cast<f64>(kPerceptionMin);
        const f64 hi = static_cast<f64>(kPerceptionMax);
        if (r < lo) r = lo;
        if (r > hi) r = hi;
    } else {
        // R35 other abilities: r = rating * lvl.
        r = rating * lvl;
    }
    return static_cast<i32>(roundHalfEven(r));
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
