#include "test_support.h"

// SKIP B_0064#64: Special-attack hit reduction by resistance percent has no equivalent function in the headers (specialAttackRawDamage and monsterBreathDamage return damage, not hits remaining after resistance).
// SKIP B_0064#65: Special-attack hit reduction by resistance percent has no equivalent function in the headers (see B_0064#64).
// SKIP B_0064#66: Special-attack hit reduction by resistance percent has no equivalent function in the headers (see B_0064#64).
// SKIP B_0064#67: No function maps a breath type to the resistance it is checked against; resistTypeForClass maps SpellClass, not BreathType, and the expected value is a text message.
// SKIP B_0064#68: No function maps a breath type to the resistance it is checked against (see B_0064#67).
// SKIP B_0064#69: No function maps an acid spit to its resistance; resistTypeForClass has no acid spell class and the expected value is a text message.
// SKIP B_0064#70: No function maps an electrocution to its resistance; the expected value is a text message (see B_0064#69).
// SKIP B_0066#49: Chute.Land and Rules.ChuteDamage do not appear in the headers; the chute message is not produced by any available function.
// SKIP B_0066#50: Chute.Land and Rules.ChuteDamage do not appear in the headers; hit points after a chute fall cannot be computed with the available API (see B_0066#49).
// SKIP B_0066#51: Chute.Land and Rules.ChuteDamage do not appear in the headers; the message prefix check needs the chute function (see B_0066#49).
