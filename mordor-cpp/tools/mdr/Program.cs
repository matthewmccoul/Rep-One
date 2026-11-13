// mdr: read and edit records of the original Mordor .MDR files through OpenDejenol's VB3 layout.
// usage: mdr dump <file> <recLen> <rec> <Type>          prints every scalar path=value
//        mdr set  <file> <recLen> <rec> <Type> path=value ...   writes in place
using System; using System.IO; using System.Linq; using System.Buffers.Binary; using System.Globalization;
using Mordor.Core.Import;
static class P {
  static VbType T(string n) => VbLayout.All.First(t => t.Name == n || (n=="Character"&&t==VbLayout.Character) || (n=="Shelf"&&t==VbLayout.Shelf));
  static System.Collections.Generic.IEnumerable<(string path, VbField f, int off)> Walk(VbType t, string pre, int baseOff) {
    foreach (var f in t.Fields) for (int i = 0; i < f.Count; i++) {
      string p = pre + f.Name + (f.Upper >= 0 ? $"({i})" : "");
      int off = baseOff + f.Offset + i * f.ElementSize;
      if (f.Kind == VbKind.Udt) foreach (var x in Walk(f.Type!, p + ".", off)) yield return x; else yield return (p, f, off);
    }
  }
  static string Read(byte[] r, VbField f, int o) => f.Kind switch {
    VbKind.Integer => BinaryPrimitives.ReadInt16LittleEndian(r.AsSpan(o)).ToString(),
    VbKind.Long => BinaryPrimitives.ReadInt32LittleEndian(r.AsSpan(o)).ToString(),
    VbKind.Single => BinaryPrimitives.ReadSingleLittleEndian(r.AsSpan(o)).ToString("R", CultureInfo.InvariantCulture),
    VbKind.Double => BinaryPrimitives.ReadDoubleLittleEndian(r.AsSpan(o)).ToString("R", CultureInfo.InvariantCulture),
    VbKind.Currency => (BinaryPrimitives.ReadInt64LittleEndian(r.AsSpan(o)) / 10000m).ToString(CultureInfo.InvariantCulture),
    VbKind.FixedString => "\"" + System.Text.Encoding.Latin1.GetString(r, o, f.StringLength).TrimEnd(' ', '\0') + "\"",
    _ => "?" };
  static void Write(byte[] r, VbField f, int o, string v) {
    var c = CultureInfo.InvariantCulture;
    switch (f.Kind) {
      case VbKind.Integer: BinaryPrimitives.WriteInt16LittleEndian(r.AsSpan(o), short.Parse(v, c)); break;
      case VbKind.Long: BinaryPrimitives.WriteInt32LittleEndian(r.AsSpan(o), int.Parse(v, c)); break;
      case VbKind.Single: BinaryPrimitives.WriteSingleLittleEndian(r.AsSpan(o), float.Parse(v, c)); break;
      case VbKind.Double: BinaryPrimitives.WriteDoubleLittleEndian(r.AsSpan(o), double.Parse(v, c)); break;
      case VbKind.Currency: BinaryPrimitives.WriteInt64LittleEndian(r.AsSpan(o), (long)(decimal.Parse(v, c) * 10000m)); break;
      case VbKind.FixedString: var b = System.Text.Encoding.Latin1.GetBytes(v.PadRight(f.StringLength)); Array.Copy(b, 0, r, o, f.StringLength); break;
      default: throw new Exception("cannot write " + f.Kind);
    }
  }
  static int Main(string[] a) {
    string file = a[1]; int len = int.Parse(a[2]); int rec = int.Parse(a[3]); var t = T(a[4]);
    byte[] data = File.ReadAllBytes(file); long at = (long)(rec - 1) * len;
    int need = (int)Math.Max(data.Length, at + t.Size);
    if (need > data.Length) Array.Resize(ref data, need);
    var r = new byte[t.Size]; Array.Copy(data, at, r, 0, t.Size);
    if (a[0] == "dump") { foreach (var (p, f, o) in Walk(t, "", 0)) Console.WriteLine($"{p}={Read(r, f, o)}"); return 0; }
    foreach (var kv in a.Skip(5)) { int eq = kv.IndexOf('='); var (o, f) = t.Locate(kv[..eq]); Write(r, f, o, kv[(eq + 1)..]); }
    Array.Copy(r, 0, data, at, t.Size); File.WriteAllBytes(file, data); return 0;
  }
}
