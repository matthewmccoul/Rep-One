#include "test_support.h"
#include "mordor/city1.h"
#include "mordor/city2a.h"

// SKIP B_0058#29: LibrarySearch.Sorted has no equivalent function in the new headers.
// SKIP B_0059#2: Asks whether the Store and Bank forms are modeless; this is UI behaviour with no header equivalent.
// SKIP B_0059#41: StoreStock.Weight has no equivalent function or type in the new headers.
// SKIP B_0059#42: StoreStock.Weight has no equivalent function or type in the new headers.
// SKIP B_0059#43: StoreStock.Weight has no equivalent function or type in the new headers.
// SKIP B_0059#44: StoreStock.Weight has no equivalent function or type in the new headers.
// SKIP B_0059#45: Needs the MDATA3 item table and its alignment-flag fields, which the headers do not expose.
// SKIP B_0059#46: Expected value is "At least 1", not an exact value, and needs the MDATA3 per-kind weight data, which the headers do not expose.
// SKIP B_0059#47: Needs a Shelf type and a mapping from the reference's scripted roll sequence to the new draw calls; neither is in the headers.
// SKIP B_0059#48: Needs a mapping from the reference's scripted roll sequence to the new draw calls and a way to count draws; neither is in the headers.
