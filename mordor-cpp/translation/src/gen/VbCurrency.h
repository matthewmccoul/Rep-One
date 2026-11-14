#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct VbCurrency {
    static constexpr int64_t Scale = 10000;

    int64_t Raw = 0;

    VbCurrency() = default;
    explicit VbCurrency(int64_t raw);

    static VbCurrency Zero;
    static VbCurrency MaxValue;
    static VbCurrency MinValue;

    static VbCurrency FromRaw(int64_t raw);
    // C#: public static implicit operator VbCurrency(long whole)
    static VbCurrency op_Implicit(int64_t whole);
    static VbCurrency FromDouble(double d);
    static VbCurrency FromDecimal(cs::Decimal m);

    double ToDouble();
    cs::Decimal ToDecimal();
    int64_t RoundToWhole();
    int64_t Whole();
    bool IsWhole();

    static bool TryAdd(VbCurrency a, VbCurrency b, VbCurrency& sum);
    static bool TrySubtract(VbCurrency a, VbCurrency b, VbCurrency& difference);
    static VbCurrency AddSaturating(VbCurrency a, VbCurrency b);
    static VbCurrency SubtractSaturating(VbCurrency a, VbCurrency b);
    static bool TryFromDouble(double d, VbCurrency& value);
    static VbCurrency Saturate(int64_t whole);

    static VbCurrency Min(VbCurrency a, VbCurrency b);
    static VbCurrency Max(VbCurrency a, VbCurrency b);

    bool Equals(VbCurrency other);
    bool Equals(std::shared_ptr<void> obj);
    int32_t GetHashCode();
    int32_t CompareTo(VbCurrency other);

    cs::String ToString();
    cs::String ToString(cs::String format, cs::Nullable<int> provider);
    cs::String Str();
    cs::String CStr();

    cs::Nullable<cs::Decimal> ExactForSave();

    static VbCurrency FromSave(int64_t whole, cs::Nullable<cs::Decimal> exact);
    static bool TryParse(cs::String text, VbCurrency& value);
};

inline VbCurrency VbCurrency::Zero{};
inline VbCurrency VbCurrency::MaxValue{INT64_MAX};
inline VbCurrency VbCurrency::MinValue{INT64_MIN};

VbCurrency operator+(VbCurrency a, VbCurrency b);
VbCurrency operator-(VbCurrency a, VbCurrency b);
VbCurrency operator-(VbCurrency a);
VbCurrency operator*(VbCurrency a, int64_t b);
bool operator==(VbCurrency a, VbCurrency b);
bool operator!=(VbCurrency a, VbCurrency b);
bool operator<(VbCurrency a, VbCurrency b);
bool operator>(VbCurrency a, VbCurrency b);
bool operator<=(VbCurrency a, VbCurrency b);
bool operator>=(VbCurrency a, VbCurrency b);

struct SavedWholeJson {
    int64_t Read(cs::Utf8JsonReader& reader, cs::Type typeToConvert, cs::Ref<cs::JsonSerializerOptions> options);
    void Write(cs::Utf8JsonWriter writer, int64_t value, cs::Ref<cs::JsonSerializerOptions> options);
};

struct SavedExactJson {
    bool HandleNull();
    cs::Nullable<cs::Decimal> Read(cs::Utf8JsonReader& reader, cs::Type typeToConvert, cs::Ref<cs::JsonSerializerOptions> options);
    void Write(cs::Utf8JsonWriter writer, cs::Nullable<cs::Decimal> value, cs::Ref<cs::JsonSerializerOptions> options);
};

}  // namespace mc
