using System.Text.Json;
using Mordor.Core;
/// <summary>Scripted uniform draws in [0,1): each Sample/NextDouble/Next takes the next value; the last value repeats.</summary>
public sealed class ScriptRandom : Random
{
    readonly double[] v; int i;
    public ScriptRandom(double[] values) { v = values.Length > 0 ? values : new[] { 0.0 }; }
    protected override double Sample() { double d = v[Math.Min(i, v.Length - 1)]; if (i < v.Length) i++; return d; }
    public override double NextDouble() => Sample();
    public override int Next() => (int)(Sample() * int.MaxValue);
    public override int Next(int maxValue) => (int)(Sample() * maxValue);
    public override int Next(int minValue, int maxValue) => minValue + (int)(Sample() * ((long)maxValue - minValue));
}
/// <summary>Game tables loaded once from the shareware folder (MORDOR_GAMEDIR).</summary>
public static class H
{
    public static string Dir; public static bool ShownExc;
    public static List<Item> Items; public static Dictionary<int, Item> ItemsById; public static List<Monster> Monsters; public static List<Spell> Spells; public static List<Floor> Dungeon;
    public static string F(double d) { var s = d.ToString("0.######", System.Globalization.CultureInfo.InvariantCulture); return s == "-0" ? "0" : s; }
    public static void Load()
    {
        Dir = Environment.GetEnvironmentVariable("MORDOR_GAMEDIR");
        Mdata1.LoadFrom(Dir); RaceData.LoadFrom(Dir); GuildData.LoadFrom(Dir);
        Items = MdrLoader.LoadItems(Path.Combine(Dir, "MDATA3.MDR")); ItemsById = Items.GroupBy(x => (int)x.Id).ToDictionary(g => g.Key, g => g.First());
        Monsters = MdrLoader.LoadMonsters(Path.Combine(Dir, "MDATA5.MDR")); Spells = MdrLoader.LoadSpells(Path.Combine(Dir, "MDATA2.MDR"));
        Dungeon = MdrLoader.LoadDungeon(Path.Combine(Dir, "MDATA11.MDR"));
    }
}
public static class Program
{
    public static int Main(string[] args)
    {
        H.Load();
        var doc = JsonDocument.Parse(File.ReadAllText(args[0]));
        int k = 0;
        foreach (var c in doc.RootElement.EnumerateArray())
        {
            var rng = new ScriptRandom(c.TryGetProperty("rng", out var r) ? r.EnumerateArray().Select(x => x.GetDouble()).ToArray() : new double[0]);
            string res;
            try { res = Case.Run(c.GetProperty("args"), rng); } catch (Exception ex) { res = "EXC:" + ex.GetType().Name; if (!H.ShownExc) { H.ShownExc = true; Console.Error.WriteLine(ex.ToString()); } }
            Console.WriteLine($"{k}\t{res}"); k++;
        }
        return 0;
    }
}
