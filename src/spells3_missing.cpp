// castMovementSpell: movement and utility spells (c106 table, R1-R5, R2 effects).
// UNKNOWN items (stat requirements R3, Feather Essence failure R2, Teleport combat R5,
// turn order R12) are not gated here. FloorState accessors are not in the provided
// headers, so rock and bounds checks are placeholders.

#include "mordor/spells3.h"
#include "mordor/ctx.h"
#include "mordor/state_party.h"
#include "mordor/state_character.h"
#include "mordor/state_common.h"

namespace mordor {

MovementCastResult castMovementSpell(Ctx& ctx,
                                     Party& party,
                                     FloorState& floor,
                                     const MovementRequest& req,
                                     MovementSpell spell,
                                     const Location& sanctuary,
                                     bool sanctuarySet) {
    MovementCastResult result{};

    // UNKNOWN: FloorState query API (rock behind wall, square bounds) is not in the provided
    // headers. Placeholder: no square is rock and every square is in bounds until wired in.
    (void)floor;
    auto squareIsRock = [](i32 x, i32 y) -> bool {
        (void)x;
        (void)y;
        return false;  // UNKNOWN: FloorState rock query
    };

    // Caster must be a valid, living party member. ASSUMED: dead casters cannot cast.
    if (req.casterIndex < 0 || req.casterIndex >= party.size) {
        result.outcome = MovementOutcome::NoEffect;
        return result;
    }
    Character& caster = party.members[static_cast<u32>(req.casterIndex)];
    if (caster.isDead()) {
        result.outcome = MovementOutcome::NoEffect;
        return result;
    }

    // ASSUMED: "in front" uses N=(0,-1), E=(1,0), S=(0,1), W=(-1,0). The SRD gives no grid orientation.
    i32 fx = 0;
    i32 fy = 0;
    switch (party.facing) {
        case Facing::N: fy = -1; break;
        case Facing::E: fx = 1; break;
        case Facing::S: fy = 1; break;
        case Facing::W: fx = -1; break;
    }

    // Shared kill routine for PartyKilled (R2 Ethereal Portal, Displacement).
    // ASSUMED: rock death uses DeathState::Rock.
    auto killParty = [&party]() {
        for (i32 i = 0; i < party.size; ++i) {
            Character& c = party.members[static_cast<u32>(i)];
            if (!c.isDead()) {
                c.deathState = DeathState::Rock;
            }
        }
        party.wiped = true;
    };

    switch (spell) {
        case MovementSpell::DetectRock: {
            // R2 Detect Rock: reports only the wall in front of the caster (c106).
            const i32 wx = party.x + fx;
            const i32 wy = party.y + fy;
            result.rockBehind = squareIsRock(wx, wy);  // UNKNOWN: always false until query wired
            ctx.say("spell.movement.detect_rock",
                    "The caster senses whether rock lies behind the wall ahead.",
                    {static_cast<i64>(result.rockBehind ? 1 : 0)},
                    {caster.name});
            result.outcome = MovementOutcome::Applied;
            break;
        }

        case MovementSpell::FeatherEssence: {
            // R2 Feather Essence: levitates one recipient.
            // UNKNOWN: failure condition and chance (R2 says it can fail). No roll is made.
            if (req.targetIndex < 0 || req.targetIndex >= party.size) {
                result.outcome = MovementOutcome::NoEffect;
                break;
            }
            Character& target = party.members[static_cast<u32>(req.targetIndex)];
            if (target.isDead()) {
                result.outcome = MovementOutcome::NoEffect;
                break;
            }
            target.ailments.levitating = true;
            result.levitated = true;
            ctx.say("spell.movement.feather_essence",
                    "The recipient floats above pits, chutes and water.",
                    {static_cast<i64>(req.targetIndex)},
                    {target.name});
            result.outcome = MovementOutcome::Applied;
            break;
        }

        case MovementSpell::EtherealPortal: {
            // R2 Ethereal Portal: moves caster and party through the wall in front.
            // ASSUMED: the destination is two squares ahead (wall square, then landing square).
            // R2: if the square behind the wall is rock, the whole party dies.
            const i32 dx2 = party.x + 2 * fx;
            const i32 dy2 = party.y + 2 * fy;
            if (squareIsRock(dx2, dy2)) {
                killParty();
                result.outcome = MovementOutcome::PartyKilled;
                ctx.say("spell.movement.ethereal_portal_rock",
                        "The party steps into solid rock and is lost.",
                        {}, {});
                break;
            }
            party.x = dx2;
            party.y = dy2;
            ctx.say("spell.movement.ethereal_portal",
                    "The party passes through the wall.",
                    {static_cast<i64>(party.x), static_cast<i64>(party.y)},
                    {});
            result.outcome = MovementOutcome::Applied;
            break;
        }

        case MovementSpell::Displacement: {
            // R2 Displacement: teleports within the current level by the requested offset.
            // R2: outside the dungeon or into rock kills caster and party.
            // UNKNOWN: dungeon bounds are not available; only rock is checked.
            const i32 nx = party.x + req.dx;
            const i32 ny = party.y + req.dy;
            if (squareIsRock(nx, ny)) {
                killParty();
                result.outcome = MovementOutcome::PartyKilled;
                ctx.say("spell.movement.displacement_rock",
                        "The party is displaced into rock and is lost.",
                        {}, {});
                break;
            }
            party.x = nx;
            party.y = ny;
            ctx.say("spell.movement.displacement",
                    "The party is displaced on this level.",
                    {static_cast<i64>(nx), static_cast<i64>(ny)},
                    {});
            result.outcome = MovementOutcome::Applied;
            break;
        }

        case MovementSpell::MassLevitate: {
            // R2 Mass Levitate: Feather Essence for the whole party (c106).
            // UNKNOWN: failure chance is not applied (see Feather Essence).
            i32 count = 0;
            for (i32 i = 0; i < party.size; ++i) {
                Character& c = party.members[static_cast<u32>(i)];
                if (!c.isDead()) {
                    c.ailments.levitating = true;
                    ++count;
                }
            }
            result.levitated = true;
            ctx.say("spell.movement.mass_levitate",
                    "The whole party floats.",
                    {static_cast<i64>(count)},
                    {});
            result.outcome = MovementOutcome::Applied;
            break;
        }

        case MovementSpell::Teleport: {
            // R2 Teleport, non-combat: moves caster and party up to 3 levels (c106).
            // Combat Teleport (R5) is teleportCombatSendAway, not this function.
            // ASSUMED: engaged party routes to the combat path, so no effect here.
            if (party.engaged) {
                result.outcome = MovementOutcome::NoEffect;
                break;
            }
            // Clamp to +/- kTeleportMaxLevels (MovementRequest comment).
            i32 delta = req.levelDelta;
            if (delta > kTeleportMaxLevels) {
                delta = kTeleportMaxLevels;
            }
            if (delta < -kTeleportMaxLevels) {
                delta = -kTeleportMaxLevels;
            }
            // UNKNOWN: floor count bounds for the destination level are not checked.
            party.dungeonLevel += delta;
            result.levelsMoved = delta;
            ctx.say("spell.movement.teleport",
                    "The party is moved between levels.",
                    {static_cast<i64>(delta), static_cast<i64>(party.dungeonLevel)},
                    {});
            result.outcome = MovementOutcome::Applied;
            break;
        }

        case MovementSpell::Sanctuary: {
            // R2 Sanctuary: returns caster and party to the set location.
            // R2: no effect if never set. Ages caster about one year.
            // CONFLICT: manual says caster and party return [c83]; archive says caster returns
            // to "that exact spot" [c105]. Implemented as party return (manual).
            if (!sanctuarySet) {
                result.outcome = MovementOutcome::NoEffect;
                break;
            }
            party.x = sanctuary.x;
            party.y = sanctuary.y;
            party.dungeonLevel = sanctuary.dungeonLevel;
            // ASSUMED: Location.facing 0..3 maps to N, E, S, W.
            switch (sanctuary.facing) {
                case 1: party.facing = Facing::E; break;
                case 2: party.facing = Facing::S; break;
                case 3: party.facing = Facing::W; break;
                default: party.facing = Facing::N; break;
            }
            // ASSUMED: "about one year" = 365 days of aging for the caster.
            constexpr i32 kSanctuaryAgingDays = 365;
            caster.ageDays += kSanctuaryAgingDays;
            result.agedDays = kSanctuaryAgingDays;
            result.sanctuaryReturned = true;
            ctx.say("spell.movement.sanctuary",
                    "The party is returned to the sanctuary.",
                    {static_cast<i64>(kSanctuaryAgingDays)},
                    {caster.name});
            result.outcome = MovementOutcome::Applied;
            break;
        }

        default:
            result.outcome = MovementOutcome::NoEffect;
            break;
    }

    return result;
}

}  // namespace mordor
