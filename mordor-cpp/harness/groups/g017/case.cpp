#include "case_api.h"
#include "mordor/attributes.h"
#include "mordor/items1a.h"
#include "mordor/items1b.h"

namespace mordor {

// Loads the table record for the id. Only the id is copied: the C++ Item and the table record
// are different types, so Item cannot be assigned from the record.
// ASSUMPTION: equipProblem and equipCheck resolve the item definition from it.id (defOf).
// The cursed flag is NOT copied, so the CursedStatsTooLow branch of equipCheck is not
// exercised by these cases.
static bool findItem(const Ctx& ctx, i32 id, Item& out) {
    for (const auto& r : ctx.data.items) {
        if (r.id == id) {
            out = Item{};
            out.id = r.id;
            return true;
        }
    }
    return false;
}

// Builds the character the same way Core's Character.Create does: Human, starting guild Nomad,
// level 1, the six stats from args.
static void buildCharacter(const JsonValue& a, Ctx& ctx, Character& c) {
    Arr<i32, 6> stats{};
    for (int i = 0; i < 6; ++i) {
        char key[3] = {'s', static_cast<char>('0' + i), 0};
        stats[i] = static_cast<i32>(A(a, key).asInt());
    }
    createCharacter(ctx, c, Race::Human, Guild::Nomad, stats);
}

// Canonical text = the reference's EquipProblem string, or "null" when none.
static Str reasonOf(EquipProblem p) {
    switch (p) {
        case EquipProblem::None: return Str("null");
        case EquipProblem::NoFreeHands: return Str("No Free Hands!");
        case EquipProblem::UsingTooMany: return Str("Using too Many!");
        case EquipProblem::ClassRestricted: return Str("Your current guild cannot use this item!");
        case EquipProblem::GuildsCannotUse: return Str("Your currently joined guilds cannot use this item!");
        case EquipProblem::GuildLevelTooLow: return Str("Your guild level is too low to use item!");
        case EquipProblem::AllGuildLevelsTooLow: return Str("All guild levels too low to use item!");
        case EquipProblem::StatsTooLow: return Str("One or more stats are too low!");
    }
    return Str("UNKNOWN");
}

// equipCheck outcomes mapped to the reference text for the same category.
// Outcomes with no reference text are emitted under their own names.
static Str outcomeOf(EquipOutcome o) {
    switch (o) {
        case EquipOutcome::Ok: return Str("null");
        case EquipOutcome::NoFreeHands: return Str("No Free Hands!");
        case EquipOutcome::TooManyWorn: return Str("Using too Many!");
        case EquipOutcome::GuildRestricted: return Str("Your currently joined guilds cannot use this item!");
        case EquipOutcome::ClassRestricted: return Str("Your current guild cannot use this item!");
        case EquipOutcome::StatsTooLow: return Str("One or more stats are too low!");
        case EquipOutcome::SilentRefusal: return Str("SilentRefusal");
        case EquipOutcome::GuildLevelTooLow: return Str("GuildLevelTooLow");
        case EquipOutcome::CursedStatsTooLow: return Str("CursedStatsTooLow");
    }
    return Str("UNKNOWN");
}

// Reference order: hands, worn cap, guild, guild level, stats. Inventory is empty (no worn items).
static Str run_equipProblem(const JsonValue& a, Ctx& ctx) {
    Item it{};
    if (!findItem(ctx, static_cast<i32>(A(a, "id").asInt()), it)) return Str("MISSING_ITEM");
    Character c{};
    buildCharacter(a, ctx, c);
    Inventory inv{};
    const Ctx& cc = ctx;  // selects the const Ctx& overload declared in items1b.h
    return reasonOf(equipProblem(cc, c, inv, it));
}

// storeLike = false so the hands and worn-count checks run, matching the reference.
static Str run_equipCheck(const JsonValue& a, Ctx& ctx) {
    Item it{};
    if (!findItem(ctx, static_cast<i32>(A(a, "id").asInt()), it)) return Str("MISSING_ITEM");
    Character c{};
    buildCharacter(a, ctx, c);
    Inventory inv{};
    return outcomeOf(equipCheck(ctx, c, inv, it, false));
}

const CopyEntry kCopies[] = {
    {"equipProblem", &run_equipProblem},
    {"equipCheck", &run_equipCheck},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
