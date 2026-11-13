// Calls every feature's register function. The feature factory (tools/phase_features.py) edits only the two marked places.
#include "mordor/features.h"

namespace mordor {

void register_swap();
void register_realign();
void register_uncurse();
void register_seer();
void register_library();
void register_confinement();
void register_companions();
void register_hall();
void register_options();
void register_quests();
void register_rescue();
void register_roster();
void register_look();
void register_dcast();
void register_duse();
void register_disarm();
void register_buffers();
void register_give();
void register_guildlog();
// FEATURE_DECLS

void registerAllFeatures() {
  static bool done = false;
  if (done) return;
  done = true;
  register_swap();
  register_realign();
  register_uncurse();
  register_seer();
  register_library();
  register_confinement();
  register_companions();
  register_hall();
  register_options();
  register_quests();
  register_rescue();
  register_roster();
  register_look();
  register_dcast();
  register_duse();
  register_disarm();
  register_buffers();
  register_give();
  register_guildlog();
  // FEATURE_CALLS
}

}  // namespace mordor
