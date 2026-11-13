#include "test_support.h"
#include "mordor/dungeon1a.h"

// SKIP B_0063#82: needs a floor/def fixture that marks a teleporter tile; FloorDef/FloorState setup for teleporter squares is not expressible from the given headers
// SKIP B_0063#83: needs a floor/def fixture that marks a rotator tile; no SquareState field carries rotator status
// SKIP B_0063#84: needs a rotator fixture plus a pre-set automap value; rotator tile setup not expressible from the given headers
// SKIP B_0063#85: expected text names the Rotator feature, but reportSquareCertainty takes no feature argument and the rotator setup is not expressible
// SKIP B_0063#86: expected text names the Rotator feature and asserts the mapped flag; reportSquareCertainty cannot produce the feature-specific text and the rotator setup is not expressible
// SKIP B_0063#87: needs a FloorDef with quicksand ahead of (1,1) facing north; FloorDef contents are not specified in the headers
// SKIP B_0063#88: needs the same quicksand-ahead FloorDef fixture and a way to read the shared map; the header does not state which SquareState field records this
// SKIP B_0063#89: needs a FloorDef with quicksand ahead and a character standing in quicksand; the quicksand fixture is not expressible from the given headers
// SKIP B_0063#90: needs the quicksand fixture and a way to read the shared map; the header does not state which SquareState field records this
// SKIP B_0063#91: needs a FloorDef with a pit ahead and a quicksand-standing character; the pit fixture and announcement event code are not expressible from the given headers
