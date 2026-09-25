#!/usr/bin/env python3
"""Cut the few Natural Earth admin-1 units that are DXCC entities of their own into a small file.

Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  GPL v3 or later, WITHOUT ANY
WARRANTY.  The file it writes carries its own note on rights.

    python3 tools/extract_admin1_subset.py [path/to/ne_10m_admin_1_states_provinces]

Natural Earth's admin-1 layer is 15 MB - too much to keep in the tree for the fifteen entities that
need it (the Canaries, Madeira, Sardinia, Sicily, Kaliningrad, Svalbard ...).  This runs ONCE against
a downloaded copy and leaves behind geodata/dxcc_admin1_subset.json, a few tens of KB, which
make_dxcc_grids.py then uses offline like the other layers.

    https://naciscdn.org/naturalearth/10m/cultural/ne_10m_admin_1_states_provinces.zip

NOTE on Sardinia: this Natural Earth release still carries the PRE-2016 Italian provinces
(Carbonia-Iglesias, Medio Campidano, Ogliastra, Olbia-Tempio) and has no Oristano or Sud Sardegna, so
the union is about 21 000 km2 against the island's 24 090.  No grid square is lost by it - the
neighbouring provinces already reach every square - but a finer release would be better.

MAPPING is by hand and cannot be otherwise: DXCC entities are not administrative units, and the
layer's names are local (Kriti for Crete, Notio Aigaio for the Dodecanese, nine provinces for
Sicily).  A name that matches nothing stops the run rather than silently shrinking an entity.
"""
import os, sys, json, shapefile
from datetime import date
from shapely.geometry import shape, mapping
from shapely.ops import unary_union

HERE = os.path.dirname(os.path.abspath(__file__))
# Run from tools/ in the work tree, or from inside a geodata/ folder that carries the layer.
LAYER = 'ne_10m_admin_1_states_provinces'
DATA = os.path.normpath(HERE if any(os.path.exists(os.path.join(HERE, LAYER + e))
                                    for e in ('.shp', '.zip'))
                        else os.path.join(HERE, '..', 'geodata'))
OUT  = os.path.join(DATA, 'dxcc_admin1_subset.json')
SIMPLIFY = 0.01          # degrees, ~1 km: far finer than the 2 x 1 degree grid squares need

# cty.dat primary prefix -> (entity name, admin, [admin-1 'name' values])
WANT = {
    'EA8':   ('Canary Islands',   'Spain',          ['Las Palmas', 'Santa Cruz de Tenerife']),
    'EA6':   ('Balearic Islands', 'Spain',          ['Baleares']),
    'EA9':   ('Ceuta & Melilla',  'Spain',          ['Ceuta', 'Melilla']),
    'CT3':   ('Madeira Islands',  'Portugal',       ['Madeira']),
    'CU':    ('Azores',           'Portugal',       ['Azores']),
    '*IT9':  ('Sicily',           'Italy',          ['Agrigento', 'Caltanissetta', 'Catania', 'Enna',
                                                     'Messina', 'Palermo', 'Ragusa', 'Siracusa', 'Trapani']),
    'IS':    ('Sardinia',         'Italy',          ['Cagliari', 'Carbonia-Iglesias', 'Medio Campidano',
                                                     'Nuoro', 'Ogliastra', 'Olbia-Tempio', 'Sassari']),
    'TK':    ('Corsica',          'France',         ['Corse-du-Sud', 'Haute-Corse']),
    'SV9':   ('Crete',            'Greece',         ['Kriti']),
    'SV5':   ('Dodecanese',       'Greece',         ['Notio Aigaio']),   # includes the Cyclades: permissive
    'SV/a':  ('Mount Athos',      'Greece',         ['Ayion Oros']),
    'UA2':   ('Kaliningrad',      'Russia',         ['Kaliningrad']),
    'JW':    ('Svalbard',         'Norway',         ['Svalbard']),
    '*GM/s': ('Shetland Islands', 'United Kingdom', ['Shetland Islands']),
    '*TA1':  ('European Turkey',  'Turkey',         ['Edirne', 'Istanbul', 'Kirklareli', 'Tekirdag',
                                                     'Çanakkale']),
    '9M6':   ('East Malaysia',    'Malaysia',       ['Sabah', 'Sarawak', 'Labuan']),
    'HC8':   ('Galapagos Islands','Ecuador',        ['Galápagos']),
}
SHP = sys.argv[1] if len(sys.argv) > 1 else os.path.join(DATA, LAYER)
if not os.path.exists(SHP + '.shp') and os.path.exists(SHP + '.zip'):
    import zipfile                  # the probe accepts the zip, so this must be able to open it
    zipfile.ZipFile(SHP + '.zip').extractall(os.path.dirname(SHP) or '.')
if not os.path.exists(SHP + '.shp'):
    raise SystemExit('FAILED: %s.shp not found.  Download and unpack\n  https://naciscdn.org/'
                     'naturalearth/10m/cultural/ne_10m_admin_1_states_provinces.zip\n'
                     'and pass its path; it is NOT kept in the tree (15 MB).' % SHP)
r = shapefile.Reader(SHP); f = [x[0] for x in r.fields[1:]]
i_name, i_adm = f.index('name'), f.index('admin')
parts, seen = {}, {}
for sr in r.shapeRecords():
    key = (sr.record[i_adm], sr.record[i_name])
    for pfx, (ent, adm, names) in WANT.items():
        if key[0] == adm and key[1] in names:
            g = shape(sr.shape.__geo_interface__)
            parts.setdefault(pfx, []).append(g if g.is_valid else g.buffer(0))
            seen.setdefault(pfx, set()).add(key[1])
out = {}
for pfx, (ent, adm, names) in WANT.items():
    # Count NAMES matched, not shapes: an entity of nine provinces can return thirteen shapes and
    # still be missing one (review 2026-09-24).  Any unmatched name stops the run.
    missing = [n for n in names if n not in seen.get(pfx, ())]
    if missing:
        raise SystemExit('FAILED: %s (%s) - these admin-1 names matched nothing in %s: %s\n'
                         '  Natural Earth may have renamed or merged them; fix WANT and re-run.'
                         % (pfx, ent, adm, ', '.join(missing)))
    got = len(seen[pfx])
    geom = unary_union(parts[pfx]).simplify(SIMPLIFY, preserve_topology=True)
    out[pfx] = {'name': ent, 'admin': adm, 'units': names, 'geometry': mapping(geom)}
    print('  %-6s %-18s %d/%d names, %d shapes, %d bytes' % (pfx, ent, got, len(names),
          len(parts[pfx]), len(json.dumps(mapping(geom)))))
json.dump({'built': date.today().isoformat(), 'tool': 'tools/extract_admin1_subset.py',
           'project': 'JTDX_CONTEST', 'author': 'Tihomir Sokcevic CE3TSK', 'tool_licence': 'GPL v3',
           'source': {'layer': 'Natural Earth 1:10m admin_1_states_provinces',
                      'url': 'https://naciscdn.org/naturalearth/10m/cultural/'
                             'ne_10m_admin_1_states_provinces.zip',
                      'licence': 'public domain'},
           'simplify_degrees': SIMPLIFY,
           'rights': 'Derived from public-domain data and stating facts about geography; no rights of '
                     'any kind are claimed over it, and it may be used freely.',
           'entities': out}, open(OUT, 'w'), indent=1)
print('wrote %s (%d entities, %.0f KB)' % (OUT, len(out), os.path.getsize(OUT) / 1024))
