#include "flow_support.h"
#include "mordor/access.h"

using namespace flowsup;

// Until features.h declares the arrival entry point, the test declares it here.
namespace mordor {
Vec<Str> hallArrivalUpdate(Session& s, i64 daySerial);
}

