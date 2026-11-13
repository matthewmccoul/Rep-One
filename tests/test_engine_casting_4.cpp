#include "test_support.h"

// Cases B_0061#67..#76: Spellbook.Cost for Human Nomad 37 / Sorcerer 37.
// SpellDef name/id fields are not in the provided headers, so the named spells
// cannot be looked up in tsup::data().spells. Without that, the inputs cannot be set up.
// SKIP B_0061#67: cannot select Mass Sight Veil from data().spells (SpellDef name/id fields not in headers)
// SKIP B_0061#68: cannot select Sphere of Flames from data().spells (SpellDef name/id fields not in headers)
// SKIP B_0061#69: cannot select Flesh to Stone from data().spells (SpellDef name/id fields not in headers)
// SKIP B_0061#70: cannot select Paralyzing Death from data().spells (SpellDef name/id fields not in headers)
// SKIP B_0061#71: cannot select Poison from data().spells (SpellDef name/id fields not in headers)
// SKIP B_0061#72: cannot select Leprosy from data().spells (SpellDef name/id fields not in headers)
// SKIP B_0061#73: cannot select Acidic Spray from data().spells (SpellDef name/id fields not in headers)
// SKIP B_0061#74: cannot select Ice Spray from data().spells (SpellDef name/id fields not in headers)
// SKIP B_0061#75: cannot select Hail Storm from data().spells (SpellDef name/id fields not in headers)
// SKIP B_0061#76: cannot select Arctic Storm from data().spells (SpellDef name/id fields not in headers)
