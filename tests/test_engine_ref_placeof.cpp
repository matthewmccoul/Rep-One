#include "test_support.h"
#include "mordor/ref_placeof.h"

// B_0057#78: The place text is built from the live square (10,11) on level 3 (lead not lost).
// The expected value "11,12,3" is what the record holds after Librarian.FileSighting, which is not ported here.
// SKIP B_0057#78: expected value is the record content after Librarian.FileSighting, which is not ported; PlaceOf alone is covered by #79-#82 below.

