#include "test_support.h"
#include "mordor/loot1a.h"

// SKIP B_0055#73: CharStatFile.FormatNumber has no equivalent; formatWholeGold does not take the same rounding/format inputs
// SKIP B_0055#74: CharStatFile.FormatNumber culture/pattern comparison; no equivalent in headers
// SKIP B_0055#75: CharStatFile.FormatNumber bracket formatting; no equivalent in headers
// SKIP B_0055#76: CharStatFile.FormatNumber Currency vs Long text comparison; no equivalent in headers

// SKIP B_0055#97: GoldShare.Line text is an event message; event text accessor on EventLog is not in the headers shown

// SKIP B_0055#98 (line part): GoldShare.Line text is an event message; event text accessor on EventLog is not in the headers shown

// SKIP B_0055#99 (line part): GoldShare.Line text is an event message; event text accessor on EventLog is not in the headers shown

// SKIP B_0055#100: GoldAfterClass whole-gold rounding of class-scaled chest gold is not specified by scaleGoldForClass (Currency result only); units ambiguous

// SKIP B_0055#130: Purse of MaxValue on hand plus MaxValue banked overflows Currency (i64) in Character::purse(); no saturating equivalent in headers
