#include "core.h"

namespace mc {

VbCurrency::VbCurrency(int64_t raw) { Raw = raw; }

VbCurrency VbCurrency::FromRaw(int64_t raw) { return VbCurrency(raw); }

VbCurrency VbCurrency::op_Implicit(int64_t whole) {
    // checked(whole * Scale)
    if (whole > INT64_MAX / Scale || whole < INT64_MIN / Scale) throw cs::OverflowException("Arithmetic operation resulted in an overflow.");
    return VbCurrency(whole * Scale);
}

VbCurrency VbCurrency::FromDouble(double d) {
    double scaled = cs::Math::Round(d * (double)Scale, cs::MidpointRounding::ToEven);
    // 2^63 is the first double past the range; anything at or beyond it, or NaN, overflows
    if (std::isnan(scaled) || scaled >= 9223372036854775808.0 || scaled < -9223372036854775808.0)
        throw cs::OverflowException("Currency overflow");
    return VbCurrency(cs::CastDouble<int64_t>(scaled));
}

VbCurrency VbCurrency::FromDecimal(cs::Decimal m) {
    cs::Decimal scaled = cs::Decimal::Round(m * cs::Decimal((long long)Scale), 0, cs::MidpointRounding::ToEven);
    if (scaled > cs::Decimal((long long)INT64_MAX) || scaled < cs::Decimal((long long)INT64_MIN)) throw cs::OverflowException("Currency overflow");
    return VbCurrency((int64_t)scaled.ToInt64());
}

double VbCurrency::ToDouble() { return (double)Raw / Scale; }

cs::Decimal VbCurrency::ToDecimal() { return cs::Decimal((long long)Raw) / cs::Decimal((long long)Scale); }

int64_t VbCurrency::RoundToWhole() {
    int64_t q = Raw / Scale, r = Raw % Scale;  // C# truncates toward zero
    int64_t half = Scale / 2;
    int64_t ar = cs::Math::Abs(r);
    if (ar > half || (ar == half && (q & 1) != 0)) q += cs::Math::Sign(r);
    return q;
}

int64_t VbCurrency::Whole() { return Raw / Scale; }

bool VbCurrency::IsWhole() { return Raw % Scale == 0; }

VbCurrency operator+(VbCurrency a, VbCurrency b) {
    VbCurrency s;
    if (!VbCurrency::TryAdd(a, b, s)) throw cs::OverflowException("Arithmetic operation resulted in an overflow.");
    return s;
}

VbCurrency operator-(VbCurrency a, VbCurrency b) {
    VbCurrency d;
    if (!VbCurrency::TrySubtract(a, b, d)) throw cs::OverflowException("Arithmetic operation resulted in an overflow.");
    return d;
}

VbCurrency operator-(VbCurrency a) {
    if (a.Raw == INT64_MIN) throw cs::OverflowException("Arithmetic operation resulted in an overflow.");
    return VbCurrency(-a.Raw);
}

VbCurrency operator*(VbCurrency a, int64_t b) {
    cs::BigInteger p = cs::BigInteger((long long)a.Raw) * cs::BigInteger((long long)b);
    if (p > cs::BigInteger((long long)INT64_MAX) || p < cs::BigInteger((long long)INT64_MIN)) throw cs::OverflowException("Arithmetic operation resulted in an overflow.");
    return VbCurrency((int64_t)(long long)p);
}

bool VbCurrency::TryAdd(VbCurrency a, VbCurrency b, VbCurrency& sum) {
    int64_t r = cs::Wrap<int64_t>((long long)((uint64_t)a.Raw + (uint64_t)b.Raw));
    // overflow happened iff both operands have the same sign and the result's sign differs
    bool ok = ((a.Raw ^ r) & (b.Raw ^ r)) >= 0;
    sum = ok ? VbCurrency(r) : Zero;
    return ok;
}

bool VbCurrency::TrySubtract(VbCurrency a, VbCurrency b, VbCurrency& difference) {
    int64_t r = cs::Wrap<int64_t>((long long)((uint64_t)a.Raw - (uint64_t)b.Raw));
    bool ok = ((a.Raw ^ b.Raw) & (a.Raw ^ r)) >= 0;
    difference = ok ? VbCurrency(r) : Zero;
    return ok;
}

VbCurrency VbCurrency::AddSaturating(VbCurrency a, VbCurrency b) {
    VbCurrency s;
    return TryAdd(a, b, s) ? s : b.Raw > 0 ? MaxValue : MinValue;
}

VbCurrency VbCurrency::SubtractSaturating(VbCurrency a, VbCurrency b) {
    VbCurrency d;
    return TrySubtract(a, b, d) ? d : b.Raw < 0 ? MaxValue : MinValue;
}

bool VbCurrency::TryFromDouble(double d, VbCurrency& value) {
    double scaled = cs::Math::Round(d * (double)Scale, cs::MidpointRounding::ToEven);
    bool ok = !std::isnan(scaled) && scaled < 9223372036854775808.0 && scaled >= -9223372036854775808.0;
    value = ok ? VbCurrency(cs::CastDouble<int64_t>(scaled)) : Zero;
    return ok;
}

VbCurrency VbCurrency::Saturate(int64_t whole) {
    return whole > MaxValue.Whole() ? MaxValue : whole < MinValue.Whole() ? MinValue : op_Implicit(whole);
}

bool operator==(VbCurrency a, VbCurrency b) { return a.Raw == b.Raw; }
bool operator!=(VbCurrency a, VbCurrency b) { return a.Raw != b.Raw; }
bool operator<(VbCurrency a, VbCurrency b) { return a.Raw < b.Raw; }
bool operator>(VbCurrency a, VbCurrency b) { return a.Raw > b.Raw; }
bool operator<=(VbCurrency a, VbCurrency b) { return a.Raw <= b.Raw; }
bool operator>=(VbCurrency a, VbCurrency b) { return a.Raw >= b.Raw; }

VbCurrency VbCurrency::Min(VbCurrency a, VbCurrency b) { return a <= b ? a : b; }
VbCurrency VbCurrency::Max(VbCurrency a, VbCurrency b) { return a >= b ? a : b; }

bool VbCurrency::Equals(VbCurrency other) { return Raw == other.Raw; }

bool VbCurrency::Equals(std::shared_ptr<void> obj) {
    (void)obj;
    // boxed object type test is not available in C++
    return false;
}

int32_t VbCurrency::GetHashCode() {
    return (int32_t)(uint32_t)(uint64_t)Raw ^ (int32_t)(uint32_t)((uint64_t)Raw >> 32);
}

int32_t VbCurrency::CompareTo(VbCurrency other) { return Raw < other.Raw ? -1 : (Raw > other.Raw ? 1 : 0); }

cs::String VbCurrency::ToString() { return cs::Format(ToDecimal(), "0.####"); }

cs::String VbCurrency::ToString(cs::String format, cs::Nullable<int> provider) {
    (void)provider;
    return format.empty() ? ToString() : cs::Format(ToDecimal(), format);
}

cs::String VbCurrency::Str() {
    cs::String digits = cs::Format(cs::Math::Abs(ToDecimal()), "0.####");
    if (cs::Str::StartsWith(digits, "0.")) digits = cs::Str::Substring(digits, 1);
    return (Raw < 0 ? "-" : " ") + digits;
}

cs::String VbCurrency::CStr() { return cs::Format(ToDecimal(), "0.####"); }

cs::Nullable<cs::Decimal> VbCurrency::ExactForSave() {
    if (IsWhole()) return cs::Nullable<cs::Decimal>{};
    return cs::Nullable<cs::Decimal>(ToDecimal());
}

VbCurrency VbCurrency::FromSave(int64_t whole, cs::Nullable<cs::Decimal> exact) {
    if (exact.has_value()) {
        cs::Decimal m = exact.value();
        try {
            auto e = FromDecimal(m);
            if (e.Whole() == whole) return e;
        } catch (const cs::OverflowException&) {
        }
    }
    return Saturate(whole);
}

bool VbCurrency::TryParse(cs::String text, VbCurrency& value) {
    value = Zero;
    cs::String s = cs::Str::Trim(text, " \t");
    if ((int32_t)s.size() == 0) return false;
    if (s[0] == '&') {
        int32_t n = 0;
        if (!VbConvert::TryCLng(s, n)) return false;
        value = op_Implicit(n);
        return true;
    }
    cs::String e = cs::Str::Replace(cs::Str::Replace(s, 'D', 'E'), 'd', 'E');
    cs::Decimal m{};
    if (!cs::Decimal::TryParse(e, m)) return false;
    try {
        value = FromDecimal(m);
        return true;
    } catch (const cs::OverflowException&) {
        return false;
    }
}

int64_t SavedWholeJson::Read(cs::Utf8JsonReader& reader, cs::Type typeToConvert, cs::Ref<cs::JsonSerializerOptions> options) {
    (void)reader; (void)typeToConvert; (void)options;
    throw cs::NotSupportedException("not translated: System.Text.Json converter");
}

void SavedWholeJson::Write(cs::Utf8JsonWriter writer, int64_t value, cs::Ref<cs::JsonSerializerOptions> options) {
    (void)writer; (void)value; (void)options;
    throw cs::NotSupportedException("not translated: System.Text.Json converter");
}

bool SavedExactJson::HandleNull() { return true; }

cs::Nullable<cs::Decimal> SavedExactJson::Read(cs::Utf8JsonReader& reader, cs::Type typeToConvert, cs::Ref<cs::JsonSerializerOptions> options) {
    (void)reader; (void)typeToConvert; (void)options;
    throw cs::NotSupportedException("not translated: System.Text.Json converter");
}

void SavedExactJson::Write(cs::Utf8JsonWriter writer, cs::Nullable<cs::Decimal> value, cs::Ref<cs::JsonSerializerOptions> options) {
    (void)writer; (void)value; (void)options;
    throw cs::NotSupportedException("not translated: System.Text.Json converter");
}

}  // namespace mc
