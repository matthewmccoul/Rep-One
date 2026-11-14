import os,sys,collections
a,b,cases=sys.argv[1:4]; res=collections.Counter(); bad=[]
rows=[l.rstrip('\n').split('\t') for l in open(os.path.join(cases,'cases.txt'))]
rows+= [(t+'.Load',f+'.load') for t,f in rows if t=='Character']
for t,f in rows:
    x=open(os.path.join(a,f),encoding='utf-8').read(); y=open(os.path.join(b,f),encoding='utf-8',errors='replace').read()
    tx, ty = x.startswith('THREW'), y.startswith('THREW')
    if tx and ty: res[t,'both threw']+=1
    elif x==y: res[t,'same']+=1
    else: res[t,'DIFF']+=1; bad.append((f,x,y))
for k in sorted(res): print(k, res[k])
print('total diffs', len(bad))
import difflib
for f,x,y in bad[:int(os.environ.get('SHOW','6'))]:
    print('==',f); 
    if x.startswith('THREW') or y.startswith('THREW'): print(' C#:',x[:200]); print(' C++:',y[:200]); continue
    xl,yl=x.splitlines(),y.splitlines()
    for d in list(difflib.unified_diff(xl,yl,'cs','cpp',n=1,lineterm=''))[:14]: print(' ',d[:200])
