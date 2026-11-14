// Reference results from C# Mordor.Core for the light test. Inputs are written with the results so the
// C++ side recomputes from identical inputs (part 1), or loads the same game files itself (part 2).
using System.Globalization; using System.Reflection; using Mordor.Core;
var inv = CultureInfo.InvariantCulture; string dir = args[0]; var o = new StreamWriter(args[1]);
string D(double d) => BitConverter.DoubleToInt64Bits(d).ToString(inv); string F(float f) => BitConverter.SingleToInt32Bits(f).ToString(inv);
Mdata1.LoadFrom(dir); RaceData.LoadFrom(dir); GuildData.LoadFrom(dir);
var rnd = new Random(7);
// ---- part 1: pure rules
foreach (var g in GuildData.All) foreach (var r in RaceData.All) for (int lv = 0; lv <= 200; lv += 3) o.WriteLine($"P1A {lv} {F(g.ExpFactor)} {F(r.ExpFactor)} {Rules.Fn12A1(lv, g.ExpFactor, r.ExpFactor)}");
foreach (var gold in new[] { -50, 0, 1, 5, 10, 25, 99, 300 }) for (int lv = 0; lv <= 999; lv += 11) o.WriteLine($"P1B {lv} {gold} {Rules.Fn12B2(lv, gold)}");
var items = MdrLoader.LoadItems(Path.Combine(dir, "MDATA3.MDR"));
foreach (var it in items) for (int n = 0; n <= 12; n += 3) { var s = new StoreStock(); for (int i = 0; i < n; i++) s.Add(it.Id, ItemAlignment.Neutral);
    o.WriteLine($"P1P {it.Id} {it.Price} {it.Floor} {it.Charges} {n} {s.PriceOf(it, ItemAlignment.Neutral)}"); }
// ---- part 2: loaders (every field), join cost through the loaded guild table, VbCurrency arithmetic and formatting
string V(object v) => v switch { null => "null", string s => "\"" + s + "\"", float f => "f" + F(f), double d => "d" + D(d), bool b => b ? "1" : "0",
    System.Collections.IEnumerable e => "[" + string.Join(",", e.Cast<object>().Select(V)) + "]", Enum en => Convert.ToInt64(en).ToString(inv), _ => Convert.ToString(v, inv) };
void Dump(string kind, object x, string[] names) { var t = x.GetType(); o.WriteLine($"P2{kind} " + string.Join(" | ", names.Select(n => {
    var p = t.GetProperty(n); var f = t.GetField(n); return n + "=" + V(p != null ? p.GetValue(x) : f?.GetValue(x)); }))); }
string[] IN = args[2].Split(','), MN = args[3].Split(','), SN = args[4].Split(',');
foreach (var it in items) Dump("I", it, IN);
foreach (var m in MdrLoader.LoadMonsters(Path.Combine(dir, "MDATA5.MDR"))) Dump("M", m, MN);
foreach (var sp in MdrLoader.LoadSpells(Path.Combine(dir, "MDATA2.MDR"))) Dump("S", sp, SN);
foreach (var g in GuildData.All) o.WriteLine($"P2G {g.Name} {Rules.GuildJoinCost(g)}");
for (int i = 0; i < 3000; i++) {
    double a = (rnd.NextDouble() - 0.5) * Math.Pow(10, rnd.Next(-3, 13)); double b = (rnd.NextDouble() - 0.5) * Math.Pow(10, rnd.Next(-3, 13));
    if (i % 4 == 0) a = Math.Round(a, rnd.Next(0, 5));
    var ca = VbCurrency.FromDouble(a); var cb = VbCurrency.FromDouble(b);
    o.WriteLine($"P2V {D(a)} {D(b)} {ca.Raw} {cb.Raw} {VbCurrency.AddSaturating(ca, cb).Raw} {VbCurrency.SubtractSaturating(ca, cb).Raw} {ca.RoundToWhole()} {ca.Whole} {(ca.IsWhole ? 1 : 0)} [{ca.Str()}] [{ca}] [{ca.ToDecimal()}]");
}
o.Close();
