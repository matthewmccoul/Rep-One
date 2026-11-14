// C# side of the stateful differential: seeded fights from generated encounters, to the end.
using System.Globalization; using Mordor.Core;
CultureInfo.CurrentCulture = CultureInfo.InvariantCulture;
string dir = args[0]; int from = int.Parse(args[1]), to = int.Parse(args[2]); var o = new StreamWriter(args[3]) { NewLine = "\n" };
Mdata1.LoadFrom(dir); RaceData.LoadFrom(dir); GuildData.LoadFrom(dir);
var items = MdrLoader.LoadItems(Path.Combine(dir, "MDATA3.MDR"));
var monsters = MdrLoader.LoadMonsters(Path.Combine(dir, "MDATA5.MDR"));
var spells = MdrLoader.LoadSpells(Path.Combine(dir, "MDATA2.MDR"));
Item? Look(short id) => items.FirstOrDefault(i => i.Id == id);
string C(Character c) => $"{c.Name} hp {c.CurHits}/{c.MaxHits} sp {c.CurSpellPoints} dead {c.IsDead} gold {c.GoldOnHand.Raw} xp {string.Join(",", c.Guilds.Select(g => g.Guild + ":" + g.Level + ":" + g.Experience))} pois {c.Poisoned} para {c.ParalysisLevel}";
for (int seed = from; seed < to; seed++)
{
    var rng = new Random(seed);
    try
    {
        var party = new Party();
        int n = rng.Next(1, 5);
        for (int k = 0; k < n; k++)
        {
            var race = RaceData.All[rng.Next(RaceData.All.Count)];
            var stats = Enumerable.Range(0, 6).Select(_ => rng.Next(8, 19)).ToArray();
            var c = Character.Create("c" + seed + "_" + k, race, (Alignment)rng.Next(3), (Sex)rng.Next(2), stats);
            var g = GuildData.All[rng.Next(GuildData.All.Count)];
            bool strong = seed % 2 == 1;
            c.Guilds.Clear(); c.Guilds.Add(new GuildMembership { Guild = g.Name, Level = strong ? rng.Next(20, 200) : rng.Next(1, 60) });
            c.MaxHits = strong ? rng.Next(500, 5000) : rng.Next(5, 400); c.CurHits = c.MaxHits; c.MaxSpellPoints = rng.Next(0, 300); c.CurSpellPoints = c.MaxSpellPoints;
            c.RecalcSwings();
            int gear = rng.Next(0, 3);
            for (int j = 0; j < gear; j++) { var it = items[rng.Next(1, items.Count)]; c.PutInPack(new InventorySlot { ItemId = it.Id, Equipped = rng.Next(2) == 0, Charges = (short)it.Charges }); }
            party.Members.Add(c);
            var ord = party.OrderFor(c);
            int kind = rng.Next(4);
            if (kind == 1) { ord.Kind = OrderKind.Spell; ord.SpellId = spells[rng.Next(1, spells.Count)].Id; ord.Target = 0; }
            else if (kind == 2) ord.Kind = OrderKind.Defend;
        }
        int level = rng.Next(1, 16);
        var enc = EncounterGenerator.Generate(monsters, level, rng);
        o.WriteLine($"FIGHT {seed} level {level} gold {enc.Gold} class {enc.TreasureClass} trap {enc.ChestTrap} lock {enc.ChestLock} lead {enc.Lead}");
        foreach (var m in enc) o.WriteLine($"  M {m.Def?.Id} {m.Def?.Name} hits {m.Hits}/{m.MaxHits} grp {m.Group}");
        var fight = new CombatEngine(party, enc, Look, rng);
        fight.UseSpellTable(spells); fight.UseMonsterTable(monsters);
        o.WriteLine($"  start gold {fight.GoldEarned} chest {fight.ChestClass}");
        for (int r = 0; r < 40 && !fight.IsOver; r++)
        {
            foreach (var l in fight.RoundPaced(spells)) o.WriteLine($"  R{r} {l.Text}");
            foreach (var c in party.Members) o.WriteLine($"  S{r} {C(c)}");
            o.WriteLine($"  F{r} " + string.Join(" ", fight.Monsters.Select(m => m.Hits)));
        }
        if (fight.Won)
        {
            fight.AwardSpoils();
            foreach (var l in fight.TakeSaid()) o.WriteLine($"  W {l.Text}");
            foreach (var c in party.Members) o.WriteLine($"  S* {C(c)}");
        }
        o.WriteLine($"  END over {fight.IsOver} won {fight.Won} wiped {fight.PartyWiped} gold {fight.GoldEarned} rng {rng.Next()}");
    }
    catch (Exception e) { o.WriteLine($"  THREW {e.GetType().Name} {e.Message}"); }
}
o.Close();
