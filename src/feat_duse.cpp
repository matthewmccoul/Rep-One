// Feature duse: `use <member> <slot>` (Dungeon) and `useitem <member> <slot>` (Town).
// Mirrors OpenDejenol ItemUse.cs: Problem, sub1149 (charge spend), SpellOf, ApplySpecial.
// Implemented against real state: party and pack lookup, the Problem checks, the charge
// spend, removal of a used-up item, spell points, years of age, and the Kind 25 age rule.
// Not yet implemented, marked NEED where they occur: the item spell cast, stat modifiers,
// the max-charges check, and the gear check after stat changes.
#include "mordor/features.h"
#include "mordor/access.h"
#include "mordor/shared_helpers.h"
#include "mordor/parse.h"

namespace mordor {
namespace {

// ---- constants (ItemUse / MAINMOD2.BAS) ----
constexpr i32 kDuseGuildIssuedBase = 500;                          // ItemUse.GuildIssuedBase
constexpr i32 kDuseGuildIssuedLast = 500 + kGuildCount - 1;        // crests 500..511
constexpr i32 kDuseKind25AgeDays = 28;                             // mc0866 (MAINMOD2.BAS:275)
constexpr i32 kDuseYearDays = 365;                                 // UNKNOWN: year length in Character.ageDays
constexpr i32 kDuseScrollSpellPoints = 50;                         // ApplySpecial case 11
constexpr i32 kDuseTomeSpellPoints = 200;                          // ApplySpecial case 12
// UNKNOWN: ItemAlign 0 is Unaligned; 1..3 are Good, Neutral, Evil (same order as CharAlign).
constexpr i32 kDuseUnaligned = 0;

// Result of the alignment fit between an item and the member using it.
enum class DuseFit : i32 { Same = 0, Adjacent, Opposed };

i32 duseAbs(i32 v) { return v < 0 ? -v : v; }

// Digits only, bounded. Rejects signs and empty words.
bool duseParseIndex(const Str& w, i32& out) {
  i64 v = 0;
  if (!parseDecimal(w, v, false, 0, 100000)) return false;
  out = static_cast<i32>(v);
  return true;
}

Str duseUsage() {
  return "use <member> <slot>  - use an item from a pack slot (potions heal, scrolls cast)";
}

// Appends one line to the reply. Lines are separated by newlines.
void duseSay(Str& reply, const Str& line) {
  if (!reply.empty()) reply += "\n";
  reply += line;
}

// ItemUse.MustBeEquippedToUse (MAINMOD2.BAS:6113-6119). Type 0 is deliberately absent.
bool duseMustBeEquipped(i32 type) {
  return (type >= 1 && type <= 22) || type == 29 || type == 32 || type == 33 || type == 34;
}

// sub1149 (MAINMOD2.BAS:5684-5711): last charge removes these types from the pack.
bool duseConsumedWhenEmpty(i32 type) {
  return (type >= 23 && type <= 28) || type == 30 || type == 31;
}

i32 duseItemAlign(const Item& it) { return static_cast<i32>(it.alignment); }
i32 duseCharAlign(const Character& c) { return static_cast<i32>(c.alignment); }

// Alignment rule for a spell out of an aligned item (MAINMOD2.BAS:6085-6119):
// an aligned item only works for its own alignment; anything else says "Nothing Happens."
bool duseAlignedToOther(const Item& it, const Character& c) {
  const i32 ia = duseItemAlign(it);
  if (ia == kDuseUnaligned) return false;
  return ia != duseCharAlign(c);
}

// ApplySpecial alignment fit (MAINMOD2.BAS:6145-6152): Adjacent stops, Opposed reverses.
// UNKNOWN: the ordering used to call a pair Adjacent or Opposed. Here the three alignments
// sit on a line (Good, Neutral, Evil): one step apart is Adjacent, two steps is Opposed.
DuseFit duseFitOfItem(const Item& it, const Character& c) {
  const i32 ia = duseItemAlign(it);
  if (ia == kDuseUnaligned) return DuseFit::Same;
  const i32 d = duseAbs(ia - duseCharAlign(c));
  if (d == 0) return DuseFit::Same;
  if (d == 1) return DuseFit::Adjacent;
  return DuseFit::Opposed;
}

// Stat modifiers of a Kind 1 item, permanently applied to abilities.stats (or reversed).
// Returns true when any modifier was applied (the Kind 25 age rule depends on it).
// NEED: stat-modifier fields on ItemDef and a stat setter for abilities.stats.
bool duseApplyStatMods(Character& c, const ItemDef& def, bool reversed) {
  (void)c;
  (void)def;
  (void)reversed;
  return false;
}

// Spell points gained (Scroll of Spells, Tome of Spells). Reaches the vitals directly.
// UNKNOWN: whether the original caps at the maximum; this port caps.
void duseGainSpellPoints(Character& c, i32 amount) {
  c.vitals.curSpellPoints += amount;
  if (c.vitals.curSpellPoints > c.vitals.maxSpellPoints) {
    c.vitals.curSpellPoints = c.vitals.maxSpellPoints;
  }
}

// Days older (Kind 25 stat item). Applies even when the fit is reversed.
void duseOlder(Character& c, i32 days) {
  c.ageDays += days;
}

// The item's spell, cast before the special effect (sub15CD order).
// NEED: item cast entry point (ItemCastRequest fields and the spell effect resolver), so a
// heal potion heals and a scroll casts. Until it exists the spell is not resolved here.
bool duseCastItemSpell(Session& s, Character& c, const Item& used, const SpellDef& spell) {
  (void)s;
  (void)c;
  (void)used;
  (void)spell;
  return false;
}

// Shared body for Dungeon "use" and Town "useitem".
bool duseUse(Session& s, const Vec<Str>& w, Str& reply) {
  reply.clear();
  if (w.size() != 3) {
    reply = duseUsage();
    return true;
  }
  i32 memberNo = 0;
  i32 slotNo = 0;
  if (!duseParseIndex(w[1], memberNo) || !duseParseIndex(w[2], slotNo)) {
    reply = duseUsage();
    return true;
  }

  Character* c = accMember(s, memberNo);
  if (c == nullptr) {
    reply = "No such member.";  // UNKNOWN wording
    return true;
  }
  // sub15CD refuses silently for an incapacitated member (MAINMOD2.BAS:6085).
  // UNKNOWN: which deathState values count as incapacitated (M5483 mapping).
  if (c->deathState != DeathState::Alive) return true;

  // Slot 1 is the hands and never an item; an empty slot is silent.
  Item* it = accPackItem(s, memberNo, slotNo);
  if (it == nullptr) {
    reply = "No such slot.";  // UNKNOWN wording
    return true;
  }
  if (it->id <= 0) return true;
  const ItemDef* def = accItemDef(s, it->id);
  if (def == nullptr) return true;

  // ---- ItemUse.Problem, in the original's order (MAINMOD2.BAS:6085-6119) ----
  // A negative specialType means the item is not usable and says nothing.
  if (def->specialType < 0) return true;
  // NEED: ItemDef max-charges field, for the "doesn't appear to do much" check
  // (MAINMOD2.BAS:6093). Skipped until the field is named.
  if (it->charges <= 0) {
    reply = "\"" + c->name + "\", the Item is out of charges!";
    return true;
  }
  // The hands and worn-count limits are skipped here (EquipProblem with skipLimits).
  if (def->spellId > 0) {
    if (duseAlignedToOther(*it, *c)) {
      reply = "Nothing Happens.";
      return true;
    }
    if (duseMustBeEquipped(def->type) && !it->equipped) {
      reply = "Item must be Equipped to use!";
      return true;
    }
  }

  // ---- sub1149: spend the charge first (MAINMOD2.BAS:5684-5711) ----
  const Item used = *it;  // snapshot: the spell and effect read the item as it was
  it->charges -= 1;
  const bool gone = it->charges <= 0 && duseConsumedWhenEmpty(def->type);
  if (gone) *it = Item();  // the used-up item leaves the pack

  // ---- the spell, cast before the special effect (sub15CD order) ----
  if (def->spellId > 0) {
    if (const SpellDef* spell = accSpellById(s, def->spellId)) {
      duseCastItemSpell(s, *c, used, *spell);
    }
  }

  // ---- ApplySpecial (MAINMOD2.BAS:6145-6230) ----
  // Guild crests are silent here. Specials at or below zero do nothing more.
  if (def->specialType <= 0) return true;
  if (def->specialType >= kDuseGuildIssuedBase && def->specialType <= kDuseGuildIssuedLast) return true;

  const DuseFit fit = duseFitOfItem(used, *c);
  if (fit == DuseFit::Adjacent) {
    reply = "Nothing Happens.";  // the charge is already spent
    return true;
  }
  const bool reversed = fit == DuseFit::Opposed;

  bool statsApplied = false;
  switch (def->specialType) {
    case 1:  // potions: stat modifiers
      statsApplied = duseApplyStatMods(*c, *def, reversed);
      break;
    case 11:  // Scroll of Spells: 50 spell points (none when reversed)
      if (!reversed) duseGainSpellPoints(*c, kDuseScrollSpellPoints);
      break;
    case 12:  // Tome of Spells: 200 spell points
      if (!reversed) duseGainSpellPoints(*c, kDuseTomeSpellPoints);
      break;
    case 13:  // Potion of Youth: one year younger, then stat modifiers
      if (!reversed) ageCharacterDays(*c, -1 * kDuseYearDays, nullptr);
      statsApplied = duseApplyStatMods(*c, *def, reversed);
      break;
    case 14:  // Dragon's Blood: ten years younger, then stat modifiers
      if (!reversed) ageCharacterDays(*c, -10 * kDuseYearDays, nullptr);
      statsApplied = duseApplyStatMods(*c, *def, reversed);
      break;
    default:
      break;
  }

  // NEED: canWear / gear check after stat modifiers: out-of-reach gear comes off.

  // Kind 25 stat items age the user 28 days, whether or not the fit reversed the modifiers.
  if (def->type == 25 && statsApplied) {
    duseOlder(*c, kDuseKind25AgeDays);
    duseSay(reply, "You are now 4 week(s) older.");
  }
  return true;
}

bool cmdDungeonUse(Session& s, const Vec<Str>& w, Str& reply) {
  return duseUse(s, w, reply);
}

bool cmdTownUseItem(Session& s, const Vec<Str>& w, Str& reply) {
  return duseUse(s, w, reply);
}

}  // namespace

void register_duse() {
  registerFeatureCommand({"use", Screen::Dungeon, &cmdDungeonUse, "use <member> <slot>  - use an item from a pack slot (potions heal, scrolls cast)"});
  registerFeatureCommand({"useitem", Screen::Town, &cmdTownUseItem, "useitem <member> <slot>  - use an item from a pack slot (potions heal, scrolls cast)"});
}

}  // namespace mordor
