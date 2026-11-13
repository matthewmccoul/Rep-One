// Batch t193: LookAround.Walk + RoomRoster.Lines cases (B_0066#13 to #20).
// No test is written yet. Our declarations include no LookAround or RoomRoster API, and each
// case's expected value depends on the two counts that Walk produces from the floor, area and
// carrying state. Deriving those counts by hand would be guessing at Walk's semantics, which the
// rules forbid. Once the NEED declarations are provided, these cases can be expressed directly.

// SKIP B_0066#13: needs LookAround.Walk (floor/area scan) and RoomRoster.Lines; neither is declared in our code.
// SKIP B_0066#14: needs LookAround.Walk floor-2 same-area exclusion and RoomRoster.Lines; neither is declared.
// SKIP B_0066#15: needs LookAround.Walk counting two dead characters in one area and RoomRoster.Lines text; neither is declared.
// SKIP B_0066#16: needs LookAround.Walk carrying-character handling and RoomRoster.Lines; neither is declared.
// SKIP B_0066#17: needs LookAround.Walk carrying-character handling with two dead and RoomRoster.Lines text; neither is declared.
// SKIP B_0066#18: needs LookAround.Walk living-stranger count and RoomRoster.Lines text; neither is declared.
// SKIP B_0066#19: needs LookAround.Walk party-size count and RoomRoster.Lines; neither is declared.
// SKIP B_0066#20: needs LookAround.Walk living-stranger plus dead-party-member counts and RoomRoster.Lines text; neither is declared.
