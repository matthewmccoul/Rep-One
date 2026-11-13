#include <cstddef>
#include <string>

#include "mordor/access.h"
#include "mordor/features.h"
#include "mordor/guilds1.h"
#include "mordor/guilds2.h"
#include "mordor/loot2.h"
#include "mordor/names.h"

namespace mordor {
namespace {

// Guild names in table order. The order matches the Guildmasters.Seeded order in the reference.
// NEED: GuildDef name field. When it exists, replace this table with GuildDef.name.

Str questsGuildName(Guild guild) {
  const i32 idx = static_cast<i32>(guild);
  if (idx < 0 || idx >= kGuildCount) return Str();
  return Str(guildNameOrNull(idx));
}

bool questsGuildFromWord(const Str& word, Guild& guild) {
  const Str want = accLower(word);
  for (i32 i = 0; i < kGuildCount; ++i) {
    if (accLower(Str(guildNameOrNull(i))) == want) {
      guild = static_cast<Guild>(i);
      return true;
    }
  }
  return false;
}

// The guild the verbs act on: the active guild when joined, otherwise the first joined guild.
// UNKNOWN: the verbs have no guild argument for quest, claim and titles, so a member in several
// guilds is always resolved this way.
GuildMembership* questsMembershipOf(Character& ch, Guild& guild) {
  const i32 active = static_cast<i32>(ch.activeGuild);
  if (active >= 0 && active < kGuildCount && ch.guilds[static_cast<size_t>(active)].joined) {
    guild = static_cast<Guild>(active);
    return &ch.guilds[static_cast<size_t>(active)];
  }
  for (i32 i = 0; i < kGuildCount; ++i) {
    GuildMembership& m = ch.guilds[static_cast<size_t>(i)];
    if (m.joined) {
      guild = static_cast<Guild>(i);
      return &m;
    }
  }
  return nullptr;
}

// Open-quest flag per (member, guild). It lives in featureState because the quest record on the
// membership cannot be read without the GuildQuest fields.
// NEED: GuildQuest fields (type, target, done) to replace this flag and to show and claim quests.
Str questsKey(i32 member1, Guild guild) {
  return "quest." + std::to_string(member1) + "." + std::to_string(static_cast<int>(guild));
}

bool questsHasOpen(const Session& s, i32 member1, Guild guild) {
  const auto it = s.featureState.find(questsKey(member1, guild));
  return it != s.featureState.end() && it->second == "1";
}

void questsSetOpen(Session& s, i32 member1, Guild guild, bool open) {
  if (open) {
    s.featureState[questsKey(member1, guild)] = "1";
  } else {
    s.featureState.erase(questsKey(member1, guild));
  }
}

// ---- verbs ----

bool cmdQuest(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() < 2) {
    reply = "Usage: quest <member>";
    return true;
  }
  const i32 mi = accMemberByName(s, w[1]);
  if (mi == 0) {
    reply = "No member called " + w[1] + ".";
    return true;
  }
  Character& ch = *accMember(s, mi);
  Guild guild = Guild::Nomad;
  GuildMembership* m = questsMembershipOf(ch, guild);
  if (m == nullptr) {
    reply = w[1] + " is not in a guild.";
    return true;
  }
  if (questsHasOpen(s, mi, guild)) {
    // UNKNOWN: the reference shows the quest text here; it needs the GuildQuest fields.
    reply = "You still have your guild quest to complete.";
    return true;
  }
  // R46: the tier is applied inside assignGuildQuest, which writes the quest onto the membership.
  Ctx ctx = s.ctx();
  assignGuildQuest(ctx, s.data, *m, static_cast<i32>(m->level));
  questsSetOpen(s, mi, guild, true);
  reply = "You have also been assigned a Quest to complete.";
  return true;
}

bool cmdClaim(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() < 2) {
    reply = "Usage: claim <member>";
    return true;
  }
  const i32 mi = accMemberByName(s, w[1]);
  if (mi == 0) {
    reply = "No member called " + w[1] + ".";
    return true;
  }
  Character& ch = *accMember(s, mi);
  Guild guild = Guild::Nomad;
  GuildMembership* m = questsMembershipOf(ch, guild);
  if (m == nullptr) {
    reply = w[1] + " is not in a guild.";
    return true;
  }
  // NEED: GuildQuest fields (type, target, done). Without them a completed quest cannot be
  // detected, so only the guildmastership is claimed here.
  Ctx ctx = s.ctx();
  // FRMCTYG.BAS:434: the seat passes when nobody holds it or the holder's level is below yours.
  // UNKNOWN: true is read as "you now hold the seat"; the refusal wording is invented.
  if (claimMastership(ctx, s.game, ch, guild, static_cast<i32>(m->level))) {
    reply = "You are the GuildMaster.";
  } else {
    reply = "You cannot take the guildmastership of " + questsGuildName(guild) + " yet.";
  }
  return true;
}

bool cmdForfeit(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() < 3) {
    reply = "Usage: forfeit <member> <guild>";
    return true;
  }
  const i32 mi = accMemberByName(s, w[1]);
  if (mi == 0) {
    reply = "No member called " + w[1] + ".";
    return true;
  }
  Character& ch = *accMember(s, mi);
  Guild guild = Guild::Nomad;
  if (!questsGuildFromWord(w[2], guild)) {
    reply = "No guild called " + w[2] + ".";
    return true;
  }
  // R19: HP loss on forfeit is handled inside Forfeit (not verified here).
  Ctx ctx = s.ctx();
  if (!Forfeit(ctx, ch, guild)) {
    reply = w[1] + " cannot forfeit " + questsGuildName(guild) + ".";
    return true;
  }
  questsSetOpen(s, mi, guild, false);
  reply = w[1] + " has left " + questsGuildName(guild) + ".";
  return true;
}

bool cmdTitles(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() < 2) {
    reply = "Usage: titles <member>";
    return true;
  }
  const i32 mi = accMemberByName(s, w[1]);
  if (mi == 0) {
    reply = "No member called " + w[1] + ".";
    return true;
  }
  Character& ch = *accMember(s, mi);
  Guild guild = Guild::Nomad;
  GuildMembership* m = questsMembershipOf(ch, guild);
  if (m == nullptr) {
    reply = w[1] + " is not in a guild.";
    return true;
  }
  // Reference shows "Vagrant (1)": title, then the level in brackets (fn12C4 via GuildTitle).
  reply = GuildTitle(*m, guild) + " (" + std::to_string(static_cast<int>(m->level)) + ")";
  return true;
}

}  // namespace

void register_quests() {
  registerFeatureCommand({"quest", Screen::Town, &cmdQuest, "quest <member>  - show or assign your guild quest"});
  registerFeatureCommand({"claim", Screen::Town, &cmdClaim, "claim <member>  - claim a completed quest or the guildmastership"});
  registerFeatureCommand({"forfeit", Screen::Town, &cmdForfeit, "forfeit <member> <guild>  - leave a guild"});
  registerFeatureCommand({"titles", Screen::Town, &cmdTitles, "titles <member>  - show your guild title"});
}

}  // namespace mordor
