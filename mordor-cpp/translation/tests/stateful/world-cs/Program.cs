// C# side of the second differential: level-ups, the store, the confinement, dungeon walks with encounters.
using System.Globalization; using Mordor.Core;
CultureInfo.CurrentCulture = CultureInfo.InvariantCulture;
string dir = args[0]; int from = int.Parse(args[1]), to = int.Parse(args[2]); var o = new StreamWriter(args[3]) { NewLine = "\n" };
Mdata1.LoadFrom(dir); RaceData.LoadFrom(dir); GuildData.LoadFrom(dir);
var items = MdrLoader.LoadItems(Path.Combine(dir, "MDATA3.MDR"));
var monsters = MdrLoader.LoadMonsters(Path.Combine(dir, "MDATA5.MDR"));
var floors = MdrLoader.LoadDungeon(Path.Combine(dir, "MDATA11.MDR"));
string T(Func<object?> f) { try { return Convert.ToString(f(), CultureInfo.InvariantCulture) ?? "null"; } catch (Exception e) { return "THREW " + e.GetType().Name; } }
for (int seed = from; seed < to; seed++)
{
    var rng = new Random(seed);
    o.WriteLine($"SEED {seed}");
    // ---- a character's levels
    var race = RaceData.All[rng.Next(RaceData.All.Count)];
    var stats = Enumerable.Range(0, 6).Select(_ => rng.Next(3, 26)).ToArray();
    var c = Character.Create("w" + seed, race, (Alignment)rng.Next(3), (Sex)rng.Next(2), stats);
    c.Guilds.Clear();
    int ng = rng.Next(1, 4);
    for (int i = 0; i < ng; i++) c.Guilds.Add(new GuildMembership { Guild = GuildData.All[rng.Next(GuildData.All.Count)].Name, Level = rng.Next(1, 5) });
    for (int step = 0; step < 25; step++)
    {
        if (rng.Next(4) == 0 && c.Guilds.Count > 1) { var g0 = c.Guilds[rng.Next(c.Guilds.Count)]; c.Guilds.Remove(g0); c.Guilds.Insert(0, g0); }
        c.Guild.Experience += rng.Next(0, 3000000);
        int made = 0;
        while (made < 60) { bool can; try { can = c.CanLevelUp(); } catch (Exception e) { o.WriteLine("  can THREW " + e.GetType().Name); break; } if (!can) break; o.WriteLine($"  L {c.Guild.Guild} -> {T(() => c.LevelUp(rng))} hp {c.MaxHits}"); made++; }
        o.WriteLine($"  C {string.Join(",", c.Guilds.Select(g => g.Guild + ":" + g.Level + ":" + g.Experience))} next {T(() => c.ExpForNextLevel())} total {T(() => c.TotalLevel)} title {T(() => c.Title)} gt {T(() => c.GuildTitle)} swings {T(() => { c.RecalcSwings(); return c.Swings; })}");
    }
    // ---- the store
    var s = new StoreStock();
    for (int k = 0; k < 40; k++)
    {
        var it = items[rng.Next(1, items.Count)]; var al = (ItemAlignment)rng.Next(3); int op = rng.Next(6);
        var slot = new InventorySlot { ItemId = it.Id, Charges = (short)rng.Next(0, Math.Max(1, (int)it.Charges + 1)), Known = (IdLevel)rng.Next(4), Alignment = al };
        string r = op switch { 0 => T(() => { s.Add(it.Id, al); return s.Count(it.Id, al); }), 1 => T(() => s.Take(it.Id, al)), 2 => T(() => s.PriceOf(it, al)),
            3 => T(() => s.SellPrice(it, slot)), 4 => T(() => s.IdentifyFee(it, slot)), _ => T(() => StoreStock.RealignCost(it, al, (ItemAlignment)rng.Next(3))) };
        o.WriteLine($"  B {it.Id} {al} {op} {r}");
    }
    // ---- the confinement
    var conf = new Confinement();
    conf.Replenish(monsters, rng.Next(0, 30), rng);
    o.WriteLine("  P " + string.Join(",", conf.Pens.Select(kv => kv.Key + "=" + kv.Value)));
    // ---- a walk
    int fi = rng.Next(floors.Count); var floor = floors[fi];
    var nav = new DungeonNav(floor, rng.Next(floor.Width), rng.Next(floor.Height), (Facing)rng.Next(4));
    for (int k = 0; k < 150; k++)
    {
        int a = rng.Next(6);
        string r = a switch { 0 => T(() => nav.MoveForward()), 1 => T(() => nav.MoveBackward()), 2 => T(() => { nav.TurnLeft(); return 1; }), 3 => T(() => { nav.TurnRight(); return 1; }), 4 => T(() => { nav.TurnAround(); return 1; }), _ => T(() => nav.StepThrough()) };
        o.WriteLine($"  N {a} {r} {nav.X},{nav.Y} {nav.Facing}");
        if (k % 25 == 0)
        {
            o.WriteLine("  D " + T(() => nav.Describe()).Replace("\n", "|"));
            try { var e = EncounterGenerator.Generate(monsters, fi + 1, rng, floor, nav.X, nav.Y);
                  o.WriteLine($"  E {e.Gold} {e.TreasureClass} {e.Lead} " + string.Join(";", e.Select(m => m.Def?.Id + ":" + m.Hits + ":" + m.Group))); }
            catch (Exception e) { o.WriteLine("  E THREW " + e.GetType().Name); }
        }
    }
    o.WriteLine($"  END rng {rng.Next()} explored {nav.Explored.Count}");
}
o.Close();
