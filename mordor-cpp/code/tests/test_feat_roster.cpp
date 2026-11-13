#include "flow_support.h"
#include "mordor/access.h"
using namespace flowsup;

bool rosterUnload(mordor::Session& s, int member, mordor::Str& reply);
bool rosterLoad(mordor::Session& s, const mordor::Str& name, mordor::Str& reply);

