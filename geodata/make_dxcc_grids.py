#!/usr/bin/env python3
"""Which 4-character Maidenhead grid squares does each DXCC entity occupy?

Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software:
you may redistribute it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 or (at your option) any later version.
It is distributed WITHOUT ANY WARRANTY.  The TABLES it writes carry their own note on rights.

    python3 tools/make_dxcc_grids.py

Writes geodata/dxcc_grids.txt and .json: one entry per entity, keyed by the cty.dat primary prefix,
listing the squares it occupies.  The plausibility test a caller wants is then a hash lookup - "is
the grid this station claims one of its entity's squares (or a neighbour of one)?" - with no
geometry at run time.

Two sources, both public domain (SESSION_RULES.md rule 6b):
  - entity list and coordinates: geodata/cty.dat (MIT), the copy the tables were built from
  - borders: Natural Earth 1:10m admin_0_map_units, kept in geodata/
    https://naciscdn.org/naturalearth/10m/cultural/ne_10m_admin_0_map_units.zip

WHERE IT IS APPROXIMATE, and it says so per entity:
  - 'poly'  the entity matched a Natural Earth map unit; its squares come from the polygon.
  - 'sovereign' only the SOVEREIGNT field matched, so the row is the whole sovereign state including
            its dependencies - wide, and weak as a test.  Reported at the end of a run.
  - 'admin1' it is a subdivision that is a DXCC entity of its own (the Canaries, Sardinia, Sicily,
            Kaliningrad, Svalbard ...), cut from Natural Earth's admin-1 layer once by
            tools/extract_admin1_subset.py into a small file kept here.
  - 'islands' it is none of those, but Natural Earth's minor-islands layer has island polygons within
            ISLAND_KM of its cty.dat coordinate; its squares come from those.  The layer carries no
            names, so the match is geographic - which is why the radius is kept modest.
  - 'point' neither (a scattered archipelago, a subdivision such as the Canaries, or one of the
            single-building entities).  Its squares come from the cty.dat coordinate and everything
            within POINT_KM of it.
  - A matched PARENT keeps its whole polygon, so Spain still lists the Canary squares that belong to
    EA8.  For a plausibility test that errs on the permissive side, which is the safe direction; a
    later pass can subtract the children.
"""
import os, sys, json, math, re, hashlib, unicodedata, shapefile
from datetime import date, datetime, timezone
from shapely.geometry import shape, box, Point
from shapely.ops import unary_union
from shapely.prepared import prep
from shapely.strtree import STRtree

HERE = os.path.dirname(os.path.abspath(__file__))
# Run from tools/ in the work tree, or from inside a geodata/ folder that carries the inputs.
DATA = os.path.normpath(HERE if os.path.exists(os.path.join(HERE, 'cty.dat'))
                        else os.path.join(HERE, '..', 'geodata'))
NE   = os.path.join(DATA, 'ne_10m_admin_0_map_units')
MINOR = os.path.join(DATA, 'ne_10m_minor_islands')      # 2795 unnamed island polygons
ADM1  = os.path.join(DATA, 'dxcc_admin1_subset.json')   # the subdivisions that are entities of their own
OUT  = os.path.join(DATA, 'dxcc_grids.txt')
OUTJ = os.path.join(DATA, 'dxcc_grids.json')
POINT_KM     = 120.0     # radius around a cty.dat coordinate when nothing else is found
ISLAND_KM    = 200.0     # how far from that coordinate to accept minor-island polygons
# A square counts if the entity touches it at all: for a plausibility test being permissive is the
# safe direction, and an area threshold silently loses the tiny entities (Spratly, the reefs).
sys.dont_write_bytecode = True          # no __pycache__ beside the sources, in either tree
sys.path.insert(0, HERE if os.path.exists(os.path.join(HERE, 'cty.py'))
                else os.path.join(HERE, '..', 'test', 'experiments', 'false_decodes'))
from cty import load_cty, cty_path                         # the resolver already in the tree

BUILT_BY = ('built by tools/make_dxcc_grids.py of JTDX_CONTEST, Tihomir Sokcevic CE3TSK '
            '(the script is GPL v3)')
CLAIM = ('derived from public-domain sources (cty.dat, Natural Earth) and stating facts about '
         'geography; no rights of any kind are claimed over it, and it may be used freely.')
# cty.dat spells some entities differently from Natural Earth
ALIAS = {
    'Fed. Rep. of Germany': 'Germany', 'Timor - Leste': 'East Timor', 'Bosnia-Herzegovina':
    'Bosnia and Herzegovina', 'Dem. Rep. of the Congo': 'Democratic Republic of the Congo',
    'Republic of Korea': 'South Korea', 'DPR of Korea': 'North Korea', 'Rep. of South Africa':
    'South Africa', 'United States': 'United States of America', 'Czech Republic': 'Czechia',
    'Trinidad & Tobago': 'Trinidad and Tobago', 'Antigua & Barbuda': 'Antigua and Barbuda',
    'St. Kitts & Nevis': 'Saint Kitts and Nevis', 'St. Vincent': 'Saint Vincent and the Grenadines',
    'St. Lucia': 'Saint Lucia', 'Turks & Caicos Islands': 'Turks and Caicos Islands',
    'Sao Tome & Principe': 'São Tomé and Principe', 'Wallis & Futuna Islands': 'Wallis and Futuna',
    'Vatican City': 'Vatican', 'Burma': 'Myanmar', 'Ivory Coast': "Côte d'Ivoire",
    'Cape Verde': 'Cabo Verde', 'Swaziland': 'eSwatini', 'Turkey': 'Turkiye',
    'European Russia': 'Russia', 'Asiatic Russia': 'Russia',
    'The Gambia': 'Gambia', 'Macedonia': 'North Macedonia',
    # found by review 2026-09-24: these nine exist in Natural Earth under another name, and without
    # the alias each fell back to a 120 km disk around its cty.dat coordinate - Bratislava was not in
    # Slovakia, Izmir not in Asiatic Turkey, Kuala Lumpur not in West Malaysia.
    'Slovak Republic': 'Slovakia', 'Asiatic Turkey': 'Turkey', 'Cote d\'Ivoire': "Côte d'Ivoire",
    'Republic of Kosovo': 'Kosovo', 'Republic of South Sudan': 'South Sudan',
    'Reunion Island': 'Reunion', 'West Malaysia': 'Malaysia',
    'Western Kiribati': 'Kiribati', 'Central Kiribati': 'Kiribati', 'Eastern Kiribati': 'Kiribati',
}
# Entities that share one ADMINISTRATION.  DXCC counts them apart - that is what DXCC is for - but a
# station of one may legitimately transmit from another: a KL7 holder living in Ohio, an Asiatic-Russia
# call west of the Urals, a Spanish operator on the Canaries.  Measured over era B (2026-09-24): letting
# a grid of any entity in the group count halves the real decodes this leg would wrongly reject
# (153 -> 78) and costs 17 of 606 false detections, all of which the other conditions catch anyway.
# The table carries the group; whether to use it is the consumer's decision.
# MEASURED per group - era B (labelled truth) and the 2022-2025 archive (60 349 well-attested pairs):
#   RU  rescues 74 real decodes in era B and 34 in the archive, costs 12 false detections
#   US  rescues 53 in the archive (a KL7 holder in Ohio, a KP3 in Texas), costs 5
#   ES, PT, IT  one rescue each in the archive, no cost - the parents mostly contain their children
#   GB  rescued nothing and cost one: DROPPED.  British regional prefixes are not portable anyway -
#       a G station operating in Scotland signs GM, so the group had no basis in practice either.
GROUPS = {
    'US': ['United States', 'Alaska', 'Hawaii', 'Puerto Rico', 'US Virgin Islands', 'Guam',
           'American Samoa', 'Mariana Islands', 'Midway Island', 'Wake Island', 'Johnston Island',
           'Baker & Howland Islands', 'Palmyra & Jarvis Islands', 'Kure Island', 'Navassa Island',
           'Guantanamo Bay', 'Desecheo Island'],
    'RU': ['European Russia', 'Asiatic Russia', 'Kaliningrad'],
    'ES': ['Spain', 'Canary Islands', 'Balearic Islands', 'Ceuta & Melilla'],
    'PT': ['Portugal', 'Azores', 'Madeira Islands'],
    'IT': ['Italy', 'Sardinia', 'Sicily', 'African Italy'],
    'GR': ['Greece', 'Crete', 'Dodecanese', 'Mount Athos'],
    'JA': ['Japan', 'Ogasawara', 'Minami Torishima'],
    'DK': ['Denmark', 'Greenland', 'Faroe Islands'],
    'NO': ['Norway', 'Svalbard', 'Jan Mayen', 'Bear Island'],
    'FR': ['France', 'Corsica'],
    'NL': ['Netherlands', 'Curacao', 'Bonaire', 'Sint Maarten', 'Aruba'],
}

MULTI = {'Palestine': ['West Bank', 'Gaza']}          # entities that need more than one map unit
def wrap(lon): return ((lon + 180.0) % 360.0) - 180.0      # the antimeridian: 180 is -180
def field(lon): return chr(ord('A') + int((wrap(lon) + 180) // 20))
def gridname(lon, lat):
    return (field(lon) + chr(ord('A') + int((lat + 90) // 10))
            + str(int(((lon + 180) % 20) // 2)) + str(int(((lat + 90) % 10) // 1)))
def norm(s):
    s = unicodedata.normalize('NFKD', s)                    # fold accents, do not strip them:
    s = ''.join(c for c in s if not unicodedata.combining(c))   # "Côte d'Ivoire" -> "cote divoire"
    s = s.lower().replace('&', 'and').replace('st.', 'saint')
    return re.sub(r'[^a-z0-9 ]', '', s).strip()

# Two splits the map units cannot give us, done from data already in the tree:
#   K / KL / KH6  - the US Census state boundaries (geodata/cb_2023_us_state_20m), exact.
#   UA / UA9      - Russia cut at 60 deg E.  The DXCC line follows the Urals and the Caucasus, so
#                   this is an APPROXIMATION, and squares near the cut belong to both halves here.
US_SHP = os.path.join(DATA, 'cb_2023_us_state_20m')
def us_parts():
    if not os.path.exists(US_SHP + '.shp'):
        if not os.path.exists(US_SHP + '.zip'):
            raise SystemExit('FAILED: %s is missing - fetch\n  https://www2.census.gov/geo/tiger/'
                             'GENZ2023/shp/cb_2023_us_state_20m.zip\ninto %s  (K/KL/KH6 are cut '
                             'from it)' % (US_SHP + '.zip', DATA))
        import zipfile; zipfile.ZipFile(US_SHP + '.zip').extractall(DATA)
    rr = shapefile.Reader(US_SHP); i = [x[0] for x in rr.fields[1:]].index('STUSPS')
    by = {}
    for sr in rr.shapeRecords():
        g = shape(sr.shape.__geo_interface__)
        if not g.is_valid: g = g.buffer(0)
        by.setdefault(sr.record[i], []).append(g)
    by = {k: unary_union(v) for k, v in by.items()}
    lower48 = unary_union([g for k, g in by.items() if k not in ('AK', 'HI', 'PR')])
    return {'K': lower48, 'KL': by['AK'], 'KH6': by['HI']}

# Every input is checked HERE, before the first polygon is read: a missing 5 MB download used to be
# reported only after unpacking the Census states and 2 795 island polygons (review 2026-09-24).
CTY = cty_path(tree_only=True)          # never the running station's copy: a table must be
                                        # reproducible from the tree it ships in
for what, url in ((US_SHP + '.zip', 'https://www2.census.gov/geo/tiger/GENZ2023/shp/'
                                    'cb_2023_us_state_20m.zip   (K/KL/KH6 are cut from it)'),
                  (MINOR + '.zip', 'https://naciscdn.org/naturalearth/10m/physical/'
                                   'ne_10m_minor_islands.zip'),
                  (NE + '.zip', 'https://naciscdn.org/naturalearth/10m/cultural/'
                                'ne_10m_admin_0_map_units.zip')):
    if not os.path.exists(what) and not os.path.exists(what[:-4] + '.shp'):
        raise SystemExit('FAILED: %s is missing - fetch\n  %s\ninto %s' % (what, url, DATA))
if not os.path.exists(ADM1):
    raise SystemExit('FAILED: %s is missing - run extract_admin1_subset.py against a downloaded\n'
                     '  ne_10m_admin_1_states_provinces.  Without it seventeen entities silently\n'
                     '  shrink to a disk around their coordinate.' % ADM1)

ents, _, _ = load_cty(CTY)
prefixes = {}
for line in open(CTY, encoding='utf-8', errors='replace'):
    f = line.split(':')
    if len(f) > 8 and line[0] not in ' \t':
        prefixes[f[0].strip()] = f[7].strip()

def minor_islands():
    if not os.path.exists(MINOR + '.shp'):
        import zipfile
        if not os.path.exists(MINOR + '.zip'):
            raise SystemExit('FAILED: %s.zip is missing - fetch it from\n  https://naciscdn.org/'
                             'naturalearth/10m/physical/ne_10m_minor_islands.zip' % MINOR)
        zipfile.ZipFile(MINOR + '.zip').extractall(DATA)
    geoms = []
    for sh in shapefile.Reader(MINOR).shapes():
        g = shape(sh.__geo_interface__)
        geoms.append(g if g.is_valid else g.buffer(0))
    return geoms, STRtree(geoms)

SPECIAL = us_parts()
ISLANDS, ITREE = minor_islands()
ADMIN1 = {}
if os.path.exists(ADM1):
    for pfx, v in json.load(open(ADM1))['entities'].items():
        g = shape(v['geometry']); ADMIN1[pfx] = g if g.is_valid else g.buffer(0)
    print('admin-1 subset: %d entities' % len(ADMIN1))
else:
    raise SystemExit('FAILED: %s is missing - run tools/extract_admin1_subset.py against a downloaded\n'
                     '  ne_10m_admin_1_states_provinces.  Without it seventeen entities silently\n'
                     '  shrink to a disk around their coordinate.' % ADM1)
_ru = None
if not os.path.exists(NE + '.shp'):                    # kept zipped in the tree: 5 MB instead of 10
    import zipfile
    if not os.path.exists(NE + '.zip'):
        raise SystemExit('FAILED: %s.zip is missing - fetch it from\n  https://naciscdn.org/'
                         'naturalearth/10m/cultural/ne_10m_admin_0_map_units.zip' % NE)
    zipfile.ZipFile(NE + '.zip').extractall(DATA)
r = shapefile.Reader(NE); flds = [x[0] for x in r.fields[1:]]
# Key by the UNIT-level names only.  SOVEREIGNT groups every dependency under its sovereign state, so
# keying by it made 'france' the union of France, French Guiana, Reunion, New Caledonia, French
# Polynesia, Mayotte and Clipperton - 140 squares in four oceans - and 'denmark' include Greenland
# (review 2026-09-24).  The sovereign names are kept apart and used only if nothing else matches, and
# an entity that lands there is marked 'sovereign' in the table, because that row is the whole state.
units, sovereign = {}, {}
for sr in r.shapeRecords():
    g = shape(sr.shape.__geo_interface__)
    if not g.is_valid: g = g.buffer(0)
    for k in ('NAME', 'NAME_LONG', 'GEOUNIT', 'SUBUNIT'):
        units.setdefault(norm(sr.record[flds.index(k)]), []).append(g)
    sovereign.setdefault(norm(sr.record[flds.index('SOVEREIGNT')]), []).append(g)
units = {k: unary_union(v) for k, v in units.items()}
sovereign = {k: unary_union(v) for k, v in sovereign.items()}
_ru = units.get(norm('Russia'))
if _ru is not None:                                   # the 60 deg E approximation, see the note above
    # The cut is in the EASTERN hemisphere only.  Chukotka and Wrangel lie at -180..-169, so a plain
    # "lon < 60" window put them in EUROPEAN Russia (review 2026-09-24): every Bering-Strait square
    # was attributed to UA and UA9 had none.
    SPECIAL['UA']  = _ru.intersection(box(19, -90, 60, 90))
    SPECIAL['UA9'] = _ru.intersection(unary_union([box(60, -90, 180, 90), box(-180, -90, -160, 90)]))
print('Natural Earth: %d name keys' % len(units))

rows, source, group = {}, {}, {}
GROUP_OF = {n: g for g, names in GROUPS.items() for n in names}
for name, cont, lat, lon in ents:
    pfx = prefixes.get(name)
    if pfx is None: raise SystemExit('FAILED: no primary prefix for %r' % name)
    key = norm(ALIAS.get(name, name))
    squares = set()
    geom = None
    if pfx in SPECIAL:
        geom = SPECIAL[pfx]
    elif pfx in ADMIN1:
        geom = ADMIN1[pfx]; source[pfx] = 'admin1'
    elif name in MULTI and all(norm(u) in units for u in MULTI[name]):
        geom = unary_union([units[norm(u)] for u in MULTI[name]])
    elif key in units:
        geom = units[key]
    elif key in sovereign:
        geom = sovereign[key]; source[pfx] = 'sovereign'
    if geom is None:                               # try the minor-island polygons
        pt = Point(lon, lat)
        kmlon = max(10.0, 111.0 * math.cos(math.radians(lat)))     # a degree of longitude, in km
        near = [ISLANDS[int(i)] for i in ITREE.query(pt.buffer(ISLAND_KM / min(111.0, kmlon)))]
        near = [g for g in near
                if math.hypot((g.distance(pt) if g.distance(pt) == 0 else
                               abs(g.centroid.x - lon) * kmlon), abs(g.centroid.y - lat) * 111.0)
                <= ISLAND_KM or g.distance(pt) * 111.0 <= ISLAND_KM]
        if near:
            geom = unary_union(near); source[pfx] = 'islands'
            squares.add(gridname(lon, lat))      # the layer has no names: never lose our own square
    if geom is not None:
        pg = prep(geom)
        minx, miny, maxx, maxy = geom.bounds
        x = (int((minx + 180) // 2) * 2) - 180
        while x <= maxx:
            y = int(miny // 1)
            while y <= maxy:
                if x >= 180: y += 1; continue           # the wrap-around duplicate of -180
                cell = box(x, y, x + 2, y + 1)
                if pg.intersects(cell):
                    squares.add(gridname(x + 1, y + 0.5))
                y += 1
            x += 2
        p = geom.representative_point()          # never lose an entity smaller than a square
        squares.add(gridname(p.x, p.y))
        source.setdefault(pfx, 'split' if pfx in SPECIAL else 'poly')
    else:                                            # the cty.dat coordinate and its surroundings
        dlat = POINT_KM / 111.0
        dlon = POINT_KM / max(10.0, 111.0 * math.cos(math.radians(lat)))
        x = (int((lon - dlon + 180) // 2) * 2) - 180
        while x <= lon + dlon:
            y = int((lat - dlat) // 1)
            while y <= lat + dlat:
                cx, cy = max(x, min(lon, x + 2)), max(y, min(lat, y + 1))   # nearest point of the square
                dx = (cx - lon) * 111.0 * math.cos(math.radians(lat)); dy = (cy - lat) * 111.0
                if math.hypot(dx, dy) <= POINT_KM:
                    squares.add(gridname(x + 1, y + 0.5))
                y += 1
            x += 2
        squares.add(gridname(lon, lat))
        source[pfx] = 'point'
    if not squares: raise SystemExit('FAILED: no grid square for %s (%s)' % (pfx, name))
    rows[pfx] = (name, sorted(squares))
    group[pfx] = GROUP_OF.get(name, '')
import collections as _c
print('entities %d: %s' % (len(rows), dict(_c.Counter(source.values()))))
_sov = [p for p, v in source.items() if v == 'sovereign']
if _sov: print('  NOTE sovereign-wide rows (state + dependencies): %s' % ' '.join(sorted(_sov)))

BUILT   = date.today().isoformat()
VERSION = datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S')   # the table's own version stamp
HEAD = ('# DXCC entity -> the 4-character Maidenhead grid squares it occupies.\n'
        '#   <primary prefix>  <source>  <group>  <squares>  <entity name>\n'
        '#   group: entities under one administration (US, RU, ES, ...) - DXCC counts them apart, but\n'
        '#   a station of one may transmit from another; "-" means the entity is in no group.\n'
        '#   source: poly = a Natural Earth map unit; sovereign = only the sovereign name matched, so\n'
        '#   the row covers that state AND its dependencies; admin1 = a subdivision cut from the admin-1\n'
        '#   layer (dxcc_admin1_subset.json); islands = minor-island polygons within %d km of\n'
        '#   the cty.dat coordinate (that layer has no names, so the match is geographic); point = the\n'
        '#   coordinate itself and everything within %d km of it.\n'
        '#   split = cut from a finer source: K/KL/KH6 from the US Census state boundaries (exact),\n'
        '#   UA/UA9 by cutting Russia at 60 deg E (approximate - the DXCC line follows the Urals).\n'
        '#   A matched parent keeps its whole polygon, so Spain still lists the Canary squares of EA8:\n'
        '#   the table errs on the permissive side, which is the safe direction for a plausibility test.\n'
        '#\n'
        '# entities: cty.dat (MIT) - borders: Natural Earth 1:10m admin_0_map_units (public domain)\n'
        '#   https://naciscdn.org/naturalearth/10m/cultural/ne_10m_admin_0_map_units.zip\n'
        '# a square is listed when the entity touches it at all - permissive by design\n'
        '#\n'
        '# version %s (built %s), %s.\n'
        '# This table is %s\n'
        '# See geodata/README.md.  A JSON twin of this table is dxcc_grids.json.\n'
        % (ISLAND_KM, POINT_KM, VERSION, BUILT, BUILT_BY, CLAIM))
body = ''.join('%-8s %-9s %-3s %4d  %-28s %s\n'
               % (pfx, source[pfx], group[pfx] or '-', len(rows[pfx][1]), rows[pfx][0],
                  ' '.join(rows[pfx][1])) for pfx in sorted(rows))
CONTENT = hashlib.sha256(body.encode()).hexdigest()[:12]      # the data itself, header excluded:
with open(OUT, 'w') as fh:                                    # two builds of the same inputs match
    fh.write(HEAD + '# content %s - the sha256 of the rows below, so a rebuild can be compared\n' % CONTENT)
    fh.write(body)
with open(OUTJ, 'w') as fh:
    json.dump({'version': VERSION, 'content': CONTENT, 'built': BUILT,
               'tool': 'tools/make_dxcc_grids.py', 'project': 'JTDX_CONTEST',
               'author': 'Tihomir Sokcevic CE3TSK', 'rights': 'This table is ' + CLAIM,
               'tool_licence': 'GPL v3', 'point_km': POINT_KM, 'island_km': ISLAND_KM,
               'groups': {g: sorted(p for p in rows if group[p] == g) for g in sorted(GROUPS)},
               'entities': {p: {'name': rows[p][0], 'source': source[p], 'group': group[p],
                                'grids': rows[p][1]}
                            for p in sorted(rows)}}, fh, indent=1)
    fh.write('\n')
import glob                                          # leave only the zips behind: EVERY member of
for base in (NE, MINOR, US_SHP):                       # each one, not a hand-written list of
    if not os.path.exists(base + '.zip'): continue     # extensions (the Census zip also carries
    for p in glob.glob(base + '.*'):                   # two .xml files, which used to survive)
        if not p.endswith('.zip'): os.remove(p)
print('wrote %s and %s  (version %s, content %s)' % (OUT, os.path.basename(OUTJ), VERSION, CONTENT))
