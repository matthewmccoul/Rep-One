// guilds2_a.cpp
//
// Implements: SwingsFor, HitsGain, AdGain, MaxAttackDefence, CharacterAdCap,
//             JoinCost, CanJoin, Join, Forfeit, MakeLevel, AdvanceGold,
//             QuestTier, ItemQuestBand.
//
// CAVEAT: the supplied SRD half (c04p2) contains tables and quirk notes only.
// The rule text for AdGain, JoinCost, AdvanceGold, CanJoin, Join, Forfeit and
// MakeLevel is in the other SRD half and was not supplied. Those bodies are
// reconstructions and are marked UNKNOWN below. Replace them with the SRD
// pseudo-code before relying on them.
// FIXED: AdPair member names changed from attack/defence to att/def. The AdPair
// body is truncated in the API INDEX, so verify these names against attributes.h.

#include "mordor/guilds2.h"
#include "mordor/attributes.h"  // FIXED: AdPair is declared in attributes.h
#include "mordor/math_util.h"

namespace mordor {

namespace {

}  // namespace

// UNKNOWN: the supplied half gives only the misspelt refusal text ("Cnanot") [c3]
// and not its trigger. This placeholder refuses when the guild is not joined or is
// Nomad (base guild). On success the membership is reset to defaults, including
// guildGold, and activeGuild falls back to Nomad if it was this guild.
bool Forfeit(Ctx& ctx, Character& ch, Guild guild) {
    const u32 g = static_cast<u32>(guild);
    if (!ch.guilds[g].joined || guild == Guild::Nomad) {
        ctx.say("guild.forfeit.refused", "Cnanot forfeit this guild.", {}, {});
        return false;
    }

    ch.guilds[g] = GuildMembership{};
    if (ch.activeGuild == guild) {
        ch.activeGuild = Guild::Nomad;
    }

    ctx.say("guild.forfeit", "You have forfeited the guild.", {}, {});
    return true;
}

}  // namespace mordor
