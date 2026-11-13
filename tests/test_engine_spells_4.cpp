#include "test_support.h"

#include "mordor/spells1b.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"

using namespace mordor;

namespace {

struct FateRunStats {
    i32 minHits = 0;
    i32 maxHits = 0;
    double meanHits = 0.0;
    i32 minSpellPoints = 0;
    i32 maxSpellPoints = 0;
};

}  // namespace

// SKIP B_0063#101: DungeonNav.StepThrough has no equivalent; rock layout behind the move cannot be set up from the headers.
// SKIP B_0063#102: DungeonNav.StepThrough has no equivalent; rock layout behind the move cannot be set up from the headers.
// SKIP B_0063#104: Teleporting.ForSpell has no equivalent; a rock destination square cannot be set up from the headers.
// SKIP B_0063#105: SpellEffect.TeleportTo body placement has no equivalent; a rock destination square cannot be set up from the headers.
