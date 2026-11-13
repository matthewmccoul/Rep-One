// Shared helpers for tests: the game data (loaded once) and a ready-made context with a scripted or seeded RNG.
#pragma once
#include <fstream>
#include <initializer_list>
#include <sstream>
#include "harness.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/rng.h"

namespace tsup {
inline const mordor::GameData& data() {
  static mordor::GameData gd;
  static bool loaded = false;
  if (!loaded) {
    std::ifstream f(MORDOR_DATA_FILE);
    std::stringstream ss; ss << f.rdbuf();
    mordor::Str err;
    mordor::loadGameDataFromJson(ss.str(), gd, err);
    loaded = true;
  }
  return gd;
}
// The reference suite's FullCheck: a figure of the FULL game (15 dungeon levels in MDATA11.MDR) is skipped on the shareware
// data (3 levels), which is the data this project ships (see UNRESOLVED.md). Returns true when the check is to be skipped.
inline bool fullGameOnly(const char* what) {
  if (data().floors.size() >= 15) return false;
  std::printf("  skip  %s: full game only (the shipped data is the %zu-level shareware)\n", what, data().floors.size());
  return true;
}
// Ctx with its own random source and event log. Example: tsup::Env env({0.5, 0.9}); fn(env.ctx, ...); CHECK(env.log.has("combat.hit"));
struct Env {
  mordor::ScriptedRng scripted;
  mordor::Rng seeded;
  mordor::EventLog log;
  mordor::IRng* rng;
  mordor::Ctx ctx;
  Env(std::initializer_list<double> script) : scripted(mordor::Vec<double>(script)), seeded(1), rng(&scripted), ctx(data(), *rng, log) {}
  explicit Env(mordor::Vec<double> script) : scripted(std::move(script)), seeded(1), rng(&scripted), ctx(data(), *rng, log) {}
  explicit Env(mordor::u64 seed = 1) : scripted({}), seeded(seed), rng(&seeded), ctx(data(), *rng, log) {}
};
}  // namespace tsup
