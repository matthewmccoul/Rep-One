#include <string>

#include "mordor/features.h"
#include "mordor/casting1.h"
#include "mordor/access.h"
#include "mordor/shared_helpers.h"

namespace mordor {
namespace {

// SpellEffect ids (reference SpellEffect.cs).
inline constexpr i32 kEffDamage = 0;           // also healing: the branch keys off Damage2
inline constexpr i32 kEffCurePoison = 1;
inline constexpr i32 kEffCureDisease = 2;
inline constexpr i32 kEffLevitate = 3;
inline constexpr i32 kEffInvisible = 4;
inline constexpr i32 kEffSeeInvisible = 5;
inline constexpr i32 kEffProtect = 6;
inline constexpr i32 kEffTeleport = 7;
inline constexpr i32 kEffCureParalysis = 10;
inline constexpr i32 kEffDetectRock = 11;
inline constexpr i32 kEffDepthPerception = 12;
inline constexpr i32 kEffFindDirection = 13;
inline constexpr i32 kEffPartyOffset = 200;    // effect + 200 = same effect on the whole party
inline constexpr i32 kHealSchool = 9;          // CastGate.HealSchool (Category 9)
inline constexpr i32 kMaxMemberDigits = 4;

// One entry per module path. Detect splits by effect so each reveal is applied on its own.
enum class DcastPlan : i32 {
  Heal = 0,
  Cure,
  Levitate,
  Invisible,
  SeeInvisible,
  Protect,
  Teleport,
  DetectRock,
  DetectDepth,
  DetectFacing,
  Unsupported
};

// Module path for a spell, from its base effect (party offset removed).
DcastPlan dcastPlanOf(i32 effect, i32 damage2, i32 category) {
  const i32 base = effect >= kEffPartyOffset ? effect - kEffPartyOffset : effect;
  if (base == kEffDamage) {
    return (category == kHealSchool && damage2 > 0) ? DcastPlan::Heal : DcastPlan::Unsupported;
  }
  switch (base) {
    case kEffCurePoison:
    case kEffCureDisease:
    case kEffCureParalysis:
      return DcastPlan::Cure;
    case kEffLevitate:
      return DcastPlan::Levitate;
    case kEffInvisible:
      return DcastPlan::Invisible;
    case kEffSeeInvisible:
      return DcastPlan::SeeInvisible;
    case kEffProtect:
      return DcastPlan::Protect;
    case kEffTeleport:
      return DcastPlan::Teleport;
    case kEffDetectRock:
      return DcastPlan::DetectRock;
    case kEffDepthPerception:
      return DcastPlan::DetectDepth;
    case kEffFindDirection:
      return DcastPlan::DetectFacing;
    default:
      return DcastPlan::Unsupported;
  }
}

// Party number from a word: digits only, 1-based. Returns 0 on anything else.
i32 dcastParseMember(const Str& word) {
  if (word.empty() || word.size() > static_cast<size_t>(kMaxMemberDigits)) return 0;
  i32 n = 0;
  for (char c : word) {
    if (c < '0' || c > '9') return 0;
    n = n * 10 + (c - '0');
  }
  return n;
}


// Depth Perception (reference effect 12): the level the party stands on. Applied.
bool dcastDepth(Ctx& ctx, const Party& party, const Str& casterName, const Str& spellName, Str& note) {
  const i32 level = static_cast<i32>(party.dungeonLevel);
  note = casterName + " cast " + spellName + ": the party is on dungeon level " + std::to_string(level) + ".";
  ctx.say("dcast.depth", note, {level}, {});
  return true;
}

// Find Direction (reference effect 13): the way the party faces. Applied as a facing code.
// NEED: facingNames (compass word for each party.facing code).
bool dcastFacing(Ctx& ctx, const Party& party, const Str& casterName, const Str& spellName, Str& note) {
  const i32 facing = static_cast<i32>(party.facing);
  note = casterName + " cast " + spellName + ": the party faces facing code " + std::to_string(facing) + ".";
  ctx.say("dcast.facing", note, {facing}, {});
  return true;
}

}  // namespace

// cast <member> <spell name or id> [args]
// Gate order follows the reference mdrcharCheckCast: gate, reach, then SP.
// SP is deducted only after the effect has been applied, so a refusal never costs SP.
// UNKNOWN: words[0] is assumed to be the verb "cast" (not stripped).
// UNKNOWN: the Dungeon screen is taken to have no engaged monsters, so only NonCombat and
// Either spells pass the combat gate (spellUse). Peaced monsters are not modelled.
// UNKNOWN: CastRefusal value 0 is taken to mean "no refusal".
// UNKNOWN: itemLevel 0 for a hand cast (p0100 is 0 when the player casts).
bool dcastRun(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() < 3) {
    reply = "Usage: cast <member> <spell name or id>";
    return true;
  }
  const i32 member = dcastParseMember(w[1]);
  Character* caster = accMember(s, member);
  if (member <= 0 || caster == nullptr) {
    reply = "Cast for which party member? Give a party number.";
    return true;
  }
  const Str spellName = seerJoinWords(w, 2);
  const SpellDef* spell = accSpellByName(s, spellName);
  if (spell == nullptr) {
    reply = "No spell called " + spellName + ".";
    return true;
  }
  if (spellUse(*spell) == SpellUse::Combat) {
    reply = spell->name + " is a combat spell and cannot be cast out of a fight.";
    return true;
  }
  const DcastPlan plan = dcastPlanOf(spell->effect, spell->damage2, spell->category);
  if (plan == DcastPlan::Unsupported) {
    reply = spell->name + " has no effect the dungeon can apply.";
    return true;
  }

  Ctx ctx = s.ctx();

  // R9 / mdrcharCheckCast: the caster's guilds must teach it, at a level they have, with stats met.
  const CastRefusal gate = unfitGateCheck(ctx, *caster, *spell);
  if (static_cast<i32>(gate) != 0) {
    reply = caster->name + " cannot cast " + spell->name + " (gate refusal).";
    return true;
  }
  if (spellUnreachable(ctx, *spell)) {
    reply = spell->name + " cannot be reached from here.";
    return true;
  }
  const i32 cost = castCostSp(ctx, *caster, *spell, 0);
  if (caster->vitals.curSpellPoints < cost) {
    reply = caster->name + " does not have the spell points for " + spell->name + ".";
    return true;
  }

  Str note;
  bool applied = false;
  switch (plan) {
    case DcastPlan::DetectDepth:
      applied = dcastDepth(ctx, s.game.party, caster->name, spell->name, note);
      break;
    case DcastPlan::DetectFacing:
      applied = dcastFacing(ctx, s.game.party, caster->name, spell->name, note);
      break;
    case DcastPlan::Heal:
      // NEED: healAmount (the heal amount for a Category 9 Damage2 > 0 spell, and the hits write).
      note = spell->name + " cannot heal yet: the heal amount is not available to the dungeon.";
      break;
    case DcastPlan::Cure:
    case DcastPlan::Levitate:
    case DcastPlan::Invisible:
    case DcastPlan::SeeInvisible:
    case DcastPlan::Protect:
      // NEED: ailmentSetters (set or clear the Levitate, Invisible, SeeInvisible, Protection and
      // poison/disease/paralysis flags on Character.ailments; SpellAilmentKind names them).
      note = spell->name + " has no effect yet: nothing in the dungeon sets that ailment.";
      break;
    case DcastPlan::Teleport:
      // NEED: teleportDestination (the destination level/x/y rule for the spell, then the
      // fall and rock outcomes via teleportLevelIsRock).
      note = spell->name + " cannot move the party yet: its destination is not available.";
      break;
    case DcastPlan::DetectRock:
      // NEED: squareWallQuery (whether the square ahead of the party is solid, from SquareState).
      note = spell->name + " cannot read the square ahead yet: wall data is not available.";
      break;
    case DcastPlan::Unsupported:
      note = spell->name + " has no effect the dungeon can apply.";
      break;
  }

  if (!applied) {
    reply = note;
    return true;  // no SP spent: the effect was not applied
  }
  caster->vitals.curSpellPoints -= cost;
  reply = note;
  return true;
}

void register_dcast() {
  registerFeatureCommand({"cast", Screen::Dungeon, &dcastRun, "cast <member> <spell name or id>  - cast a non-combat spell"});
}

}  // namespace mordor
