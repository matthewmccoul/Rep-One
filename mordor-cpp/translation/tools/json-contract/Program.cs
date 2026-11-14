using System.Reflection;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text.Json.Serialization.Metadata;
using Mordor.Core;

var asm = typeof(Character).Assembly;
var std = new JsonSerializerOptions { WriteIndented = true, TypeInfoResolver = new DefaultJsonTypeInfoResolver() };
var help = new JsonSerializerOptions { DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, TypeInfoResolver = new DefaultJsonTypeInfoResolver() };
var roots = new List<(Type, JsonSerializerOptions)> {
  (typeof(Party).GetNestedType("Roster", BindingFlags.Public|BindingFlags.NonPublic)!, std),
  (typeof(StoreStock), std), (typeof(Confinement), std), (typeof(SharedMap), std), (typeof(Guildmasters), std),
  (typeof(List<string>), std), (typeof(DungeonState), std), (typeof(HallRecords), std), (typeof(Character), std),
  (asm.GetTypes().First(t => t.FullName == "Mordor.Core.Librarian+Store"), std),
};
var topic = asm.GetTypes().First(t => t.Name == "Topic");
roots.Add((typeof(List<>).MakeGenericType(topic), help));
var seen = new Dictionary<Type, object>();
var queue = new Queue<(Type, JsonSerializerOptions)>(roots);
Type[] GA(Type t, Type g) => (t.IsGenericType && t.GetGenericTypeDefinition()==g ? t : t.GetInterfaces().First(i => i.IsGenericType && i.GetGenericTypeDefinition()==g)).GetGenericArguments();
string TN(Type t) => t.IsGenericType ? t.Name.Split('`')[0] + "<" + string.Join(",", t.GetGenericArguments().Select(TN)) + ">" : (t.FullName ?? t.Name).Replace("Mordor.Core.", "");
while (queue.Count > 0) {
  var (t, o) = queue.Dequeue();
  if (seen.ContainsKey(t)) continue;
  var ti = o.GetTypeInfo(t);
  var conv = t.GetCustomAttribute<JsonConverterAttribute>()?.ConverterType?.Name;
  var e = new Dictionary<string, object?> { ["type"] = TN(t), ["kind"] = ti.Kind.ToString(), ["isValue"] = t.IsValueType, ["converter"] = conv, ["helpOpts"] = o == help };
  seen[t] = e;
  void Enq(Type x) { var u = Nullable.GetUnderlyingType(x) ?? x; if (u.Assembly == asm || u.IsGenericType) queue.Enqueue((u, o)); }
  if (ti.Kind == JsonTypeInfoKind.Enumerable) { var el = GA(t, typeof(IEnumerable<>))[0]; e["elem"] = TN(el); Enq(el); }
  if (ti.Kind == JsonTypeInfoKind.Dictionary) { var kv = GA(t, typeof(IDictionary<,>)); e["key"] = TN(kv[0]); e["elem"] = TN(kv[1]); Enq(kv[1]); }
  if (ti.Kind == JsonTypeInfoKind.Object && conv == null) {
    var props = new List<object>();
    foreach (var p in ti.Properties) {
      var m = p.AttributeProvider as MemberInfo;
      var ign = m?.GetCustomAttribute<JsonIgnoreAttribute>()?.Condition.ToString();
      props.Add(new Dictionary<string, object?> { ["json"] = p.Name, ["member"] = m?.Name, ["memberKind"] = m?.MemberType.ToString(),
        ["type"] = TN(p.PropertyType), ["get"] = p.Get != null, ["set"] = p.Set != null, ["ignore"] = ign,
        ["converter"] = p.CustomConverter?.GetType().Name, ["order"] = p.Order });
      if (p.CustomConverter == null) Enq(p.PropertyType);
    }
    e["props"] = props;
    var ctors = t.GetConstructors();
    var jc = ctors.FirstOrDefault(c => c.GetCustomAttribute<JsonConstructorAttribute>() != null);
    var pless = t.GetConstructor(Type.EmptyTypes);
    var use = jc ?? (pless != null || t.IsValueType ? null : (ctors.Length == 1 ? ctors[0] : null));
    e["ctorParams"] = use?.GetParameters().Select(p => p.Name + ":" + TN(p.ParameterType)).ToList();
    e["onDeserialized"] = typeof(IJsonOnDeserialized).IsAssignableFrom(t);
    e["abstractOrIface"] = t.IsAbstract || t.IsInterface;
  }
  // converters with nested types
  if (conv == "CompanionSlotsJson") Enq(typeof(List<>).MakeGenericType(typeof(Companion)));
}
Console.WriteLine(JsonSerializer.Serialize(seen.Values, new JsonSerializerOptions { WriteIndented = true }));
