#include <type_traits>

#include "test_support.h"
#include "mordor/casting1.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"

using namespace mordor;

// FIXED: REQUIRE is not declared by any included header (test_support.h does not provide it).
// Local fallback, used only when test_support.h does not define REQUIRE. It records the
// failure through CHECK_EQ, which the compiler accepted, then stops the test without exceptions.
#ifndef REQUIRE
#define REQUIRE(cond)                 \
  do {                                \
    if (!(cond)) {                    \
      CHECK_EQ((cond), true);         \
      return;                         \
    }                                 \
  } while (0)
#endif

// FIXED: `Spell` is not declared in the API INDEX or the state headers. The spell
// element type is deduced from the same container the lookup scans, so no type is invented.
// ASSUMPTION: tsup::data().spells is a container of spell records with a `name` member.
using SpellT = std::remove_cv_t<std::remove_reference_t<decltype(*tsup::data().spells.begin())>>;

// FIXED: `mordor::spellbookCost` is not declared in the API INDEX or the included headers.
// Declared static in namespace mordor per the fix rules, so the qualified calls below resolve.
// NEED: the signature (i32 return, parameter types) is inferred from the call sites, and
// there is no definition in this TU. If it is defined elsewhere, the declaration must come
// from the header that defines it, and this static declaration must be removed.
namespace mordor {
static i32 spellbookCost(const SpellT& spell, const Character& c) {
  // The reference Spellbook.Cost is our castCostSp (the spell-point cost the caster pays).
  Rng rng(1);
  EventLog log;
  const Ctx ctx(tsup::data(), rng, log);
  return castCostSp(ctx, c, spell, 0);
}
}

// FIXED: `REQUIRE` and `CHECK_EQ` are not defined by the included headers as far as the index shows.
// They are kept as written; test_support.h must provide them.
// NEED: test_support.h (or the header that defines these macros) must be included.
// FIXED: REQUIRE was reported undeclared at every use site; it is not defined by any
// included header in the index, so it must come from test_support.h (already included above).
// NEED: if test_support.h does not define REQUIRE, the macro must be added there.

// Assumed helpers (see NEED). Spell lookup by name against the shipped data.
static const SpellT* spellNamed(const char* name) {
  for (const auto& s : tsup::data().spells) {  // ASSUMPTION: GameData::spells
    if (s.name == name) return &s;             // ASSUMPTION: spell record::name
  }
  return nullptr;
}

// FIXED: Guild membership is stored as Arr<GuildMembership, kGuildCount> indexed by Guild.
static void setGuild(Character& c, Guild g, i32 level) {
  GuildMembership& m = c.guilds[static_cast<u32>(g)];
  m.joined = true;
  m.level = level;
}

// FIXED: Stats live in Abilities::stats indexed by StatIndex.
static void setStat(Character& c, StatIndex s, i32 value) {
  c.abilities.stats[static_cast<u32>(s)] = value;
}

// Human Nomad 37 and Sorcerer 37 (spell level 19 in both guilds).
static Character nomadSorcerer37() {
  Character c;
  c.race = Race::Human;                        // FIXED: Race is an enum, not a string.
  setGuild(c, Guild::Nomad, 37);               // FIXED: was c.guilds = {{"Nomad", 37}, ...}
  setGuild(c, Guild::Sorcerer, 37);
  return c;
}

// B_0061#63..#85: Human Nomad 37 and Sorcerer 37, spell costs.
TEST(engine_re_spellcost_charm_of_opening) {
  auto* s = spellNamed("Charm of Opening");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 143);
}

TEST(engine_re_spellcost_cold_blast) {
  auto* s = spellNamed("Cold Blast");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 2);
}

TEST(engine_re_spellcost_sight_veil) {
  auto* s = spellNamed("Sight Veil");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 2);
}

TEST(engine_re_spellcost_see_invisible) {
  auto* s = spellNamed("See Invisible");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 2);
}

TEST(engine_re_spellcost_mass_sight_veil) {
  auto* s = spellNamed("Mass Sight Veil");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 35);
}

TEST(engine_re_spellcost_sphere_of_flames) {
  auto* s = spellNamed("Sphere of Flames");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 74);
}

TEST(engine_re_spellcost_flesh_to_stone) {
  auto* s = spellNamed("Flesh to Stone");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 95);
}

TEST(engine_re_spellcost_paralyzing_death) {
  auto* s = spellNamed("Paralyzing Death");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 89);
}

TEST(engine_re_spellcost_poison) {
  auto* s = spellNamed("Poison");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 2);
}

TEST(engine_re_spellcost_leprosy) {
  auto* s = spellNamed("Leprosy");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 9);
}

TEST(engine_re_spellcost_acidic_spray) {
  auto* s = spellNamed("Acidic Spray");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 23);
}

TEST(engine_re_spellcost_ice_spray) {
  auto* s = spellNamed("Ice Spray");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 15);
}

TEST(engine_re_spellcost_hail_storm) {
  auto* s = spellNamed("Hail Storm");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 52);
}

TEST(engine_re_spellcost_arctic_storm) {
  auto* s = spellNamed("Arctic Storm");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 119);
}

TEST(engine_re_spellcost_firebolt_nomad_sorcerer37) {
  auto* s = spellNamed("Firebolt");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 1);
}

TEST(engine_re_spellcost_blue_flame) {
  auto* s = spellNamed("Blue Flame");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 3);
}

TEST(engine_re_spellcost_flamesheet) {
  auto* s = spellNamed("Flamesheet");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 10);
}

TEST(engine_re_spellcost_pillar_of_fire) {
  auto* s = spellNamed("Pillar of Fire");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 31);
}

TEST(engine_re_spellcost_resist_fire) {
  auto* s = spellNamed("Resist Fire");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 4);
}

TEST(engine_re_spellcost_resist_draining) {
  auto* s = spellNamed("Resist Draining");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 4);
}

TEST(engine_re_spellcost_resist_cold) {
  auto* s = spellNamed("Resist Cold");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 4);
}

TEST(engine_re_spellcost_resist_poison) {
  auto* s = spellNamed("Resist Poison");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 7);
}

TEST(engine_re_spellcost_resist_disease) {
  auto* s = spellNamed("Resist Disease");
  REQUIRE(s != nullptr);
  auto c = nomadSorcerer37();
  CHECK_EQ(mordor::spellbookCost(*s, c), 7);
}

// B_0061#61, #62: new Human Sorcerer (Int 18, Wis 16, Sorcerer level 1).
static Character newSorcerer1() {
  Character c;
  c.race = Race::Human;                        // FIXED: Race is an enum.
  setStat(c, StatIndex::Int, 18);              // FIXED: was c.intelligence (no such field).
  setStat(c, StatIndex::Wis, 16);              // FIXED: was c.wisdom (no such field).
  setGuild(c, Guild::Sorcerer, 1);             // FIXED: was c.guilds = {{"Sorcerer", 1}}.
  return c;
}

TEST(engine_re_spellcost_shock_new_sorcerer) {
  auto* s = spellNamed("Shock");
  REQUIRE(s != nullptr);
  auto c = newSorcerer1();
  CHECK_EQ(mordor::spellbookCost(*s, c), 18);
}

TEST(engine_re_spellcost_firebolt_new_sorcerer) {
  auto* s = spellNamed("Firebolt");
  REQUIRE(s != nullptr);
  auto c = newSorcerer1();
  CHECK_EQ(mordor::spellbookCost(*s, c), 26);
}

// B_0061#57: Harm (base level 6, spell point ratio 40); Wizard 40 and Healer 28.
// Expected 32 is taken from the cheapest guild.
TEST(engine_re_spellcost_harm_cheapest_guild) {
  auto* s = spellNamed("Harm");
  REQUIRE(s != nullptr);
  Character c;
  setGuild(c, Guild::Wizard, 40);              // FIXED: was c.guilds = {{"Wizard", 40}, ...}
  setGuild(c, Guild::Healer, 28);
  CHECK_EQ(mordor::spellbookCost(*s, c), 32);
}

// SKIP B_0066#25: expected value is relational ("name|Spellbook.Cost of the first
// positive-cost spell"), not a literal; it needs a scan of MDATA2 for that spell
// and the not-yet-declared SpellBuffers Describe/CostText. Pending NEED above.
