#include "test_support.h"
#include "mordor/items3b.h"

// SKIP B_0063#65: needs MDATA3.MDR item records (Floor field) loaded from the data file; no header function counts items by floor.
// SKIP B_0063#129: needs enumeration of the MDATA1 item classes; headers only offer maxWornOfClass(ctx, wearClass) for a given code, with no class list.
// SKIP B_0063#130: needs enumeration of the MDATA1 item classes to count those with maximum worn of 1; no class iterator in headers.
// SKIP B_0063#131: needs enumeration of the MDATA1 item classes to count those with maximum worn of 0; no class iterator in headers.
// SKIP B_0063#132: needs enumeration of the MDATA1 item classes to count those with maximum worn of -1; no class iterator in headers.
// SKIP B_0063#133: needs MDATA3 items mapped to their class by name and id; no header function provides this lookup.
// SKIP B_0063#134: needs the seven specific items of the maximum-minus-one class; the headers give no class-to-item listing.
// SKIP B_0063#135: needs a lookup of "Medusa's Head" by name; no name-to-item function is declared in the headers.
// SKIP B_0063#136: needs the seven specific items and a multi-item wear simulation; the expected count is a range and the item ids are not given.
// SKIP B_0063#137: the case does not give the wear-class code of the maximum-minus-one class, so maxWornOfClass cannot be called without inventing that mapping.
