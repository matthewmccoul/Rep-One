#pragma once
// cs.h: the .NET pieces Mordor.Core uses, with C# semantics, in plain C++17 (no compiler extensions).
// Reference types (classes, List, Dictionary, HashSet, arrays) are shared handles, as in C#.
// Collections keep insertion order. Rounding, formatting and Random follow .NET.
#include <algorithm>
#include <cfenv>
#include <charconv>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace cs {

// ------------------------------------------------------------------ basics
using String = std::string;  // C# string (Latin-1 bytes; null strings become "")
template <class T> using Nullable = std::optional<T>;  // T? for value types
template <class F> using Func = std::function<F>;
inline constexpr std::nullopt_t null_value = std::nullopt;

struct Exception : std::runtime_error { using std::runtime_error::runtime_error; Exception() : std::runtime_error("Exception") {} };
// the .NET exception hierarchy, so that catch (const cs::ArgumentException&) also takes ArgumentOutOfRangeException, etc.
#define CS_EXCEPTION(N, B) struct N : B { using B::B; N() : B(#N) {} };
CS_EXCEPTION(SystemException, Exception)
CS_EXCEPTION(ArgumentException, SystemException) CS_EXCEPTION(ArgumentNullException, ArgumentException) CS_EXCEPTION(ArgumentOutOfRangeException, ArgumentException)
CS_EXCEPTION(InvalidOperationException, SystemException) CS_EXCEPTION(ObjectDisposedException, InvalidOperationException)
CS_EXCEPTION(IndexOutOfRangeException, SystemException) CS_EXCEPTION(KeyNotFoundException, SystemException) CS_EXCEPTION(FormatException, SystemException)
CS_EXCEPTION(IOException, SystemException) CS_EXCEPTION(FileNotFoundException, IOException) CS_EXCEPTION(DirectoryNotFoundException, IOException)
CS_EXCEPTION(EndOfStreamException, IOException) CS_EXCEPTION(InvalidDataException, SystemException)
CS_EXCEPTION(ArithmeticException, SystemException) CS_EXCEPTION(DivideByZeroException, ArithmeticException) CS_EXCEPTION(OverflowException, ArithmeticException)
CS_EXCEPTION(NotSupportedException, SystemException) CS_EXCEPTION(NotImplementedException, SystemException)
CS_EXCEPTION(UnauthorizedAccessException, SystemException) CS_EXCEPTION(NullReferenceException, SystemException)
CS_EXCEPTION(JsonException, Exception)
#undef CS_EXCEPTION

// A C# class reference (nullptr = null). Member access through a null reference throws
// NullReferenceException, as in C#, instead of being undefined behaviour.
template <class T> class Ref : public std::shared_ptr<T> {
public:
    using std::shared_ptr<T>::shared_ptr;
    Ref() = default;
    Ref(std::nullptr_t) {}
    template <class U, class = std::enable_if_t<std::is_convertible<U*, T*>::value>> Ref(const std::shared_ptr<U>& p) : std::shared_ptr<T>(p) {}
    template <class U, class = std::enable_if_t<std::is_convertible<U*, T*>::value>> Ref(std::shared_ptr<U>&& p) : std::shared_ptr<T>(std::move(p)) {}
    T* operator->() const { T* p = this->get(); if (!p) throw NullReferenceException(); return p; }
    T& operator*() const { T* p = this->get(); if (!p) throw NullReferenceException(); return *p; }
};
// C# object, for the members Core types as object: a polymorphic root that type tests (dynamic_pointer_cast) work on
struct Object { virtual ~Object() = default; };
template <class T, class... A> Ref<T> New(A&&... a) { return Ref<T>(std::make_shared<T>(std::forward<A>(a)...)); }
template <class T> T& deref(const Ref<T>& r) { if (!r) throw NullReferenceException(); return *r; }

// ------------------------------------------------------------------ numbers
enum class MidpointRounding { ToEven, AwayFromZero, ToZero, ToNegativeInfinity, ToPositiveInfinity };

namespace Math {
inline double Round(double x) { return std::nearbyint(x); }  // ties to even under the default FE_TONEAREST
inline double Round(double x, MidpointRounding m) {
    switch (m) {
        case MidpointRounding::ToEven: return std::nearbyint(x);
        case MidpointRounding::AwayFromZero: return std::round(x);
        case MidpointRounding::ToZero: return std::trunc(x);
        case MidpointRounding::ToNegativeInfinity: return std::floor(x);
        default: return std::ceil(x);
    }
}
inline double Round(double x, int digits, MidpointRounding m = MidpointRounding::ToEven) {
    // .NET: scale by 10^digits, round, unscale (values beyond 1e16 returned unchanged)
    if (std::fabs(x) >= 1e16) return x;
    double p = std::pow(10.0, digits); return Round(x * p, m) / p;
}
template <class T> T Abs(T v) { return v < 0 ? -v : v; }
template <class A, class B> auto Max(A a, B b) -> std::common_type_t<A, B> { return a > b ? a : b; }
template <class A, class B> auto Min(A a, B b) -> std::common_type_t<A, B> { return a < b ? a : b; }
template <class T> T Clamp(T v, T lo, T hi) { if (lo > hi) throw ArgumentException("Clamp"); return v < lo ? lo : (v > hi ? hi : v); }
inline double Floor(double x) { return std::floor(x); }
inline double Ceiling(double x) { return std::ceil(x); }
inline double Truncate(double x) { return std::trunc(x); }
inline double Pow(double a, double b) { return std::pow(a, b); }
inline double Sqrt(double x) { return std::sqrt(x); }
inline double Log(double x) { return std::log(x); }
inline double Log(double x, double b) { return std::log(x) / std::log(b); }
inline double Log10(double x) { return std::log10(x); }
inline double Exp(double x) { return std::exp(x); }
inline double Sin(double x) { return std::sin(x); }
inline double Cos(double x) { return std::cos(x); }
inline double Atan2(double y, double x) { return std::atan2(y, x); }
template <class T> int Sign(T v) { return v > 0 ? 1 : (v < 0 ? -1 : 0); }
inline constexpr double PI = 3.14159265358979323846;
}  // namespace Math

// C# (int)/(long) cast of a double: truncation; out-of-range gives the "integer indefinite" value as x64 .NET does.
template <class I> I CastDouble(double d) {
    if (std::isnan(d) || d >= (double)std::numeric_limits<I>::max() + 1.0 || d < (double)std::numeric_limits<I>::min())
        return sizeof(I) <= 4 ? (I)INT32_MIN : (I)INT64_MIN;
    return (I)d;
}
// C# unchecked integer wrap (C++ signed overflow is undefined; do it in unsigned)
template <class I> I Wrap(long long v) { return (I)(typename std::make_unsigned<I>::type)(unsigned long long)v; }

// ------------------------------------------------------------------ big integer (System.Numerics.BigInteger subset)
struct BigInteger {
    bool neg = false; std::vector<uint32_t> w;
    BigInteger() = default;
    BigInteger(long long v) { unsigned long long m = v < 0 ? (unsigned long long)(-(v + 1)) + 1u : (unsigned long long)v; neg = v < 0; while (m) { w.push_back((uint32_t)m); m >>= 32; } }
    static BigInteger One() { return BigInteger(1); }
    bool IsZero() const { return w.empty(); }
    bool IsEven() const { return w.empty() || (w[0] & 1u) == 0; }
    int Sign() const { return w.empty() ? 0 : (neg ? -1 : 1); }
    void trim() { while (!w.empty() && w.back() == 0) w.pop_back(); if (w.empty()) neg = false; }
    long long GetBitLength() const { if (w.empty()) return 0; uint32_t t = w.back(); int b = 0; while (t) { b++; t >>= 1; } return (long long)(w.size() - 1) * 32 + b; }
    static int cmpMag(const BigInteger& a, const BigInteger& b) {
        if (a.w.size() != b.w.size()) return a.w.size() < b.w.size() ? -1 : 1;
        for (size_t i = a.w.size(); i-- > 0;) if (a.w[i] != b.w[i]) return a.w[i] < b.w[i] ? -1 : 1;
        return 0;
    }
    int CompareTo(const BigInteger& o) const {
        if (neg != o.neg) return neg ? -1 : 1; int c = cmpMag(*this, o); return neg ? -c : c;
    }
    static BigInteger addMag(const BigInteger& a, const BigInteger& b) {
        BigInteger r; unsigned long long c = 0; size_t n = std::max(a.w.size(), b.w.size());
        for (size_t i = 0; i < n; i++) { unsigned long long s = c + (i < a.w.size() ? a.w[i] : 0) + (i < b.w.size() ? b.w[i] : 0); r.w.push_back((uint32_t)s); c = s >> 32; }
        if (c) r.w.push_back((uint32_t)c); r.trim(); return r;
    }
    static BigInteger subMag(const BigInteger& a, const BigInteger& b) {
        BigInteger r; long long br = 0;
        for (size_t i = 0; i < a.w.size(); i++) { long long s = (long long)a.w[i] - (i < b.w.size() ? b.w[i] : 0) - br; br = s < 0; if (s < 0) s += (1LL << 32); r.w.push_back((uint32_t)s); }
        r.trim(); return r;
    }
    BigInteger operator-() const { BigInteger r = *this; if (!r.IsZero()) r.neg = !r.neg; return r; }
    friend BigInteger operator+(const BigInteger& a, const BigInteger& b) {
        if (a.neg == b.neg) { BigInteger r = addMag(a, b); r.neg = a.neg && !r.IsZero(); return r; }
        int c = cmpMag(a, b); if (c == 0) return BigInteger();
        BigInteger r = c > 0 ? subMag(a, b) : subMag(b, a); r.neg = (c > 0 ? a.neg : b.neg) && !r.IsZero(); return r;
    }
    friend BigInteger operator-(const BigInteger& a, const BigInteger& b) { return a + (-b); }
    friend BigInteger operator*(const BigInteger& a, const BigInteger& b) {
        if (a.IsZero() || b.IsZero()) return BigInteger();
        BigInteger r; r.w.assign(a.w.size() + b.w.size(), 0);
        for (size_t i = 0; i < a.w.size(); i++) { unsigned long long c = 0;
            for (size_t j = 0; j < b.w.size(); j++) { unsigned long long t = (unsigned long long)a.w[i] * b.w[j] + r.w[i + j] + c; r.w[i + j] = (uint32_t)t; c = t >> 32; }
            size_t k = i + b.w.size(); while (c) { unsigned long long t = (unsigned long long)r.w[k] + c; r.w[k] = (uint32_t)t; c = t >> 32; k++; } }
        r.neg = a.neg != b.neg; r.trim(); return r;
    }
    // C# shifts on BigInteger: << multiplies by 2^n; >> is an arithmetic (floor) shift
    friend BigInteger operator<<(const BigInteger& a, long long n) {
        if (n < 0) return a >> -n; if (a.IsZero() || n == 0) return a; BigInteger r; r.neg = a.neg; int ws = (int)(n / 32), bs = (int)(n % 32);
        r.w.assign(ws, 0); uint32_t carry = 0;
        for (uint32_t x : a.w) { r.w.push_back(bs ? (x << bs) | carry : x); carry = bs ? x >> (32 - bs) : 0; }
        if (carry) r.w.push_back(carry); r.trim(); return r;
    }
    friend BigInteger operator>>(const BigInteger& a, long long n) {
        if (n < 0) return a << -n; if (a.IsZero() || n == 0) return a;
        BigInteger mag = a; mag.neg = false; int ws = (int)(n / 32), bs = (int)(n % 32); BigInteger r;
        bool lost = false;
        for (int i = 0; i < ws && i < (int)mag.w.size(); i++) if (mag.w[i]) lost = true;
        if (bs && ws < (int)mag.w.size() && (mag.w[ws] & ((1u << bs) - 1))) lost = true;
        for (size_t i = ws; i < mag.w.size(); i++) { unsigned long long v = mag.w[i] >> bs; if (bs && i + 1 < mag.w.size()) v |= (unsigned long long)mag.w[i + 1] << (32 - bs); r.w.push_back((uint32_t)v); }
        r.trim();
        if (a.neg) { r = r + BigInteger(lost ? 1 : 0); r.neg = !r.IsZero(); }  // floor for negatives
        return r;
    }
    static BigInteger Abs(const BigInteger& a) { BigInteger r = a; r.neg = false; return r; }
    static BigInteger DivRem(const BigInteger& a, const BigInteger& b, BigInteger& rem) {
        if (b.IsZero()) throw DivideByZeroException();
        BigInteger A = Abs(a), B = Abs(b), q, r;
        for (long long i = A.GetBitLength() - 1; i >= 0; i--) {
            r = r << 1; if ((A.w[(size_t)(i / 32)] >> (i % 32)) & 1u) { if (r.IsZero()) r.w.push_back(1); else r.w[0] |= 1u; }
            if (cmpMag(r, B) >= 0) { r = subMag(r, B); if (q.w.size() <= (size_t)(i / 32)) q.w.resize((size_t)(i / 32) + 1, 0); q.w[(size_t)(i / 32)] |= 1u << (i % 32); }
        }
        q.trim(); r.trim(); q.neg = !q.IsZero() && (a.neg != b.neg); r.neg = !r.IsZero() && a.neg; rem = r; return q;
    }
    friend BigInteger operator/(const BigInteger& a, const BigInteger& b) { BigInteger r; return DivRem(a, b, r); }
    friend BigInteger operator%(const BigInteger& a, const BigInteger& b) { BigInteger r; DivRem(a, b, r); return r; }
    friend bool operator==(const BigInteger& a, const BigInteger& b) { return a.CompareTo(b) == 0; }
    friend bool operator!=(const BigInteger& a, const BigInteger& b) { return a.CompareTo(b) != 0; }
    friend bool operator<(const BigInteger& a, const BigInteger& b) { return a.CompareTo(b) < 0; }
    friend bool operator>(const BigInteger& a, const BigInteger& b) { return a.CompareTo(b) > 0; }
    friend bool operator<=(const BigInteger& a, const BigInteger& b) { return a.CompareTo(b) <= 0; }
    friend bool operator>=(const BigInteger& a, const BigInteger& b) { return a.CompareTo(b) >= 0; }
    explicit operator long long() const { unsigned long long m = 0; for (size_t i = w.size(); i-- > 0;) m = (m << 32) | w[i]; return neg ? (long long)(0 - m) : (long long)m; }
    explicit operator double() const { double d = 0; for (size_t i = w.size(); i-- > 0;) d = d * 4294967296.0 + w[i]; return neg ? -d : d; }
    String ToString() const {
        if (IsZero()) return "0"; BigInteger a = Abs(*this), r, ten9(1000000000); String s;
        while (!a.IsZero()) { BigInteger q = DivRem(a, ten9, r); long long c = (long long)r; char b[16];
            std::snprintf(b, sizeof b, q.IsZero() ? "%lld" : "%09lld", c); s = String(b) + s; a = q; }
        return (neg ? "-" : "") + s;
    }
};

// ------------------------------------------------------------------ decimal (exact: mantissa * 10^-scale, .NET-style rounding)
struct Decimal {
    BigInteger m; int scale = 0;
    Decimal() = default;
    Decimal(int v) : m((long long)v) {}
    Decimal(long long v) : m(v) {}
    Decimal(long v) : m((long long)v) {}
    static Decimal Raw(const BigInteger& m, int s) { Decimal d; d.m = m; d.scale = s; return d.Normalize(); }
    static BigInteger P10(int n) { BigInteger r(1), t(10); for (int i = 0; i < n; i++) r = r * t; return r; }
    Decimal Normalize() const { return *this; }  // keeps trailing zeros like .NET (scale is significant for ToString)
    static void align(Decimal& a, Decimal& b) { if (a.scale < b.scale) { a.m = a.m * P10(b.scale - a.scale); a.scale = b.scale; } else if (b.scale < a.scale) { b.m = b.m * P10(a.scale - b.scale); b.scale = a.scale; } }
    // conversion from double: .NET rounds to 15 significant digits
    static Decimal FromDouble(double d) {
        if (d == 0) return Decimal(); char buf[64]; std::snprintf(buf, sizeof buf, "%.14e", d);
        String s(buf); bool neg = s[0] == '-'; if (neg) s = s.substr(1);
        size_t e = s.find('e'); int exp = std::stoi(s.substr(e + 1)); String digits = s.substr(0, 1) + s.substr(2, e - 2);
        BigInteger mm(0); for (char c : digits) mm = mm * BigInteger(10) + BigInteger(c - '0');
        int sc = 14 - exp; Decimal r; if (sc >= 0) { r.m = mm; r.scale = sc; } else { r.m = mm * P10(-sc); r.scale = 0; }
        while (r.scale > 0) { BigInteger rem; BigInteger q = BigInteger::DivRem(r.m, BigInteger(10), rem); if (!rem.IsZero()) break; r.m = q; r.scale--; }
        if (neg) r.m = -r.m; return r;
    }
    double ToDouble() const { return (double)m / std::pow(10.0, scale); }
    long long ToInt64() const { BigInteger r; return (long long)BigInteger::DivRem(m, P10(scale), r); }  // truncation, as (long)decimal
    friend Decimal operator+(Decimal a, Decimal b) { align(a, b); return Raw(a.m + b.m, a.scale); }
    friend Decimal operator-(Decimal a, Decimal b) { align(a, b); return Raw(a.m - b.m, a.scale); }
    Decimal operator-() const { return Raw(-m, scale); }
    friend Decimal operator*(const Decimal& a, const Decimal& b) { return Raw(a.m * b.m, a.scale + b.scale).Limit(); }
    friend Decimal operator/(const Decimal& a, const Decimal& b) {
        if (b.m.IsZero()) throw DivideByZeroException();
        // 28 fractional digits, then trailing zeros dropped (as .NET's division result)
        int s = 28; BigInteger num = a.m * P10(s + b.scale), rem; BigInteger q = BigInteger::DivRem(num, b.m, rem);
        // round half to even on the last digit
        BigInteger twice = BigInteger::Abs(rem) * BigInteger(2); int c = BigInteger::cmpMag(twice, BigInteger::Abs(b.m));
        if (c > 0 || (c == 0 && !q.IsEven())) q = q + BigInteger(q.Sign() < 0 || (q.IsZero() && (a.m.Sign() * b.m.Sign() < 0)) ? -1 : 1);
        Decimal r = Raw(q, s + a.scale);
        while (r.scale > 0) { BigInteger rr; BigInteger qq = BigInteger::DivRem(r.m, BigInteger(10), rr); if (!rr.IsZero()) break; r.m = qq; r.scale--; }
        return r.Limit();
    }
    Decimal Limit() const { Decimal r = *this; while (r.scale > 28) r = Round(r, r.scale - 1, MidpointRounding::ToEven); return r; }
    static Decimal Round(const Decimal& d, int digits = 0, MidpointRounding mode = MidpointRounding::ToEven) {
        if (d.scale <= digits) return d;
        BigInteger p = P10(d.scale - digits), rem; BigInteger q = BigInteger::DivRem(d.m, p, rem);
        BigInteger twice = BigInteger::Abs(rem) * BigInteger(2); int c = BigInteger::cmpMag(twice, p);
        bool up = false;
        switch (mode) {
            case MidpointRounding::ToEven: up = c > 0 || (c == 0 && !q.IsEven()); break;
            case MidpointRounding::AwayFromZero: up = c >= 0; break;
            case MidpointRounding::ToZero: up = false; break;
            case MidpointRounding::ToNegativeInfinity: up = d.m.Sign() < 0 && !rem.IsZero(); break;
            case MidpointRounding::ToPositiveInfinity: up = d.m.Sign() > 0 && !rem.IsZero(); break;
        }
        if (up) q = q + BigInteger(d.m.Sign() < 0 ? -1 : 1);
        return Raw(q, digits);
    }
    static Decimal Truncate(const Decimal& d) { return Round(d, 0, MidpointRounding::ToZero); }
    static Decimal Floor(const Decimal& d) { return Round(d, 0, MidpointRounding::ToNegativeInfinity); }
    int CompareTo(Decimal b) const { Decimal a = *this; align(a, b); return a.m.CompareTo(b.m); }
    friend bool operator==(const Decimal& a, const Decimal& b) { return a.CompareTo(b) == 0; }
    friend bool operator!=(const Decimal& a, const Decimal& b) { return a.CompareTo(b) != 0; }
    friend bool operator<(const Decimal& a, const Decimal& b) { return a.CompareTo(b) < 0; }
    friend bool operator>(const Decimal& a, const Decimal& b) { return a.CompareTo(b) > 0; }
    friend bool operator<=(const Decimal& a, const Decimal& b) { return a.CompareTo(b) <= 0; }
    friend bool operator>=(const Decimal& a, const Decimal& b) { return a.CompareTo(b) >= 0; }
    String ToString() const {
        String s = BigInteger::Abs(m).ToString(); if (scale > 0) { if ((int)s.size() <= scale) s = String(scale - s.size() + 1, '0') + s; s.insert(s.size() - scale, "."); }
        return (m.Sign() < 0 ? "-" : "") + s;
    }
    static bool TryParse(const String& t, Decimal& out) {
        String s = t; size_t a = s.find_first_not_of(" \t"), b = s.find_last_not_of(" \t"); if (a == String::npos) return false; s = s.substr(a, b - a + 1);
        bool neg = false; size_t i = 0; if (s[0] == '-' || s[0] == '+') { neg = s[0] == '-'; i = 1; }
        BigInteger mm(0); int sc = 0; bool dot = false, any = false;
        for (; i < s.size(); i++) { char c = s[i]; if (c == '.' && !dot) { dot = true; continue; } if (c == ',') continue; if (c < '0' || c > '9') return false; mm = mm * BigInteger(10) + BigInteger(c - '0'); if (dot) sc++; any = true; }
        if (!any) return false; out = Raw(neg ? -mm : mm, sc); return true;
    }
};


// value comparison helpers for generated tuple structs: use the member's own == / < when it has one
template <class T, class = void> struct HasLess : std::false_type {};
template <class T> struct HasLess<T, std::void_t<decltype(std::declval<const T&>() < std::declval<const T&>())>> : std::true_type {};
template <class T, class = void> struct HasEq : std::false_type {};
template <class T> struct HasEq<T, std::void_t<decltype(std::declval<const T&>() == std::declval<const T&>())>> : std::true_type {};
template <class T> bool Less(const T& a, const T& b) { if constexpr (HasLess<T>::value) return a < b; else return false; }
template <class T> bool Eq(const T& a, const T& b) { if constexpr (HasEq<T>::value) return a == b; else return &a == &b; }

// ------------------------------------------------------------------ text
inline String ToStr(const String& s) { return s; }
inline String ToStr(const char* s) { return s ? String(s) : String(); }
inline String ToStr(char c) { return String(1, c); }
inline String ToStr(bool b) { return b ? "True" : "False"; }
template <class I, std::enable_if_t<std::is_integral<I>::value && !std::is_same<I, bool>::value && !std::is_same<I, char>::value, int> = 0>
String ToStr(I v) { return std::to_string(v); }
// double/float.ToString() (.NET Core 3.0+): shortest round-trip digits; scientific ("1E+17", "1E-05") when the
// decimal exponent is >= 17 (double) / >= 9 (float), or <= -5
template <class F> String ShortestText(F d, int sciFrom) {
    if (std::isnan(d)) return "NaN"; if (std::isinf(d)) return d > 0 ? "Infinity" : "-Infinity";  // invariant culture (.NET Core 3.0+)
    char buf[64]; auto r = std::to_chars(buf, buf + sizeof buf, d, std::chars_format::scientific); String s(buf, r.ptr);
    size_t e = s.find('e'); int exp = std::stoi(s.substr(e + 1)); String mant = s.substr(0, e);
    if (exp >= sciFrom || exp <= -5) return mant + "E" + (exp < 0 ? "-" : "+") + (std::abs(exp) < 10 ? "0" : "") + std::to_string(std::abs(exp));
    char b2[400]; auto r2 = std::to_chars(b2, b2 + sizeof b2, d, std::chars_format::fixed); return String(b2, r2.ptr);
}
inline String ToStr(double d) { return ShortestText(d, 17); }
inline String ToStr(float f) { return ShortestText(f, 9); }
inline String ToStr(const Decimal& d) { return d.ToString(); }
inline String ToStr(const BigInteger& b) { return b.ToString(); }
template <class T> String ToStr(const Nullable<T>& v) { return v ? ToStr(*v) : String(); }
template <class E, std::enable_if_t<std::is_enum<E>::value, int> = 0> String ToStr(E e);  // generated per enum (EnumName)

inline String Group3(String digits) { String out; int n = (int)digits.size(); for (int i = 0; i < n; i++) { out += digits[i]; int left = n - i - 1; if (left > 0 && left % 3 == 0) out += ','; } return out; }
// Fixed-point text with 'dec' decimals, rounded half away from zero like .NET's number formatting.
inline String FixedText(double v, int dec, bool group) {
    double p = std::pow(10.0, dec); double r = std::round(std::fabs(v) * p);
    char buf[512]; std::snprintf(buf, sizeof buf, "%.0f", r); String digits(buf);
    if ((int)digits.size() <= dec) digits = String(dec - digits.size() + 1, '0') + digits;
    String ip = digits.substr(0, digits.size() - dec), fp = digits.substr(digits.size() - dec);
    if (group) ip = Group3(ip);
    bool negZero = r == 0; return (v < 0 && !negZero ? "-" : "") + ip + (dec > 0 ? "." + fp : "");
}
inline String FixedText(const Decimal& v, int dec, bool group) {
    Decimal r = Decimal::Round(v, dec, MidpointRounding::AwayFromZero); String s = BigInteger::Abs(r.m).ToString();
    int sc = r.scale; if ((int)s.size() <= sc) s = String(sc - s.size() + 1, '0') + s;
    String ip = s.substr(0, s.size() - sc), fp = s.substr(s.size() - sc); while ((int)fp.size() < dec) fp += '0';
    if (group) ip = Group3(ip); return (r.m.Sign() < 0 ? "-" : "") + ip + (dec > 0 ? "." + fp : "");
}
// Custom formats like "0.######", "0.00", "#,##0", "00"
template <class V> String CustomText(V v, const String& f) {
    size_t dot = f.find('.'); String ip = f.substr(0, dot), fp = dot == String::npos ? "" : f.substr(dot + 1);
    int req = (int)std::count(fp.begin(), fp.end(), '0'), opt = (int)std::count(fp.begin(), fp.end(), '#');
    bool group = ip.find(',') != String::npos; int minInt = (int)std::count(ip.begin(), ip.end(), '0');
    String s = FixedText(v, req + opt, group); bool neg = !s.empty() && s[0] == '-'; if (neg) s = s.substr(1);
    size_t d = s.find('.'); String a = d == String::npos ? s : s.substr(0, d), b = d == String::npos ? "" : s.substr(d + 1);
    while ((int)b.size() > req && !b.empty() && b.back() == '0') b.pop_back();
    String digitsOnly; for (char c : a) if (c != ',') digitsOnly += c;
    if (digitsOnly == "0" && minInt == 0) a = "";
    while ((int)digitsOnly.size() < minInt) { digitsOnly = "0" + digitsOnly; a = group ? Group3(digitsOnly) : digitsOnly; }
    String out = a + (b.empty() ? "" : "." + b); if (out.empty()) out = "0";
    bool allZero = out.find_first_not_of("0.,") == String::npos;
    return (neg && !allZero ? "-" : "") + out;
}
// value.ToString(format) / {value:format}, invariant culture
template <class V> String Format(V v, const String& f) {
    if (f.empty()) return ToStr(v);
    char k = (char)std::toupper((unsigned char)f[0]); bool std1 = f.size() <= 3 && std::all_of(f.begin() + 1, f.end(), ::isdigit) && std::strchr("NFDXCPGE", k);
    if (std1) {
        int n = f.size() > 1 ? std::stoi(f.substr(1)) : -1;
        if constexpr (std::is_integral<V>::value) {
            if (k == 'D') { String s = std::to_string(v < 0 ? -(long long)v : (long long)v); while ((int)s.size() < n) s = "0" + s; return (v < 0 ? "-" : "") + s; }
            if (k == 'X') { char b[32]; std::snprintf(b, sizeof b, f[0] == 'x' ? "%llx" : "%llX", (unsigned long long)(typename std::make_unsigned<V>::type)v); String s(b); while ((int)s.size() < n) s = "0" + s; return s; }
            if (k == 'N') return FixedText((double)v, n < 0 ? 2 : n, true);
            if (k == 'F') return FixedText((double)v, n < 0 ? 2 : n, false);
            if (k == 'G') return ToStr(v);
        } else {
            if (k == 'N') return FixedText(v, n < 0 ? 2 : n, true);
            if (k == 'F') return FixedText(v, n < 0 ? 2 : n, false);
            if (k == 'G') return ToStr(v);
            if (k == 'P') return FixedText(v * 100, n < 0 ? 2 : n, true) + " %";
        }
    }
    return CustomText(v, f);
}
template <class V> String Align(const String& s, int width) {  // {v,width}
    (void)sizeof(V); if (width >= 0) return (int)s.size() >= width ? s : String(width - s.size(), ' ') + s;
    width = -width; return (int)s.size() >= width ? s : s + String(width - s.size(), ' ');
}
inline String AlignText(const String& s, int width) { if (width >= 0) return (int)s.size() >= width ? s : String(width - s.size(), ' ') + s; width = -width; return (int)s.size() >= width ? s : s + String(width - s.size(), ' '); }

namespace Str {
inline bool IsNullOrEmpty(const String& s) { return s.empty(); }
inline bool IsNullOrWhiteSpace(const String& s) { return s.find_first_not_of(" \t\r\n\v\f") == String::npos; }
inline String Trim(const String& s) { size_t a = s.find_first_not_of(" \t\r\n\v\f"); if (a == String::npos) return ""; size_t b = s.find_last_not_of(" \t\r\n\v\f"); return s.substr(a, b - a + 1); }
inline String TrimEnd(const String& s) { size_t b = s.find_last_not_of(" \t\r\n\v\f"); return b == String::npos ? "" : s.substr(0, b + 1); }
inline String TrimStart(const String& s) { size_t a = s.find_first_not_of(" \t\r\n\v\f"); return a == String::npos ? "" : s.substr(a); }
inline String TrimEnd(const String& s, const String& chars) { size_t b = s.find_last_not_of(chars); return b == String::npos ? "" : s.substr(0, b + 1); }
inline String TrimStart(const String& s, const String& chars) { size_t a = s.find_first_not_of(chars); return a == String::npos ? "" : s.substr(a); }
inline String Trim(const String& s, const String& chars) { return TrimStart(TrimEnd(s, chars), chars); }
inline String TrimEnd(const String& s, char c) { return TrimEnd(s, String(1, c)); }
inline String TrimStart(const String& s, char c) { return TrimStart(s, String(1, c)); }
inline String Trim(const String& s, char c) { return Trim(s, String(1, c)); }
inline String ToUpper(String s) { for (auto& c : s) c = (char)std::toupper((unsigned char)c); return s; }
inline String ToLower(String s) { for (auto& c : s) c = (char)std::tolower((unsigned char)c); return s; }
inline String ToUpperInvariant(const String& s) { return ToUpper(s); }
inline String ToLowerInvariant(const String& s) { return ToLower(s); }
inline bool EqualsIgnoreCase(const String& a, const String& b) { return ToLower(a) == ToLower(b); }
inline bool StartsWith(const String& s, const String& p) { return s.size() >= p.size() && s.compare(0, p.size(), p) == 0; }
inline bool StartsWith(const String& s, char c) { return !s.empty() && s[0] == c; }
inline bool EndsWith(const String& s, const String& p) { return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0; }
inline bool EndsWith(const String& s, char c) { return !s.empty() && s.back() == c; }
inline bool StartsWithIgnoreCase(const String& s, const String& p) { return StartsWith(ToLower(s), ToLower(p)); }
inline bool EndsWithIgnoreCase(const String& s, const String& p) { return EndsWith(ToLower(s), ToLower(p)); }
inline bool Contains(const String& s, const String& p) { return s.find(p) != String::npos; }
inline bool Contains(const String& s, char c) { return s.find(c) != String::npos; }
inline bool ContainsIgnoreCase(const String& s, const String& p) { return ToLower(s).find(ToLower(p)) != String::npos; }
inline int IndexOf(const String& s, const String& p, int start = 0) { size_t i = s.find(p, (size_t)start); return i == String::npos ? -1 : (int)i; }
inline int IndexOf(const String& s, char c, int start = 0) { size_t i = s.find(c, (size_t)start); return i == String::npos ? -1 : (int)i; }
inline int IndexOfIgnoreCase(const String& s, const String& p) { return IndexOf(ToLower(s), ToLower(p)); }
inline int LastIndexOf(const String& s, const String& p) { size_t i = s.rfind(p); return i == String::npos ? -1 : (int)i; }
inline int LastIndexOf(const String& s, char c) { size_t i = s.rfind(c); return i == String::npos ? -1 : (int)i; }
inline int IndexOfAny(const String& s, const String& chars) { size_t i = s.find_first_of(chars); return i == String::npos ? -1 : (int)i; }
inline String Substring(const String& s, int start) { if (start < 0 || start > (int)s.size()) throw ArgumentOutOfRangeException("Substring"); return s.substr((size_t)start); }
inline String Substring(const String& s, int start, int len) { if (start < 0 || len < 0 || start + len > (int)s.size()) throw ArgumentOutOfRangeException("Substring"); return s.substr((size_t)start, (size_t)len); }
inline String Replace(String s, const String& a, const String& b) { if (a.empty()) return s; size_t p = 0; while ((p = s.find(a, p)) != String::npos) { s.replace(p, a.size(), b); p += b.size(); } return s; }
inline String Replace(String s, char a, char b) { for (auto& c : s) if (c == a) c = b; return s; }
inline String PadLeft(const String& s, int w, char c = ' ') { return (int)s.size() >= w ? s : String(w - s.size(), c) + s; }
inline String PadRight(const String& s, int w, char c = ' ') { return (int)s.size() >= w ? s : s + String(w - s.size(), c); }
inline String Repeat(char c, int n) { return n > 0 ? String((size_t)n, c) : String(); }
inline String Insert(String s, int at, const String& t) { s.insert((size_t)at, t); return s; }
inline String Remove(String s, int at) { return s.substr(0, (size_t)at); }
inline String Remove(String s, int at, int n) { s.erase((size_t)at, (size_t)n); return s; }
inline int CompareOrdinal(const String& a, const String& b) { int c = a.compare(b); return c < 0 ? -1 : (c > 0 ? 1 : 0); }
inline int CompareIgnoreCase(const String& a, const String& b) { return CompareOrdinal(ToLower(a), ToLower(b)); }
}  // namespace Str

// StringBuilder
struct StringBuilder {
    String s;
    StringBuilder() = default; explicit StringBuilder(const String& t) : s(t) {}
    template <class T> StringBuilder& Append(const T& v) { s += ToStr(v); return *this; }
    StringBuilder& Append(const String& v) { s += v; return *this; }
    StringBuilder& Append(const char* v) { s += v; return *this; }
    StringBuilder& Append(char c, int n) { s += String((size_t)n, c); return *this; }
    StringBuilder& AppendLine(const String& v = "") { s += v; s += "\n"; return *this; }
    StringBuilder& Clear() { s.clear(); return *this; }
    int Length() const { return (int)s.size(); }
    String ToString() const { return s; }
};

// ------------------------------------------------------------------ collections (reference semantics)
template <class T> class List;
template <class T> using IEnumerable = List<T>;  // sequences are materialised (yield return builds a List)
template <class T> using Array = List<T>;        // arrays are fixed-size Lists here

template <class K, class V> struct KeyValuePair { K Key; V Value; };

template <class T> using Store = std::conditional_t<std::is_same<T, bool>::value, std::deque<bool>, std::vector<T>>;  // std::vector<bool> has no element references
// Enumerable.Sum: int and long sums are checked (OverflowException); narrower integers sum as int;
// a float sum is accumulated in double, as .NET does
template <class R> struct SumOf {
    using Acc = std::conditional_t<std::is_integral<R>::value && sizeof(R) < 4, int32_t, std::conditional_t<std::is_same<R, float>::value, double, R>>;
    Acc s{};
    void Add(const R& x) {
        if constexpr (std::is_integral<Acc>::value) { if (__builtin_add_overflow(s, (Acc)x, &s)) throw OverflowException("Arithmetic operation resulted in an overflow."); }
        else s = s + x;
    }
    auto Result() const { if constexpr (std::is_same<R, float>::value) return (float)s; else return s; }
};
template <class T>
class List {
    std::shared_ptr<Store<T>> p;
    Store<T>& v() const { if (!p) throw NullReferenceException(); return *p; }
public:
    List() = default;                          // null
    List(std::nullptr_t) {}
    List(std::initializer_list<T> il) : p(std::make_shared<Store<T>>(il)) {}
    explicit List(std::vector<T> vec) : p(std::make_shared<Store<T>>(vec.begin(), vec.end())) {}
    template <class U = T, std::enable_if_t<std::is_same<U, bool>::value, int> = 0> explicit List(Store<U> d) : p(std::make_shared<Store<T>>(std::move(d))) {}
    static List New() { return List(std::vector<T>()); }
    static List New(int n) { if (n < 0) throw ArgumentOutOfRangeException("array size"); List r = New(); r.v().resize((size_t)n); return r; }  // new T[n]
    static List Of(std::initializer_list<T> il) { return List(std::vector<T>(il)); }
    template <class Seq> static List From(const Seq& s) { List r = New(); for (const auto& x : s) r.v().push_back(x); return r; }
    bool IsNull() const { return !p; }
    const void* Identity() const { return p.get(); }   // which list object this handle refers to
    explicit operator bool() const { return (bool)p; }
    friend bool operator==(const List& a, std::nullptr_t) { return !a.p; }
    friend bool operator!=(const List& a, std::nullptr_t) { return (bool)a.p; }
    friend bool operator==(const List& a, const List& b) { return a.p == b.p; }  // reference equality, as C#
    friend bool operator!=(const List& a, const List& b) { return a.p != b.p; }
    Store<T>& Raw() const { return v(); }
    int Count() const { return (int)v().size(); }
    int Length() const { return (int)v().size(); }
    T& operator[](int i) const { if (i < 0 || i >= (int)v().size()) throw ArgumentOutOfRangeException("index"); return v()[(size_t)i]; }
    auto begin() const { return v().begin(); }
    auto end() const { return v().end(); }
    void Add(const T& x) const { v().push_back(x); }
    template <class Seq> void AddRange(const Seq& s) const { std::vector<T> tmp; for (const auto& x : s) tmp.push_back(x); for (auto& x : tmp) v().push_back(x); }
    void Insert(int i, const T& x) const { if (i < 0 || i > Count()) throw ArgumentOutOfRangeException("index"); v().insert(v().begin() + i, x); }
    void RemoveAt(int i) const { if (i < 0 || i >= Count()) throw ArgumentOutOfRangeException("index"); v().erase(v().begin() + i); }
    void RemoveRange(int i, int n) const { v().erase(v().begin() + i, v().begin() + i + n); }
    bool Remove(const T& x) const { auto it = std::find(v().begin(), v().end(), x); if (it == v().end()) return false; v().erase(it); return true; }
    template <class F> int RemoveAll(F f) const { auto it = std::stable_partition(v().begin(), v().end(), [&](const T& x) { return !f(x); }); int n = (int)(v().end() - it); v().erase(it, v().end()); return n; }
    void Clear() const { v().clear(); }
    bool Contains(const T& x) const { return std::find(v().begin(), v().end(), x) != v().end(); }
    int IndexOf(const T& x) const { auto it = std::find(v().begin(), v().end(), x); return it == v().end() ? -1 : (int)(it - v().begin()); }
    template <class F> int FindIndex(F f) const { for (int i = 0; i < Count(); i++) if (f(v()[(size_t)i])) return i; return -1; }
    template <class F> T Find(F f) const { for (auto& x : v()) if (f(x)) return x; return T(); }
    template <class F> List FindAll(F f) const { return Where(f); }
    template <class F> bool Exists(F f) const { return Any(f); }
    template <class F> void ForEach(F f) const { for (auto& x : v()) f(x); }
    void Reverse() const { std::reverse(v().begin(), v().end()); }  // List<T>.Reverse(): in place
    List Reversed() const { List r = ToList(); r.Reverse(); return r; }  // LINQ .Reverse()
    void Sort() const { std::stable_sort(v().begin(), v().end()); }  // note: .NET's List.Sort is unstable (introsort)
    template <class C> void Sort(C cmp) const { std::stable_sort(v().begin(), v().end(), [&](const T& a, const T& b) { return cmp(a, b) < 0; }); }
    void Fill(const T& x) const { std::fill(v().begin(), v().end(), x); }
    List ToList() const { List r = New(); for (auto& x : v()) r.v().push_back(x); return r; }
    List ToArray() const { return ToList(); }
    List Clone() const { return ToList(); }
    // ---- LINQ (eager)
    template <class F> auto Select(F f) const { using R = std::decay_t<decltype(f(std::declval<T&>()))>; List<R> r = List<R>::New(); for (auto& x : v()) r.Add(f(x)); return r; }
    template <class F> auto SelectWithIndex(F f) const { using R = std::decay_t<decltype(f(std::declval<T&>(), 0))>; List<R> r = List<R>::New(); int i = 0; for (auto& x : v()) r.Add(f(x, i++)); return r; }
    template <class F> auto SelectMany(F f) const { using L = std::decay_t<decltype(f(std::declval<T&>()))>; using R = std::decay_t<decltype(*std::declval<L>().begin())>; List<R> r = List<R>::New(); for (auto& x : v()) for (auto& y : f(x)) r.Add(y); return r; }
    template <class F> List Where(F f) const { List r = New(); for (auto& x : v()) if (f(x)) r.Add(x); return r; }
    template <class F> List WhereWithIndex(F f) const { List r = New(); int i = 0; for (auto& x : v()) if (f(x, i++)) r.Add(x); return r; }
    bool Any() const { return !v().empty(); }
    template <class F> bool Any(F f) const { for (auto& x : v()) if (f(x)) return true; return false; }
    template <class F> bool All(F f) const { for (auto& x : v()) if (!f(x)) return false; return true; }
    template <class F> int Count(F f) const { int n = 0; for (auto& x : v()) if (f(x)) n++; return n; }
    template <class F> auto Sum(F f) const { using R = std::decay_t<decltype(f(std::declval<T&>()))>; SumOf<R> s; for (auto& x : v()) s.Add(f(x)); return s.Result(); }
    auto Sum() const { SumOf<T> s; for (auto& x : v()) s.Add(x); return s.Result(); }
    template <class F> auto Max(F f) const { if (v().empty()) throw InvalidOperationException("Sequence contains no elements"); auto m = f(v()[0]); for (auto& x : v()) { auto y = f(x); if (y > m) m = y; } return m; }
    T Max() const { if (v().empty()) throw InvalidOperationException("Sequence contains no elements"); T m = v()[0]; for (auto& x : v()) if (x > m) m = x; return m; }
    template <class F> auto Min(F f) const { if (v().empty()) throw InvalidOperationException("Sequence contains no elements"); auto m = f(v()[0]); for (auto& x : v()) { auto y = f(x); if (y < m) m = y; } return m; }
    T Min() const { if (v().empty()) throw InvalidOperationException("Sequence contains no elements"); T m = v()[0]; for (auto& x : v()) if (x < m) m = x; return m; }
    template <class F> T MaxBy(F f) const { if (v().empty()) throw InvalidOperationException("Sequence contains no elements"); size_t b = 0; for (size_t i = 1; i < v().size(); i++) if (f(v()[i]) > f(v()[b])) b = i; return v()[b]; }
    template <class F> T MinBy(F f) const { if (v().empty()) throw InvalidOperationException("Sequence contains no elements"); size_t b = 0; for (size_t i = 1; i < v().size(); i++) if (f(v()[i]) < f(v()[b])) b = i; return v()[b]; }
    template <class F> double Average(F f) const { if (v().empty()) throw InvalidOperationException("Sequence contains no elements"); double s = 0; for (auto& x : v()) s += (double)f(x); return s / v().size(); }
    T First() const { if (v().empty()) throw InvalidOperationException("Sequence contains no elements"); return v()[0]; }
    template <class F> T First(F f) const { for (auto& x : v()) if (f(x)) return x; throw InvalidOperationException("Sequence contains no matching element"); }
    T FirstOrDefault() const { return v().empty() ? T() : v()[0]; }
    template <class F> T FirstOrDefault(F f) const { for (auto& x : v()) if (f(x)) return x; return T(); }
    template <class F> T FirstOrDefault(F f, const T& dflt) const { for (auto& x : v()) if (f(x)) return x; return dflt; }
    T Last() const { if (v().empty()) throw InvalidOperationException("Sequence contains no elements"); return v().back(); }
    template <class F> T Last(F f) const { for (size_t i = v().size(); i-- > 0;) if (f(v()[i])) return v()[i]; throw InvalidOperationException("Sequence contains no matching element"); }
    T LastOrDefault() const { return v().empty() ? T() : v().back(); }
    template <class F> T LastOrDefault(F f) const { for (size_t i = v().size(); i-- > 0;) if (f(v()[i])) return v()[i]; return T(); }
    T Single() const { if (v().size() != 1) throw InvalidOperationException("Sequence does not contain exactly one element"); return v()[0]; }
    template <class F> T Single(F f) const { List r = Where(f); return r.Single(); }
    template <class F> T SingleOrDefault(F f) const { List r = Where(f); if (r.Count() > 1) throw InvalidOperationException("more than one"); return r.Count() ? r[0] : T(); }
    T ElementAt(int i) const { return (*this)[i]; }
    T ElementAtOrDefault(int i) const { return i >= 0 && i < Count() ? v()[(size_t)i] : T(); }
    // OrderBy is stable in .NET; ThenBy keys are applied as tie-breakers in order
    template <class F> List OrderBy(F f) const { List r = ToList(); std::stable_sort(r.v().begin(), r.v().end(), [&](const T& a, const T& b) { return f(a) < f(b); }); return r; }
    template <class F> List OrderByDescending(F f) const { List r = ToList(); std::stable_sort(r.v().begin(), r.v().end(), [&](const T& a, const T& b) { return f(b) < f(a); }); return r; }
    template <class F1, class F2> List OrderByThenBy(F1 f1, F2 f2, bool d1 = false, bool d2 = false) const {
        List r = ToList(); std::stable_sort(r.v().begin(), r.v().end(), [&](const T& a, const T& b) {
            auto ka = f1(a), kb = f1(b); if (ka < kb) return !d1; if (kb < ka) return d1;
            auto la = f2(a), lb = f2(b); return d2 ? (lb < la) : (la < lb); }); return r; }
    List Distinct() const { List r = New(); for (auto& x : v()) if (!r.Contains(x)) r.Add(x); return r; }
    template <class F> List DistinctBy(F f) const { List r = New(); std::vector<std::decay_t<decltype(f(std::declval<T&>()))>> seen; for (auto& x : v()) { auto k = f(x); if (std::find(seen.begin(), seen.end(), k) == seen.end()) { seen.push_back(k); r.Add(x); } } return r; }
    List Take(int n) const { List r = New(); for (int i = 0; i < n && i < Count(); i++) r.Add(v()[(size_t)i]); return r; }
    List Skip(int n) const { List r = New(); for (int i = std::max(0, n); i < Count(); i++) r.Add(v()[(size_t)i]); return r; }
    template <class F> List TakeWhile(F f) const { List r = New(); for (auto& x : v()) { if (!f(x)) break; r.Add(x); } return r; }
    template <class F> List SkipWhile(F f) const { List r = New(); bool skipping = true; for (auto& x : v()) { if (skipping && f(x)) continue; skipping = false; r.Add(x); } return r; }
    template <class Seq> List Concat(const Seq& s) const { List r = ToList(); for (const auto& x : s) r.Add(x); return r; }
    List Append(const T& x) const { List r = ToList(); r.Add(x); return r; }
    List Prepend(const T& x) const { List r = ToList(); r.Insert(0, x); return r; }
    template <class Seq> bool SequenceEqual(const Seq& s) const { int i = 0; for (const auto& x : s) { if (i >= Count() || !(v()[(size_t)i] == x)) return false; i++; } return i == Count(); }
    template <class Seq> List Except(const Seq& s) const { List r = New(); for (auto& x : v()) { bool in = false; for (const auto& y : s) if (y == x) in = true; if (!in && !r.Contains(x)) r.Add(x); } return r; }
    template <class Seq> List Intersect(const Seq& s) const { List r = New(); for (auto& x : v()) { bool in = false; for (const auto& y : s) if (y == x) in = true; if (in && !r.Contains(x)) r.Add(x); } return r; }
    template <class A, class F> A Aggregate(A seed, F f) const { for (auto& x : v()) seed = f(seed, x); return seed; }
    template <class Seq, class F> auto Zip(const Seq& s, F f) const { using R = std::decay_t<decltype(f(std::declval<T&>(), *s.begin()))>; List<R> r = List<R>::New(); auto it = s.begin(); for (auto& x : v()) { if (it == s.end()) break; r.Add(f(x, *it)); ++it; } return r; }
    template <class F> auto GroupBy(F f) const;
    template <class FK, class FV> auto ToDictionary(FK fk, FV fv) const;
    template <class FK> auto ToDictionary(FK fk) const;
    template <class FK> auto ToLookupList(FK fk) const { return GroupBy(fk); }
    List<T> CopyTo(List<T> dst, int at = 0) const { for (int i = 0; i < Count(); i++) dst[at + i] = v()[(size_t)i]; return dst; }
    List GetRange(int i, int n) const { List r = New(); for (int k = i; k < i + n; k++) r.Add((*this)[k]); return r; }
};

namespace Enumerable {
inline List<int> Range(int start, int count) { List<int> r = List<int>::New(); for (int i = 0; i < count; i++) r.Add(start + i); return r; }
template <class T> List<T> Repeat(const T& x, int n) { List<T> r = List<T>::New(); for (int i = 0; i < n; i++) r.Add(x); return r; }
template <class T> List<T> Empty() { return List<T>::New(); }
}  // namespace Enumerable

template <class K, class V>
class Dictionary {
    struct Body { std::vector<KeyValuePair<K, V>> items; std::map<K, size_t> index; };
    std::shared_ptr<Body> p;
    Body& b() const { if (!p) throw NullReferenceException(); return *p; }
    void reindex() const { b().index.clear(); for (size_t i = 0; i < b().items.size(); i++) b().index[b().items[i].Key] = i; }
public:
    Dictionary() = default; Dictionary(std::nullptr_t) {}
    static Dictionary New() { Dictionary d; d.p = std::make_shared<Body>(); return d; }
    static Dictionary Of(std::initializer_list<KeyValuePair<K, V>> il) { Dictionary d = New(); for (auto& kv : il) d.Set(kv.Key, kv.Value); return d; }
    bool IsNull() const { return !p; }
    explicit operator bool() const { return (bool)p; }
    friend bool operator==(const Dictionary& a, std::nullptr_t) { return !a.p; }
    friend bool operator!=(const Dictionary& a, std::nullptr_t) { return (bool)a.p; }
    int Count() const { return (int)b().items.size(); }
    bool ContainsKey(const K& k) const { return b().index.count(k) > 0; }
    bool TryGetValue(const K& k, V& out) const { auto it = b().index.find(k); if (it == b().index.end()) { out = V(); return false; } out = b().items[it->second].Value; return true; }
    V Get(const K& k) const { auto it = b().index.find(k); if (it == b().index.end()) throw KeyNotFoundException("key not found"); return b().items[it->second].Value; }
    V& GetRef(const K& k) const { auto it = b().index.find(k); if (it == b().index.end()) throw KeyNotFoundException("key not found"); return b().items[it->second].Value; }
    void Set(const K& k, const V& v) const { auto it = b().index.find(k); if (it != b().index.end()) b().items[it->second].Value = v; else { b().index[k] = b().items.size(); b().items.push_back({k, v}); } }
    void Add(const K& k, const V& v) const { if (ContainsKey(k)) throw ArgumentException("An item with the same key has already been added."); Set(k, v); }
    bool TryAdd(const K& k, const V& v) const { if (ContainsKey(k)) return false; Set(k, v); return true; }
    bool Remove(const K& k) const { auto it = b().index.find(k); if (it == b().index.end()) return false; b().items.erase(b().items.begin() + (long)it->second); reindex(); return true; }
    void Clear() const { b().items.clear(); b().index.clear(); }
    V GetValueOrDefault(const K& k, const V& d = V()) const { auto it = b().index.find(k); return it == b().index.end() ? d : b().items[it->second].Value; }
    // d[k] read (throws if missing) and d[k] = v write
    struct Proxy { const Dictionary* d; K k; operator V() const { return d->Get(k); } Proxy& operator=(const V& v) { d->Set(k, v); return *this; }
        V operator+(const V& o) const { return d->Get(k) + o; } Proxy& operator+=(const V& o) { d->Set(k, d->Get(k) + o); return *this; } Proxy& operator-=(const V& o) { d->Set(k, d->Get(k) - o); return *this; }
        Proxy& operator++() { d->Set(k, d->Get(k) + 1); return *this; } };
    Proxy operator[](const K& k) const { return Proxy{this, k}; }
    List<K> Keys() const { List<K> r = List<K>::New(); for (auto& kv : b().items) r.Add(kv.Key); return r; }
    List<V> Values() const { List<V> r = List<V>::New(); for (auto& kv : b().items) r.Add(kv.Value); return r; }
    List<KeyValuePair<K, V>> Pairs() const { return List<KeyValuePair<K, V>>(b().items); }
    typename std::vector<KeyValuePair<K, V>>::iterator begin() const { return b().items.begin(); }
    typename std::vector<KeyValuePair<K, V>>::iterator end() const { return b().items.end(); }
};

template <class T>
class HashSet {
    List<T> l;
public:
    HashSet() = default; HashSet(std::nullptr_t) {}
    static HashSet New() { HashSet h; h.l = List<T>::New(); return h; }
    template <class Seq> static HashSet From(const Seq& s) { HashSet h = New(); for (const auto& x : s) h.Add(x); return h; }
    bool IsNull() const { return l.IsNull(); }
    friend bool operator==(const HashSet& a, std::nullptr_t) { return a.l.IsNull(); }
    friend bool operator!=(const HashSet& a, std::nullptr_t) { return !a.l.IsNull(); }
    bool Add(const T& x) const { if (l.Contains(x)) return false; l.Add(x); return true; }
    bool Contains(const T& x) const { return l.Contains(x); }
    bool Remove(const T& x) const { return l.Remove(x); }
    int Count() const { return l.Count(); }
    void Clear() const { l.Clear(); }
    template <class Seq> void UnionWith(const Seq& s) const { for (const auto& x : s) Add(x); }
    template <class Seq> void ExceptWith(const Seq& s) const { for (const auto& x : s) Remove(x); }
    List<T> ToList() const { return l.ToList(); }
    auto begin() const { return l.begin(); }
    auto end() const { return l.end(); }
};

template <class K, class T> struct Grouping { K Key; List<T> Items; auto begin() const { return Items.begin(); } auto end() const { return Items.end(); } int Count() const { return Items.Count(); } };
template <class T> template <class F> auto List<T>::GroupBy(F f) const {
    using Kt = std::decay_t<decltype(f(std::declval<T&>()))>; List<Grouping<Kt, T>> r = List<Grouping<Kt, T>>::New();
    for (auto& x : v()) { Kt k = f(x); int at = -1; for (int i = 0; i < r.Count(); i++) if (r[i].Key == k) { at = i; break; }
        if (at < 0) { r.Add(Grouping<Kt, T>{k, List<T>::New()}); at = r.Count() - 1; } r[at].Items.Add(x); }
    return r;
}
template <class T> template <class FK, class FV> auto List<T>::ToDictionary(FK fk, FV fv) const {
    using Kt = std::decay_t<decltype(fk(std::declval<T&>()))>; using Vt = std::decay_t<decltype(fv(std::declval<T&>()))>;
    auto d = Dictionary<Kt, Vt>::New(); for (auto& x : v()) d.Add(fk(x), fv(x)); return d;
}
template <class T> template <class FK> auto List<T>::ToDictionary(FK fk) const { return ToDictionary(fk, [](const T& x) { return x; }); }

namespace Str {
template <class Seq> String Join(const String& sep, const Seq& s) { String r; bool first = true; for (const auto& x : s) { if (!first) r += sep; r += ToStr(x); first = false; } return r; }
inline List<String> Split(const String& s, char c) { List<String> r = List<String>::New(); size_t a = 0; while (true) { size_t p = s.find(c, a); if (p == String::npos) { r.Add(s.substr(a)); break; } r.Add(s.substr(a, p - a)); a = p + 1; } return r; }
inline List<String> Split(const String& s, const String& sep) { List<String> r = List<String>::New(); size_t a = 0; while (true) { size_t p = s.find(sep, a); if (p == String::npos) { r.Add(s.substr(a)); break; } r.Add(s.substr(a, p - a)); a = p + sep.size(); } return r; }
inline List<String> SplitAny(const String& s, const String& chars, bool removeEmpty = false, bool trim = false) {
    List<String> r = List<String>::New(); String cur; auto push = [&]() { String t = trim ? Trim(cur) : cur; if (!(removeEmpty && t.empty())) r.Add(t); cur.clear(); };
    for (char c : s) { if (chars.find(c) != String::npos) push(); else cur += c; } push(); return r; }
inline List<String> SplitLines(const String& s) { String t = Replace(Replace(s, "\r\n", "\n"), "\r", "\n"); return Split(t, '\n'); }
inline List<char> ToCharArray(const String& s) { List<char> r = List<char>::New(); for (char c : s) r.Add(c); return r; }
inline String FromChars(const List<char>& cs) { String r; for (char c : cs) r += c; return r; }
}  // namespace Str

// ------------------------------------------------------------------ parsing
namespace Parse {
inline bool TryInt(const String& t, long long& out, long long lo = LLONG_MIN, long long hi = LLONG_MAX) {
    String s = Str::Trim(t); if (s.empty()) return false; size_t i = 0; bool neg = false; if (s[0] == '-' || s[0] == '+') { neg = s[0] == '-'; i = 1; }
    if (i >= s.size()) return false; BigInteger v(0); for (; i < s.size(); i++) { if (s[i] < '0' || s[i] > '9') return false; v = v * BigInteger(10) + BigInteger(s[i] - '0'); if (v.GetBitLength() > 64) return false; }
    if (neg) v = -v; if (v < BigInteger(lo) || v > BigInteger(hi)) return false; out = (long long)v; return true;
}
template <class I> bool TryParse(const String& t, I& out) { long long v; if (!TryInt(t, v, (long long)std::numeric_limits<I>::min(), (long long)std::numeric_limits<I>::max())) { out = 0; return false; } out = (I)v; return true; }
// System.Globalization: NumberStyles and the culture symbols number parsing reads
enum NumberStyles { None = 0, AllowLeadingWhite = 1, AllowTrailingWhite = 2, AllowLeadingSign = 4, AllowTrailingSign = 8, AllowParentheses = 16,
    AllowDecimalPoint = 32, AllowThousands = 64, AllowExponent = 128, AllowCurrencySymbol = 256, AllowHexSpecifier = 512,
    Integer = 7, Number = 111, Float = 167, Currency = 383, Any = 511 };
struct NumberFormat { String PositiveSign = "+", NegativeSign = "-", DecimalSeparator = ".", GroupSeparator = ",", CurrencySymbol = "\xa4",
    CurrencyDecimalSeparator = ".", CurrencyGroupSeparator = ","; };
inline const NumberFormat& InvariantCulture() { static NumberFormat f; return f; }
// CultureInfo.CurrentCulture: the invariant culture's symbols unless the host sets others (e.g. en-US: CurrencySymbol "$")
inline NumberFormat& CurrentCulture() { static NumberFormat f; return f; }
// Number.TryParseNumber (.NET): sign, digits, group and decimal separators, exponent, currency symbol, by the styles given.
// On success: neg, the digits (integer part then fraction) and the decimal exponent applied to them.
inline bool TryParseNumber(const String& s, int st, const NumberFormat& nf, bool& neg, String& digits, long long& exp10) {
    auto white = [](char c) { return c == 0x20 || (c >= 0x09 && c <= 0x0D); };
    auto match = [&](size_t p, const String& m) { return !m.empty() && s.compare(p, m.size(), m) == 0; };
    bool cur = (st & AllowCurrencySymbol) != 0;
    String currSym = cur ? nf.CurrencySymbol : String();
    const String& dec = cur ? nf.CurrencyDecimalSeparator : nf.DecimalSeparator;
    const String& grp = cur ? nf.CurrencyGroupSeparator : nf.GroupSeparator;
    size_t p = 0, n = s.size(); bool sign = false, currency = false; neg = false; digits.clear(); exp10 = 0;
    while (p < n) {   // leading part
        char ch = s[p];
        if (white(ch) && (st & AllowLeadingWhite) && !(sign && !currency)) { p++; continue; }
        if ((st & AllowLeadingSign) && !sign && match(p, nf.PositiveSign)) { sign = true; p += nf.PositiveSign.size(); continue; }
        if ((st & AllowLeadingSign) && !sign && match(p, nf.NegativeSign)) { sign = true; neg = true; p += nf.NegativeSign.size(); continue; }
        if (!currSym.empty() && match(p, currSym)) { currency = true; p += currSym.size(); currSym.clear(); continue; }
        break;
    }
    bool anyDigit = false, inFrac = false; long long fracCount = 0;
    while (p < n) {   // digits
        char ch = s[p];
        if (ch >= '0' && ch <= '9') { anyDigit = true; digits += ch; if (inFrac) fracCount++; p++; continue; }
        if ((st & AllowDecimalPoint) && !inFrac && match(p, dec)) { inFrac = true; p += dec.size(); continue; }
        if ((st & AllowThousands) && anyDigit && !inFrac && match(p, grp)) { p += grp.size(); continue; }
        break;
    }
    if (!anyDigit) return false;
    exp10 = -fracCount;
    if ((st & AllowExponent) && p < n && (s[p] == 'e' || s[p] == 'E')) {
        size_t q = p + 1; bool eneg = false;
        if (match(q, nf.PositiveSign)) q += nf.PositiveSign.size(); else if (match(q, nf.NegativeSign)) { eneg = true; q += nf.NegativeSign.size(); }
        if (q < n && s[q] >= '0' && s[q] <= '9') {
            long long e = 0; while (q < n && s[q] >= '0' && s[q] <= '9') { if (e < 100000000) e = e * 10 + (s[q] - '0'); q++; }
            exp10 += eneg ? -e : e; p = q;
        }   // an E with no digits is left unread (and fails below)
    }
    while (p < n) {   // trailing part
        char ch = s[p];
        if (white(ch) && (st & AllowTrailingWhite)) { p++; continue; }
        if ((st & AllowTrailingSign) && !sign && match(p, nf.PositiveSign)) { sign = true; p += nf.PositiveSign.size(); continue; }
        if ((st & AllowTrailingSign) && !sign && match(p, nf.NegativeSign)) { sign = true; neg = true; p += nf.NegativeSign.size(); continue; }
        if (!currSym.empty() && match(p, currSym)) { p += currSym.size(); currSym.clear(); continue; }
        break;
    }
    while (p < n && s[p] == 0) p++;   // trailing NULs are allowed
    return p == n;
}
// double.TryParse(s, styles, culture): IEEE-correct rounding; out of range gives +-Infinity (.NET Core 3.0+);
// the culture's "Infinity" / "-Infinity" / "NaN" symbols are read too
inline bool TryParse(const String& t, int st, const NumberFormat& nf, double& out) {
    bool neg; String dg; long long e;
    if (!TryParseNumber(t, st, nf, neg, dg, e)) {
        String u = Str::Trim(t); for (auto& c : u) c = (char)std::tolower((unsigned char)c);
        if (u == "infinity" || u == "+infinity" || u == "\xe2\x88\x9e") { out = INFINITY; return true; }
        if (u == "-infinity" || u == "-\xe2\x88\x9e") { out = -INFINITY; return true; }
        if (u == "nan" || u == "-nan" || u == "+nan") { uint64_t bits = 0xFFF8000000000000ULL; std::memcpy(&out, &bits, 8); return true; }   // double.NaN
        out = 0; return false;
    }
    if (e > 400000) e = 400000; if (e < -400000) e = -400000;
    String lit = (neg ? "-" : "") + dg + "e" + std::to_string(e);
    out = std::strtod(lit.c_str(), nullptr); return true;
}
// decimal.TryParse(s, styles, culture): false past decimal's range; more than 28 places round half to even
inline bool TryParse(const String& t, int st, const NumberFormat& nf, Decimal& out) {
    bool neg; String dg; long long e;
    if (!TryParseNumber(t, st, nf, neg, dg, e)) { out = Decimal(); return false; }
    size_t nz = dg.find_first_not_of('0'); dg = nz == String::npos ? "0" : dg.substr(nz);
    if (dg != "0" && (long long)dg.size() + e > 29) { out = Decimal(); return false; }
    Decimal d; if (e >= 0) { if (!Decimal::TryParse(dg + String((size_t)e, '0'), d)) return false; }
    else { long long sc = -e; if (sc > 400) { dg = "0"; sc = 0; } d = Decimal::Raw(BigInteger(0), 0); Decimal::TryParse(dg, d); d = Decimal::Raw(d.m, (int)sc); while (d.scale > 28) d = Decimal::Round(d, d.scale - 1, MidpointRounding::ToEven); }
    if (neg) d = -d;
    static const BigInteger maxM = [] { Decimal x; Decimal::TryParse("79228162514264337593543950335", x); return x.m; }();
    if (BigInteger::Abs(d.m) > maxM) { out = Decimal(); return false; }
    out = d; return true;
}
// double.TryParse(s) / decimal.TryParse(s): NumberStyles.Float | AllowThousands (decimal: Number), current culture
inline bool TryParse(const String& t, double& out) { return TryParse(t, Float | AllowThousands, CurrentCulture(), out); }
inline bool TryParse(const String& t, Decimal& out) { return TryParse(t, Number, CurrentCulture(), out); }
template <class I> I Parse(const String& t) { I v; if (!TryParse(t, v)) throw FormatException("Input string was not in a correct format."); return v; }
}  // namespace Parse

// ------------------------------------------------------------------ System.Random
class Random {
    static const int MBIG = INT_MAX; static const int MSEED = 161803398;
    int seedArray[56] = {0}; int inext = 0, inextp = 21;
    int internalSample() {
        int a = inext + 1; if (a >= 56) a = 1; int b = inextp + 1; if (b >= 56) b = 1;
        int ret = seedArray[a] - seedArray[b]; if (ret == MBIG) ret--; if (ret < 0) ret += MBIG;
        seedArray[a] = ret; inext = a; inextp = b; return ret;
    }
    void init(int seed) {
        int sub = seed == INT_MIN ? INT_MAX : std::abs(seed); int mj = MSEED - sub; seedArray[55] = mj; int mk = 1; int ii = 0;
        for (int i = 1; i < 55; i++) { if ((ii += 21) >= 55) ii -= 55; seedArray[ii] = mk; mk = mj - mk; if (mk < 0) mk += MBIG; mj = seedArray[ii]; }
        for (int k = 1; k < 5; k++) for (int i = 1; i < 56; i++) { int n = i + 30; if (n >= 55) n -= 55; seedArray[i] -= seedArray[1 + n]; if (seedArray[i] < 0) seedArray[i] += MBIG; }
        inext = 0; inextp = 21;
    }
public:
    Random();  // unseeded: seeded from the clock (cs_runtime.cpp)
    explicit Random(int seed) { init(seed); }
    virtual ~Random() = default;
    // .NET routes every public call through Sample(); a subclass may override it (as test harnesses do)
    virtual double Sample() { return internalSample() * (1.0 / MBIG); }
    virtual double NextDouble() { return Sample(); }
    virtual int Next() { return internalSample(); }
    virtual int Next(int maxValue) { if (maxValue < 0) throw ArgumentOutOfRangeException("maxValue"); return (int)(Sample() * maxValue); }
    virtual int Next(int minValue, int maxValue) {
        if (minValue > maxValue) throw ArgumentOutOfRangeException("minValue");
        long long range = (long long)maxValue - minValue;
        if (range <= INT_MAX) return (int)(Sample() * (double)range) + minValue;
        // large range: .NET's GetSampleForLargeRange
        int r = internalSample(); bool neg = internalSample() % 2 == 0; if (neg) r = -r;
        double d = (r + 2147483646.0) / (2.0 * 2147483647.0 - 1); return (int)((long long)(d * (double)range) + minValue);
    }
};

// ------------------------------------------------------------------ bytes, files, encodings
using Byte = uint8_t;
using Bytes = List<uint8_t>;
namespace BinaryPrimitives {
inline int16_t ReadInt16LittleEndian(const uint8_t* p) { return (int16_t)(p[0] | (p[1] << 8)); }
inline uint16_t ReadUInt16LittleEndian(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
inline int32_t ReadInt32LittleEndian(const uint8_t* p) { return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24)); }
inline uint32_t ReadUInt32LittleEndian(const uint8_t* p) { return (uint32_t)ReadInt32LittleEndian(p); }
inline int64_t ReadInt64LittleEndian(const uint8_t* p) { uint64_t v = 0; for (int i = 7; i >= 0; i--) v = (v << 8) | p[i]; return (int64_t)v; }
inline float ReadSingleLittleEndian(const uint8_t* p) { uint32_t u = ReadUInt32LittleEndian(p); float f; std::memcpy(&f, &u, 4); return f; }
inline double ReadDoubleLittleEndian(const uint8_t* p) { uint64_t u = (uint64_t)ReadInt64LittleEndian(p); double d; std::memcpy(&d, &u, 8); return d; }
inline void WriteInt16LittleEndian(uint8_t* p, int16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)((uint16_t)v >> 8); }
inline void WriteInt32LittleEndian(uint8_t* p, int32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)((uint32_t)v >> (8 * i)); }
inline void WriteInt64LittleEndian(uint8_t* p, int64_t v) { for (int i = 0; i < 8; i++) p[i] = (uint8_t)((uint64_t)v >> (8 * i)); }
inline void WriteSingleLittleEndian(uint8_t* p, float f) { uint32_t u; std::memcpy(&u, &f, 4); WriteInt32LittleEndian(p, (int32_t)u); }
inline void WriteDoubleLittleEndian(uint8_t* p, double d) { uint64_t u; std::memcpy(&u, &d, 8); WriteInt64LittleEndian(p, (int64_t)u); }
}  // namespace BinaryPrimitives
namespace BitConverter {
inline long long DoubleToInt64Bits(double d) { long long b; std::memcpy(&b, &d, 8); return b; }
inline double Int64BitsToDouble(long long b) { double d; std::memcpy(&d, &b, 8); return d; }
inline int SingleToInt32Bits(float f) { int b; std::memcpy(&b, &f, 4); return b; }
inline float Int32BitsToSingle(int b) { float f; std::memcpy(&f, &b, 4); return f; }
}  // namespace BitConverter
namespace Encoding {
inline String Latin1GetString(const uint8_t* p, int n) { return String((const char*)p, (size_t)n); }
inline String Latin1GetString(const Bytes& b) { return String(b.Raw().begin(), b.Raw().end()); }
inline Bytes Latin1GetBytes(const String& s) { return Bytes(std::vector<uint8_t>(s.begin(), s.end())); }
inline String ASCIIGetString(const Bytes& b) { String s = Latin1GetString(b); for (auto& c : s) if ((unsigned char)c >= 0x80) c = '?'; return s; }   // non-ASCII bytes decode as '?'
inline Bytes ASCIIGetBytes(const String& s) { return Latin1GetBytes(s); }
// UTF-8 <-> String (Latin-1): characters past U+00FF, and malformed bytes (U+FFFD in .NET), become '?'
inline String Utf8Decode(const uint8_t* p, size_t n) {
    String o; size_t i = 0;
    while (i < n) {
        uint8_t c = p[i];
        if (c < 0x80) { o += (char)c; i++; continue; }
        int k = c >= 0xF0 && c < 0xF8 ? 3 : c >= 0xE0 ? 2 : c >= 0xC2 && c < 0xE0 ? 1 : 0;
        if (k == 0 || (c >= 0xF8)) { o += '?'; i++; continue; }
        unsigned cp = c & (0x3F >> k); size_t j = 1;
        for (; j <= (size_t)k; j++) { if (i + j >= n || (p[i + j] & 0xC0) != 0x80) break; cp = (cp << 6) | (p[i + j] & 0x3F); }
        if (j <= (size_t)k) { o += '?'; i += j; continue; }
        o += cp <= 0xFF ? (char)cp : '?'; i += (size_t)k + 1;
    }
    return o;
}
inline String Utf8Encode(const String& s) { String o; for (unsigned char c : s) { if (c < 0x80) o += (char)c; else { o += (char)(0xC0 | (c >> 6)); o += (char)(0x80 | (c & 0x3F)); } } return o; }
inline String UTF8GetString(const Bytes& b) { return Utf8Decode(b.Raw().data(), (size_t)b.Count()); }
inline Bytes UTF8GetBytes(const String& s) { return Latin1GetBytes(Utf8Encode(s)); }
}  // namespace Encoding
namespace File {
inline bool Exists(const String& p) { std::error_code e; return std::filesystem::is_regular_file(p, e); }
inline Bytes ReadAllBytes(const String& p) { std::ifstream f(p, std::ios::binary); if (!f) throw FileNotFoundException("Could not find file '" + p + "'."); return Bytes(std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>())); }
// File.ReadAllText / WriteAllText: UTF-8 (a UTF-8 BOM is skipped on reading; none is written)
inline String ReadAllText(const String& p) { std::ifstream f(p, std::ios::binary); if (!f) throw FileNotFoundException("Could not find file '" + p + "'."); std::stringstream ss; ss << f.rdbuf(); String raw = ss.str();
    size_t b = raw.size() >= 3 && (uint8_t)raw[0] == 0xEF && (uint8_t)raw[1] == 0xBB && (uint8_t)raw[2] == 0xBF ? 3 : 0;
    return Encoding::Utf8Decode((const uint8_t*)raw.data() + b, raw.size() - b); }
inline List<String> ReadAllLines(const String& p) { List<String> r = Str::SplitLines(ReadAllText(p)); if (r.Count() && r[r.Count() - 1].empty()) r.RemoveAt(r.Count() - 1); return r; }
inline void WriteAllBytes(const String& p, const Bytes& b) { std::ofstream f(p, std::ios::binary); if (!f) throw IOException("cannot write " + p); f.write((const char*)b.Raw().data(), (std::streamsize)b.Count()); }
inline void WriteAllText(const String& p, const String& s) { std::ofstream f(p, std::ios::binary); if (!f) throw IOException("cannot write " + p); f << Encoding::Utf8Encode(s); }
inline void Delete(const String& p) { std::error_code e; std::filesystem::remove(p, e); }
inline void Copy(const String& a, const String& b, bool over = false) { std::filesystem::copy_file(a, b, over ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none); }
inline void Move(const String& a, const String& b, bool overwrite = false) {   // File.Move(a, b, overwrite)
    if (!std::filesystem::exists(a)) throw FileNotFoundException("Could not find file '" + a + "'.");
    if (!overwrite && std::filesystem::exists(b)) throw IOException("The file '" + b + "' already exists.");
    std::filesystem::rename(a, b); }
}  // namespace File
namespace Directory {
inline bool Exists(const String& p) { std::error_code e; return std::filesystem::is_directory(p, e); }
inline void CreateDirectory(const String& p) { std::error_code e; std::filesystem::create_directories(p, e); }
inline void Delete(const String& p, bool rec = false) { std::error_code e; if (rec) std::filesystem::remove_all(p, e); else std::filesystem::remove(p, e); }
inline List<String> GetFiles(const String& p, const String& pattern = "*", bool recursive = false);
inline List<String> GetDirectories(const String& p) { List<String> r = List<String>::New(); std::error_code e; for (auto& x : std::filesystem::directory_iterator(p, e)) if (x.is_directory()) r.Add(x.path().string()); return r; }
inline List<String> EnumerateFileSystemEntries(const String& p) { List<String> r = List<String>::New(); std::error_code e; for (auto& x : std::filesystem::directory_iterator(p, e)) r.Add(x.path().string()); return r; }
}  // namespace Directory
namespace Path {
inline String Combine(const String& a, const String& b) { if (a.empty()) return b; return (std::filesystem::path(a) / b).string(); }
inline String Combine(const String& a, const String& b, const String& c) { return Combine(Combine(a, b), c); }
inline String GetFileName(const String& p) { return std::filesystem::path(p).filename().string(); }
inline String GetFileNameWithoutExtension(const String& p) { return std::filesystem::path(p).stem().string(); }
inline String GetExtension(const String& p) { return std::filesystem::path(p).extension().string(); }
inline String GetDirectoryName(const String& p) { return std::filesystem::path(p).parent_path().string(); }
inline String GetFullPath(const String& p) { std::error_code e; return std::filesystem::absolute(p, e).string(); }
inline String GetRelativePath(const String& a, const String& b) { return std::filesystem::relative(b, a).string(); }
inline String ChangeExtension(const String& p, const String& ext) { return std::filesystem::path(p).replace_extension(ext).string(); }
inline String GetTempPath() { return std::filesystem::temp_directory_path().string(); }
inline char DirectorySeparatorChar() { return (char)std::filesystem::path::preferred_separator; }
}  // namespace Path
inline bool WildcardMatch(const String& name, const String& pat) {
    String rx; for (char c : pat) { if (c == '*') rx += ".*"; else if (c == '?') rx += "."; else if (std::strchr(".^$+()[]{}|\\", c)) { rx += '\\'; rx += c; } else rx += c; }
    return std::regex_match(Str::ToLower(name), std::regex(Str::ToLower(rx)));
}
inline List<String> Directory::GetFiles(const String& p, const String& pattern, bool recursive) {
    List<String> r = List<String>::New(); std::error_code e;
    if (recursive) { for (auto& x : std::filesystem::recursive_directory_iterator(p, e)) if (x.is_regular_file() && WildcardMatch(x.path().filename().string(), pattern)) r.Add(x.path().string()); }
    else for (auto& x : std::filesystem::directory_iterator(p, e)) if (x.is_regular_file() && WildcardMatch(x.path().filename().string(), pattern)) r.Add(x.path().string());
    std::sort(r.begin(), r.end()); return r;
}

// ------------------------------------------------------------------ regex
struct Regex {
    std::regex rx;
    explicit Regex(const String& p, bool ignoreCase = false) : rx(p, ignoreCase ? std::regex::ECMAScript | std::regex::icase : std::regex::ECMAScript) {}
    static bool IsMatch(const String& s, const String& p, bool ignoreCase = false) { return std::regex_search(s, Regex(p, ignoreCase).rx); }
    static String Replace(const String& s, const String& p, const String& rep) { return std::regex_replace(s, std::regex(p), rep); }
    bool IsMatch(const String& s) const { return std::regex_search(s, rx); }
};




// decimal literal: 2.5m -> cs::Dec("2.5")
inline Decimal Dec(const String& s) { Decimal d; if (!Decimal::TryParse(s, d)) throw FormatException("decimal literal"); return d; }

// ------------------------------------------------------------------ not translated (stubs that keep signatures compiling)
// System.Text.Json readers/writers/options, streams, zip archives and reflection have no mechanical C++ equivalent.
struct Type { String Name; };
struct Stream { Bytes Data; int Position = 0; };
using MemoryStream = Stream;
using FileStream = Stream;
struct DirectoryInfo { String FullName; };
using Guid = String;

// ------------------------------------------------------------------ misc
struct DateTime {
    long long ticks = 0;  // 100 ns since 0001-01-01
    static DateTime Now();
    static DateTime UtcNow() { return Now(); }
    String ToString(const String& fmt = "") const;
};
struct TimeSpan { long long ticks = 0; double TotalSeconds() const { return ticks / 1e7; } double TotalMilliseconds() const { return ticks / 1e4; } };
namespace Environment { inline int TickCount() { return (int)(std::chrono::steady_clock::now().time_since_epoch() / std::chrono::milliseconds(1)); } }

}  // namespace cs
#include "cs_json.h"
#include "cs_io.h"
