#!/usr/bin/env python3
"""The state of each US amateur radio licence, for Worked All States in grid squares several states share.

Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software:
you may redistribute it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License, or (at your option) any
later version.  It is distributed WITHOUT ANY WARRANTY; see the GNU General Public License for more
details.  The FILE it writes is a different matter - see the note it carries.

    python3 tools/make_license_states.py path/to/l_amat.zip [out.txt]
    python3 geodata/make_license_states.py path/to/l_amat.zip            (inside a source tree: writes ../us-license-states.txt)

Input : the FCC's weekly complete Universal Licensing System file for the Amateur Radio Service,
        https://data.fcc.gov/download/pub/uls/complete/l_amat.zip (about 200 MB, rebuilt every Sunday),
        a public record of the US Government.  Read straight from the zip: HD.dat (the licence: call
        sign, status, grant, expiry and cancellation dates), EN.dat (the licensee: mailing address)
        and `counts` (the file's creation date, which becomes the version).  Licences of the
        Amateur (HA) and Vanity (HV) radio services only; the licensee's own entity row (type L).

        The FCC records no station location for an amateur licence - only the MAILING address - so
        the state is that address's.  JTDX_contest uses it only where a decoded grid square is shared
        by several states, and only when the licence's state is one of them (us_states::clearState):
        a traveller sending a square in another state is left unresolved, never put in the wrong one.

Output: us-license-states.txt (default: src/ and jtdx_contest/ when run from tools/ in the work tree,
        the tree's root when run from a tree's geodata/) - a header of '#' lines, then one line per call,
        sorted:

            AA0AI CO

        Which calls: every ACTIVE licence, and every licence that ENDED (expired, cancelled,
        terminated) within the two years before the file - the FCC's grace period, inside which no
        call is given to anyone else, so the last holder's state is still the only answer (a vanity
        change leaves the old call cancelled; QSOs made under it still resolve).  A call with several
        licences takes the one granted last.  Washington DC is counted as Maryland, as Worked All
        States counts it and as geodata/grid_states.json folds it.  Only states that share a grid
        square with another state are kept (the others are never asked): the list is read from
        geodata/grid_states.json.

        Every call must be a US call of at most six characters (the program packs it in 32 bits);
        anything else stops the run - a format change at the FCC must fail here, loudly, not ship.
        So does an input with fewer than 100 000 licences or calls selected (--minimum=N lowers that for
        the tool's own test, test/license_states_tool_check.py).

Measured 2026-10-03 on the file of 2026-09-27: 914 442 calls, 8.8 MB of text (2.5 MB gzip).
"""
import datetime, io, json, os, re, sys, zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..'))
GEO = next((p for p in (os.path.join(ROOT, 'geodata'), HERE) if os.path.exists(os.path.join(p, 'grid_states.json'))), None)
NAME = 'us-license-states.txt'
SERVICES = {'HA', 'HV'}
GRACE = datetime.timedelta(days=730)
FOLD = {'DC': 'MD'}
US_CALL = re.compile(r'^[AKNW][A-Z]?[0-9][A-Z]{1,3}$')   # the shape the program's lookup accepts (licensestates.cpp)


def die(msg):
    sys.exit('make_license_states: ' + msg)


def date(s):
    try:
        return datetime.datetime.strptime(s, '%m/%d/%Y').date()
    except ValueError:
        return None


def shared_states():
    if not GEO:
        die('geodata/grid_states.json not found')
    squares = json.load(open(os.path.join(GEO, 'grid_states.json'), encoding='utf-8'))['squares']
    return {e['s'] for v in squares.values() if len(v['states']) > 1 for e in v['states']}


def build(zip_path, minimum=100000):
    try:
        z = zipfile.ZipFile(zip_path)
    except (OSError, zipfile.BadZipFile) as e:
        die('%s: %s - the input is https://data.fcc.gov/download/pub/uls/complete/l_amat.zip' % (zip_path, e))
    names = set(z.namelist())
    for need in ('counts', 'HD.dat', 'EN.dat'):
        if need not in names:
            die('%s has no %s - not the FCC amateur file' % (zip_path, need))
    counts = z.read('counts').decode('latin-1')
    m = re.search(r'File Creation Date:\s*\w{3}\s+(\w{3})\s+(\d{1,2})\s+[\d:]+\s+\w+\s+(\d{4})', counts)
    if not m:
        die('no "File Creation Date" in counts: %r' % counts[:200])
    created = datetime.datetime.strptime(' '.join(m.groups()), '%b %d %Y').date()

    licences = {}   # system id -> (call, status, grant, ended)
    for raw in io.TextIOWrapper(z.open('HD.dat'), encoding='latin-1'):
        f = raw.rstrip('\r\n').split('|')
        if len(f) < 10 or f[0] != 'HD' or f[6] not in SERVICES:
            continue
        # a cancelled licence keeps its (future) expiry date: its end is the cancellation
        licences[f[1]] = (f[4].strip().upper(), f[5], date(f[7]), date(f[9]) or date(f[8]))
    if len(licences) < minimum:
        die('only %d amateur licences in HD.dat - not the complete file' % len(licences))

    keep_states = shared_states()
    latest = {}     # call -> (grant, status, ended, state)
    for raw in io.TextIOWrapper(z.open('EN.dat'), encoding='latin-1'):
        f = raw.rstrip('\r\n').split('|')
        if len(f) < 19 or f[0] != 'EN' or f[5] != 'L' or f[1] not in licences:
            continue
        call, status, grant, ended = licences[f[1]]
        g = grant or datetime.date(1900, 1, 1)
        if call not in latest or g > latest[call][0]:
            latest[call] = (g, status, ended, f[17].strip().upper())

    out = {}
    for call, (grant, status, ended, state) in latest.items():
        if status != 'A' and not (ended and created - GRACE <= ended <= created):
            continue
        state = FOLD.get(state, state)
        if state not in keep_states:
            continue
        if not US_CALL.match(call):
            die('call %r does not have the shape of a US call of at most six characters' % call)
        out[call] = state
    if len(out) < minimum:
        die('only %d calls selected - something in the input changed' % len(out))
    return created, out


def write(path, created, calls):
    head = [
        '# us-license-states.txt - the state of each US amateur radio licence\'s mailing address, for JTDX_contest\'s',
        '# Worked All States in grid squares several states share (a call counts for its licence\'s state only when that',
        '# state is one of the square\'s). Derived from the FCC Universal Licensing System weekly amateur file',
        '# https://data.fcc.gov/download/pub/uls/complete/l_amat.zip - a public record of the US Government - by',
        '# tools/make_license_states.py: active licences and those that ended within the two years before the file;',
        '# Washington DC counted as Maryland; only states that share a grid square with another state.',
        '# version %s' % created.isoformat(),
        '# calls %d' % len(calls),
    ]
    body = ''.join('%s %s\n' % (c, calls[c]) for c in sorted(calls))
    with open(path, 'w', encoding='ascii', newline='\n') as fh:
        fh.write('\n'.join(head) + '\n' + body)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    minimum = next((int(a.split('=', 1)[1]) for a in sys.argv[1:] if a.startswith('--minimum=')), 100000)
    if not args:
        die('usage: make_license_states.py path/to/l_amat.zip [out.txt]')
    created, calls = build(args[0], minimum)
    targets = [args[1]] if len(args) > 1 else [p for p in (os.path.join(ROOT, 'src', NAME), os.path.join(ROOT, 'jtdx_contest', NAME))
                                               if os.path.isdir(os.path.dirname(p))]
    if not targets and len(args) == 1 and os.path.exists(os.path.join(ROOT, 'CMakeLists.txt')):
        targets = [os.path.join(ROOT, NAME)]   # run from a source tree's geodata/
    if not targets:
        die('no output given and no src/ or jtdx_contest/ beside tools/')
    for t in targets:
        write(t, created, calls)
        print('%s: version %s, %d calls, %.1f MB' % (t, created, len(calls), os.path.getsize(t) / 1e6))


if __name__ == '__main__':
    main()
