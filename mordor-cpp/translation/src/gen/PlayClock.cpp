#include "core.h"

namespace mc {

// defined out of class: PlayClock must be complete for cs::New
cs::Ref<PlayClock> PlayClock::Shared = cs::New<PlayClock>();

float PlayClock::Timer(cs::DateTime now) {
    // DateTime.TimeOfDay: ticks within the day (864000000000 ticks per day)
    cs::TimeSpan timeOfDay{now.ticks % 864000000000LL};
    return (float)timeOfDay.TotalSeconds();
}

void PlayClock::Tick(cs::List<cs::Ref<Character>> party, float timer) {
    while (_started.Count() < party.Count()) _started.Add(0);   // ReDim Preserve as the party grows
    for (int32_t i = 0; i < party.Count(); i++)
    {
        auto c = party[i];
        if (_started[i] == 0 || c->PlaySeconds == -1)
        {
            _started[i] = timer;
            return;
        }
        if (!FramePause::Paused)                                       // MAINMOD2.BAS:6012
        {
            float gap = timer < _started[i] ? (86340.0f - _started[i]) + timer : timer - _started[i];
            c->PlaySeconds = cs::Math::Round(c->PlaySeconds + gap, 4, cs::MidpointRounding::ToEven);
        }
        _started[i] = timer;                                          // 6019, paused or not
    }
}

}  // namespace mc
