#!/usr/bin/env python3
"""Do the compiled geodata tables say exactly what geodata/*.json says?

    python3 test/geodata_check.py          # from the work tree
    python3 geodata_check.py               # from a geodata/ folder in either program tree

Compiles the geodata directory with the little dump program beside it and compares, entity by
entity and square by square: the sources, every grid square, every population figure and share, and
the version/content stamps.  Also exercises the lookup API itself - packing round-trips, the
neighbour set at the antimeridian and the poles, and a handful of real call/grid pairs.

Part of JTDX_CONTEST, Tihomir Sokcevic CE3TSK, GPL v3 or later.
"""
import json, os, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
if os.path.exists(os.path.join(HERE, 'geodata.h')):
    GEO = CXX = HERE                   # run from inside a geodata/ folder: json and C++ are both here
else:                                  # run from the work tree
    ROOT = os.path.dirname(HERE)
    GEO  = os.path.join(ROOT, 'geodata')
    CXX  = os.path.join(ROOT, 'jtdx_contest', 'geodata')
fail = []

def check(cond, what):
    if not cond: fail.append(what)

with tempfile.TemporaryDirectory() as tmp:
    exe = os.path.join(tmp, 'geodata_dump')
    cmd = ['g++', '-O1', '-std=c++11', '-Wall', '-Wextra', '-Werror', '-o', exe] + \
          [os.path.join(CXX, f) for f in ('geodata_dump.cpp', 'geodata.cpp',
                                          'dxcc_grids_data.cpp', 'grid_states_data.cpp')]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode: sys.exit('FAILED to compile:\n' + r.stderr[:4000])
    out = subprocess.run([exe], capture_output=True, text=True).stdout.splitlines()

dx = json.load(open(os.path.join(GEO, 'dxcc_grids.json')))
gs = json.load(open(os.path.join(GEO, 'grid_states.json')))
head = dict(l.split(' ', 1) for l in out[:2])
check(head['dxcc'] == '%s %s' % (dx['version'], dx['content']), 'dxcc version/content stamp')
check(head['states'] == '%s %s' % (gs['version'], gs['content']), 'states version/content stamp')

ents, squares = {}, {}
for l in out[2:]:
    t = l.split()
    if t[0] == 'E': ents[t[1]] = (t[2], int(t[3]), t[4:])
    elif t[0] == 'S': squares[t[1]] = (int(t[2]), t[3:])

check(len(ents) == len(dx['entities']), 'entity count %d vs %d' % (len(ents), len(dx['entities'])))
for p, v in dx['entities'].items():
    c = ents.get(p)
    if c is None: fail.append('entity %s missing from the compiled table' % p); continue
    check(c[0] == v['source'], '%s source %s vs %s' % (p, c[0], v['source']))
    check(c[1] == len(v['grids']), '%s count %d vs %d' % (p, c[1], len(v['grids'])))
    check(c[2] == sorted(v['grids']), '%s squares differ' % p)

check(len(squares) == len(gs['squares']), 'square count')
for g, v in gs['squares'].items():
    c = squares.get(g)
    if c is None: fail.append('square %s missing' % g); continue
    check(c[0] == v['pop'], '%s population %d vs %d' % (g, c[0], v['pop']))
    got = [(x.split(':')[0], float(x.split(':')[1]), float(x.split(':')[2])) for x in c[1]]
    want = [(e['s'], e['p'], e['a']) for e in v['states']]
    check([x[0] for x in got] == [x[0] for x in want], '%s state order' % g)
    for (cs, cp, ca), (ws, wp, wa) in zip(got, want):
        check(abs(cp - wp) <= 0.004 and abs(ca - wa) <= 0.004,
              '%s %s shares %.3f/%.3f vs %.3f/%.3f' % (g, cs, cp, ca, wp, wa))

# ---- the API itself: packing, the antimeridian, the poles, and real call/grid pairs ----------
with tempfile.TemporaryDirectory() as tmp:
    exe = os.path.join(tmp, 'selftest')
    r = subprocess.run(['g++', '-O1', '-std=c++11', '-Wall', '-Wextra', '-Werror', '-o', exe]
                       + [os.path.join(CXX, f) for f in ('geodata_selftest.cpp', 'geodata.cpp',
                          'dxcc_grids_data.cpp', 'grid_states_data.cpp')],
                       capture_output=True, text=True)
    if r.returncode: sys.exit('FAILED to compile the self-test:\n' + r.stderr[:3000])
    run = subprocess.run([exe], capture_output=True, text=True)
    if run.returncode:              # a crashing or truncated self-test used to read as "all match"
        sys.exit('FAILED: the self-test exited %d\n%s' % (run.returncode, run.stderr[:2000]))
    api = run.stdout.splitlines()

seen = {}
for line in api:
    t = line.split()
    if t[0] == 'ROUNDTRIP': fail.append('pack/unpack round-trip broken: ' + line)
    elif t[0] == 'roundtrip': pass
    elif t[0] == 'bad': check(t[1:] == ['1', '1', '1', '1'], 'packGrid accepts/rejects: ' + line)
    elif t[0] == 'corner':
        # AA00 is the south-west corner: longitude wraps into the R field, latitude stops at the
        # pole, so six squares including itself - RA90 RA91 AA00 AA01 AA10 AA11.
        check(t[1] == '6' and 'RA90' in t[2:] and 'AA11' in t[2:] and
              not any(x[1] < 'A' for x in t[2:]), 'AA00 neighbours: ' + line)
    elif t[0] == 'wrap':
        check(t[1] == '9' and any(x[0] == 'A' for x in t[2:]) and 'RJ95' in t[2:],
              'RJ95 neighbours must wrap into the A field: ' + line)
    elif t[0] == 'fit':
        check((t[3] == '1') == (t[5] == '1'),
              'gridFitsEntity %s %s -> %s, wanted %s' % (t[1], t[2], t[3], t[5]))
    elif t[0] == 'case':
        # the same entity however it is spelled, a real index, and the entity actually asked for -
        # two spellings agreeing on the WRONG entity would otherwise pass
        check(t[2] == t[3] and t[2] != '-1' and t[4].lower() == t[1].lower(),
              'case-insensitive entityIndex: ' + line)
    elif t[0] == 'case-unknown':
        check(t[1:] == ['-1', '-1'], 'an unknown prefix must stay unknown: ' + line)
    elif t[0] == 'admin':
        # with sameAdmin the group's squares count, without it only the entity's own
        check(t[3] == '1' and t[4] == '0',
              'administration group: %s %s should be accepted only with sameAdmin: %s' % (t[1], t[2], line))
    elif t[0] == 'group':
        # KL is in the US group, UA9 in RU; Chile is in none, and so is a bad index - both empty
        check(t[1:] == ['[US]', '[RU]', '[]', '[]'], 'entityGroup: ' + line)
    elif t[0] == 'cross':
        check(t[1] == '0' and t[2] == '0', 'a different administration must still be rejected: ' + line)
    elif t[0] == 'neighbour-tolerance':
        check(t[1] == '1' and t[2] == '0', 'tolerance accepts a neighbour only when asked: ' + line)
    else:
        fail.append('the self-test printed a line this checker does not know: ' + line)
    seen[t[0]] = seen.get(t[0], 0) + 1

# Every check the self-test is supposed to print, and how many.  Without this a block deleted from
# geodata_selftest.cpp - or a self-test that stops half way - was a green run (review 2026-09-24).
for kind, n in (('roundtrip', 1), ('bad', 1), ('corner', 1), ('wrap', 1), ('fit', 18), ('case', 10),
                ('case-unknown', 1), ('admin', 6), ('group', 1), ('cross', 1),
                ('neighbour-tolerance', 1)):
    check(seen.get(kind, 0) == n,
          'the self-test printed %d %r lines, expected %d' % (seen.get(kind, 0), kind, n))

print('entities %d, squares %d, API checks %d, %s' % (len(ents), len(squares), len(api),
      'all match the json' if not fail else '%d MISMATCHES' % len(fail)))
for f in fail[:20]: print('   ' + f)
sys.exit(1 if fail else 0)
