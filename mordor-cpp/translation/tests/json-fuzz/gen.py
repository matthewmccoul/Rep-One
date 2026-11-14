#!/usr/bin/env python3
"""Random JSON documents shaped by the .NET contracts (../contract.json), with edge cases, for the round-trip differential."""
import json, random, sys, os
C = {t['type']: t for t in json.load(open(os.path.join(os.path.dirname(__file__), '..', 'contract.json')))}
RACES = ['Human', 'Elf', 'Giant', 'Gnome', 'Dwarf', 'Ogre', 'Morloch', 'Osiri', 'Troll']
GUILDS = ['Nomad', 'Warrior', 'Paladin', 'Ninja', 'Villain', 'Seeker', 'Thief', 'Scavenger', 'Mage', 'Sorcerer', 'Wizard', 'Healer']
R = random.Random(int(sys.argv[2]) if len(sys.argv) > 2 else 1)
NOSTRNULL = os.environ.get('NOSTRNULL') == '1'
BAD = 0.0  # set per document: most documents are well-formed, a few carry bad values

def jstr(s):  # JSON text for a str, mixing escapes and raw UTF-8
    out = '"'
    for ch in s:
        o = ord(ch)
        if ch == '"': out += '\\"'
        elif ch == '\\': out += '\\\\'
        elif o < 0x20: out += '\\u%04x' % o if R.random() < .5 else {'\n': '\\n', '\t': '\\t', '\r': '\\r'}.get(ch, '\\u%04x' % o)
        elif o > 0x7e and R.random() < .5: out += '\\u%04X' % o
        else: out += ch
    return out + '"'

def rstr():
    pool = 'abcXYZ 019<>&+`\'"\\/\n\té ÿ×~\x7f\x01'
    k = R.choice([0, 1, 3, 8, 15])
    return ''.join(R.choice(pool) for _ in range(k))

INTS = {'System.Int32': (-2**31, 2**31 - 1), 'System.Int64': (-2**63, 2**63 - 1), 'System.Int16': (-2**15, 2**15 - 1)}
ENUM_MAX = {'Alignment': 2, 'Sex': 1, 'RaceSize': 2, 'IdLevel': 3, 'ItemAlignment': 2}

def num_int(lo, hi):
    r = R.random()
    if r < BAD: return R.choice([str(hi + 1), str(lo - 1), '1.0', '1e2', '"5"'])
    if r < .15: return str(R.choice([lo, hi, 0, -1]))
    return str(R.randint(max(lo, -100000), min(hi, 100000)))

def num_double(single=False):
    r = R.random()
    if r < .2: return str(R.randint(-1000, 1000))
    if r < .3: return R.choice(['1e20', '-2.5E-7', '0.1', '1E+15', '123456789012345678', '0.00001', '3.4e38' if single else '1.7976931348623157e308', '-0'])
    v = R.uniform(-1e6, 1e6) * 10 ** R.randint(-8, 8)
    return repr(float(v)) if not single else repr(float(R.uniform(-1000, 1000)))

def num_decimal():
    r = R.random()
    if r < .1: return R.choice(['1e3', '2.50E-1', '0.000', '79228162514264337593543950335', '-12345678901234.5678'])
    return ('%s%d.%s' % (R.choice(['', '-']), R.randint(0, 10**9), ''.join(R.choice('0123456789') for _ in range(R.randint(0, 6))))).rstrip('.')

def val(t, depth=0, prop=None):
    if R.random() < BAD / 2: return R.choice(['null', '[]', '{}', '"x"', 'true', '7'])
    if t.startswith('Nullable<'):
        return 'null' if R.random() < .3 else val(t[9:-1], depth)
    if t in INTS and prop == 'Level': return str(R.randint(-2, 500))
    if t in INTS: return num_int(*INTS[t])
    if t == 'System.Boolean': return R.choice(['true', 'false'])
    if t == 'System.Double': return num_double()
    if t == 'System.Single': return num_double(True)
    if t == 'System.Decimal': return num_decimal()
    if t == 'System.String':
        if prop == 'Race': return jstr(R.choice(RACES))
        if prop == 'Guild' and R.random() < .9: return jstr(R.choice(GUILDS))
        return 'null' if (R.random() < .05 and not NOSTRNULL) else jstr(rstr())
    if t in ENUM_MAX: return str(R.randint(0, ENUM_MAX[t]))
    if t.endswith('[]') or t.startswith('List<') or t.startswith('HashSet<') or t.startswith('IEnumerable<'):
        if R.random() < .08: return 'null'
        el = t[:-2] if t.endswith('[]') else t[t.index('<') + 1:-1]
        n = R.choice([0, 1, 2, 4, 6]) if depth < 3 else 0
        if prop == 'Pack': n = R.choice([0, 3, 40, 41])
        return '[' + ','.join(val(el, depth + 1, prop) for _ in range(n)) + ']'
    if t.startswith('Dictionary<'):
        if R.random() < .08: return 'null'
        k, v = t[11:-1].split(',', 1)
        n = R.choice([0, 1, 3]) if depth < 3 else 0
        keys = []
        for _ in range(n):
            keys.append(str(R.randint(-5, 300)) if k == 'System.Int32' else rstr().replace('"', ''))
        return '{' + ','.join(jstr(kk) + ':' + val(v, depth + 1) for kk in keys) + '}'
    if t.startswith('ValueTuple'): return '{}'
    if t == 'CompanionSlots':
        if R.random() < .1: return 'null'
        return '[' + ','.join('null' if R.random() < .3 else val('Companion', depth + 1) for _ in range(R.choice([0, 2, 4, 6]))) + ']'
    c = C[t]
    if not c['isValue'] and R.random() < .05: return 'null'
    parts = []
    for p in c['props']:
        if R.random() < .15 and not (NOSTRNULL and p['type'] == 'System.String'): continue
        parts.append(jstr(p['json']) + ':' + val(p['type'], depth + 1, p['json']))
    if R.random() < .1: parts.append('"UnknownProp":' + R.choice(['1', '[1,{"a":null}]', '"s"']))
    R.shuffle(parts) if R.random() < .1 else None
    return '{' + ','.join(parts) + '}'

ROOTS = ['Party+Roster', 'StoreStock', 'Confinement', 'SharedMap', 'Guildmasters', 'List<System.String>', 'DungeonState',
         'HallRecords', 'Character', 'Librarian+Store', 'List<Extraction.HelpBundle+Topic>']
out = sys.argv[1]; os.makedirs(out, exist_ok=True)
n = int(os.environ.get('N', '40'))
with open(os.path.join(out, 'cases.txt'), 'w') as idx:
    for root in ROOTS:
        for i in range(n):
            f = '%s_%03d.json' % (root.replace('<', '(').replace('>', ')').replace('+', '_'), i)
            global_bad = R.random() < .15
            globals()['BAD'] = 0.01 if global_bad else 0.0
            ws = R.random() < .3
            text = val(root)
            if ws: text = '  \n' + text.replace(',', ', ').replace(':', ' : ') + '\n'
            open(os.path.join(out, f), 'w', encoding='utf-8').write(text)
            idx.write(root + '\t' + f + '\n')
