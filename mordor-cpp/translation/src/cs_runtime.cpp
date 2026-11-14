#include "cs.h"
#include <ctime>
namespace cs {
Random::Random() { init((int)(std::chrono::high_resolution_clock::now().time_since_epoch().count() & 0x7fffffff)); }
DateTime DateTime::Now() { DateTime d; d.ticks = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() * 10 + 621355968000000000LL; return d; }
String DateTime::ToString(const String&) const { std::time_t t = (std::time_t)((ticks - 621355968000000000LL) / 10000000); char b[64]; std::strftime(b, sizeof b, "%Y-%m-%d %H:%M:%S", std::localtime(&t)); return b; }
}
