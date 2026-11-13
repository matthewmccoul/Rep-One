// casting1_missing.cpp: the six casting1.h functions that had no definition.
// Rules: R5, R6, R9, R10, R11, R20, plus ASSUMED buffer firing.
#include "mordor/casting1.h"

namespace mordor {

// R5 / R6: cast refusal order. Pure check over the already-computed cost.
CastRefusal checkCastGate(const Character& caster, i32 costSp, SpellUse use, i32 spellLevel,
                          const CastSituation& situation) {
    // R6.1 / R5 row 1: cost above current SP refuses. Nothing is spent.
    if (costSp > caster.vitals.curSpellPoints) {
        return CastRefusal::SpellPoints;
    }
    // R6.2 / R5 row 2: monsters present, engaged, and a NonCombat spell.
    if (situation.monstersPresent && situation.engaged && use == SpellUse::NonCombat) {
        return CastRefusal::NonCombat;
    }
    // R6.3 / R5 row 3: anti-magic square or extinguisher.
    if (situation.antiMagicSquare) {
        return CastRefusal::AntiMagic;
    }
    // R6.4a / R5 row 4a: a Combat spell with no monsters present.
    // CONFLICT: R5 row 4a says "use Combat or Either", but R6 4a says "a Combat spell".
    //           Implemented as use == Combat (Either is never refused here).
    if (use == SpellUse::Combat && !situation.monstersPresent) {
        return CastRefusal::CombatOnly;
    }
    // R6.4b / R5 row 4b: Combat or Either, monsters present but peaced, spell level above 7.
    if ((use == SpellUse::Combat || use == SpellUse::Either) && situation.monstersPresent &&
        !situation.engaged && spellLevel > kPeacedSpellLevel) {
        return CastRefusal::TooLongPeaced;
    }
    // R6.5: the cast proceeds.
    return CastRefusal::None;
}

// R9: full check. Unfit gate first (R3/R4), then the refusal order (R5/R6).
CastRefusal checkCastFull(const Ctx& ctx, const Character& caster, const SpellDef& spell,
                          i32 itemLevel, bool fromBufferOrItem, const CastSituation& situation) {
    // R9: Unfit first.
    if (unfitGateApplies(fromBufferOrItem, itemLevel)) {
        CastRefusal unfit = unfitGateCheck(ctx, caster, spell);
        if (unfit != CastRefusal::None) {
            return unfit;
        }
    }
    // R9: then Refuse. Cost comes from the shared cost path (R18 gives 0 for non-zero item level).
    // ASSUMED: SpellDef exposes the base spell level as field `level` (data-file name not in the SRD).
    i32 cost = castCostSp(ctx, caster, spell, itemLevel);
    return checkCastGate(caster, cost, spellUse(spell), spell.level, situation);
}

// R11: every combat round re-checks the cast before any draw.
CastRefusal recheckCastForRound(const Ctx& ctx, Character& caster, CombatOrder& order,
                                const SpellDef& spell, i32 itemLevel, bool fromBufferOrItem,
                                const CastSituation& situation) {
    CastRefusal refusal =
        checkCastFull(ctx, caster, spell, itemLevel, fromBufferOrItem, situation);
    if (refusal != CastRefusal::None) {
        // R11: a refused cast sets the order to Fight. Spends no SP and draws no random number.
        // CONFLICT: R11 says "Fight" in the prose, while the conflict note says None.
        //           Implemented as Fight, the stated behaviour. The target is kept.
        // R15: a standing spell order whose caster lacks SP becomes Fight, keeping its target.
        //      The SpellPoints refusal above covers that case.
        order.kind = CombatOrderKind::Fight;
        return refusal;
    }
    // R11 / R12: passing check. The caller draws drawCastRoll() after this, before any effect.
    return CastRefusal::None;
}

// R10 / engine item combat round: an item spell is ordered only if the check passes.
CastRefusal beginItemSpellOrder(const Ctx& ctx, Party& party, const ItemCastRequest& request,
                                const SpellDef& spell, const CastSituation& situation) {
    // ASSUMED: request.memberIndex is a valid party member index.
    const Character& caster = party.members[static_cast<u32>(request.memberIndex)];
    // Item casts are fromBufferOrItem = true (R3).
    CastRefusal refusal =
        checkCastFull(ctx, caster, spell, request.itemLevel, true, situation);
    if (refusal != CastRefusal::None) {
        // R10: a refused check sets RefusedOrder and does not charge the item.
        // The caller clears the item slot using refusalClearsItemSlot().
        return refusal;
    }
    // R10 / engine item round [c46]: Foes(0).Engaged = 1.
    if (!party.foes.empty()) {
        party.foes[0].engaged = true;
    }
    // UNKNOWN: Sub0BDE() is cited in the engine comment but its behaviour is not in the SRD text.
    // Engine item round [c46]: order = spell; ItemOrders[m] = (slot, item, cast = true).
    party.order = OrderKind::SpellOrder;
    ItemOrder& io = party.itemOrders[static_cast<u32>(request.memberIndex)];
    io.slot = request.itemSlot;
    io.item = request.itemIndex;
    io.cast = true;
    // R19: charges are spent only on a completed cast, not here.
    return CastRefusal::None;
}

// R20: spell level used to resolve the effect.
i32 effectSpellLevel(const Ctx& ctx, const Character& caster, const SpellDef& spell,
                     i32 itemLevel, bool isHeal, bool free) {
    // R20.1: positive item level is used directly.
    if (itemLevel > 0) {
        return itemLevel;
    }
    // R20.2: negative item level uses the caster's current guild spell level (R21).
    if (itemLevel < 0) {
        return guildSpellLevel(caster.activeLevel());
    }
    // R20.3: level-0 item heal uses the guild spell level, minimum 1.
    // CONFLICT: R20.3's scope conflicts with the sl formula in R20, which gives no heal case
    //           for level 0. Implemented as the prose rule.
    if (isHeal) {
        i32 guildLevelSpell = guildSpellLevel(caster.activeLevel());
        return guildLevelSpell < 1 ? 1 : guildLevelSpell;
    }
    // Formula in R20: a free cast uses 1.
    if (free) {
        return 1;
    }
    // R20.4 / R26: otherwise the caster's spell level for this spell.
    return casterSpellLevel(ctx, caster, spell);
}

// ASSUMED: The SRD has no explicit buffer-firing rule. Decision follows the enum and the
// neighbouring rules. Spell buffers are cast or ordered (R13). Item buffers use an item (R10).
BufferFire bufferFireDecision(bool isSpellBuffer, i32 packSlot, bool itemSlotEmptyOrOutOfRange,
                              bool monstersInFront) {
    if (isSpellBuffer) {
        // ASSUMED: packSlot is the spell buffer index here, in 0..kSpellBuffers-1.
        if (packSlot < 0 || packSlot >= kSpellBuffers) {
            return BufferFire::Nothing;
        }
        // ASSUMED: with monsters in front the spell is kept as a standing order, else cast now (R13).
        return monstersInFront ? BufferFire::StandingOrder : BufferFire::CastNow;
    }
    // Item buffer: the item must exist in a valid pack slot.
    if (itemSlotEmptyOrOutOfRange || packSlot < 0 || packSlot >= kPackSlots) {
        return BufferFire::Nothing;
    }
    // RESOLVED-BY-REFERENCE: src/Mordor.Core/CombatOrder.cs:47 (ApplyDefault): "One holding a pack
    // slot is sub15CD on that slot (174-175), the Use command, which needs the fight and the pack -
    // the caller runs it (DungeonWindow.CombatSettingItem)." An item buffer is a Use command run
    // now by the caller, never a standing order, so monstersInFront does not change it.
    return BufferFire::UseItem;
}

}  // namespace mordor
