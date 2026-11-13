// Every rules function takes a Ctx: the data tables, the random source and the event sink. No globals anywhere.
#pragma once
#include "mordor/events.h"
#include "mordor/game_data.h"
#include "mordor/rng.h"

namespace mordor {

struct Ctx {
  const GameData& data;
  IRng& rng;
  IEventSink& events;
  Ctx(const GameData& d, IRng& r, IEventSink& e) : data(d), rng(r), events(e) {}
  void say(const Str& code, const Str& text = Str(), Vec<i64> nums = {}, Vec<Str> strs = {}) { events.emit(makeEvent(code, text, std::move(nums), std::move(strs))); }
};

}  // namespace mordor
