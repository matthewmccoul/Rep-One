#include "core.h"

namespace mc {

cs::List<cs::String> CombatSounds::Of(CombatEngine_CombatLine line, cs::Ref<Character> watched, bool noPartySound,
                                      bool swing, bool blow) {
    cs::List<cs::String> _result = cs::List<cs::String>::New();
    if (swing && line.PickSounds != nullptr) for (auto& s : line.PickSounds) _result.Add(s);
    if (noPartySound && line.Actor != nullptr && line.Actor != watched) return _result;
    if (swing && line.CastSounds != nullptr) for (auto& s : line.CastSounds) _result.Add(s);
    if (swing && !line.SwingSound.empty()) _result.Add(line.SwingSound);
    if (blow && line.HitSounds != nullptr) for (auto& s : line.HitSounds) _result.Add(s);
    return _result;
}

cs::List<cs::String> CombatSounds::OfLines(cs::List<CombatEngine_CombatLine> lines, cs::Ref<Character> watched, bool noPartySound) {
    auto all = cs::List<cs::String>::New();
    for (auto& line : lines) {
        if (line.OpensChest) continue;
        all.AddRange(Of(line, watched, noPartySound));
    }
    return all;
}

}  // namespace mc
