#include "test_support.h"

// SKIP B_0066#131: itemUseProblem returns i32 (no message text), and UseProblemResult has no producing function; "held" text is not reachable.
// SKIP B_0067#9: itemDisplayName is the equivalent of Loot.NameFor, but the Iron Dagger's item id cannot be derived from the provided headers (ItemDef fields not shown), so the item cannot be set up.
// SKIP B_0067#10: same as B_0067#9 (Iron Dagger item id not derivable from the provided headers).
// SKIP B_0067#11: same as B_0067#9 (Iron Dagger item id not derivable from the provided headers).
// SKIP B_0067#12: same as B_0067#9 (Iron Dagger item id not derivable from the provided headers).
// SKIP B_0067#15: the expected value (buffer names slot 3) is not reproducible from the described setup: with buffersFollow false the buffer should stay on slot 2, and no SpellBuffers.PackSlotOf exists in the headers.
// SKIP B_0067#16: the expected value (buffer names slot 5) is not reproducible from the described setup: the buffer starts on slot 2, which neither swap touches, and no SpellBuffers.PackSlotOf exists in the headers.
// SKIP A_03#18: the MDATA3 importer (Pack slot to Item import) has no equivalent function in the provided headers.
// SKIP A_03#19: the MDATA3 importer (bank entries import) has no equivalent function in the provided headers.
