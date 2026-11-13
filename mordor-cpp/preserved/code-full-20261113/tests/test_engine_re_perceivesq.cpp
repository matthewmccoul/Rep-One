// Translation of reference assertions B_0063#80..#86 (Perception.PerceiveSquare).
// Our perceiveSquare(Ctx&, SquareState&) is declared in casting1.h, but the
// SquareState fields and the perception inputs (best, bestName, inParty, lost,
// noMessages) are not declared, so none of the cases can be set up yet.
#include "test_support.h"
#include "mordor/casting1.h"

using namespace mordor;

// SKIP B_0063#80: SquareState and perception inputs (zero perception, already mapped, lone character) not declared
// SKIP B_0063#81: SquareState and party/lost inputs not declared
// SKIP B_0063#82: messages-off option and SquareState not declared
// SKIP B_0063#83: SquareState and rotator bit constant not declared
// SKIP B_0063#84: SquareState automap flag and perception 100 input not declared
// SKIP B_0063#85: certainty text output (Say/Certainty) and perception 200 input not declared
// SKIP B_0063#86: lost flag, mapped result and certainty text output not declared
