#include "mordor/status1a.h"

namespace mordor {

// R24, R26: maps a special attack kind to the Resistances::resist index that reduces it.
i32 specialResistIndex(SpecialAttackKind kind) {
    constexpr i32 kFireIndex = 0;          // R26 index 0: Fire (breath)
    constexpr i32 kAcidIndexAssumed = 2;   // UNKNOWN: acid index not given in SRD (R26); ASSUMED 2
    constexpr i32 kElectricalIndexAssumed = 3; // UNKNOWN: electrical index not given in SRD (R26); ASSUMED 3

    switch (kind) {
        case SpecialAttackKind::Breath:
            // R24: fire breath is resisted by Fire (0), cold breath by Cold (1).
            // ASSUMED: the signature carries no BreathType, so Fire (0) is returned.
            // A cold-breathing monster must use index 1 at the call site.
            return kFireIndex;
        case SpecialAttackKind::AcidSpit:
            // R24: acid spit is resisted by Acid. UNKNOWN index (R26); ASSUMED 2.
            return kAcidIndexAssumed;
        case SpecialAttackKind::Electrocute:
            // R24: electrocution is resisted by Electrical. UNKNOWN index (R26); ASSUMED 3.
            return kElectricalIndexAssumed;
    }
    return kFireIndex;
}

}  // namespace mordor
