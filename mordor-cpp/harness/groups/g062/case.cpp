#include "case_api.h"
#include "mordor/state_character.h"

namespace mordor {

// Same-name copy from src/attributes.cpp, renamed by the harness to createCharacter__attributes.
void createCharacter__attributes(Ctx& ctx, Character& c, Race race, Guild startGuild,
                                 const Arr<i32, kStatCount>& stats);

static Str joinBar(const Vec<Str>& p) {
    Str out;
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (i != 0) out += "|";
        out += p[i];
    }
    return out;
}

static Str run_attributes(const JsonValue& a, Ctx& ctx) {
    const i32 raceIdx = static_cast<i32>(A(a, "race").asInt());

    // Stats in STR, INT, WIS, CON, CHA, DEX order (same as the reference).
    Arr<i32, kStatCount> st{};
    st[0] = static_cast<i32>(A(a, "STR").asInt());
    st[1] = static_cast<i32>(A(a, "INT").asInt());
    st[2] = static_cast<i32>(A(a, "WIS").asInt());
    st[3] = static_cast<i32>(A(a, "CON").asInt());
    st[4] = static_cast<i32>(A(a, "CHA").asInt());
    st[5] = static_cast<i32>(A(a, "DEX").asInt());

    Character c;
    createCharacter__attributes(ctx, c, static_cast<Race>(raceIdx), Guild::Nomad, st);

    Vec<Str> p;
    p.push_back(I(static_cast<long long>(c.race)));
    for (u32 i = 0; i < static_cast<u32>(kStatCount); ++i) p.push_back(I(c.abilities.stats[i]));
    for (u32 i = 0; i < static_cast<u32>(kStatCount); ++i) p.push_back(I(c.abilities.statMax[i]));
    p.push_back(I(c.vitals.maxHits));
    p.push_back(I(c.vitals.curHits));
    // Reference stores whole gold; C++ stores 1/10000 gold.
    p.push_back(F(static_cast<double>(c.wallet.goldOnHand) / 10000.0));
    p.push_back(I(c.location.area));

    i32 joined = 0;
    for (u32 i = 0; i < static_cast<u32>(kGuildCount); ++i) {
        if (c.guilds[i].joined) ++joined;
    }
    const GuildMembership& m = c.guilds[static_cast<u32>(Guild::Nomad)];
    p.push_back(I(joined));
    p.push_back(I(m.level));
    p.push_back(I(m.experience));

    return joinBar(p);
}

const CopyEntry kCopies[] = {
    {"createCharacter@attributes", &run_attributes},
};
const std::size_t kNumCopies = 1;

}  // namespace mordor
