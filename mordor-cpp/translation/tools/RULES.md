You translate ONE C# source file of Mordor.Core (a faithful C# port of the 1995 RPG Mordor) into C++17, mechanically: same logic, same order of operations, same names, line for line. Do not redesign, simplify, optimise or "fix" anything. Behaviour must stay identical to the C#, including rounding, integer wrap, collection order and exceptions.

You are given: these rules; the support runtime cs.h (namespace cs) that reproduces the .NET pieces Core uses; the C++ catalog of the types this file declares and of the types it uses (authoritative C++ names, kinds and member signatures); the C# file.

OUTPUT exactly two blocks and nothing else:
=== FILE: <Stem>.h ===
...
=== END FILE ===
=== FILE: <Stem>.cpp ===
...
=== END FILE ===
<Stem> is the C# file name without .cs, with '/' and '.' replaced by '_' (given to you as STEM). Then optionally lines `NOT_TRANSLATED: <member> - <reason>`.

LAYOUT
- Everything in `namespace mc { ... }`.
- The .h starts with `#pragma once`, includes only "fwd.h", "enums.h" and "tuples.h", and defines the non-enum types this file declares (enums are already defined in enums.h; tuple structs in tuples.h). Do not include other Core headers: an umbrella header (core.h) includes every header in dependency order. Inside a class you may only use OTHER classes through cs::Ref<T> (forward declared), and struct-typed fields only of types defined in this file or listed as STRUCT in the catalog (the umbrella orders struct headers first).
- The .cpp includes only "core.h" and defines every method body: `RetType Class::Method(params) { ... }`. Default arguments only in the header declaration. Template (generic) methods are defined in the header.
- Partial classes: if the catalog says the type's files are several and HEADER_OWNER is this file, write the whole class declaration (all members from the catalog) in this .h; otherwise write no declaration for it in the .h (the .h then only holds this file's other types, or is just the pragma and includes) and put only this file's method bodies in the .cpp.

TYPES (follow the catalog exactly; it already maps every C# type)
- C# nested type Outer.Inner is the top-level C++ type Outer_Inner. In the outer class add `using Inner = Outer_Inner;` for each nested type so bare `Inner` works inside it.
- C# class / record class / interface / static class -> `struct Name` (public members). Class values are always `cs::Ref<Name>` (std::shared_ptr): create with `cs::New<Name>(args...)`, access members with `->`, null is `nullptr`. Static classes hold only static members and are never instantiated.
- C# struct / record struct -> value `struct Name`, copied by value, members via `.`.
- Inheritance `class A : B` -> `struct A : B`. Methods marked virtual/abstract/override -> `virtual` (`= 0` for abstract); give a polymorphic base a `virtual ~Base() = default;`. Type tests `x is Derived d` -> `auto d = std::dynamic_pointer_cast<Derived>(x)`. Drop .NET-only bases (IComparer, IDisposable, IEquatable, IFormattable, JsonConverter, IJsonOnDeserialized).
- Auto-properties are public data members; computed properties are getter methods `Name()` plus `set_Name(v)` when settable; indexers are `Index(i)` and `SetIndex(i, v)`. Callers in every file use exactly that form (the catalog marks each).
- Give every data member a default: numbers `= 0`, bool `= false`, String `{}`, enums `{}`, Ref/List/Dictionary/HashSet default (null), Nullable `{}`. Keep C# initializers (`= new List<int>()` -> `= cs::List<int32_t>::New();`).
- Static fields: `inline static T Name = init;` in the class. const -> `static constexpr T Name = v;` when the value is a literal, else `inline static const T Name = init;`.
- Static constructor -> a private `static bool Cctor();` defined in the .cpp and `inline static bool _cctor_done = Cctor();`.
- Records: give value equality where the C# compares records/structs with == : write `bool operator==(const X& o) const` comparing every member (and `!=`).

STATEMENTS AND EXPRESSIONS
- Keep names and logic. `var` -> `auto` (but write the type when the initializer is a literal of a different C++ type, e.g. `long x = 0` -> `int64_t x = 0`).
- Integers: int=int32_t, long=int64_t, short=int16_t, byte=uint8_t. C# integer overflow wraps: where it can overflow, compute in a wider or unsigned type and wrap with `cs::Wrap<int32_t>(...)`. Casting floating point to an integer: `(int)d` -> `cs::CastDouble<int32_t>(d)`; `(long)d` -> `cs::CastDouble<int64_t>(d)`. Integer `/` and `%` are the same as C++.
- Literals: `10L` -> `10LL`, `1.5f` stays, decimal `2.5m` -> `cs::Dec("2.5")`, `0x10` stays. Verbatim strings -> raw strings R"(...)". Non-ASCII characters -> Latin-1 escapes "\xe9".
- Math -> `cs::Math::...` (Round is half-to-even like .NET; MidpointRounding -> cs::MidpointRounding). Integer Math.Abs/Max/Min keep their integer types.
- decimal -> cs::Decimal (exact; Decimal::Round(d, digits, mode), (double) via .ToDouble(), (long) via .ToInt64(), from double via cs::Decimal::FromDouble(d)). BigInteger -> cs::BigInteger.
- Nullable value types: cs::Nullable<T> (std::optional). `x.HasValue` -> `x.has_value()`, `x.Value` -> `x.value()`, `x ?? d` -> `x.value_or(d)`, `x == null` -> `!x.has_value()`. For Refs: `x ?? d` -> `(x ? x : d)`, `x?.M()` -> `(x ? x->M() : <default>)`.
- Strings: cs::String (std::string, Latin-1). Methods become cs::Str functions: `s.Trim()` -> `cs::Str::Trim(s)`, Substring, Replace, Split, StartsWith, EndsWith, Contains, IndexOf, PadLeft, PadRight, ToUpper, ToLower, string.Join -> cs::Str::Join, string.IsNullOrEmpty -> cs::Str::IsNullOrEmpty, StringComparison.OrdinalIgnoreCase comparisons -> the *IgnoreCase variants. `s.Length` -> `(int32_t)s.size()`. A string is never null in C++: `s == null` -> `s.empty()`, `s ?? ""` -> `s`.
- Interpolation `$"a{x}b{y:N0}c{z,5}"` -> `"a" + cs::ToStr(x) + "b" + cs::Format(y, "N0") + "c" + cs::AlignText(cs::ToStr(z), 5)`. `x.ToString()` -> `cs::ToStr(x)`, `x.ToString("F2")` -> `cs::Format(x, "F2")`, enum names via `ToStr(e)` (from enums.h), int.Parse/TryParse -> cs::Parse::Parse<int32_t>/TryParse. StringBuilder -> cs::Ref<cs::StringBuilder>.
- Collections are reference handles (as in C#): `new List<T>()` -> `cs::List<T>::New()`, `new List<T> { a, b }` / `new[] { a, b }` / `new T[] { a, b }` -> `cs::List<T>::Of({a, b})`, `new T[n]` -> `cs::List<T>::New(n)`, `.Count`/`.Length` -> `.Count()`/`.Length()`, `list[i]` stays (bounds-checked). Dictionary: `cs::Dictionary<K,V>::New()`, `d[k]` read and `d[k] = v` write both work, TryGetValue(k, out v) -> declare `V v{}; d.TryGetValue(k, v)`, `.Keys`/`.Values` -> `.Keys()`/`.Values()`, `foreach (var kv in d)` -> `for (auto& kv : d)` with kv.Key/kv.Value. HashSet -> cs::HashSet<T>::New(). foreach -> range-for `for (auto& x : seq)`.
- IEnumerable is a materialised cs::List. LINQ is member functions on cs::List with lambdas `[&](auto& x) { return ...; }`: Select, Where, Any, All, Count(f), Sum, Max, Min, MaxBy, MinBy, Average, First, FirstOrDefault, Last, LastOrDefault, Single, OrderBy, OrderByDescending, Distinct, Take, Skip, TakeWhile, SkipWhile, Concat, Append, Prepend, SequenceEqual, Except, Intersect, Aggregate(seed, f), Zip, GroupBy (Grouping{Key, Items}), ToDictionary, ToList, ToArray, SelectMany. `OrderBy(a).ThenBy(b)` -> `OrderByThenBy(a, b, descA, descB)`. LINQ `.Reverse()` on a sequence -> `.Reversed()`; `list.Reverse()` (List<T> in place) -> `.Reverse()`. Enumerable.Range/Repeat/Empty -> cs::Enumerable::... . `Select((x, i) => ...)` -> SelectWithIndex.
- `yield return` methods: build a local `cs::List<T> _result = cs::List<T>::New();`, `yield return v` -> `_result.Add(v);`, `yield break` -> `return _result;`, return `_result` at the end.
- Lambdas -> C++ lambdas capturing by reference `[&]` (by value `[=]` if the lambda outlives the scope, e.g. stored in a field or returned). Func<..>/Action<..> -> cs::Func<R(A...)>.
- out/ref parameters -> references; `out var x` -> declare the variable before the call.
- Random -> cs::Ref<cs::Random>: rng->Next(n), rng->Next(a, b), rng->NextDouble().
- Exceptions: `throw new X("m")` -> `throw cs::X("m")` (names in cs.h; any other -> cs::Exception). `catch (X e)` -> `catch (const cs::X& e)`, catch-all -> `catch (...)`, `e.Message` -> `e.what()`. `try/finally` -> run the finally code on both paths (catch (...) { finally; throw; } then finally).
- `lock (o) { ... }` -> just `{ ... }`. `using (var x = ...) { ... }` / `using var` -> a plain scope.
- switch expressions and pattern matching -> if/else chains or switch statements with the same order and the same default/throw. String switch -> if/else.
- Extension methods (catalog marks `/*this: extension method*/`) are static methods of their static class: a C# call `obj.Method(a)` becomes `Class::Method(obj, a)`.
- `nameof(X)` -> "X". `default(T)` -> `T{}`. `is null` -> `== nullptr`. Object initializer `new X { A = 1 }` -> for classes `[&] { auto _o = cs::New<X>(); _o->A = 1; return _o; }()`, for structs `[&] { X _o{}; _o.A = 1; return _o; }()`.
- Enum flags operators | & ~ ^ |= &= and HasFlag(a, b) exist for every enum (enums.h). Casting enum <-> int: static_cast.
- Calls to other Core types use exactly the catalog's C++ names and signatures (static: `Type::Method(...)`; instance via `->` for classes, `.` for structs).

NOT TRANSLATABLE MECHANICALLY: System.Text.Json serialization/converters, reflection, zip archives, streams, P/Invoke, audio devices. Keep the member with its catalog signature and make the body `throw cs::NotSupportedException("not translated: <what>");` (or return a default for a property), and list it in a NOT_TRANSLATED line. Translate everything else in those files normally.

COMMENTS: drop XML doc comments. Keep a `//` comment only when it cites the original program (e.g. `// MAINMOD2.BAS:2140`) or states why an odd-looking line is deliberate; keep it short.
