#include "test_support.h"
#include "mordor/city2b.h"
#include "mordor/state_character.h"

using mordor::Currency;

// SKIP B_0055#128: Party.BankTyped (typed gold withdrawal) has no equivalent in the headers.
// SKIP B_0055#129: Party.BankTyped (typed gold deposit) has no equivalent in the headers.
// SKIP B_0057#5: libraryFileFind needs an item from MDATA3 with non-zero Id; ItemDef field names are not in the headers, so the item cannot be selected.
// SKIP B_0057#6: same as B_0057#5; the item selection cannot be set up.
// SKIP B_0057#7: same as B_0057#5; the item selection cannot be set up.
