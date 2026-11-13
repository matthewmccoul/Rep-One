// The rules never print. They emit events; a front end (console now, Unreal later) turns events into text, sound and animation.
#pragma once
#include "mordor/types.h"

namespace mordor {

struct Event {
  Str code;        // stable machine id, e.g. "combat.hit", "town.rest", "dungeon.fall"
  Str text;        // default English line for simple front ends (may be empty)
  Vec<i64> nums;   // numeric arguments (damage dealt, member index, ...)
  Vec<Str> strs;   // string arguments (names)
};

class IEventSink {
 public:
  virtual ~IEventSink() = default;
  virtual void emit(const Event& e) = 0;
};

// Collects events in memory (used by tests and by the console front end).
class EventLog : public IEventSink {
 public:
  void emit(const Event& e) override { events.push_back(e); }
  Vec<Event> events;
  void clear() { events.clear(); }
  bool has(const Str& code) const {
    for (const auto& e : events) if (e.code == code) return true;
    return false;
  }
  int count(const Str& code) const {
    int n = 0;
    for (const auto& e : events) if (e.code == code) ++n;
    return n;
  }
};

inline Event makeEvent(Str code, Str text = Str(), Vec<i64> nums = {}, Vec<Str> strs = {}) {
  Event e; e.code = std::move(code); e.text = std::move(text); e.nums = std::move(nums); e.strs = std::move(strs); return e;
}

}  // namespace mordor
