// Round trip with C# Mordor.Core: Deserialize<T> then Serialize, as Core's own Load/Save do.
using System.Reflection; using System.Text.Json; using System.Text.Json.Serialization; using Mordor.Core;
string dir = args[0], cases = args[1], outDir = args[2];
Mdata1.LoadFrom(dir); RaceData.LoadFrom(dir); GuildData.LoadFrom(dir);
var asm = typeof(Character).Assembly;
Type T(string n) => n switch {
    "Party+Roster" => typeof(Party).GetNestedType("Roster", BindingFlags.Public | BindingFlags.NonPublic)!,
    "List<System.String>" => typeof(List<string>),
    "List<Extraction.HelpBundle+Topic>" => typeof(List<>).MakeGenericType(asm.GetTypes().First(t => t.Name == "Topic")),
    _ => asm.GetType("Mordor.Core." + n)! };
var ind = new JsonSerializerOptions { WriteIndented = true };
var help = new JsonSerializerOptions { WriteIndented = false, DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull };
Directory.CreateDirectory(outDir);
foreach (var line in File.ReadAllLines(Path.Combine(cases, "cases.txt"))) {
    var p = line.Split('\t'); var o = p[0].StartsWith("List<Extraction") ? help : ind;
    string res;
    try { var v = JsonSerializer.Deserialize(File.ReadAllText(Path.Combine(cases, p[1])), T(p[0]), o); res = JsonSerializer.Serialize(v, T(p[0]), o); }
    catch (Exception e) { res = "THREW " + e.GetType().Name + (Environment.GetEnvironmentVariable("STACK") == "1" ? "\n" + e : ""); }
    File.WriteAllText(Path.Combine(outDir, p[1]), res);
    if (p[0] == "Character") {   // Core's own Load (names checked, legacy pack migrated, swings recalculated) then Save
        string o2 = Path.Combine(outDir, p[1] + ".load");
        try { Character.Load(Path.Combine(cases, p[1])).Save(o2); } catch (Exception e) { File.WriteAllText(o2, "THREW " + e.GetType().Name); }
    }
}
