import zipfile, io, random, struct, zlib, os
R = random.Random(5); out = 'in'
# ---- number strings
base = ['0','1','-1','+1',' 12 ','\t7\t','1,234','1,,2',',5','1,','1.5','.5','5.','.','1e3','1E+3','1e-2','1e','1e+','2.5e1','1D3','1d-1','- 5','-$5','$5','5$',
        '\xa45','5\xa4','-\xa45','\xa4-5','\xa4 5','( 5)','(5)','5-','Infinity','-Infinity','NaN','inf','nan','0x10','&H10','&HFFFF','&H10000','&O17','&o8','&H',
        '2147483647','2147483648','-2147483648','-2147483649','2147483647.5','2147483646.5','0.5','1.5','2.5','-0.5','922337203685477.5807','922337203685477.5808',
        '-922337203685477.5808','79228162514264337593543950335','79228162514264337593543950336','1e28','1e29','0.00000000000000000000000000001','1.00005','1.00015',
        '123,456.789','12 34','1_000','٣','','  ','1e400','-1e400','1e-400','00012','1.2.3','1,2.3,4','--1','+-1','1e5000000000']
for _ in range(300):
    s = ''.join(R.choice('0123456789,.-+eE $\xa4&H') for _ in range(R.randint(1, 9)))
    base.append(s)
open(os.path.join(out, 'numbers.txt'), 'w', encoding='utf-8').write('\n'.join(x.replace('\n', ' ') for x in base) + '\n')
# ---- zips
def inner(files, method=zipfile.ZIP_DEFLATED):
    b = io.BytesIO(); z = zipfile.ZipFile(b, 'w', method)
    for n, d in files: z.writestr(n, d)
    z.close(); return b.getvalue()
def outer(entries, method=zipfile.ZIP_DEFLATED):
    return inner(entries, method)
data = lambda n: bytes(R.randrange(256) for _ in range(n))
cases = {
 'ok': outer([('M1/MORDOR.WIP', inner([('MORDOR.EXE', data(500)), ('MDATA1.MDR', data(900))])), ('M1/WAVES.WIP', inner([('A.WAV', data(300))])), ('M1/README.TXT', b'hi')]),
 'stored': outer([('MORDOR.WIP', inner([('X.DAT', data(100))], zipfile.ZIP_STORED))], zipfile.ZIP_STORED),
 'noaudio': outer([('mordor.wip', inner([('B.DAT', data(10))])), ('Setup.exe', b'MZ')]),
 'two': outer([('a/MORDOR.WIP', inner([])), ('b/MORDOR.WIP', inner([]))]),
 'none': outer([('X.TXT', b'x')]),
 'dupcase': outer([('MORDOR.WIP', inner([('A.DAT', b'1'), ('a.dat', b'2')]))]),
 'badname': outer([('MORDOR.WIP', inner([('../EVIL', b'1')]))]),
 'dirs': outer([('d1/', b''), ('d1/MORDOR.WIP', inner([('C.DAT', data(50))]))]),
 'repeat': outer([('MORDOR.WIP', inner([('A.DAT', b'1')])), ('mordor.WIP', inner([('A.DAT', b'1')]))]),
}
# a bad CRC: flip a byte in the stored payload of a stored inner entry
b = bytearray(outer([('MORDOR.WIP', inner([('X.DAT', b'hello world')], zipfile.ZIP_STORED))], zipfile.ZIP_STORED))
i = b.find(b'hello world'); b[i] ^= 1; cases['badcrc'] = bytes(b)
cases['truncated'] = cases['ok'][:len(cases['ok']) // 2]
cases['notzip'] = b'PK\x03\x04' + data(50)
for k, v in cases.items(): open(os.path.join(out, 'zip_' + k + '.zip'), 'wb').write(v)
# ---- PNGs of every colour type and filter
def png(w, h, ct, depth, rows, pal=None, trns=None):
    def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    ch = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ct]; bpp = max(1, ch * depth // 8); stride = (w * ch * depth + 7) // 8
    raw = b''; prev = bytes(stride)
    for r in rows:
        f = R.randrange(5); cur = r; o = bytearray(stride)
        for i in range(stride):
            a = cur[i - bpp] if i >= bpp else 0; bb = prev[i]; c = prev[i - bpp] if i >= bpp else 0
            if f == 0: pr = 0
            elif f == 1: pr = a
            elif f == 2: pr = bb
            elif f == 3: pr = (a + bb) // 2
            else:
                p = a + bb - c; pa, pb, pc = abs(p - a), abs(p - bb), abs(p - c); pr = a if pa <= pb and pa <= pc else bb if pb <= pc else c
            o[i] = (cur[i] - pr) & 255
        raw += bytes([f]) + bytes(o); prev = cur
    s = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, depth, ct, 0, 0, 0))
    if pal: s += chunk(b'PLTE', pal)
    if trns: s += chunk(b'tRNS', trns)
    z = zlib.compress(raw); s += chunk(b'IDAT', z[:len(z) // 2]) + chunk(b'IDAT', z[len(z) // 2:]) + chunk(b'IEND', b'')
    return s
n = 0
for ct, depth in [(0, 1), (0, 2), (0, 4), (0, 8), (2, 8), (3, 1), (3, 2), (3, 4), (3, 8), (4, 8), (6, 8)]:
    for _ in range(3):
        w, h = R.randint(1, 17), R.randint(1, 9)
        ch = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ct]; stride = (w * ch * depth + 7) // 8
        rows = [bytes(R.randrange(256) if ct != 3 else R.randrange(256) & ((1 << depth) - 1) * (1 if depth == 8 else 0) | 0 for _ in range(stride)) for _ in range(h)]
        pal = bytes(R.randrange(256) for _ in range(3 * 256)) if ct == 3 else None
        trns = bytes(R.randrange(256) for _ in range(R.randint(0, 10))) if ct == 3 else None
        open(os.path.join(out, 'png_%02d.png' % n), 'wb').write(png(w, h, ct, depth, rows, pal, trns)); n += 1
open(os.path.join(out, 'png_bad.png'), 'wb').write(b'\x89PNG\r\n\x1a\n' + b'\x00' * 5)
print('ok', n)
