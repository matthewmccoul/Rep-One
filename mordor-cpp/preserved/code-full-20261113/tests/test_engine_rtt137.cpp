#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"

using namespace mordor;

// Batch t137: reference EncounterGenerator.AcceptJoin and EncounterGenerator.Generate.
// Our declarations have no encounter generator, no group/monster table types, and no
// Companions/JoinOffer accessors. The only related declaration is
// acceptJoin(Ctx&, const Creature&, Companion&, i32) in companions2a.h, which takes one
// front creature and one companion slot, not a group, so the offer/left/joined cases
// cannot be driven through it. Every case is skipped below.

// SKIP B_0065#93: needs an EncounterGenerator.AcceptJoin over a group with a full companion roster; our acceptJoin takes a Creature/Companion slot, not a group, and no join-message path is exposed for it
// SKIP B_0065#95: needs group-level AcceptJoin with a two-monster mixed group (offer 2, kin first); no group type or joined/left count is declared
// SKIP B_0065#89: needs AcceptJoin over a group with a join offer of 2 and an empty companion list; our acceptJoin has no group parameter and no message output for this case
// SKIP B_0065#91: needs Companions.All binding level after a group join; no Companions list or binding-level accessor is declared
// SKIP B_0065#94: needs JoinOffer on the monster after a refused group join; no monster group or JoinOffer field is declared
// SKIP B_0065#92: needs the remaining group count after a join of two; no group type with Count is declared
// SKIP B_0066#80: needs EncounterGenerator.Generate with a three-group table, seeds 0..2999, level 3 and a character with Str 15/Dex 18; no generator or table type is declared
// SKIP B_0066#81: needs foe-slot references (slot 1 to P, slot 2 to Q) from Generate over the same table; no generator or foe-slot type is declared

