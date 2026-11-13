#include "test_support.h"

// SKIP B_0061#92: ItemUse.ChangesStats has no equivalent in the headers. The closest functions (applyStatItem, applyStatModifier) apply stat changes and do not report whether a stat change re-lists spells.
// SKIP B_0062#24: MdrLoader.LoadItems is not declared in any header, so the MDATA3 crest records cannot be loaded.
// SKIP B_0062#25: MdrLoader.LoadItems is not declared in any header, so the MDATA3 crest mismatch check cannot be run.
// SKIP B_0062#26: MdrLoader.LoadItems and LoadSpells are not declared in any header, so the Crest of the Nomad spell cannot be loaded.
// SKIP B_0062#27: MdrLoader.LoadItems and LoadSpells are not declared in any header, so the Crest of the Warrior spell cannot be loaded.
// SKIP B_0062#28: MdrLoader.LoadItems and LoadSpells are not declared in any header, so the Crest of the Paladin spell cannot be loaded.
// SKIP B_0062#29: MdrLoader.LoadItems and LoadSpells are not declared in any header, so the Crest of the Ninja spell cannot be loaded.
// SKIP B_0062#30: MdrLoader.LoadItems and LoadSpells are not declared in any header, so the Crest of the Villain spell cannot be loaded.
// SKIP B_0062#31: MdrLoader.LoadItems and LoadSpells are not declared in any header, so the Crest of the Seeker spell cannot be loaded.
// SKIP B_0062#32: MdrLoader.LoadItems and LoadSpells are not declared in any header, so the Crest of the Thief spell cannot be loaded.
