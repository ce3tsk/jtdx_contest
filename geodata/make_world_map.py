#!/usr/bin/env python3
"""The world map the Reception map window draws: land, borders and lakes as polygons, in one small file.

Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software:
you may redistribute it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 or (at your option) any later version.
It is distributed WITHOUT ANY WARRANTY.  The FILE it writes carries its own note on rights.

    python3 tools/make_world_map.py [--out FILE]

Writes geodata/worldmap.bin (RECEPTION_MAP_PLAN.md 5.2, TODO.md 3.-260).  The program carries a copy as
the resource ":/worldmap.bin" (geodata/worldmap.bin in each program tree) and draws it with QPainter -
no map library, nothing fetched at run time.

The source, open (SESSION_RULES.md rule 6b) - Natural Earth, public domain, naturalearthdata.com:
  - level 0, the world view:  ne_50m_admin_0_map_units  (1:50m)
        https://naciscdn.org/naturalearth/50m/cultural/ne_50m_admin_0_map_units.zip
  - level 1, zoomed in:       ne_10m_admin_0_map_units  (1:10m) and ne_10m_minor_islands
        https://naciscdn.org/naturalearth/10m/cultural/ne_10m_admin_0_map_units.zip
        https://naciscdn.org/naturalearth/10m/physical/ne_10m_minor_islands.zip
  - level 2, the lakes:       ne_50m_lakes
        https://naciscdn.org/naturalearth/50m/physical/ne_50m_lakes.zip
MAP UNITS, not countries: they cut the United Kingdom into England, Scotland, Wales and Northern Ireland,
as DXCC does, and they are the layer the DXCC tables are built from (make_dxcc_grids.py).

Nothing is generalised here: each level is Natural Earth's own, only rounded to 1e-4 degree (11 m).

THE FILE, little endian:
    "JTDXWMAP"  u32 version (1)
    u16 n, n bytes     the note on source and rights, UTF-8
    u32 names          then each: u16 n, n bytes UTF-8 - Natural Earth's GEOUNIT of a map unit
    u32 layers         then each:
        u8 kind (0 land 1:50m, 1 land 1:10m, 2 lakes)  u32 polygons  u32 rings  u32 points  u32 bytes
        and `bytes` bytes of polygons, each:
            i32 minLon minLat maxLon maxLat   (1e-4 degree)
            u32 name                          (index into the names, 0xFFFFFFFF for none)
            varint rings, and each ring:      (the first is the outline, the others its holes)
                varint points, then per point two zigzag varints: the step in longitude and in latitude from
                the point before - the ring's first point from (minLon, minLat)
A ring is not closed in the file (its last point is not its first again).  A polygon's name is its map unit's
GEOUNIT, as Natural Earth has it; which DXCC entity that is comes with the plan's stage 4, where it is used.
"""
import os, sys, struct, hashlib

HERE = os.path.dirname(os.path.abspath(__file__))
# Run from tools/ in the work tree, or from inside a geodata/ folder that carries the inputs.
DATA = os.path.normpath(HERE if os.path.exists(os.path.join(HERE, 'ne_50m_admin_0_map_units.zip'))
                        else os.path.join(HERE, '..', 'geodata'))
NE = 'https://naciscdn.org/naturalearth/'
INPUTS = (('ne_50m_admin_0_map_units.zip', NE + '50m/cultural/ne_50m_admin_0_map_units.zip'),
          ('ne_10m_admin_0_map_units.zip', NE + '10m/cultural/ne_10m_admin_0_map_units.zip'),
          ('ne_10m_minor_islands.zip',     NE + '10m/physical/ne_10m_minor_islands.zip'),
          ('ne_50m_lakes.zip',             NE + '50m/physical/ne_50m_lakes.zip'))
LAYERS = ((0, (('ne_50m_admin_0_map_units.zip', 'GEOUNIT'),)),
          (1, (('ne_10m_admin_0_map_units.zip', 'GEOUNIT'), ('ne_10m_minor_islands.zip', None))),
          (2, (('ne_50m_lakes.zip', None),)))
MAGIC, VERSION, NONE = b'JTDXWMAP', 1, 0xFFFFFFFF
SCALE = 10000                                  # 1e-4 degree
NOTE = ('Natural Earth (naturalearthdata.com), public domain: ne_50m_admin_0_map_units, ne_10m_admin_0_map_units, '
        'ne_10m_minor_islands, ne_50m_lakes, each as published, rounded to 1e-4 degree. Built by tools/make_world_map.py '
        'of JTDX_CONTEST, Tihomir Sokcevic CE3TSK (the script is GPL v3); no rights of its own are claimed over this file.')


def fail(text):
    sys.exit('FAILED: ' + text)


def varint(n):
    out = bytearray()
    while True:
        b = n & 0x7F
        n >>= 7
        if n:
            out.append(b | 0x80)
        else:
            out.append(b)
            return bytes(out)


def zigzag(n):
    return (n << 1) ^ (n >> 63)


def area2(ring):
    """twice the signed area: negative for a clockwise ring, a shapefile's outline"""
    s = 0
    for (x1, y1), (x2, y2) in zip(ring, ring[1:] + ring[:1]):
        s += x1 * y2 - x2 * y1
    return s


def inside(pt, ring):
    x, y = pt
    hit = False
    for (x1, y1), (x2, y2) in zip(ring, ring[1:] + ring[:1]):
        if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1) + x1:
            hit = not hit
    return hit


def rounded(points):
    """the ring at 1e-4 degree, no point twice in a row, not closed"""
    out = []
    for lon, lat in points:
        q = (int(round(lon * SCALE)), int(round(lat * SCALE)))
        if not (-180 * SCALE <= q[0] <= 180 * SCALE and -90 * SCALE <= q[1] <= 90 * SCALE):
            fail('a point outside the world: %r' % ((lon, lat),))
        if not out or out[-1] != q:
            out.append(q)
    while len(out) > 1 and out[-1] == out[0]:
        out.pop()
    return out


def polygons_of(shape):
    """a shape's rings as polygons: each outline with its holes - a hole goes to the smallest outline around it"""
    edges = list(shape.parts) + [len(shape.points)]
    outlines, holes = [], []
    for a, b in zip(edges, edges[1:]):
        ring = rounded(shape.points[a:b])
        if len(ring) < 3:
            polygons_of.dropped += 1
            continue
        (outlines if area2(ring) < 0 else holes).append(ring)
    polys = [[o] for o in outlines]
    boxes = [(min(p[0] for p in o), min(p[1] for p in o), max(p[0] for p in o), max(p[1] for p in o)) for o in outlines]
    for h in holes:
        best = None
        for i, (x1, y1, x2, y2) in enumerate(boxes):
            if x1 <= h[0][0] <= x2 and y1 <= h[0][1] <= y2 and inside(h[0], outlines[i]):
                if best is None or (x2 - x1) * (y2 - y1) < (boxes[best][2] - boxes[best][0]) * (boxes[best][3] - boxes[best][1]):
                    best = i
        if best is None:
            fail('a hole with no outline around it (%d points, first %r)' % (len(h), h[0]))
        polys[best].append(h)
    return polys
polygons_of.dropped = 0


def build(out_path):
    try:
        import shapefile
    except ImportError:
        fail('the pyshp module is needed (python3-pyshp)')
    missing = [(f, url) for f, url in INPUTS if not os.path.exists(os.path.join(DATA, f))]
    if missing:                                 # every input before any work - and loudly
        fail('missing in %s:\n' % DATA + '\n'.join('  %s   from %s' % m for m in missing))

    names, index = [], {}
    layers = []
    for kind, sources in LAYERS:
        body, npoly, nring, npoint = bytearray(), 0, 0, 0
        for fname, field in sources:
            reader = shapefile.Reader(os.path.join(DATA, fname))
            if reader.shapeType != 5:
                fail('%s holds no polygons (shape type %d)' % (fname, reader.shapeType))
            for sr in reader.iterShapeRecords():
                tag = NONE
                if field:
                    name = sr.record[field]
                    if not name:
                        fail('%s: a map unit without %s' % (fname, field))
                    if name not in index:
                        index[name] = len(names)
                        names.append(name)
                    tag = index[name]
                for poly in polygons_of(sr.shape):
                    xs = [p[0] for p in poly[0]]
                    ys = [p[1] for p in poly[0]]
                    box = (min(xs), min(ys), max(xs), max(ys))
                    body += struct.pack('<iiiiI', box[0], box[1], box[2], box[3], tag)
                    body += varint(len(poly))
                    for ring in poly:
                        body += varint(len(ring))
                        px, py = box[0], box[1]
                        for x, y in ring:
                            body += varint(zigzag(x - px)) + varint(zigzag(y - py))
                            px, py = x, y
                        nring += 1
                        npoint += len(ring)
                    npoly += 1
        layers.append((kind, npoly, nring, npoint, bytes(body)))

    note = NOTE.encode('utf-8')
    data = bytearray(MAGIC + struct.pack('<I', VERSION) + struct.pack('<H', len(note)) + note)
    data += struct.pack('<I', len(names))
    for n in names:
        b = n.encode('utf-8')
        data += struct.pack('<H', len(b)) + b
    data += struct.pack('<I', len(layers))
    for kind, npoly, nring, npoint, body in layers:
        data += struct.pack('<BIIII', kind, npoly, nring, npoint, len(body)) + body
    with open(out_path, 'wb') as f:
        f.write(data)

    for kind, npoly, nring, npoint, body in layers:
        print('layer %d: %6d polygons %6d rings %7d points %8d bytes' % (kind, npoly, nring, npoint, len(body)))
    print('names %d   rings dropped for fewer than 3 points: %d' % (len(names), polygons_of.dropped))
    print('%s  %d bytes  sha256 %s' % (out_path, len(data), hashlib.sha256(data).hexdigest()))


if __name__ == '__main__':
    args = sys.argv[1:]
    out = os.path.join(DATA, 'worldmap.bin')
    if args[:1] == ['--out'] and len(args) == 2:
        out = args[1]
    elif args:
        fail('usage: make_world_map.py [--out FILE]')
    build(out)
