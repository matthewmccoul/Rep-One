#include "test_support.h"

#include "mordor/death1a.h"
#include "mordor/death1b.h"
#include "mordor/death2a.h"
#include "mordor/death2b.h"
#include "mordor/death3.h"

// SKIP B_0059#92: the morgue room walk is a MAZEMOD.BAS UI/map traversal; no walk function is declared in the headers (Party::morgueList is only a data member).
// SKIP B_0059#93: Character.Save / Character.LoadAllFrom are not declared in any header.
// SKIP B_0059#94: LookAround.Roster is a UI "who is here" walk; no equivalent function is declared in the headers.
// SKIP B_0064#8: kHallCategories is declared nowhere and HallOfRecords::records is an untyped Vec<i32>, so the count of town-wide record kinds cannot be read from the headers.
// SKIP B_0065#99: the expected line depends on the race-minimum Constitution, which needs RaceDef field names not present in the headers (resetConToRaceMinimum/resetConToRacialMinimum set it but do not expose the value), plus a scripted Con-loss roll whose RNG sequence is not specified.
// SKIP B_0065#100: depends on the race-minimum Constitution value, which cannot be obtained from the headers (see B_0065#99).
// SKIP B_0065#101: counter-raise Constitution loss needs the race-minimum value and a scripted roll; the message text format is not specified by any declared function (see B_0065#99).
// SKIP B_0065#102: depends on the race-minimum Constitution value and the counter-raise roll (see B_0065#99).
// SKIP B_0065#112: "that same blow" refers to a blow sequence from earlier cases that is not specified; recordKill is the closest function but the setup cannot be reconstructed from the given cases.
// SKIP B_0065#113: same as B_0065#112; the m1 kill count depends on the unspecified blow sequence.
