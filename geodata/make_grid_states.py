#!/usr/bin/env python3
"""Which US state(s) does each 4-character Maidenhead grid square cover?

Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software:
you may redistribute it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License, or (at your option) any
later version.  It is distributed WITHOUT ANY WARRANTY; see the GNU General Public License for more
details.  The TABLES it writes are a different matter - see the note they carry.


    python3 tools/make_grid_states.py [path/to/another/state/shapefile]

Input : the US Census Bureau's cartographic state boundaries (a US Government work, public domain;
        SESSION_RULES.md rule 6b).  Default 1:20m, 186 KB:
        https://www2.census.gov/geo/tiger/GENZ2023/shp/cb_2023_us_state_20m.zip
        Population comes from the Census centers of population by tract, 2020 (also public domain):
        https://www2.census.gov/geo/docs/reference/cenpop2020/tract/CenPop2020_Mean_TR.txt
        Each of its 85 395 tracts carries its population and its population-weighted centroid, and a
        tract's people are counted in the square that centroid falls in; 83 848 of them land in the
        50 states once DC is folded into Maryland, the territories dropped and the empty tracts
        skipped.

Output: grid_states.txt - one line per grid square that any state touches:

            FM18 5022671 VA:0.549:0.479 MD:0.451:0.360
            <square> <people in it> <state>:<share of the people>:<share of the area> ...

        **States are ranked by their share of the POPULATION, with the area share as the tie-break** -
        so the first entry is where a caller in that square most likely is, not merely which state has
        the most ground.  Squares with no population at all fall back to the area ranking.  Squares are
        2 deg of longitude by 1 deg of latitude.

        Washington DC is folded into Maryland, as Worked All States counts it; --keep-dc reports it
        separately instead.  The island territories (PR, VI, GU, AS, MP) are left out: they are not
        states but separate DXCC entities, which cty.dat resolves by prefix; --keep-territories keeps
        whichever of them the boundary file carries.

Needs pyshp and shapely.  Reads the shapefile from geodata/ and writes geodata/grid_states.txt;
both live in the tree so the table rebuilds with no network.  See geodata/README.md.
"""
import os, sys, json, hashlib, shapefile
from datetime import date, datetime, timezone
from shapely.geometry import shape, box
from shapely.ops import unary_union
from shapely.prepared import prep

HERE = os.path.dirname(os.path.abspath(__file__))
# Run from tools/ in the work tree, or from inside a geodata/ folder that carries the inputs.
# The probe asks for an input this script itself reads - it used to ask for cty.dat, which it never
# opens, so a folder holding the Census files but no cty.dat looked wrong (review 2026-09-24).
DATA = os.path.normpath(HERE if any(os.path.exists(os.path.join(HERE, f))
                                    for f in ('cb_2023_us_state_20m.zip', 'cb_2023_us_state_20m.shp',
                                              'CenPop2020_Mean_TR.txt'))
                        else os.path.join(HERE, '..', 'geodata'))
ARGS = [a for a in sys.argv[1:] if not a.startswith('--')]
KEEP_DC = '--keep-dc' in sys.argv[1:]
KEEP_TERR = '--keep-territories' in sys.argv[1:]
SHP  = ARGS[0] if ARGS else os.path.join(DATA, 'cb_2023_us_state_20m')
POP  = os.path.join(DATA, 'CenPop2020_Mean_TR.txt')
OUT  = os.path.join(DATA, 'grid_states.txt')
OUTJ = os.path.join(DATA, 'grid_states.json')      # the same table as JSON - what a program should read
# STATEFP -> USPS, the Census FIPS codes, for the tract file
FIPS = {'01':'AL','02':'AK','04':'AZ','05':'AR','06':'CA','08':'CO','09':'CT','10':'DE','11':'DC',
        '12':'FL','13':'GA','15':'HI','16':'ID','17':'IL','18':'IN','19':'IA','20':'KS','21':'KY',
        '22':'LA','23':'ME','24':'MD','25':'MA','26':'MI','27':'MN','28':'MS','29':'MO','30':'MT',
        '31':'NE','32':'NV','33':'NH','34':'NJ','35':'NM','36':'NY','37':'NC','38':'ND','39':'OH',
        '40':'OK','41':'OR','42':'PA','44':'RI','45':'SC','46':'SD','47':'TN','48':'TX','49':'UT',
        '50':'VT','51':'VA','53':'WA','54':'WV','55':'WI','56':'WY','60':'AS','66':'GU','69':'MP',
        '72':'PR','78':'VI'}
MIN_FRACTION = 0.001          # ignore slivers below 0.1 % of a square
# CE3TSK (the operator, 2026-09-24): for Worked All States, Washington DC is NOT a state of its own -
# it counts as Maryland.  The table therefore folds DC into MD; pass --keep-dc to report it separately,
# which is the geographic truth but not what an award consumer wants.
FOLD = {'DC': 'MD'}
# The island territories - Puerto Rico (KP4), the US Virgin Islands (KP2), Guam (KH2), American Samoa
# (KH8), the Northern Marianas (KH0) - are NOT states and do not count toward the 50; the ARRL treats
# them as separate DXCC entities, which cty.dat already resolves by prefix.  They are left out of this
# table; pass --keep-territories to include whichever of them the boundary file happens to carry.
TERRITORIES = {'PR', 'VI', 'GU', 'AS', 'MP'}
# The 50 states the table must end up with, no more and no fewer.  Checked at the end: a boundary file
# that drops a state (or carries one we do not expect) fails the build loudly rather than writing a
# table with a hole in it - a missing state would silently mean "never needed" to a WAS preference.
WAS_STATES = set(('AL AK AZ AR CA CO CT DE FL GA HI ID IL IN IA KS KY LA ME MD MA MI MN MS MO MT NE NV '
                  'NH NJ NM NY NC ND OH OK OR PA RI SC SD TN TX UT VT VA WA WV WI WY').split())

def wrap(lon):   return ((lon + 180.0) % 360.0) - 180.0    # the antimeridian: 180 is -180
def field(lon):  return chr(ord('A') + int((wrap(lon) + 180) // 20))
def square(lon): return str(int(((wrap(lon) + 180) % 20) // 2))
def gridname(lon, lat):
    lat = min(89.999, max(-90.0, lat))                      # fields are A..R, never S
    return field(lon) + chr(ord('A') + int((lat + 90) // 10)) + square(lon) + str(int(((lat + 90) % 10) // 1))

# Both inputs are checked before the polygon pass: the population file used to be looked for only
# after every state had been scanned, minutes in (review 2026-09-24).
if not os.path.exists(SHP + '.shp') and not os.path.exists(SHP + '.zip'):
    raise SystemExit('FAILED: %s.shp is missing - fetch and unpack\n  https://www2.census.gov/geo/'
                     'tiger/GENZ2023/shp/cb_2023_us_state_20m.zip\ninto %s' % (SHP, DATA))
if not os.path.exists(POP):
    raise SystemExit('FAILED: %s is missing - fetch it from\n  https://www2.census.gov/geo/docs/'
                     'reference/cenpop2020/tract/CenPop2020_Mean_TR.txt' % POP)
UNPACKED = []
if not os.path.exists(SHP + '.shp'):
    # into the directory the shapefile will be READ from, which is not DATA when a path was given
    import zipfile, glob
    zipfile.ZipFile(SHP + '.zip').extractall(os.path.dirname(SHP) or '.')
    UNPACKED = [p for p in glob.glob(SHP + '.*') if not p.endswith('.zip')]
r = shapefile.Reader(SHP)
i_usps = [f[0] for f in r.fields[1:]].index('STUSPS')
states = {}
for sr in r.shapeRecords():
    g = shape(sr.shape.__geo_interface__)
    if not g.is_valid: g = g.buffer(0)
    usps = sr.record[i_usps]
    if not KEEP_DC: usps = FOLD.get(usps, usps)      # DC counts as Maryland for WAS
    if usps in TERRITORIES and not KEEP_TERR: continue   # separate DXCC entities, not states
    states.setdefault(usps, []).append(g)
states = {k: unary_union(v) for k, v in states.items()}
print('states read: %d' % len(states))

rows = {}
for usps, geom in sorted(states.items()):
    minx, miny, maxx, maxy = geom.bounds
    pgeom = prep(geom)
    lon = (int((minx + 180) // 2) * 2) - 180
    while lon <= maxx:
        lat = int(miny // 1)
        while lat <= maxy:
            if lon >= 180: lat += 1; continue           # the wrap-around duplicate of -180
            cell = box(lon, lat, lon + 2, lat + 1)
            if pgeom.intersects(cell):
                inter = geom.intersection(cell)
                frac = inter.area / cell.area
                if frac >= MIN_FRACTION:
                    rows.setdefault(gridname(lon + 1, lat + 0.5), {})[usps] = frac
            lat += 1
        lon += 2
    print('  %-3s %d squares' % (usps, sum(1 for v in rows.values() if usps in v)))

# the population side: tract centres, counted into the square they fall in
import csv
pop, tracts = {}, 0
with open(POP, encoding='utf-8-sig') as fh:
    for rec in csv.DictReader(fh):
        usps = FIPS.get(rec['STATEFP'])
        if usps is None: raise SystemExit('FAILED: unknown STATEFP %r' % rec['STATEFP'])
        if not KEEP_DC: usps = FOLD.get(usps, usps)
        if usps in TERRITORIES and not KEEP_TERR: continue
        n = int(rec['POPULATION'])
        if n <= 0: continue
        g = gridname(float(rec['LONGITUDE']), float(rec['LATITUDE']))
        pop.setdefault(g, {})[usps] = pop.setdefault(g, {}).get(usps, 0) + n
        tracts += 1
print('tracts read: %d' % tracts)
# a state can hold people in a square whose area share fell below the sliver threshold: keep it, area 0
for g, byst in pop.items():
    for usps in byst: rows.setdefault(g, {}).setdefault(usps, 0.0)

seen = {usps for v in rows.values() for usps in v}
expected = WAS_STATES | (TERRITORIES & set(states)) if KEEP_TERR else WAS_STATES
expected = expected | {'DC'} if KEEP_DC else expected
if seen != expected:
    raise SystemExit('FAILED: the table holds %d of the expected %d\n  missing: %s\n  unexpected: %s'
                     % (len(seen), len(expected), sorted(expected - seen) or 'none',
                        sorted(seen - expected) or 'none'))
print('all %d states present' % len(seen))
BUILT_BY = ('built by tools/make_grid_states.py of JTDX_CONTEST, Tihomir Sokcevic CE3TSK '
            '(the script is GPL v3)')
CLAIM = ('This table is derived from public-domain US Census data and states facts about geography; '
         'no rights of any kind are claimed over it, and it may be used freely for any purpose.')
SOURCES = [
    {'what': 'state boundaries',
     'file': os.path.basename(SHP),
     'url': 'https://www2.census.gov/geo/tiger/GENZ2023/shp/cb_2023_us_state_20m.zip',
     'publisher': 'US Census Bureau, cartographic boundary files 2023 (1:20,000,000)',
     'licence': 'US Government work, public domain'},
    {'what': 'population',
     'file': os.path.basename(POP),
     'url': 'https://www2.census.gov/geo/docs/reference/cenpop2020/tract/CenPop2020_Mean_TR.txt',
     'publisher': 'US Census Bureau, centers of population by census tract, 2020',
     'licence': 'US Government work, public domain'},
]
RULES = {'order': 'by share of population, ties broken by share of area',
         'dc': 'kept separate' if KEEP_DC else 'counted as Maryland (Worked All States)',
         'territories': 'included' if KEEP_TERR else 'left out (separate DXCC entities, not states)',
         'sliver': 'a state under %.1f%% of a squares area is dropped unless it holds people'
                   % (MIN_FRACTION * 100),
         'square': '2 degrees of longitude by 1 degree of latitude'}
BUILT   = date.today().isoformat()
VERSION = datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S')   # the table's own version stamp

def ordered(g):
    people = pop.get(g, {})
    total = sum(people.values())
    parts = sorted(rows[g].items(),
                   key=lambda kv: (-(people.get(kv[0], 0) / total if total else 0), -kv[1]))
    return total, [(k, (people.get(k, 0) / total if total else 0), v) for k, v in parts]

with open(OUT, 'w') as fh:
    fh.write('# 4-character Maidenhead grid square -> the US state(s) it covers.\n'
             '#   <square> <people in it> <state>:<share of the people>:<share of the area> ...\n'
             '#\n')
    for src in SOURCES:
        fh.write('# %s: %s\n#   %s\n#   %s - %s\n'
                 % (src['what'], src['file'], src['url'], src['publisher'], src['licence']))
    fh.write('#\n')
    for k in ('order', 'dc', 'territories', 'sliver', 'square'):
        fh.write('# %-12s %s\n' % (k + ':', RULES[k]))
    fh.write('#\n# version %s (built %s), %s.\n' % (VERSION, BUILT, BUILT_BY))
    fh.write('# This table is derived from public-domain US Census data and states facts about\n'
             '# geography; no rights of any kind are claimed over it, and it may be used freely for\n'
             '# any purpose.\n')
    fh.write('# See geodata/README.md.  A JSON twin of this table, easier for a program to read, is\n'
             '# grid_states.json.\n')
    body = ''.join('%s %d %s\n' % (g, ordered(g)[0], ' '.join('%s:%.3f:%.3f' % p for p in ordered(g)[1]))
                   for g in sorted(rows))
    fh.write('# content %s - the sha256 of the rows below, so a rebuild can be compared\n'
             % hashlib.sha256(body.encode()).hexdigest()[:12])
    fh.write(body)

with open(OUTJ, 'w') as fh:
    json.dump({'version': VERSION,
               'content': hashlib.sha256(''.join('%s %d %s\n' % (g, ordered(g)[0],
                   ' '.join('%s:%.3f:%.3f' % p for p in ordered(g)[1])) for g in sorted(rows)).encode()
                   ).hexdigest()[:12],
               'built': BUILT, 'tool': 'tools/make_grid_states.py',
               'project': 'JTDX_CONTEST', 'author': 'Tihomir Sokcevic CE3TSK',
               'rights': CLAIM, 'tool_licence': 'GPL v3',
               'sources': SOURCES, 'rules': RULES,
               'fields': {'pop': 'people living in the square',
                          's': 'state, two-letter USPS code', 'p': 'its share of those people',
                          'a': 'its share of the square area'},
               'squares': {g: {'pop': t, 'states': [{'s': k, 'p': round(p, 3), 'a': round(a, 3)}
                                                    for k, p, a in parts]}
                           for g, (t, parts) in ((g, ordered(g)) for g in sorted(rows))}},
              fh, indent=1, sort_keys=False)
    fh.write('\n')
print('wrote %s and %s: %d grid squares  (version %s)' % (OUT, os.path.basename(OUTJ), len(rows), VERSION))

for p in UNPACKED:                  # leave only the zip behind, as make_dxcc_grids.py does
    if os.path.exists(p): os.remove(p)
