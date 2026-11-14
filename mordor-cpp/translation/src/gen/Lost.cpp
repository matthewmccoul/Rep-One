#include "core.h"

namespace mc {

cs::String Lost::Realisation(cs::Ref<Character> c) {
    return c->LostPosition && c->LostLevel ? "You realize that you're completely lost..."
                                           : "You realize that you're lost...";
}

bool Lost::LoseDirection(cs::Ref<Character> c, cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    double perception = Rules::BestAbilityChance(c, Perception, c->DungeonLevel, 2, gear);
    if (perception >= rng->NextDouble() * 70 + c->DungeonLevel * 2) return false;
    c->LostPosition = true;
    c->LostFacing = true;
    return true;
}

cs::String Lost::DirectionLostText(bool inAParty) {
    return inAParty ? "The party leader lost their sense of direction!" : "You've lost your sense of direction!";
}

cs::List<cs::String> Lost::TryToWorkItOut(cs::Ref<Character> c, cs::Ref<cs::Random> rng) {
    return TryToWorkItOutWith(c, rng, nullptr);
}

cs::List<cs::String> Lost::TryToWorkItOutWith(cs::Ref<Character> c, cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    auto said = cs::List<cs::String>::New();
    if (!c->IsLost()) return said;

    double perception = Rules::BestAbilityChance(c, Perception, c->DungeonLevel, 2, gear);
    double weight = perception <= 0 ? 0 : cs::Math::Log(perception) * cs::Math::Log(perception);

    if (c->LostLevel && weight / 15 > rng->NextDouble() * 100)
    {
        c->LostLevel = false;
        said.Add("You've figured out what level you're on...");
    }
    if (c->LostFacing && weight / 20 > rng->NextDouble() * 100)
    {
        c->LostFacing = false;
        said.Add("You've figured out what direction you're facing...");
    }
    if (weight / 25 > rng->NextDouble() * 100)
    {
        c->FoundYourself();
        said.Add("You've figured out where you are!");
    }
    return said;
}

cs::String Lost::OnKnownGround(cs::Ref<Character> c) {
    bool wasLost = c->IsLost();
    c->FoundYourself();
    return wasLost ? "You've found yourself!" : cs::String();
}

cs::List<cs::String> Lost::TryToWorkItOut(cs::Ref<Character> c, cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    return TryToWorkItOutWith(c, rng, gear);
}

}  // namespace mc
