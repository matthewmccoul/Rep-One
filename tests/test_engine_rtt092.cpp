#include "test_support.h"

using namespace mordor;

// SKIP B_0061#105: no declaration for CombatEngine::RoundPaced, Spell or the Sorcerer fixture in our headers; RNG-draw comparison needs the round driver (NEED: CombatEngine, Spell)
// SKIP B_0061#106: no declaration for CombatEngine::RoundPaced or an anti-magic square callback in our headers (NEED: CombatEngine, anti-magic hook)
// SKIP B_0065#36: no declaration for CombatEngine::RoundPaced(sp) or standing-order handling in our headers (NEED: CombatEngine, Party::OrderFor)
// SKIP B_0065#37: no declaration for CombatEngine::RoundPaced(sp) or the named "doesn't have enough spell points" message (NEED: CombatEngine, EventLog message codes)
// SKIP B_0065#39: no declaration for Party::OrderFor or the m1 target field (NEED: Party, OrderChoice fields)
// SKIP B_0061#86: no declaration for CombatOrder::CheckCastClearsAtt; our checkCast(Ctx&, Character&) in clock.h has a different signature and no declared order state (NEED: CombatOrder, OrderChoice fields)
// SKIP B_0061#87: no declaration for CombatOrder::CheckCastClearsAtt or group-aim field (NEED: CombatOrder, OrderChoice fields)
// SKIP B_0061#88: no declaration for CombatOrder::CheckCastClearsAtt (NEED: CombatOrder, OrderChoice fields)
