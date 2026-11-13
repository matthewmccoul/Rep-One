// Batch t172: C# -> C++ engine assertions. Every case here is blocked on
// declarations that are not in our headers yet (see the NEED line above), so
// each case is skipped with its reason. No expected value was altered.
#include "test_support.h"

using namespace mordor;

// SKIP B_0055#86: needs the bank-typed withdrawal function, BankAnswer and Character gold fields; unchecked withdrawal of -5 must give Moved with -4.75/104.75 (1/10000 gold: -47500/1047500)
// SKIP B_0055#87: needs the bank-typed withdrawal function, BankAnswer and Character gold fields; sequence must give (Moved, True, Beep)
// SKIP B_0055#128: needs the bank-typed withdrawal function and Character gold fields; unchecked withdrawal of -100 at MaxValue-10 banked must give no exception and 0/922337203685467.5807 unchanged on-hand/banked split as reported by the reference (False -100/922337203685467.5807)
// SKIP B_0055#129: needs the bank-typed deposit function, BankAnswer and Character gold fields; deposit of 50 at banked MaxValue must give Moved 50/922337203685477.5807
// SKIP B_0055#80: needs the bank-typed deposit function, a BankAllText equivalent (LTrim of Str of balance) and Character gold fields; All-deposit of 89.5 must give Moved 0/100
// SKIP B_0059#48: needs a draw counter on ScriptedRng and StoreStock replenish; expected is exactly the number of scripted rolls
// SKIP B_0059#50: needs a draw counter on ScriptedRng and the selling-in restock path; expected is exactly the number of scripted rolls
// SKIP B_0057#102: needs StoreStock replenish with seeded Rng, the Library entries accessor, LibraryRecord nSeen and the shelved count; expected is sum of nSeen equals count shelved

