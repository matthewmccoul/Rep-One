I can't fix this without inventing an API, so I haven't written the three files.

The C# harness needs to build a `VbCurrency` from an integer count of 1/10000-gold units. The shown API doesn't provide that. The only construction I can see is the implicit conversion in `VbCurrency moved = 0;`, and it doesn't say what unit an integer maps to. Guessing could make the differential test pass or fail for the wrong reason. The `Units()` parser also assumes `Str()` prints plain decimal gold, which isn't shown either.

To finish the Case.cs side, I need one of these:
- The `VbCurrency` constructor or factory that takes gold×10000 units (or the exact unit of the implicit int conversion), plus confirmation that `Str()` prints plain decimal gold.
- Or permission to build amounts from the implicit int conversion and check the unit at startup with a round-trip (`Units(Amt(x)) == x`) that fails loudly on mismatch.

The C++ side is ready to go once that's settled. There's also a behavioural difference the test should expose. The reference `PartyDeposit` is all-or-nothing through `AllOrNothing`, so a failed overflow leaves every member unchanged and returns zero. The C++ `partyDepositGold` returns false mid-loop without rolling back earlier members, so the test should catch that as a mismatch on any case where a later member overflows.

REFERENCE: Party.PartyDeposit