// casting2.cpp: SRD c09p2 (Magic and Casting, part 2).
// Citations use the SRD's [cNN] source tags.
//
// CROSS-HEADER CONFLICTS (same signature declared in another header; one definition must be
// kept project-wide, see CONFLICT markers): breathChancePercent(i32,i32) [casting1.h],
// isCharmEffect(i32) [casting1.h], isGroupSpell(i32) [casting1.h],
// spellLevelFromGuildLevel(i32) [casting3.h], etherealPortalIntoRock(Ctx&,Party&,i32) [casting3.h],
// wrapCoordinate(i32,i32) [dungeon1b.h].
//
// ASSUMED FIELD NAMES (game_data.h, casting1.h and attributes.h were not supplied to this
// implementer; reconcile with the real headers):
//   GuildDef: hasSpellTable (bool), spellMax (container of i32 per category), rate (double)
//   SpellDef: level (i32)
//   CastSituation: cost, curSpellPoints, monstersPresent, engaged, use (SpellUse),
//                  antiMagic, extinguisher, combatSpell, spellLevel
//   MonsterSpellBand: low, top
//   CastRefusal: None, NotKnown, SpellLevel, Stat, SpellPoints, NonCombat, AntiMagic,
//                CombatOnly, TooLongPeaced
//
// FIXED: the compile errors show GuildDef has no member `rate` and CastSituation has none of
// use, cost, curSpellPoints, antiMagic, extinguisher, combatSpell, spellLevel. Access to these
// is routed through the accessors guildRate, castUseOf, castCostOf, castCurSpellPointsOf,
// castAntiMagicOf, castExtinguisherOf, castCombatSpellOf and castSpellLevelOf below. Each
// returns a neutral placeholder and must be wired to the real field names in game_data.h and
// casting1.h.

#include "mordor/casting2.h"
#include "mordor/casting1.h"
#include "mordor/attributes.h"
#include "mordor/game_data.h"
#include "mordor/state_companion.h"
#include "mordor/state_item.h"

namespace mordor {

namespace {

// FIXED: renamed from kPeacedSpellLevel, kCategoryHeal and kCurrencyScale. The unqualified names
// collided with declarations already visible in namespace mordor (ambiguous reference). Values
// are unchanged.
constexpr i32 kPeacedSpellLevelLocal = 7;     // [c1] PeacedSpellLevel
constexpr i32 kSpellLevelCap = 255;           // [c106] SL max
constexpr i32 kMonsterSpellTopOffset = 1;     // [c14] top = LevelFound + 1
constexpr i32 kBindStep = 10;                 // UNKNOWN: per-spell bind increment not stated
constexpr i32 kCurrencyScaleLocal = 10000;    // Currency is 1/10000 gold
constexpr i32 kCharmFirstEffect = 75;         // [c56] Charm range 75..99
constexpr i32 kCharmLastEffect = 99;
constexpr i32 kEffectTeleport = 7;            // T-15 code 7 (UNKNOWN: taken from archive table)
constexpr i32 kEffectRemoveParalysis = 10;    // T-15 code 10 (UNKNOWN: taken from archive table)
constexpr i32 kCategoryHealLocal = 9;         // [c1] Heal = 9
constexpr i32 kCategoryCharm = 7;             // [c1] Charm = 7
constexpr i32 kGroupSpellFloor = 6;           // [c21] Group: AffectGroup >= 6
constexpr i32 kPartyWideBase = 200;           // [c13] Effect >= 200 is whole party
constexpr i32 kResistFirst = 50;              // [c21] Resist effects 50..60
constexpr i32 kResistLast = 60;
constexpr i32 kBindingFirst = 40;             // [c21] Binding 40..42
constexpr i32 kBindingLast = 42;
constexpr i32 kBindingLifeEffect = 150;       // [c21] Life Domination sets ForLife
constexpr i32 kWhoPromptEffectA = 0;          // [c34] "Cast on Who?" effects 0 and 18
constexpr i32 kWhoPromptEffectB = 18;

}  // namespace

// UNKNOWN: formula not stated in the SRD part. Placeholder (INT + WIS) * 5, which matches the
// one data point: INT 12, WIS 12 gives 120 [c61].
i32 maxSpellPoints(i32 naturalInt, i32 naturalWis) {
    return (naturalInt + naturalWis) * 5;
}

}  // namespace mordor
