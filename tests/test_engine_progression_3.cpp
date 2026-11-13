#include "test_support.h"
#include "mordor/progression1a.h"

// SKIP A_03#11: the case reads a binary character record at byte offset 36 and
// asserts the imported Age. The headers expose no record importer or byte-offset
// reader, so there is no equivalent function to call.
