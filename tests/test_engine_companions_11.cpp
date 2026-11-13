#include "test_support.h"

// Every case in this chunk asserts a count over a seeded simulation (many
// rounds, kills, or blows) and/or the order and recipients of event-log
// captions. The headers expose single-step helpers (mishapDeathCry,
// mishapAttackCry, rollMishap, companionCastFizzles) but no kill, blow, or
// round loop, and no way to query which listener hears a cry. None of these
// can be asserted faithfully against the reference expectations, so every
// case is skipped.

// SKIP B_0065#5: needs a full kill sequence and event-log ordering; no kill-loop API in headers
// SKIP B_0065#6: needs a full kill sequence and per-listener sound recipients; not exposed
// SKIP B_0065#7: needs per-listener sound recipients under No Party Sound; not exposed
// SKIP B_0065#8: needs per-listener sound recipients under No Party Sound; not exposed
// SKIP B_0065#9: 4000-round count of accidental hits without kill; the dragon/undead setup and kill outcome are not expressible with the headers
// SKIP B_0065#10: needs a blow sequence and per-listener DRAGON cry counts; not exposed
// SKIP B_0065#11: needs a blow sequence and per-listener sound recipients under No Party Sound; not exposed
// SKIP B_0065#12: needs a blow sequence and per-listener sound recipients under No Party Sound; not exposed
// SKIP B_0065#56: 200-round caption count for 'Comp. #1' requires a full combat round and caption output; no round API
// SKIP B_0065#57: counts all non-Opening captions across full rounds; no round or caption-enumeration API
