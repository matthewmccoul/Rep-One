#include "test_support.h"
#include "mordor/monsters3a.h"

using namespace mordor;

// FIXED: UNRESOLVED. The scripted rolls are probabilities in [0,1), passed as
// Vec<f64> lists. The compiler rejects every tsup::Env(Vec<f64>{...}) call, so
// tsup::Env's constructor parameter must take a different element type. That
// signature is in test_support.h, which was not provided, so this file is left
// unchanged. Fix it there: make Env accept Vec<f64> (preferred, since the rolls
// are probabilities). Do not convert the rolls to integers here, because that
// would change the meaning of the tests.

