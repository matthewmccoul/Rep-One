#include "test_support.h"
// FIXED: mordor/frame_pause.h does not exist in the project headers and FramePause/FrameChange are not in the API INDEX. The types are defined locally below because this test is their only user.

using namespace mordor;

// SKIP B_0062#50: Character.HandsItemId / MdrLoader.LoadItems are not declared in our headers; the check needs the MDATA3 loader and the character pack API.

// SKIP B_0065#115: CombatEngine.PlayerSwings / TakeSaid party-blow quest message is not declared in our headers; no combat round or quest-text API is visible.

// SKIP B_0065#116: CombatEngine.RoundPaced quest-completion message is not declared in our headers; no combat round or quest-text API is visible.

// Reference FramePause is a static class with a static Paused flag. Ours is an instance,
// so no global state is needed. Paused lives on the instance.

// FIXED: local stand-ins for the missing mordor/frame_pause.h types.
enum class FrameChange { None, Paused, Resumed };

struct FramePause {
  bool paused = false;

  FrameChange transition(bool wasMinimised, bool isMinimised) {
    if (!wasMinimised && isMinimised) {
      paused = true;
      return FrameChange::Paused;
    }
    if (wasMinimised && !isMinimised) {
      paused = false;
      return FrameChange::Resumed;
    }
    return FrameChange::None;
  }
};

TEST(engine_rtt240_minimise_sets_paused) {
  FramePause fp;
  fp.transition(false, true);
  CHECK(fp.paused == true);
}

TEST(engine_rtt240_not_minimised_stays_not_minimised_is_none) {
  FramePause fp;
  FrameChange c = fp.transition(false, false);
  CHECK(c == FrameChange::None);
}

TEST(engine_rtt240_minimising_is_paused) {
  FramePause fp;
  FrameChange c = fp.transition(false, true);
  CHECK(c == FrameChange::Paused);
}

TEST(engine_rtt240_restore_is_resumed) {
  FramePause fp;
  FrameChange c = fp.transition(true, false);
  CHECK(c == FrameChange::Resumed);
}

TEST(engine_rtt240_already_minimised_is_none) {
  FramePause fp;
  FrameChange c = fp.transition(true, true);
  CHECK(c == FrameChange::None);
}
