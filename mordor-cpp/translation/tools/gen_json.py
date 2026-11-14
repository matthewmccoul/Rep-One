#!/usr/bin/env python3
"""Generate gen/JsonContracts.{h,cpp}: one JsonOut/JsonIn pair per Core type that Core serializes.

Input: json/contract.json, dumped from .NET's own System.Text.Json contracts for Mordor.Core
(json/dump/Program.cs), and catalog.json for the C++ member forms. The output follows the
contract exactly: property order, JSON names, getter/setter presence, [JsonIgnore] conditions,
[JsonConverter] converters and IJsonOnDeserialized.
"""
import json, re, sys, os

X = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
contract = json.load(open(os.path.join(X, 'json/contract.json')))
catalog = json.load(open(os.path.join(X, 'catalog.json')))
by_cpp = {e['cpp']: e for e in catalog.values()}

def cpp_name(cs):
    return cs.split('.')[-1].replace('+', '_') if '<' not in cs else None

def member_form(cls, member):
    """('field', name) | ('prop', getter, setter or None)."""
    decls = by_cpp[cls]['decl']
    getter = setter = None
    for d in decls:
        code = d.split('//')[0]
        if re.search(r'(^|[\s*&>])' + re.escape(member) + r'\s*(=[^;]*)?;\s*$', code.strip()) and '(' not in code:
            return ('field', member)
        if re.search(r'(^|[\s*&>])' + re.escape(member) + r'\(\)\s*(const)?\s*;', code):
            getter = member + '()'
        if re.search(r'\bset_' + re.escape(member) + r'\(', code):
            setter = 'set_' + member
    if getter:
        return ('prop', getter, setter)
    raise SystemExit(f'no C++ member for {cls}.{member}')

VALUE_CONVERTERS = {'SavedWholeJson': True, 'SavedExactJson': True}   # HandleNull (value types, or HandleNull => true)

decl_lines, body = [], []
for t in contract:
    name = cpp_name(t['type'])
    if not name or t['kind'] != 'Object' and not t.get('converter'):
        continue
    if name.startswith('ValueTuple'):
        continue  # written through the property that holds them (no public properties: "{}")
    e = by_cpp.get(name)
    if e is None:
        raise SystemExit('not in catalog: ' + name)
    decl_lines.append(f'cs::Json::Node JsonOut(const {name}& v);')
    decl_lines.append(f'void JsonIn(const cs::Json::Node& n, {name}& v);')
    if t.get('converter'):
        conv = t['converter']
        # a type-level [JsonConverter]: the converter writes and reads the whole value
        body.append(f'''cs::Json::Node JsonOut(const {name}& v) {{
    cs::Json::Node t; {conv}{{}}.Write(cs::Utf8JsonWriter{{&t}}, std::const_pointer_cast<{name}>(v.shared_from_this()), nullptr); return t;
}}
void JsonIn(const cs::Json::Node& n, {name}& v) {{
    cs::Utf8JsonReader r{{&n}}; v = *{conv}{{}}.Read(r, cs::Type{{}}, nullptr);
}}
''')
        continue
    out = [f'cs::Json::Node JsonOut(const {name}& v0) {{', f'    auto& v = const_cast<{name}&>(v0);', '    cs::Json::Node o = cs::Json::Node::Obj();']
    inn = [f'void JsonIn(const cs::Json::Node& n, {name}& v) {{',
           f'    if (n.kind != cs::Json::Node::Object) cs::Json::Mismatch("{t["type"]}", n);',
           '    for (const auto& kv : n.props) {', '        const auto& x = kv.second; (void)x;']
    for p in t['props']:
        if not p['get'] and not p['set']:
            continue  # [JsonIgnore] (Always)
        form = member_form(name, p['member'])
        get = 'v.' + (form[1])
        conv, ign, js = p['converter'], p['ignore'], p['json']
        if p['get']:
            if p['type'].startswith('IEnumerable<ValueTuple'):
                val = '[&] { if (w.IsNull()) return cs::Json::Node(); auto a = cs::Json::Node::Arr(); for (auto& _ : w) { (void)_; a.items.push_back(cs::Json::Node::Obj()); } return a; }()'
            elif conv:
                val = f'[&] {{ cs::Json::Node t; {conv}{{}}.Write(cs::Utf8JsonWriter{{&t}}, w, nullptr); return t; }}()'
            else:
                val = 'cs::Json::ToJson(w)'
            cond = {'WhenWritingNull': 'cs::Json::IsNullValue(w)', 'WhenWritingDefault': 'cs::Json::IsDefaultValue(w)'}.get(ign, 'cs::Json::IgnoreNullsByDefault() && cs::Json::IsNullValue(w)')
            out.append(f'    {{ auto w = {get}; if (!({cond})) o.Add("{js}", {val}); }}')
        if p['set']:
            if form[0] == 'field':
                tgt_t = f'std::decay_t<decltype(v.{form[1]})>'
                assign = lambda e: f'v.{form[1]} = {e};'
            else:
                if not form[2]:
                    raise SystemExit(f'{name}.{p["member"]}: settable in C# but no setter in C++')
                tgt_t = f'std::decay_t<decltype(v.{form[1]})>'
                assign = lambda e, s=form[2]: f'v.{s}({e});'
            if conv:
                handle_null = VALUE_CONVERTERS.get(conv, False)
                pre = '' if handle_null else 'if (x.kind == cs::Json::Node::Null) { ' + assign(tgt_t + '{}') + ' continue; } '
                inn.append(f'        if (kv.first == "{js}") {{ {pre}cs::Utf8JsonReader r{{&x}}; {assign(f"{conv}{{}}.Read(r, cs::Type{{}}, nullptr)")} continue; }}')
            else:
                inn.append(f'        if (kv.first == "{js}") {{ {tgt_t} t{{}}; cs::Json::FromJson(x, t); {assign("t")} continue; }}')
    out.append('    return o;')
    out.append('}')
    inn.append('    }')
    if t.get('onDeserialized'):
        inn.append('    v.OnDeserialized();')
    inn.append('}')
    body.append('\n'.join(out) + '\n' + '\n'.join(inn) + '\n')

hdr = '#pragma once\n// Generated by scripts/gen_json.py from the .NET contracts (json/contract.json). Do not edit.\nnamespace mc {\n' + '\n'.join(decl_lines) + '\n}  // namespace mc\n'
cpp = '// Generated by scripts/gen_json.py from the .NET contracts (json/contract.json). Do not edit.\n#include "core.h"\nnamespace mc {\n\n' + '\n'.join(body) + '}  // namespace mc\n'
open(os.path.join(X, 'cpp/gen/JsonContracts.h'), 'w').write(hdr)
open(os.path.join(X, 'cpp/gen/JsonContracts.cpp'), 'w').write(cpp)
print(len(decl_lines) // 2, 'types')
