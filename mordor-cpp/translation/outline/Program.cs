// Outline of every type in Mordor.Core with the facts a C++ translation needs:
// kind (class = reference type, struct = value type), fields, auto-properties (become plain fields),
// computed properties (become methods), methods (signatures), enum members. One JSON per type.
using Microsoft.CodeAnalysis; using Microsoft.CodeAnalysis.CSharp; using Microsoft.CodeAnalysis.CSharp.Syntax; using System.Text.Json;
var root = args[0]; var outF = args[1]; var types = new List<object>();
foreach (var f in Directory.GetFiles(root, "*.cs", SearchOption.AllDirectories)) {
    if (f.Contains("/obj/") || f.Contains("/bin/")) continue;
    var tree = CSharpSyntaxTree.ParseText(File.ReadAllText(f)); var rel = Path.GetRelativePath(root, f);
    foreach (var t in tree.GetRoot().DescendantNodes().OfType<BaseTypeDeclarationSyntax>()) {
        string kind = t switch {
            EnumDeclarationSyntax => "enum",
            StructDeclarationSyntax => "struct",
            RecordDeclarationSyntax r => r.ClassOrStructKeyword.Text == "struct" ? "struct" : "class",
            InterfaceDeclarationSyntax => "interface",
            _ => "class" };
        bool isStatic = t.Modifiers.Any(m => m.Text == "static");
        string outer = string.Join(".", t.Ancestors().OfType<BaseTypeDeclarationSyntax>().Reverse().Select(a => a.Identifier.Text));
        var fields = new List<string>(); var autoProps = new List<string>(); var compProps = new List<string>(); var methods = new List<string>(); var enumMembers = new List<string>();
        string baseList = t.BaseList?.ToString() ?? "";
        if (t is EnumDeclarationSyntax e) enumMembers.AddRange(e.Members.Select(m => m.ToString()));
        if (t is RecordDeclarationSyntax rec && rec.ParameterList != null) foreach (var p in rec.ParameterList.Parameters) autoProps.Add($"{p.Type} {p.Identifier}");
        if (t is TypeDeclarationSyntax td) foreach (var m in td.Members) {
            string mods = string.Join(" ", m.Modifiers.Select(x => x.Text));
            switch (m) {
                case FieldDeclarationSyntax fd: foreach (var v in fd.Declaration.Variables) fields.Add($"{mods} {fd.Declaration.Type} {v.Identifier}".Trim()); break;
                case PropertyDeclarationSyntax pd:
                    bool auto = pd.AccessorList != null && pd.AccessorList.Accessors.All(a => a.Body == null && a.ExpressionBody == null);
                    bool settable = pd.AccessorList != null && pd.AccessorList.Accessors.Any(a => a.Keyword.Text is "set" or "init");
                    (auto ? autoProps : compProps).Add($"{mods} {pd.Type} {pd.Identifier}{(auto ? "" : settable ? " [get+set]" : " [get]")}".Trim()); break;
                case MethodDeclarationSyntax md: methods.Add($"{mods} {md.ReturnType} {md.Identifier}{md.TypeParameterList}{md.ParameterList}".Trim()); break;
                case ConstructorDeclarationSyntax cd: methods.Add($"{mods} ctor {cd.Identifier}{cd.ParameterList}".Trim()); break;
                case IndexerDeclarationSyntax ix: compProps.Add($"{mods} {ix.Type} this{ix.ParameterList} [indexer]".Trim()); break;
                case OperatorDeclarationSyntax op: methods.Add($"{mods} {op.ReturnType} operator {op.OperatorToken.Text}{op.ParameterList}".Trim()); break;
                case ConversionOperatorDeclarationSyntax co: methods.Add($"{mods} {co.ImplicitOrExplicitKeyword.Text} operator {co.Type}{co.ParameterList}".Trim()); break;
            }
        }
        types.Add(new { name = t.Identifier.Text, outer, file = rel, kind, isStatic, baseList, fields, autoProps, compProps, methods, enumMembers });
    }
}
File.WriteAllText(outF, JsonSerializer.Serialize(types, new JsonSerializerOptions { WriteIndented = true }));
Console.WriteLine($"{types.Count} types");
