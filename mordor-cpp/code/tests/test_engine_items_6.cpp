#include "test_support.h"
#include "mordor/items2b.h"

using namespace mordor;

// SKIP B_0063#46: lists item names from MDATA3.MDR data; no equivalent function in the headers
// SKIP B_0063#47: counts Cursed values across MDATA3.MDR data; no equivalent function in the headers
// SKIP B_0063#48: needs the first Cursed=1 item from MDATA3.MDR; no data lookup function in the headers
// SKIP B_0063#49: needs the first Cursed=2 item from MDATA3.MDR; no data lookup function in the headers
// SKIP B_0063#55: needs a Ctx for applyEquipCurse(ctx, slot, wearer); mordor::Ctx has no default constructor and its constructor arguments are not in the API index
// SKIP B_0063#57: counts items by Floor from MDATA3.MDR data; no equivalent function in the headers

