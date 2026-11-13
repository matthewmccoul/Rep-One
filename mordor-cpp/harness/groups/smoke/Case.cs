using System.Text.Json;
public static class Case { public static string Run(JsonElement a, System.Random rng) { int x = a.GetProperty("a").GetInt32(), y = a.GetProperty("b").GetInt32(); return $"{x + y}|{rng.Next(10)}|{H.F(x / 3.0)}|{H.Items.Count}"; } }
