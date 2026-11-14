// C# reference for the probe: number parsing, ZIP installer reading, PNG decode/encode, SHA-256 keys.
using System.Globalization; using System.Security.Cryptography; using System.Text; using Mordor.Core; using Mordor.Core.Extraction;
CultureInfo.CurrentCulture = CultureInfo.InvariantCulture;
string inDir = args[0], game = args[1]; var o = new StreamWriter(args[2]) { NewLine = "\n" };
string H(byte[] b) => Convert.ToHexString(SHA256.HashData(b))[..16];
int k = 0;
foreach (var s in File.ReadAllLines(Path.Combine(inDir, "numbers.txt"))) {
    bool a = VbConvert.TryCLng(s, out int cl);
    bool b = VbCurrency.TryParse(s, out var cur);
    long? g = GiveCommand.CLng(s);
    bool d = double.TryParse(s, NumberStyles.Float | NumberStyles.AllowThousands, CultureInfo.InvariantCulture, out double dv);
    bool m = decimal.TryParse(s, NumberStyles.Number, CultureInfo.InvariantCulture, out decimal mv);
    o.WriteLine($"N{k++} {(a ? cl.ToString() : "F")} {(b ? cur.Raw.ToString() : "F")} {(g?.ToString() ?? "null")} {(d ? BitConverter.DoubleToInt64Bits(dv).ToString() : "F")} {(m ? mv.ToString(CultureInfo.InvariantCulture) : "F")}");
}
foreach (var f in Directory.GetFiles(inDir, "zip_*.zip").OrderBy(x => x, StringComparer.Ordinal)) {
    string r;
    try { var c = InstallerUnpack.Parse(File.ReadAllBytes(f)); r = string.Join(",", c.Files.Select(e => e.RelativePath + ":" + e.Data.Length + ":" + H(e.Data))) + " | " + string.Join(",", c.Skipped); }
    catch (Exception e) { r = "THREW " + e.GetType().Name; }
    o.WriteLine($"Z {Path.GetFileName(f)} {r}");
}
foreach (var f in Directory.GetFiles(inDir, "png_*.png").OrderBy(x => x, StringComparer.Ordinal)) {
    string r;
    try { var img = Png.Decode(File.ReadAllBytes(f)); var px = img.Pixels.SelectMany(BitConverter.GetBytes).ToArray();
          r = img.Width + "x" + img.Height + " " + H(px) + " enc " + H(Png.Encode(img)); }
    catch (Exception e) { r = "THREW " + e.GetType().Name; }
    o.WriteLine($"P {Path.GetFileName(f)} {r}");
}
o.WriteLine("K " + GameArt.Key(game) + " " + (GameFiles.Recognise(Path.Combine(game, "MORDOR.EXE")) ?? "null"));
o.Close();
