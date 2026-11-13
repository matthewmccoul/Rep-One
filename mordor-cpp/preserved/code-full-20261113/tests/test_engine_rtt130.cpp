#include "test_support.h"
#include "mordor/state_common.h"

using namespace mordor;

// SKIP B_0065#81: needs CombatEngine::castSpell (Charm Humanoid) and the companion roster API to read the joined monster's hits, max and bind level. Neither is declared in our headers.
// SKIP B_0065#85: needs CombatEngine::castSpell and a companion-slot count on the caster. Our CompanionSlots occupancy API is not declared.
// SKIP B_0065#87: needs CombatEngine::castSpell with a four-companion caster and the event log's last entry. CombatEngine and the log message text for "No room for more Companions!" are not declared.
// SKIP B_0064#73: needs CombatEngine::roundPaced with a companion breath, run over 3000 seeded rounds. CombatEngine is not declared.
// SKIP B_0065#56: needs CombatEngine::roundPaced(csp) over 200 seeded rounds with a companion caster and "Comp. #1" captions. CombatEngine and the caption events are not declared.
// SKIP B_0065#57: needs CombatEngine::roundPaced(csp) and a count of non-Opening caption events. CombatEngine and the caption events are not declared.
// SKIP B_0065#58: needs CombatEngine::roundPaced with two identical companion casters and a per-round cast count. CombatEngine and companion-slot iteration are not declared.
// SKIP B_0065#61: needs CombatEngine::roundPaced on odd runs with a 0-charge item and the "Your item has run out of charges!" caption. CombatEngine and the item-use path are not declared.
