#include "test_support.h"

// Cases B_0055#78-#87 exercise Party.BankAllText and Party.BankTyped (deposit/withdraw
// text parsing and bank balance moves). Neither function appears in the provided headers,
// and no equivalent exists in city1.h, city2a.h, city2b.h, city3a.h, or city3b.h.
// The nearest functions (bankFreeSlots, charPay, townCharge) do not implement typed
// bank entry, so every case is skipped rather than mapped to an unrelated function.

// SKIP B_0055#78: Party.BankAllText is not in the headers; no equivalent function
// SKIP B_0055#79: Party.BankAllText is not in the headers; no equivalent function
// SKIP B_0055#80: Party.BankTyped deposit and BankAllText are not in the headers; no equivalent function
// SKIP B_0055#81: Party.BankTyped withdraw is not in the headers; no equivalent function
// SKIP B_0055#82: Party.BankTyped withdraw is not in the headers; no equivalent function
// SKIP B_0055#83: Party.BankTyped deposit is not in the headers; no equivalent function
// SKIP B_0055#84: Party.BankTyped deposit is not in the headers; no equivalent function
// SKIP B_0055#85: Party.BankTyped deposit is not in the headers; no equivalent function
// SKIP B_0055#86: Party.BankTyped unchecked withdraw is not in the headers; no equivalent function
// SKIP B_0055#87: Party.BankTyped withdraw and set-to-banked are not in the headers; no equivalent function
