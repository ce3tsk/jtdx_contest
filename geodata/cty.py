import re,os,sys,json,math
STATION_CTY = os.path.expanduser('~/.local/share/JTDX/cty.dat')

def cty_path(tree_only=False):
    """The copy the tables are built from, so a rebuild is reproducible from the tree.

    Review 2026-09-24: this used to read the running station's file, which no tree copy matched.
    Second review the same day: it still FELL BACK to that file without a word, so a table could be
    built from the station's cty.dat while its own header claimed the tree copy.  Third review, same
    day again: tree_only refused the fallback but not $JTDX_CTY_DAT POINTING AT the station file, so
    the hole was one step sideways rather than closed.  tree_only - what the table builders and the
    table tests pass - now means "a file in the tree, whatever route it came by"."""
    here=os.path.dirname(os.path.abspath(__file__))
    cands=[os.environ.get('JTDX_CTY_DAT'),
           os.path.join(here,'cty.dat')]                          # a copy beside this file
    d=here                       # a geodata/cty.dat above us: tools/, test/experiments/..., a tree.
    for _ in range(4):           # Only inside the work tree - a stray ~/dev/geodata is not "the tree"
        d=os.path.dirname(d)
        if os.path.isdir(os.path.join(d,'tools')) or os.path.isdir(os.path.join(d,'jtdx_contest')) \
           or os.path.basename(d)=='geodata':
            cands.append(os.path.join(d,'geodata','cty.dat'))
    for p in cands:
        if not p or not os.path.exists(p): continue
        p=os.path.normpath(p)
        if tree_only and os.path.realpath(p)==os.path.realpath(STATION_CTY):
            raise SystemExit('FAILED: %s is the RUNNING STATION\'s cty.dat\n'
                             '  (reached through $JTDX_CTY_DAT).  A table must be reproducible from\n'
                             '  the tree, so this file is deliberately not used here.' % p)
        return p
    where=('no cty.dat in the tree - looked at $JTDX_CTY_DAT, a copy beside %s\n'
           '  and geodata/cty.dat above it.  Fetch http://www.country-files.com/bigcty/cty.dat\n'
           % os.path.basename(__file__))
    if tree_only: raise SystemExit('FAILED: ' + where +
                                   '  (the running station copy is deliberately NOT used here:\n'
                                   '   a table must be reproducible from the tree)')
    if os.path.exists(STATION_CTY):
        print('NOTE: ' + where + '  falling back to the running station file %s' % STATION_CTY,
              file=sys.stderr)
        return STATION_CTY
    raise SystemExit('FAILED: ' + where + '  and %s does not exist either' % STATION_CTY)

def load_cty(path=None):
    path = path or cty_path()
    txt=open(path,encoding='utf-8',errors='replace').read()
    ents=[]; pref={}; exact={}
    for rec in txt.split(';'):
        rec=rec.strip()
        if not rec: continue
        head,_,tail=rec.partition('\n')
        f=[x.strip() for x in head.split(':')]
        if len(f)<8: continue
        name,cq,itu,cont,lat,lon,gmt,main=f[:8]
        try: lat=float(lat); lon=-float(lon)          # cty.dat gives west-positive longitude
        except ValueError: continue
        e=len(ents); ents.append((name,cont,lat,lon))
        for p in re.split(r'[,\s]+',(main+','+tail).strip()):
            p=p.strip()
            if not p: continue
            p=re.sub(r'\([^)]*\)|\[[^\]]*\]|\{[^}]*\}|~[^~]*~','',p)
            if not p: continue
            if p.startswith('='): exact[p[1:].upper()]=e
            else: pref[p.upper()]=e
    return ents,pref,exact
def resolve(call,ents,pref,exact):
    c=call.upper()
    if c in exact: return exact[c]
    # KG4 is Guantanamo Bay only with a TWO-letter suffix; KG4 plus three letters is an ordinary US
    # call.  cty.dat cannot say that with a prefix, it only lists the exceptions it knows, so a KG4
    # station not on that list resolved to Guantanamo and its US grid was rejected (air test,
    # 2026-09-24: KG4OJT FM18, KG4SRK EM57).
    if c.startswith('KG4') and '/' not in c and len(c) != 5:
        for n in ('K','W'):
            if n in pref: return pref[n]
    parts=c.split('/')
    if len(parts)>1:                      # the shorter part is usually the location prefix
        cand=[p for p in parts if p not in ('P','R','M','QRP','A','MM','AM')]
        if len(cand)>1:
            # The shorter part decides - even a single letter: ON4ABC/F is France, not Belgium
            # (review 2026-09-24; the old guard required two characters and fell back to the home
            # call).  A one-character part counts only when it resolves on its own.
            short=min(cand,key=len)
            if len(short)>=2 or any(short[:n] in pref for n in range(len(short),0,-1)): c=short
            else: c=cand[0]
        elif cand: c=cand[0]
    for n in range(len(c),0,-1):
        if c[:n] in pref: return pref[c[:n]]
    return None
def grid_ll(g):
    g=g.upper()
    if not re.fullmatch(r'[A-R]{2}\d{2}([A-X]{2})?',g): return None
    lon=(ord(g[0])-65)*20-180+(int(g[2]))*2+1
    lat=(ord(g[1])-65)*10-90+(int(g[3]))*1+0.5
    return lat,lon
def km(a,b):
    (la1,lo1),(la2,lo2)=a,b
    p=math.pi/180
    x=math.sin((la2-la1)*p/2)**2+math.cos(la1*p)*math.cos(la2*p)*math.sin((lo2-lo1)*p/2)**2
    return 12742*math.asin(min(1,math.sqrt(x)))
if __name__=='__main__':
    ents,pref,exact=load_cty(); print('cty.dat: %d entities, %d prefixes, %d exact'%(len(ents),len(pref),len(exact)))
    for c in ['CE3TSK','T1TA7I1J/RI','5NVR/B7FIBP','VO4MEQRXPIH','PV9DJZ/R','9B4NFA','L64ANH/R','MG0QYG','KM8XIH','WF6RRZ','LZ1ST','VE7SL','VK2EFM','KH8WW','VP8PJ']:
        e=resolve(c,ents,pref,exact)
        print('  %-12s %s'%(c, ents[e][0] if e is not None else 'NO DXCC PREFIX'))
