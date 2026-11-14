#include "core.h"

namespace mc {

cs::String GoldShare::Line(int64_t foundPlusRecovered, int64_t recoveredByViewer, int32_t partySize) {
    if (foundPlusRecovered <= 0) return cs::String();
    // the figure shown is not the figure paid: formatNumber(CCur(l039C / n)) keeps the
    // quotient to four places and Format$ rounds a half away from zero, where the share
    // itself is CLng'd half to even - 5 between two shows "3" and pays 2 each
    cs::String shown = CharStatFile::FormatNumber(Shown(foundPlusRecovered, partySize));
    return recoveredByViewer != 0
        ? "Your share is " + shown + " and you recovered " + CharStatFile::FormatNumber(recoveredByViewer) + " Gold!"
        : "Your share is " + shown + " Gold!";
}

VbCurrency GoldShare::Shown(int64_t pot, int32_t partySize) {
    return VbCurrency::FromDouble(pot / (double)cs::Math::Max(1, partySize));
}

int64_t GoldShare::Share(int64_t pot, int32_t partySize) {
    return cs::CastDouble<int64_t>(cs::Math::Round(pot / (double)cs::Math::Max(1, partySize), cs::MidpointRounding::ToEven));
}

}  // namespace mc
