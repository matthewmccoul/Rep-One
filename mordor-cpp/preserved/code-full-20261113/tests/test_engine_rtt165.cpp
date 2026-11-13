#include "test_support.h"
// NEED: perception header with Noticeable set and Floor square-kind constants
// NEED: PerceiveSquare-equivalent with explicit bit/perception/party/lost/noMessages/alreadyMapped inputs

using namespace mordor;

// SKIP B_0063#78: no declaration of the Noticeable square-kind set in our API (NEED above)
// SKIP B_0063#79: no declaration of the Noticeable square-kind set in our API (NEED above)
// SKIP B_0063#80: our perceiveSquare(Ctx&, SquareState&) takes no bit/perception/party/lost/alreadyMapped inputs
// SKIP B_0063#81: our perceiveSquare(Ctx&, SquareState&) takes no party/lost inputs
// SKIP B_0063#82: our perceiveSquare(Ctx&, SquareState&) takes no noMessages option input
// SKIP B_0063#83: our perceiveSquare(Ctx&, SquareState&) takes no perception-value input
// SKIP B_0063#84: our perceiveSquare(Ctx&, SquareState&) takes no perception-value or alreadyMapped input
// SKIP B_0063#85: our perceiveSquare(Ctx&, SquareState&) takes no perception-value or lost input
