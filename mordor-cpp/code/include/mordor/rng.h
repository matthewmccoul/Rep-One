// Random numbers enter the rules only through IRng, so tests are deterministic and an Unreal port can use its own generator.
#pragma once
#include "mordor/math_util.h"
#include "mordor/types.h"

namespace mordor {

class IRng {
 public:
  virtual ~IRng() = default;
  virtual u32 nextU32() = 0;
  // Uniform in [0, 1): the Visual Basic Rnd.
  virtual double nextDouble() = 0;

  // Int(Rnd * n): uniform integer in [0, n-1]; n <= 0 gives 0.
  i32 below(i32 n) { return n <= 0 ? 0 : static_cast<i32>(vbInt(nextDouble() * n)); }
  // Inclusive range [lo, hi].
  i32 range(i32 lo, i32 hi) { return hi <= lo ? lo : lo + below(hi - lo + 1); }
  // True with the given percent probability (0..100): Rnd*100 < percent.
  bool percent(double p) { return nextDouble() * 100.0 < p; }
};

// Small, fast, deterministic generator (xoshiro128**), seeded by splitmix64.
class Rng : public IRng {
 public:
  explicit Rng(u64 seed = 1) { reseed(seed); }
  void reseed(u64 seed) {
    for (int i = 0; i < 4; ++i) s_[i] = static_cast<u32>(splitmix(seed) >> 16);
    if ((s_[0] | s_[1] | s_[2] | s_[3]) == 0) s_[0] = 1;
  }
  u32 nextU32() override {
    u32 r = rotl(s_[1] * 5u, 7) * 9u;
    u32 t = s_[1] << 9;
    s_[2] ^= s_[0]; s_[3] ^= s_[1]; s_[1] ^= s_[2]; s_[0] ^= s_[3];
    s_[2] ^= t; s_[3] = rotl(s_[3], 11);
    return r;
  }
  double nextDouble() override { return static_cast<double>(nextU32()) / 4294967296.0; }

 private:
  static u32 rotl(u32 x, int k) { return (x << k) | (x >> (32 - k)); }
  static u64 splitmix(u64& x) {
    x += 0x9E3779B97F4A7C15ull;
    u64 z = x;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  }
  u32 s_[4];
};

// Test helper: replays a fixed list of doubles in [0,1), then repeats the last one.
class ScriptedRng : public IRng {
 public:
  explicit ScriptedRng(Vec<double> values) : v_(std::move(values)) {}
  u32 nextU32() override { return static_cast<u32>(nextDouble() * 4294967296.0); }
  double nextDouble() override {
    if (v_.empty()) return 0.0;
    double d = v_[i_ < v_.size() ? i_ : v_.size() - 1];
    if (i_ < v_.size()) ++i_;
    return d;
  }
 private:
  Vec<double> v_;
  std::size_t i_ = 0;
};

}  // namespace mordor
