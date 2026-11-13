// Module companions1a: companion slots and identity, binding scale, join offers,
// Confinement pen, sale, and combat mishap / breath / spit rules (SRD c12p1).
//
// ASSUMPTIONS (game_data.h and the GameState layer were not supplied to this ant;
// verify against the generated headers):
//   ctx.data.monsters        : Vec<MonsterDef>
//   MonsterDef.id            : i32   (creature identifier, not unique)
//   MonsterDef.alignment     : i32   (0 Good, 1 Neutral, 2 Evil)
//   MonsterDef.levelFound    : i32
//   MonsterDef.guildLevel    : i32
//   MonsterDef.hits          : i32   (rolled-maximum source, "M4EAC" in docs)
//   ctx.rng.below(n) returns an integer; ctx.rng.percent(p) returns bool.
//   ctx.say(code, text, nums, strs) with empty braces for nums/strs.
//   GameState is opaque here; no field is read through it.
#include <cmath>

#include "mordor/companions1a.h"
#include "mordor/events.h"
#include "mordor/rng.h"

namespace mordor {

namespace {

constexpr i32 kPenShelfCap = 9;  // R19: mc0298 (pen shelf cap constant)
constexpr i32 kBreathChanceBase = 50;  // R23.2 base fire roll (c28)

// R2: an empty slot has Ref = -1.
inline bool slotEmpty(const Companion& c) {
    return c.ref < 0;
}

// Living for combat and spit counts (R22.1, R25.3): occupied, Hits > 0, not dead.
inline bool isLiving(const Companion& c) {
    return !slotEmpty(c) && c.hits > 0 && !c.isDead;
}

inline i32 absI(i32 v) {
    return v < 0 ? -v : v;
}

inline bool flagBit(u32 flags, u32 bit) {
    return ((flags >> bit) & 1u) != 0u;
}

inline i32 monsterCount(const Ctx& ctx) {
    return static_cast<i32>(ctx.data.monsters.size());
}

inline const MonsterDef& monsterAt(const Ctx& ctx, i32 index) {
    return ctx.data.monsters[static_cast<u32>(index)];
}

// Index of the first pen shelf matching monster ref and alignment, or -1.
i32 findShelf(const Pen& pen, i32 monsterRef, CompanionAlignment alignment) {
    const i32 n = static_cast<i32>(pen.shelves.size());
    for (i32 i = 0; i < n; ++i) {
        const PenShelf& s = pen.shelves[static_cast<u32>(i)];
        if (s.monsterRef == monsterRef && s.alignment == alignment) {
            return i;
        }
    }
    return -1;
}

// R19/R20: return one unit of stock to the pen.
void returnToPen(Pen& pen, i32 monsterRef, CompanionAlignment alignment) {
    const i32 idx = findShelf(pen, monsterRef, alignment);
    if (idx >= 0) {
        pen.shelves[static_cast<u32>(idx)].stock += 1;
        return;
    }
    PenShelf shelf{};
    shelf.monsterRef = monsterRef;
    shelf.alignment = alignment;
    shelf.stock = 1;
    pen.shelves.push_back(shelf);
}

}  // namespace

// R2: first hole in the four fixed slots, or -1.
// R2 / R11.3: the first slot not yet occupied by a Companion, or -1 if none.
// ASSUMPTION: CompanionSlots is a fixed-size Arr<Companion, N> exposing size(),
// and Companion carries a bool `occupied`. Confirm against the real definitions.
i32 firstFreeCompanionSlot(CompanionSlots& slots) {
    // Only kCompanionSlotsPerCharacter companion slots exist per character;
    // the scan must not run past them even if the backing array is larger.
    for (i32 i = 0; i < kCompanionSlotsPerCharacter; ++i) {
        if (!slots[static_cast<u32>(i)].occupied) {
            return i;
        }
    }
    return -1;
}

// R3 Resolve.
i32 resolveMonsterRef(const Ctx& ctx, i32 ref, i32 monsterId) {
    const i32 n = monsterCount(ctx);
    if (ref >= 0 && ref < n && monsterAt(ctx, ref).id == monsterId) {
        return ref;
    }
    for (i32 i = 0; i < n; ++i) {
        if (monsterAt(ctx, i).id == monsterId) {
            return i;
        }
    }
    return ref;
}

// R3 FromId: one match -> it; several and alignment known -> the one with that
// alignment; otherwise the first match. No match returns -1.
// UNKNOWN: no-match return value is not stated; -1 chosen.
i32 monsterRefFromId(const Ctx& ctx, i32 monsterId, bool alignmentKnown, CompanionAlignment alignment) {
    const i32 n = monsterCount(ctx);
    i32 first = -1;
    i32 byAlign = -1;
    i32 matches = 0;
    for (i32 i = 0; i < n; ++i) {
        const MonsterDef& m = monsterAt(ctx, i);
        if (m.id != monsterId) {
            continue;
        }
        matches += 1;
        if (first < 0) {
            first = i;
        }
        if (alignmentKnown && byAlign < 0 && alignmentOfDefinition(m.alignmentValue) == alignment) {
            byAlign = i;
        }
    }
    if (matches == 0) {
        return -1;
    }
    if (matches > 1 && alignmentKnown && byAlign >= 0) {
        return byAlign;
    }
    return first;
}

// R3 COMPANION SORTER: if MonsterId != Monsters[Ref].Id, Ref = first i with that Id.
void resyncCompanionRef(const Ctx& ctx, Companion& c) {
    const i32 n = monsterCount(ctx);
    if (c.ref >= 0 && c.ref < n && monsterAt(ctx, c.ref).id == c.monsterId) {
        return;
    }
    for (i32 i = 0; i < n; ++i) {
        if (monsterAt(ctx, i).id == c.monsterId) {
            c.ref = i;
            return;
        }
    }
    // UNKNOWN: no match leaves Ref unchanged.
}

// R4: Id 307 resolves by alignment.
i32 id307Ref(const Ctx& ctx, CompanionAlignment alignment) {
    if (alignment == CompanionAlignment::Neutral) {
        return kId307NeutralRef;  // Morgul 278
    }
    if (alignment == CompanionAlignment::Evil) {
        return kId307EvilRef;  // Shadow Ninja 389
    }
    // Good (or unaligned): Morgul, the first index with Id 307.
    const i32 n = monsterCount(ctx);
    for (i32 i = 0; i < n; ++i) {
        if (monsterAt(ctx, i).id == kCompanionId307) {
            return i;
        }
    }
    return kId307NeutralRef;  // UNKNOWN: fallback if table lacks Id 307
}

// R5: character scheme 0..2 maps to item scheme 1..3 (+1). Other values use the definition.
// NOTE (quirk per R5 formula): the test is on the raw stored value, so an item-scheme
// value 1..2 is read as a character-scheme value and shifted again.
CompanionAlignment companionAlignmentFromStored(i32 storedAlignment, i32 monsterAlignmentValue) {
    if (storedAlignment >= 0 && storedAlignment <= 2) {
        return static_cast<CompanionAlignment>(storedAlignment + 1);
    }
    return alignmentOfDefinition(monsterAlignmentValue);
}

// R5 Confinement.AlignmentOf: 0 Good, 1 Neutral, 2 Evil, other Neutral.
CompanionAlignment alignmentOfDefinition(i32 alignmentValue) {
    switch (alignmentValue) {
        case 0:
            return CompanionAlignment::Good;
        case 2:
            return CompanionAlignment::Evil;
        case 1:
        default:
            return CompanionAlignment::Neutral;
    }
}

// R6 CharmIdLevel(owner, def).
// UNKNOWN: the owner-knowledge formula is not in this SRD part. Port uses the owner's
// kill record for the monster ref: no kills -> Barely, otherwise Partially.
OwnerIdLevel charmIdLevel(const Ctx& ctx, const Character& owner, i32 monsterRef) {
    (void)ctx;
    i32 kills = 0;
    if (monsterRef >= 0 && monsterRef < static_cast<i32>(owner.monsterKills.size())) {
        kills = owner.monsterKills[static_cast<u32>(monsterRef)];
    }
    if (kills <= 0) {
        return OwnerIdLevel::Barely;
    }
    return OwnerIdLevel::Partially;
}

// R6: if CharmIdLevel < c.IdLevel then c.IdLevel = that value (lower enum = more knowledge).
void sharpenIdLevel(const Ctx& ctx, const Character& owner, Companion& c) {
    const OwnerIdLevel lvl = charmIdLevel(ctx, owner, c.ref);
    if (static_cast<i32>(lvl) < static_cast<i32>(c.idLevel)) {
        c.idLevel = lvl;
    }
}

// R8: Initial = min(5 + monsterFloor, 10).
i32 startingBinding(i32 monsterFloor) {
    return minv(kBindingBase + monsterFloor, kStartBindingCap);
}

// R8: BoughtBinding = min(10, 5 + LevelFound).
i32 boughtBinding(i32 levelFound) {
    return minv(kBoughtBindingCap, kBindingBase + levelFound);
}

// R9 NextLevel(cur): magnitude plus step, capped at Max.
i32 nextBindingLevel(i32 current) {
    const i32 a = absI(current);
    return (a + kBindingStep > kBindingPurchaseMax) ? kBindingPurchaseMax : a + kBindingStep;
}

// R9 f: step factor from the magnitude of the current binding.
i32 bindingCostFactor(i32 current) {
    const i32 a = absI(current);
    return (a + kBindingStep > kBindingPurchaseMax) ? kBindingPurchaseMax - a : a + kBindingStep;
}

// R9 Cost = guildLevel * 1237 * f * current / 4, Charge = round_half_even(Cost).
// Negative current gives a negative charge (quirk, R9).
Currency bindingRaiseCharge(i32 monsterGuildLevel, i32 current) {
    const i64 f = bindingCostFactor(current);
    const i64 scaled = static_cast<i64>(monsterGuildLevel) * kBindingCostConst * f * static_cast<i64>(current);
    const f64 cost = static_cast<f64>(scaled) / static_cast<f64>(kBindingCostDivisor);
    return roundHalfEven(cost);
}

// UNKNOWN: charPay (city2b) is not called; this is a local payment. Pays from on-hand
// gold first, then bank. A negative amount credits gold (quirk, R9 negative cost).
bool payCharacterGold(Ctx& ctx, Character& c, Currency amount) {
    if (amount <= 0) {
        c.wallet.goldOnHand -= amount;
        return true;
    }
    if (c.purse() < amount) {
        ctx.say("companion.pay.short", "Not enough gold.", {}, {});
        return false;
    }
    const Currency fromHand = minv(c.wallet.goldOnHand, amount);
    c.wallet.goldOnHand -= fromHand;
    c.wallet.goldInBank -= (amount - fromHand);
    return true;
}

// R18: raise binding by one step at Confinement. Binding 100 cannot be bought.
// UNKNOWN: whether binding 12 (cost 0) is blocked is not decided; allowed here.
bool raiseBinding(Ctx& ctx, Character& owner, Companion& c) {
    if (companionForLifeBinding(c)) {
        return false;
    }
    const Currency charge = bindingRaiseCharge(c.guildLevel, c.bindingLevel);
    if (!payCharacterGold(ctx, owner, charge)) {
        return false;
    }
    c.bindingLevel = nextBindingLevel(c.bindingLevel);
    return true;
}

// R10.1: a non-engaged group may offer if its alignment is 1 or matches the viewer,
// and its GuildLevel does not exceed the viewer's top guild level (0 if none).
// UNKNOWN: the R13.2 "only group in a room" rule conflicts with R10 and is not applied.
bool groupMayOfferJoin(const MonsterGroup& group, Align viewerAlign, i32 viewerTopGuildLevel) {
    if (group.state == GroupState::Engaged) {
        return false;
    }
    const i32 monsterAlign = static_cast<i32>(group.front.align);
    const bool alignOk = (monsterAlign == 1) || (monsterAlign == static_cast<i32>(viewerAlign));
    const i32 topGuild = maxv(0, viewerTopGuildLevel);
    return alignOk && group.front.guildLevel <= topGuild;
}

// R10: bar = max(0, Cha*10 - (LevelFound-1)*10).
i32 joinBar(i32 cha, i32 levelFound) {
    return maxv(0, cha * kJoinBarChaScale - (levelFound - 1) * kJoinBarLevelScale);
}

// R10: t = Int(Rnd*3000)+1; if t <= bar then JoinOffer = Int(Rnd*min(GroupCount,4))+1.
// Only group 0's offer is shown (quirk), which the caller enforces.
i32 rollJoinOffer(Ctx& ctx, i32 cha, i32 levelFound, i32 groupCount) {
    const i32 t = static_cast<i32>(ctx.rng.below(kJoinRollMax)) + 1;
    if (t > joinBar(cha, levelFound)) {
        return 0;
    }
    i32 n = minv(groupCount, kJoinOfferGroupCap);
    if (n < 1) {
        n = 1;  // UNKNOWN: zero groups with an offer; treated as one.
    }
    return static_cast<i32>(ctx.rng.below(n)) + 1;
}

// R11.3: joiner copies the front monster's current Hits and rolled maximum, bound at -10.
// Every joiner is made from the front monster (quirk).
// UNKNOWN: stat and def mapping from Creature to Companion is not given; left at defaults.
Companion makeJoinerFromFront(const Creature& front, i32 ownerIndex, i32 slot) {
    Companion j{};
    j.ref = front.ref;
    j.occupied = true;
    j.slot = slot;
    j.owner = ownerIndex;
    j.monsterId = front.id;
    j.name = front.knownAs;
    j.alignment = static_cast<CompanionAlignment>(static_cast<i32>(front.align) + 1);  // R5 char->item
    j.hits = front.hits;
    j.maxHits = front.maxHits;
    j.isDead = false;
    j.bindingLevel = kFreeWillBinding;  // R11.2, R14
    j.bound = companionBound(j);
    j.forLife = companionForLifeBinding(j);
    j.joinedFreely = true;
    j.guildLevel = front.guildLevel;
    j.level = front.levelFound;
    j.dungeonLevel = front.floor;
    return j;
}

// R11, R12: accept up to the offered count, stopping when slots run out or the group is empty.
// Returns the number of joiners added. Nothing is reported on success.
// UNKNOWN: offer bookkeeping after acceptance (offerCount reduced, joinOffer cleared when
// used up) is not stated; chosen as the simplest consistent behaviour.
i32 acceptJoinOffer(Ctx& ctx, const Character& owner, i32 ownerIndex, CompanionSlots& slots, MonsterGroup& group) {
    (void)owner;
    i32 joined = 0;
    const i32 offered = group.offerCount;
    for (i32 k = 0; k < offered; ++k) {
        if (group.count <= 0) {
            break;
        }
        const i32 slot = firstFreeCompanionSlot(slots);
        if (slot < 0) {
            // R12.1: the offer stands.
            ctx.say("companion.join.noRoom", "Join - No room for more Companions!", {}, {});
            break;
        }
        // R12.2 / R37: a joiner is dead only when its Hits have reached 0.
        // UNKNOWN: joinRefusedDead's body was not supplied; this inline test
        // replaces it so that Creature.alive (default false) cannot refuse a live front monster.
        if (group.front.hits <= 0) {
            break;
        }
        Companion joiner = makeJoinerFromFront(group.front, ownerIndex, slot);
        // R2 / R11.3: the joiner now occupies its slot, so the next pass must
        // not hand the same slot back to firstFreeCompanionSlot.
        joiner.occupied = true;
        slots[static_cast<u32>(slot)] = joiner;
        group.count -= 1;
        joined += 1;
    }
    if (joined > 0) {
        group.offerCount -= joined;
        if (group.offerCount <= 0) {
            group.offerCount = 0;
            group.joinOffer = false;
        }
    }
    return joined;
}

// R12.2: a dead joiner is refused while monsters are present.
// UNKNOWN: "dead" taken as not alive.
bool joinRefusedDead(const Creature& joiner, bool monstersPresent) {
    return monstersPresent && !joiner.alive;
}

// R13.1: rand(1,3000) <= CHA*10 - (mLvl-1)*10.
bool monsterJoinRoll(Ctx& ctx, i32 cha, i32 mLvl) {
    const i32 roll = static_cast<i32>(ctx.rng.below(kJoinRollMax)) + 1;
    return roll <= joinBar(cha, mLvl);
}

// R13.4 (proposed): P_join = max(0, CHA - M + 1) / 300.
f64 voluntaryJoinProbability(i32 cha, i32 monsterLevel) {
    const f64 raw = static_cast<f64>(cha - monsterLevel + 1);
    const f64 clamped = raw < 0.0 ? 0.0 : raw;
    return clamped / kVoluntaryJoinDivisor;
}

// R13.3: a Charm forces a join if damage inflicted exceeds remaining hits.
// UNKNOWN: research gives magic potency instead (R13.3 note); damage rule used.
bool charmForcesJoin(i64 inflicted, i64 remainingHits) {
    return inflicted > remainingHits;
}

// R15: each rejected draw increments tries; >150 resets subtype to 0; >300 fails.
// Shared counter quirk (R15).
SubtypePick rejectSubtypeDraw(SubtypePick pick) {
    pick.tries += 1;
    if (pick.tries > kSubtypeResetTries) {
        pick.subtype = 0;
    }
    if (pick.tries > kSubtypeFailTries) {
        pick.failed = true;
    }
    return pick;
}

// R16 spawn mode table.
// UNKNOWN: window is not given for table-entry modes and for "lead itself"; the previous
// window is kept there. Mode 7 and negative modes reuse the previous window (quirk).
// Quirk: no monster has modes 5, 6 or 7 in MDATA5 (R16); m0292 > 10000 never increments.
SpawnPick spawnPickForMode(i32 mode, SpawnWindow previous) {
    SpawnPick p{};
    p.window = previous;
    p.tableEntry = -1;
    switch (mode) {
        case 0:
            p.window.below = 3;
            p.window.above = -1;
            break;
        case 1:
            p.window.below = 3;
            p.window.above = 0;
            break;
        case 2:
            p.window.below = 0;
            p.window.above = 0;
            break;
        case 3:
            p.leadItself = true;
            break;
        case 4:
            p.tableEntry = 4;  // the Slave
            break;
        case 5:
            p.tableEntry = 5;
            break;
        case 6:
            p.tableEntry = 4;
            break;
        default:
            if (mode > 7) {
                p.tableEntry = mode - 7;
            }
            // 7 or negative: no case; previous window reused.
            break;
    }
    return p;
}

// R17: purchase at Confinement.
// Order: free slot, stock at the alignment (floor > 0 needs stock), gold charged first,
// then the pen and companion are changed.
// UNKNOWN: owner index is not in the signature; Companion.owner is left at -1 for the caller.
bool buyCompanion(Ctx& ctx, Character& owner, CompanionSlots& slots, Pen& pen,
                  CompanionAlignment alignment, i32 monsterRef, Currency price) {
    const i32 slot = firstFreeCompanionSlot(slots);  // R17.1
    if (slot < 0) {
        ctx.say("companion.buy.noSlot", "No free companion slot.", {}, {});
        return false;
    }
    if (monsterRef < 0 || monsterRef >= monsterCount(ctx)) {
        return false;
    }
    const MonsterDef& def = monsterAt(ctx, monsterRef);
    const i32 idx = findShelf(pen, monsterRef, alignment);
    if (idx < 0) {
        ctx.say("companion.buy.noStock", "None of that kind is in stock.", {}, {});
        return false;
    }
    PenShelf& shelf = pen.shelves[static_cast<u32>(idx)];
    if (shelf.stock <= 0 && def.levelFound > 0) {  // R17.3: floor 0 never runs out
        ctx.say("companion.buy.noStock", "None of that kind is in stock.", {}, {});
        return false;
    }
    if (!payCharacterGold(ctx, owner, price)) {  // R17.2: charge first
        return false;
    }
    Companion c{};
    c.ref = monsterRef;
    c.occupied = true;
    c.slot = slot;
    c.owner = -1;  // UNKNOWN: owner index not available; caller sets it.
    c.monsterId = def.id;
    c.alignment = alignment;
    c.hits = def.hits;  // R17.4 / R17.5: full hits, capped at max
    c.maxHits = def.hits;
    c.name = "";
    c.idLevel = OwnerIdLevel::Completely;  // IdLevel 0
    c.bindingLevel = boughtBinding(def.levelFound);  // R8, R17.2
    c.bound = companionBound(c);
    c.forLife = companionForLifeBinding(c);
    c.guildLevel = def.guildLevel;
    c.level = def.levelFound;
    c.dungeonLevel = def.levelFound;
    slots[static_cast<u32>(slot)] = c;
    if (def.levelFound > 0) {  // R17.3
        shelf.stock -= 1;
    }
    return true;
}

// R19: pen replenish with the aligned shelf cap of nine.
// UNKNOWN: replenish amount and rate are not given; one unit per call for shelves below cap.
void replenishPen(Ctx& ctx, Pen& pen) {
    (void)ctx;
    const i32 n = static_cast<i32>(pen.shelves.size());
    for (i32 i = 0; i < n; ++i) {
        PenShelf& s = pen.shelves[static_cast<u32>(i)];
        if (s.stock < kPenShelfCap) {
            s.stock += 1;
        }
    }
}

// R19 variant (mdrReplenishStore).
// CONFLICT: the SRD says the alignment index is assigned before the test in this variant,
// but gives no way to recover the difference. Implemented identically to replenishPen.
void replenishPenStoreVariant(Ctx& ctx, Pen& pen) {
    replenishPen(ctx, pen);
}

// R20: sale. Gold to the seller. Floor-0 companions are not returned to the pen.
// Other slots keep their positions. Dead companions are not checked (quirk).
// UNKNOWN: mod-designated bank account route is not modelled; price comes from the caller.
bool sellCompanion(Ctx& ctx, Character& seller, CompanionSlots& slots, Pen& pen, i32 slot, Currency price) {
    if (slot < 0 || slot >= kCompanionSlotsPerCharacter) {
        return false;
    }
    Companion& c = slots[static_cast<u32>(slot)];
    if (slotEmpty(c)) {
        return false;
    }
    seller.wallet.goldOnHand += price;
    if (c.dungeonLevel != 0) {  // R20.2: "not floor 0" read from the companion floor field
        returnToPen(pen, c.ref, c.alignment);
    }
    slots[static_cast<u32>(slot)] = Companion{};  // release; nothing moves up (R2)
    ctx.say("companion.sold", "Companion sold.", {}, {});
    return true;
}

// R22.1: skip any companion with Hits <= 0.
bool companionCanAct(const Companion& c) {
    return c.hits > 0;
}

// Count of living companions in the four slots (R22, R25.3).
i32 livingCompanionCount(const CompanionSlots& slots) {
    i32 n = 0;
    for (i32 i = 0; i < kCompanionSlotsPerCharacter; ++i) {
        const Companion& c = slots[static_cast<u32>(i)];
        // UNKNOWN: isLiving source not provided. Predicate inferred from tests 52/53:
        // occupied, not dead, and hits > 0 means living.
        if (c.occupied && !c.isDead && c.hits > 0) {
            n += 1;
        }
    }
    return n;
}

// R23.1 VB precedence: bit3 Or (bit4 And Not Breathed).
// Quirk: a fire breather is never stopped by an earlier breath; only cold is.
bool companionBreathGuard(bool fireBit, bool coldBit, bool coldBlocked) {
    return fireBit || (coldBit && !coldBlocked);
}

// R23.2: roll percent for the breath. "By more than a quarter" is read as a ratio.
// UNKNOWN: ratio versus difference (see R60); in-between case uses the base roll of 50.
i32 companionBreathPercent(i32 companionGuildLevel, i32 leadFoeGuildLevel) {
    const i64 comp = companionGuildLevel;
    const i64 lead = leadFoeGuildLevel;
    if (4 * lead > 5 * comp) {
        return kBreathChanceStrong;  // 80: lead foe exceeds companion by more than a quarter
    }
    if (4 * lead < 3 * comp) {
        return kBreathChanceWeak;  // 20: lead foe below companion by more than a quarter
    }
    return kBreathChanceBase;
}

// R23: companion breath. Gate, percent roll, then the mishap check (R24).
// UNKNOWN: the lead foe guild level, party size and the monster-group sweep (R23.3) need
// GameState access, which this signature does not expose. The lead foe level is taken as
// the companion's own level and the sweep is not performed here.
bool companionBreath(Ctx& ctx, GameState& game, Character& owner, Companion& c, bool coldBlocked) {
    (void)game;
    (void)owner;
    const bool fire = flagBit(c.specialPropertyFlags, 3);  // R23.1 fire bit
    const bool cold = flagBit(c.specialPropertyFlags, 4);  // R23.1 cold bit
    if (!companionBreathGuard(fire, cold, coldBlocked)) {
        return false;
    }
    const i32 pct = companionBreathPercent(c.guildLevel, c.guildLevel);  // UNKNOWN lead foe GL
    if (!ctx.rng.percent(pct)) {
        return false;
    }
    const i32 partySize = 1;  // UNKNOWN: party size not reachable here
    const f64 chance = mishapChance(MishapKind::Breath, c.guildLevel, countBodies(partySize, 1));
    if (rollMishap(ctx, chance)) {
        const i32 victim = drawMishapVictim(ctx, partySize);
        if (victim != c.owner) {
            // R23.4: lands on the party; no sweep. Fatal caption ordering not modelled.
            ctx.say("companion.breath.mishap", "The breath strikes the party.", {}, {});
            return true;
        }
    }
    // R23.3: sweep of monster groups not performed here (UNKNOWN, see above).
    ctx.say("companion.breath.sweep", "The breath is released at the monsters.", {}, {});
    return true;
}

// Companion breath special range (R23, c64):
//   a = atk + 1
//   range = Int((Log(a) + a/15) / Log(1.08) - tgt/10 + 1)
i32 companionBreathRange(i32 atk, i32 tgt) {
    const f64 a = static_cast<f64>(atk + 1);
    const f64 v = (std::log(a) + a / kBreathSeedDivisor) / std::log(kBreathLogBase)
                  - static_cast<f64>(tgt) / 10.0 + 1.0;
    return static_cast<i32>(vbInt(v));
}

// Monster's own breath range (sub0E85, c64), deliberately different from the companion one:
//   seed = Round(monLvl * Log(monLvl+1), ToEven)
//   range = Int((Log(max(1,seed)) + seed/15)/Log(1.08) - charLevel/20 + 1)
i32 monsterBreathRange(i32 monLvl, i32 charLevel) {
    const f64 m = static_cast<f64>(monLvl);
    const f64 seed = static_cast<f64>(roundHalfEven(m * std::log(m + 1.0)));
    const f64 safeSeed = seed < 1.0 ? 1.0 : seed;
    const f64 v = (std::log(safeSeed) + seed / kBreathSeedDivisor) / std::log(kBreathLogBase)
                  - static_cast<f64>(charLevel) / 20.0 + 1.0;
    return static_cast<i32>(vbInt(v));
}

// R24: chance = clamp((999 / max(1, guildLevel)) * (bodies / 20) * factor, low, high).
f64 mishapChance(MishapKind kind, i32 guildLevel, i32 bodies) {
    const MishapRow& row = kMishapRows[static_cast<i32>(kind)];
    const f64 gl = static_cast<f64>(maxv(1, guildLevel));
    const f64 raw = (999.0 / gl) * (static_cast<f64>(bodies) / 20.0) * row.factor;
    return clampv(raw, row.low, row.high);
}

// R24 bodies: every character plus every living companion in slots 0 to 3.
i32 countBodies(i32 partySize, i32 livingCompanions) {
    return partySize + livingCompanions;
}

// R24: if rng()*100 < chance then a mishap happens.
bool rollMishap(Ctx& ctx, f64 chance) {
    return ctx.rng.nextDouble() * 100.0 < chance;
}

// R24 victim: drawn = round_half_even(rng*(partySize-1)) + 1.
// Returned here as a 0-based character index (drawn - 1); the caller treats a victim equal
// to the owner as no mishap.
// Quirk: the end indices get half the weight of the middle ones (round-half-even).
i32 drawMishapVictim(Ctx& ctx, i32 partySize) {
    const f64 r = ctx.rng.nextDouble() * static_cast<f64>(partySize - 1);
    return static_cast<i32>(roundHalfEven(r));
}

// R25.3: spit target. Chance on a companion is count/(count+1). Slot is drawn until a living
// companion is found, capped at kSpitRetryCap (port). The count is taken from the attacker's
// companions and the slot is indexed into the victim's list, as in the original (quirk).
// CONFLICT: VB And does not short-circuit; the C++ version draws the roll only when count > 0.
i32 drawSpitTarget(Ctx& ctx, const CompanionSlots& attackerSlots, const CompanionSlots& victimSlots) {
    // RESOLVED-BY-REFERENCE: Combat.cs:1467 (CompanionSpits): "the slot ... is read in the victim's
    // list with no test of its own: a fallen companion lying there is spat on like a living one, and
    // so is an empty slot". The victim-side liveness test and retry loop are removed.
    (void)victimSlots;
    const i32 count = livingCompanionCount(attackerSlots);
    if (count <= 0) {
        return -1;
    }
    const f64 chance = static_cast<f64>(count) / static_cast<f64>(count + 1) * 100.0;
    if (!(ctx.rng.nextDouble() * 100.0 < chance)) {
        return -1;
    }
    // UNKNOWN: the reference draws the slot from the attacker's living companions (CompanionSpit.Slot,
    // not supplied). The uniform draw over all slots is kept. The audit (SRD ch11, c30) says the original
    // draws on slots 1..4 with slot 4 nonexistent; not applied, because callers index the array directly.
    return static_cast<i32>(ctx.rng.below(kCompanionSlotsPerCharacter));
}

// R25: spit (bit 5). No once-a-round guard. Off a mishap it hits the front of the owner's
// group with no sweep. UNKNOWN: party size, the owner's slot list and the monster group
// are not reachable through this signature, so the landing effect is only announced.
void companionSpit(Ctx& ctx, GameState& game, Character& owner, Companion& c) {
    (void)game;
    (void)owner;
    if (!flagBit(c.specialPropertyFlags, 5)) {  // bit 5: spit
        return;
    }
    const i32 partySize = 1;  // UNKNOWN: party size not reachable here
    const f64 chance = mishapChance(MishapKind::Spit, c.guildLevel, countBodies(partySize, 1));
    if (rollMishap(ctx, chance)) {
        const i32 victim = drawMishapVictim(ctx, partySize);
        if (victim != c.owner) {
            ctx.say("companion.spit.mishap", "The spit lands on the party.", {}, {});
            return;  // R25: a mishap ends the routine before its line
        }
    }
    ctx.say("companion.spit", "The companion spits at the foe.", {}, {});
}

// R26: every companion makes the draw at the spit odds (quirk: not only spitters).
// A mishap draws a victim; the owner is never the victim (R24).
// UNKNOWN: the victim slot draw needs the victim's slot list, which this signature omits.
bool companionFriendlyFire(Ctx& ctx, GameState& game, Character& owner, Companion& c) {
    (void)game;
    (void)owner;
    const i32 partySize = 1;  // UNKNOWN: party size not reachable here
    const f64 chance = mishapChance(MishapKind::Spit, c.guildLevel, countBodies(partySize, 1));
    if (!rollMishap(ctx, chance)) {
        return false;
    }
    const i32 victim = drawMishapVictim(ctx, partySize);
    if (victim == c.owner) {
        return false;
    }
    ctx.say("companion.friendlyFire", "A companion strikes a party member.", {}, {});
    return true;
}

// R27 (spell or blow) is not part of this SRD excerpt.
// UNKNOWN: no effect until R27 is specified.
void companionSpellOrBlow(Ctx& ctx, GameState& game, Character& owner, Companion& c) {
    (void)ctx;
    (void)game;
    (void)owner;
    (void)c;
}

}  // namespace mordor
