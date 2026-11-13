// Batch t103. Source cases come from the reference's DungeonNav, SpellEffect,
// SquareInfo and MdrLoader tests. None of the APIs they call is declared in the
// headers provided, so every case is skipped until those declarations exist.
// Expected values are the reference's ground truth and must not be changed to
// match our code once the APIs are available.
#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/types.h"

using namespace mordor;

// SKIP B_0063#102: DungeonNav::StepThrough and the Floor/Character position API are not declared in the headers provided.
// SKIP B_0063#18: SpellEffect::ApplyToCharacter and the Character state flags (levitating, invisible, seeInvisible, protected) are not declared.
// SKIP B_0063#19: SpellEffect::ApplyToCharacter and the Character state flags are not declared.
// SKIP B_0063#20: SpellEffect::ApplyToCharacter and the Character state flags are not declared.
// SKIP B_0063#21: SpellEffect::ApplyToCharacter and the Character state flags are not declared.
// SKIP B_0061#54: SpellEffect::ApplyToCharacter, the Protection field and the guild-level cast path are not declared.
// SKIP B_0062#56: needs SpellEffect::Of, Mdata1.MonsterClasses and MdrLoader::LoadSpells; none are declared.
// SKIP B_0063#105: SpellEffect::TeleportTo is not declared in the headers provided.
