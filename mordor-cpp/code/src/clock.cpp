// src/clock.cpp
// Implements include/mordor/clock.h. Rule numbers (R, P) refer to the c06p1 SRD part.
#include "mordor/clock.h"

#include <cmath>

namespace mordor {

namespace {

// R7 / P8: the original's play clock uses 86340 where a day has 86400, so each midnight crossed loses a minute [c17].
constexpr i64 kPlayDayWrapSeconds = 86340;

// Indices into Abilities::stats follow StatIndex (Str=0 ... Cha=4, Dex=5).
constexpr i32 kStatIdxCha = static_cast<i32>(StatIndex::Cha);
constexpr i32 kStatIdxDex = static_cast<i32>(StatIndex::Dex);

// P1 timer values. The form designs 1500 ms; the code resets to 1000 on dungeon entry [c25].
constexpr i32 kCombatTickMs = 1000;
constexpr i32 kAilmentTickMs = 4000;
constexpr i32 kPlayTickMs = 250;

// sub0A64 caller ids. 0 = monsters' own tick (no roll). 1 = Moved (floor only). 2 = ChangedLevel (cap 98) [c27].
constexpr i32 kCallerMonsterTick = 0;
constexpr i32 kCallerMoved = 1;
constexpr i32 kCallerChangedLevel = 2;

// onMove arguments: 0 = same area (none), 1 = Moved (R21), 2 = ChangedLevel (R22) [c23].
constexpr i32 kMoveSameArea = 0;
constexpr i32 kMoveArgMoved = 1;
constexpr i32 kMoveArgChangedLevel = 2;

// UNKNOWN: "standing" monster group is taken as livingCount > 0; the state headers do not define it further.
bool groupStanding(const GameState& game, i32 g) {
    if (g < 0 || g >= kMaxGroups) {
        return false;
    }
    return game.encounter.groups[static_cast<u32>(g)].livingCount > 0;
}

// R4 helper: index of the first living party member, or -1.
i32 firstLivingIndex(const GameState& game) {
    for (i32 i = 0; i < game.party.size; ++i) {
        if (!game.party.members[static_cast<u32>(i)].isDead()) {
            return i;
        }
    }
    return -1;
}

bool isCounted(const GameState& game, i32 slot) {
    for (i32 v : game.countedDeadSlots) {
        if (v == slot) {
            return true;
        }
    }
    return false;
}

// R25: clock stops, sub0A64 runs once, clock restarts [c25].
void freeRoundAs(Ctx& ctx, GameState& game, i32 caller) {
    game.combatClockRunning = false;
    monstersFreeAttack(ctx, game, caller);
    game.combatClockRunning = true;
}

}  // namespace

// R1, R5, P2: one round per combat tick, below ground and not minimised; guarded against overlap.
void combatTick(Ctx& ctx, GameState& game) {
    if (!game.underground || game.minimised) {  // R1 / quirk: minimising gates the timer [c23, c25]
        return;
    }
    if (!game.combatClockRunning) {
        return;
    }
    if (game.inCombatRound) {  // R5: no overlapping rounds
        return;
    }
    game.inCombatRound = true;
    playerRound(ctx, game);  // R2-R4 (P2.2)
    // P2.3: retarget living members again before monsters act [c5].
    for (i32 i = 0; i < game.party.size; ++i) {
        Character& m = game.party.members[static_cast<u32>(i)];
        if (!m.isDead()) {
            retarget(ctx, game, m);
        }
    }
    noteDeadAndSendUpdate(ctx, game);                    // P2.4
    monstersFreeAttack(ctx, game, kCallerMonsterTick);   // P2.5, R3: monsters act after the party, no roll [c27]
    game.inCombatRound = false;                          // P2.6
}

// R2, R4: each living character in party order, then that character's companions.
void playerRound(Ctx& ctx, GameState& game) {
    for (i32 i = 0; i < game.party.size; ++i) {
        Character& who = game.party.members[static_cast<u32>(i)];
        if (who.isDead()) {
            continue;
        }
        characterAct(ctx, game, who);   // R2: standing order first
        companionsAct(ctx, game, who);  // R2: then companions; R6: runs even when paralysed
        if (!roundGuard(ctx, game)) {   // P2.2e: If fn0EEF() = False Then Exit For [c29]
            return;
        }
    }
}

// R6, R15: retarget, then the standing order unless paralysed.
void characterAct(Ctx& ctx, GameState& game, Character& who) {
    retarget(ctx, game, who);            // R15 (P2.2b)
    if (who.ailments.paralysed) {        // R6: paralysis skips only this character's own action
        return;
    }
    switch (who.order.kind) {
        case CombatOrderKind::Fight:
            // UNKNOWN: melee resolution lives in another module, not in this API index; this module dispatches the order.
            ctx.say("clock.order_fight", "Fight order", {static_cast<i64>(who.order.aimedGroup)}, {who.name});
            break;
        case CombatOrderKind::Defend:
            // R9 / quirk: Defend is refused when paralysed; the early return above covers that.
            ctx.say("clock.order_defend", "Defend order", {}, {who.name});
            break;
        case CombatOrderKind::Buffer:
            // R9: the SRD "Spell" order maps to CombatOrderKind::Buffer. Cast is handled by the spell module.
            ctx.say("clock.order_spell", "Spell order", {static_cast<i64>(who.order.spellId)}, {who.name});
            break;
        case CombatOrderKind::None:
        default:
            break;
    }
}

// R6: companions act even when the owner is paralysed.
void companionsAct(Ctx& ctx, GameState& game, Character& owner) {
    // UNKNOWN: companion records are not reachable from the state given to this module.
    // Only the party-level alive count is known, so this emits the dispatch event when companions are alive.
    if (game.party.aliveCompanions <= 0) {
        return;
    }
    ctx.say("clock.companions_act", "Companions act", {static_cast<i64>(game.party.aliveCompanions)}, {owner.name});
}

// R4 (fn0EEF): false when no monsters remain, so the round exits.
bool roundGuard(const Ctx& ctx, const GameState& game) {
    (void)ctx;
    for (i32 g = 0; g < kMaxGroups; ++g) {
        if (game.encounter.groups[static_cast<u32>(g)].livingCount > 0) {
            return true;
        }
    }
    return false;
}

// P2.4: note newly dead members and send the party list update (modUpdateCharacterList).
void noteDeadAndSendUpdate(Ctx& ctx, GameState& game) {
    for (i32 i = 0; i < game.party.size; ++i) {
        const Character& m = game.party.members[static_cast<u32>(i)];
        if (m.isDead() && !isCounted(game, i)) {
            game.countedDeadSlots.push_back(i);
        }
    }
    // The list update is sent on each call (P2.4). modUpdateCharacterList is a mod hook; emitted as an event.
    ctx.say("clock.party_list_update", "Party list update", {static_cast<i64>(game.party.size)}, {});
}

// R28-R30, P8: PlayClock.Shared.Tick over the members with the current time.
void playClockTick(Ctx& ctx, Vec<Character>& members, bool paused, i64 nowSeconds) {
    (void)ctx;
    const i32 n = static_cast<i32>(members.size());
    for (i32 i = 0; i < n; ++i) {
        Character& m = members[static_cast<u32>(i)];
        if (m.playStart == 0 || m.playSeconds == -1) {  // R30: set start, return early
            m.playStart = nowSeconds;
            return;                                     // later members that would tick are skipped [c17]
        }
        if (paused) {                                   // R29: paused time is not counted, start still advances
            m.playStart = nowSeconds;
            continue;
        }
        // R7 gap formula, using the original's 86340 wrap [c17].
        const i64 gap = (nowSeconds < m.playStart)
                            ? (kPlayDayWrapSeconds - m.playStart) + nowSeconds
                            : nowSeconds - m.playStart;
        // R7: PlaySeconds = round(PlaySeconds + gap, 4).
        // UNKNOWN: PlaySeconds is i64 whole seconds in the state, so the 4-place rounding is a no-op here.
        m.playSeconds = m.playSeconds + gap;
        m.playStart = nowSeconds;
    }
}

// R8: ailment timer, guarded by inAilment (mirrors m0026).
void ailmentTick(Ctx& ctx, GameState& game) {
    if (game.inAilment) {
        return;
    }
    game.inAilment = true;
    // UNKNOWN: ailment effects (poison, disease, paralysis countdown) are resolved by the ailment module.
    // This module owns the timer and the re-entrancy guard only.
    ctx.say("clock.ailment_tick", "Ailment tick", {static_cast<i64>(game.ailmentIntervalMs)}, {});
    game.inAilment = false;
}

// P1: start-up timers. Combat 1000 ms, ailment 4000 ms, play 250 ms.
void clockSetup(Ctx& ctx, GameState& game) {
    // CONFLICT: form design 1500 ms vs code reset 1000 ms [c25]. Operative value: 1000 ms.
    game.combatIntervalMs = kCombatTickMs;   // P1.1
    game.ailmentIntervalMs = kAilmentTickMs; // P1.2
    game.playIntervalMs = kPlayTickMs;       // P1.3
    ctx.say("clock.setup", "Clock setup",
            {static_cast<i64>(kCombatTickMs), static_cast<i64>(kAilmentTickMs), static_cast<i64>(kPlayTickMs)}, {});
}

// R10, R11: standing orders set from CombatSetting. No initiative roll and no first mover.
void startFight(Ctx& ctx, GameState& game) {
    for (i32 i = 0; i < game.party.size; ++i) {
        Character& m = game.party.members[static_cast<u32>(i)];
        // REFERENCE (Party.ApplyDefaultOrders): only a member with no order yet takes up the setting
        // ("if (o.Kind != OrderKind.None) continue;"); members with an order keep it.
        if (m.order.kind != CombatOrderKind::None) {
            continue;
        }
        // UNKNOWN: CombatSetting is not carried by the Character state headers; 0 maps to None.
        const i32 combatSetting = 0;
        const OrderChoice choice = orderFromSetting(ctx, combatSetting, m.order.spellBuffers);  // R10
        m.order.kind = choice.kind;
        m.order.spellId = choice.spellId;
    }
}

// R10 CombatSetting table.
OrderChoice orderFromSetting(const Ctx& ctx, i32 combatSetting, const Arr<i32, kSpellBuffers>& buffers) {
    (void)ctx;
    OrderChoice out;
    if (combatSetting == kSettingFight) {        // 1 -> Fight
        out.kind = CombatOrderKind::Fight;
        return out;
    }
    if (combatSetting == kSettingDefend) {       // 2 -> Defend
        out.kind = CombatOrderKind::Defend;
        return out;
    }
    if (combatSetting >= kSettingBufferBase) {   // >= 3 -> buffer (setting - 3)
        const i32 idx = combatSetting - kSettingBufferBase;
        // CONFLICT: buffer 10 is stored as setting 13, but only kSpellBuffers (10) buffers exist. Out-of-range gives None.
        if (idx >= kSpellBuffers) {
            return out;
        }
        const i32 held = buffers[static_cast<u32>(idx)];
        // UNKNOWN: buffer encoding. 0 = empty; positive = spell id used as spells[idx].Id; negative = pack item (Use, caller handles).
        if (held > 0) {
            out.kind = CombatOrderKind::Buffer;
            out.spellId = held;
        } else if (held < 0) {
            out.useItem = true;
        }
    }
    return out;  // other values -> None
}

// R10: clear standing orders at fight end. CombatSetting is unchanged.
void endFight(Ctx& ctx, GameState& game) {
    (void)ctx;
    for (i32 i = 0; i < game.party.size; ++i) {
        Character& m = game.party.members[static_cast<u32>(i)];
        m.order.kind = CombatOrderKind::None;
        m.order.spellId = 0;
    }
    // UNKNOWN: fight-end flags. Simplest sensible choice: the fight is over and no round is running.
    game.inCombat = false;
    game.inCombatRound = false;
}

// R12, P4.1: Fight and Defend become None. A queued spell and its target are kept.
void checkCast(Ctx& ctx, Character& who) {
    (void)ctx;
    if (who.order.kind == CombatOrderKind::Fight || who.order.kind == CombatOrderKind::Defend) {
        who.order.kind = CombatOrderKind::None;
    }
}

// R13, P4.2: opening a spell list sets Att to false, then the cast check applies.
void openSpellList(Ctx& ctx, Character& who) {
    // UNKNOWN: "Att" is taken as GuildMembership.att of the active guild; false is 0.
    who.guilds[static_cast<u32>(who.activeGuild)].att = 0;
    checkCast(ctx, who);  // Fight and Defend become None; queued spell kept
}

// R14, P5: Sub102F(-1). Kind = Defend if None or Fight, else Fight. Queued spell dropped.
void attReorder(Ctx& ctx, Character& who, i32 delta) {
    (void)ctx;
    // UNKNOWN: delta is the Att -1 trigger value; the formula changes only Kind and SpellId, so delta is not applied.
    (void)delta;
    const bool toDefend = (who.order.kind == CombatOrderKind::None || who.order.kind == CombatOrderKind::Fight);
    who.order.kind = toDefend ? CombatOrderKind::Defend : CombatOrderKind::Fight;
    who.order.spellId = 0;
}

// R15: if the aimed group has no standing monsters, aim at the first standing group.
void retarget(Ctx& ctx, GameState& game, Character& actor) {
    const i32 aimed = actor.order.aimedGroup;
    if (groupStanding(game, aimed)) {
        return;
    }
    const i32 first = standingGroupIndex(ctx, game, -1);
    if (first >= 0) {
        actor.order.aimedGroup = first;
    }
}

// R15 fn0DCD: index of the first standing monster group, or -1 when none.
i32 standingGroupIndex(const Ctx& ctx, const GameState& game, i32 delta) {
    (void)ctx;
    // UNKNOWN: effect of delta on the search is not stated; the search always starts at group 0.
    (void)delta;
    for (i32 g = 0; g < kMaxGroups; ++g) {
        if (groupStanding(game, g)) {
            return g;
        }
    }
    return -1;
}

// R16-R18, P6: encounter timer.
void engagementTick(Ctx& ctx, GameState& game) {
    // UNKNOWN: countdown semantics. A positive countdown is decremented and the tick waits.
    if (game.encounter.countdown > 0) {
        game.encounter.countdown -= 1;
        return;
    }
    const i32 lead = standingGroupIndex(ctx, game, 0);
    if (lead < 0) {
        return;
    }
    MonsterGroup& grp = game.encounter.groups[static_cast<u32>(lead)];

    if (grp.state == GroupState::Hostile) {
        // R17: a hostile waiting encounter attacks on the transition tick.
        // UNKNOWN: caller id for timer attacks; the monsters' own tick (0, no roll) is used.
        monstersFreeAttack(ctx, game, kCallerMonsterTick);
        return;
    }
    if (grp.state == GroupState::Peaced) {
        if (grp.joinOffer) {  // R18: a peaced encounter with a pending join offer stays put
            return;           // UNKNOWN: definition of join offer state
        }
        const i32 sel = game.party.selectedIndex;
        if (sel < 0 || sel >= game.party.size) {
            return;
        }
        Character& who = game.party.members[static_cast<u32>(sel)];
        // R19: lead monster and selected character, rolled against the hostile base formula.
        const i32 base = hostileBase(grp.front.align, who.alignment);
        const i32 cha = who.abilities.stats[static_cast<u32>(kStatIdxCha)];
        if (hostileRoll(ctx, base, cha)) {
            // UNKNOWN: effect of a successful roll. Assumed to make the group hostile; it attacks on a later tick.
            grp.state = GroupState::Hostile;
        }
    }
    // GroupState::Engaged: no timer action in the SRD text.
}

// R19: hostile base table lookup. Rows monster alignment, columns character alignment [c23].
i32 hostileBase(CharAlign monsterAlign, Align charAlign) {
    return kHostileBase[static_cast<i32>(monsterAlign)][static_cast<i32>(charAlign)];
}

// R19: T = trunc(base - 7.5 * ln(max(1, cha))). True if draw 1..100 is strictly below T.
bool hostileRoll(Ctx& ctx, i32 base, i32 cha) {
    const i32 effCha = (cha < 1) ? 1 : cha;  // max(1, cha)
    const double x = static_cast<double>(base) - kHostileLnCoef * std::log(static_cast<double>(effCha));
    const i32 threshold = static_cast<i32>(x);  // truncation toward zero, as the SRD trunc()
    const i32 draw = static_cast<i32>(ctx.rng.below(kHostileRollMax)) + 1;  // 1..100
    return draw < threshold;
}

// R22, R26: max(5, 50 + 5 * (monsterDex - charDex)); capped at 98 only when cap is set.
i32 freeAttackThreshold(i32 monsterDex, i32 charDex, bool cap) {
    i32 t = kFreeAttackBase + kFreeAttackPerDex * (monsterDex - charDex);
    if (t < kFreeAttackFloor) {
        t = kFreeAttackFloor;
    }
    if (cap && t > kFreeAttackCap) {
        t = kFreeAttackCap;  // R22: cap applies only on ChangedLevel
    }
    return t;
}

// R25-R27, sub0A64. Caller 0 = monsters' own tick: every group attacks, no roll.
// Callers 1 and 2: each standing group draws separately against a character.
void monstersFreeAttack(Ctx& ctx, GameState& game, i32 caller) {
    const i32 size = game.party.size;
    if (size <= 0) {
        return;
    }
    const bool capped = (caller == kCallerChangedLevel);  // cap 98 only on ChangedLevel [c27]

    // RESOLVED-BY-REFERENCE: target weights are 100, 50, 25, 12, 6, 3, 1, 0, halved as integers,
    // summed once over the party as it stood when the monsters began (MULTIMOD.BAS:305-308).
    double total = 0.0;
    i32 share = 100;
    for (i32 i = 0; i < size; ++i) {
        total += static_cast<double>(share);
        share /= 2;
    }

    for (i32 g = 0; g < kMaxGroups; ++g) {
        if (!groupStanding(game, g)) {
            continue;
        }
        // REFERENCE: UBound(Chars) is read afresh for every group (MULTIMOD.BAS:320-324).
        const i32 last = game.party.size - 1;
        if (last < 0) {
            return;
        }
        const MonsterGroup& grp = game.encounter.groups[static_cast<u32>(g)];

        // REFERENCE: the target draw happens for every group, whatever the caller.
        // The front of the party draws the fire; the cumulative weight walk is ported literally.
        i32 target = 0;
        const i32 roll = static_cast<i32>(ctx.rng.below(kHostileRollMax)) + 1;  // 1..100
        i32 acc = 100;
        i32 step = 100;
        i32 guard = 0;
        while (static_cast<double>(acc) / total * 100.0 < static_cast<double>(roll) && guard < 32) {
            ++guard;
            ++target;
            if (target > last) {
                target = last;
            }
            step /= 2;
            acc += step;
        }
        if (target > last) {
            target = last;
        }
        const Character& who = game.party.members[static_cast<u32>(target)];
        // UNKNOWN: REFERENCE says a dead member can be picked and then the group wastes its turn.
        // The alive field is not in the shown Character headers, so that skip is not applied here.

        if (caller == kCallerMonsterTick) {
            // R26: monsters' own tick, no roll; every group goes.
            ctx.say("clock.monster_attack", "Monster group attacks",
                    {static_cast<i64>(g), static_cast<i64>(target)}, {});
            continue;
        }
        // R22/R26: per-group DEX free-attack roll (REFERENCE: Moved and ChangedLevel paths).
        const i32 threshold = freeAttackThreshold(grp.front.dex, who.abilities.stats[static_cast<u32>(kStatIdxDex)], capped);
        const i32 draw = static_cast<i32>(ctx.rng.below(kHostileRollMax)) + 1;  // 1..100
        if (draw < threshold) {
            ctx.say("clock.monster_attack", "Monster group attacks",
                    {static_cast<i64>(g), static_cast<i64>(target), static_cast<i64>(threshold)}, {});
        } else {
            ctx.say("clock.monster_miss", "Monster group does not attack",
                    {static_cast<i64>(g), static_cast<i64>(target), static_cast<i64>(threshold)}, {});
        }
    }
    // UNKNOWN: damage resolution is in the combat-resolution module; this module reports the attack decisions.
}

// R25: one free round for engaged monsters.
void freeRound(Ctx& ctx, GameState& game) {
    // UNKNOWN: caller id for the free round; Moved (1) assumed since no argument is given.
    freeRoundAs(ctx, game, kCallerMoved);
}

// R21-R23: movement triggers. Same area: none. Moved: no cap. ChangedLevel: cap 98.
void onMove(Ctx& ctx, GameState& game, i32 arg) {
    if (arg == kMoveSameArea) {  // R23
        return;
    }
    if (!game.party.engaged) {   // R25: a free round applies only while engaged
        return;
    }
    if (arg == kMoveArgMoved) {  // R21: hostile room arrival, argument 1, uncapped
        freeRoundAs(ctx, game, kCallerMoved);
        return;
    }
    if (arg == kMoveArgChangedLevel) {  // R22: departure or level change, argument 2, capped at 98
        freeRoundAs(ctx, game, kCallerChangedLevel);
    }
}

// R31, P9.1: display wait on each side of a melee result.
void meleeHold(Ctx& ctx, i32 ms) {
    // The core never reads the clock, so the host performs the wait; the core reports the requested hold.
    // CONFLICT: port uses 250 ms each side; the VB3 trace describes a 500 ms hold per group result [c23]. The caller passes ms.
    ctx.say("clock.melee_hold", "Melee hold", {static_cast<i64>(ms)}, {});
}

// R34, P9.4: key pressed while a won round is still drawn.
void endCombat(Ctx& ctx, GameState& game) {
    endFight(ctx, game);            // R10: clear standing orders; CombatSetting unchanged
    game.inCombatRound = false;
    // Keys pressed during a hold were queued. The host replays them in order; the core reports each one.
    for (i32 k : game.queuedKeys) {
        ctx.say("clock.replay_key", "Replay queued key", {static_cast<i64>(k)}, {});
    }
    game.queuedKeys.clear();
}

}  // namespace mordor
