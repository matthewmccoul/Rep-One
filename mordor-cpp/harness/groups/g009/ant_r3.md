I can't fix this yet, so I haven't written the three files. Retrying with a guess would likely fail the same way.

The harness found no `int` field whose name contains `TopGuildLevel`. An ordinary auto-property would have a compiler-generated backing field with that name, so `TopGuildLevel` is probably a computed getter, such as a value derived from guild memberships or other state. I can't set a computed value directly, and the Character outline you gave me is truncated and doesn't show how it's derived. Writing a reflection setter for some other member would break the "do not invent fields" rule.

To proceed, I need one of these:

1. **The definition of `TopGuildLevel` in `Character.cs`.** Run `grep -n "TopGuildLevel" <Mordor.Core>/Character.cs` and paste the matching lines plus the members they reference. With that I can build the inputs from the real state that drives it.
2. **Permission to search the repo myself.** I can grep for `TopGuildLevel` under the Mordor.Core source and read the relevant members.

Two other notes for the rewrite:

- **Scope of the cases.** The C# `Drift` has a separate `BindingLevel < 0` branch with a 0.5 floor, and the C++ copies implement only the `Binding > 0` rule. All cases should use `binding_level >= 1` so the two sides test the same rule.
- **Floor semantics.** The C# uses `Math.Max(1, drift)`, which matches the C++ explicit comparison against 1.0. Fractional results above the floor should match exactly, so the cases don't need special handling for them.