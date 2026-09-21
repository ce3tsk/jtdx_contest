// ------------------------------------------------------------------------------
// qpc_measured.c
//
// The definition of the SuperFox q-ary polar code (127,50), Q = 128, as MEASURED from the
// tone sequence of one transmission of WSJT-X 3.0.2.      GENERATED - do not edit:
//                                      lib/superfox/qpc/measured/measure.py
// ------------------------------------------------------------------------------
// CE3TSK 2026-09-20, for JTDX_CONTEST.                  Contact: jtdx_contest@ce3tsk.com
// ------------------------------------------------------------------------------
//
// NO CLAIM IS MADE ON THE TABLE
//   The numbers in the structure below were not composed by anybody here. They were read off
//   the tone sequence WSJT-X's own transmit tool produces, and they are what every receiver of
//   the mode has to use. Neither CE3TSK nor JTDX_CONTEST claims a copyright or any other right
//   in them, and nothing in this file is meant to say who, if anybody, holds one.
//   The ONLY part of this file for which authorship is claimed is what stands between the
//   "#if 0" and the "#endif" at its end: the programs the measurement was made with, their
//   description and their record. (This very text is part of it: it is written by measure.py,
//   which is copied there.) That part is free software: measure.py, build_packprobe.sh and the
//   texts under the GNU General Public License, version 3 or later; packprobe.f90, which is
//   derived from WSJT-X's sftx.f90 by Joseph Taylor K1JT, under the GPL version 3 as WSJT-X is.
//
// WHAT THIS FILE IS
//   np_qpc.c (Nico Palermo IV3NWV, GPL v3, from WSJT-X) encodes and decodes a polar code over
//   seven-bit symbols; which code, it takes from one structure, qpccode (np_qpc.h):
//     xpos[k]   the position, among the 128 inputs of the polar transform, of message symbol k.
//               Only xpos[0..49] is ever read (qpc_encode, qpc_decode). The other 78 entries
//               are filled here with the remaining positions in ascending order.
//     f[p]      the value of input p before encoding: zero everywhere. For a frozen position
//               this is its frozen value; f[0] = 0 makes codeword symbol 0 zero, which is why
//               it is not transmitted (127 symbols on the air).
//     fsize[p]  1 = position p carries a message symbol, 0 = frozen. The same set as xpos[0..49].
//   So the whole content is ONE list: the positions of the 50 message symbols, in message order.
//   This program uses it to RECEIVE SuperFox, and in its test tool sfoxsim to write test audio.
//   It does not transmit the mode.
//
// WHERE THE NUMBERS COME FROM
//   WSJT-X ships this structure in lib/superfox/qpc/qpc_n127k50q128.c, a file whose author
//   reserves its tables: "licensed only for use with WSJT-X", published "so that the transmitted
//   messages can be decoded by anybody", written authorisation asked for before the tables are
//   used in a derived work. He was asked on 2026-09-10 and again later; no answer came. That file
//   is NOT part of this program, and the tables below were not copied from it. (What this file
//   shares with it is what np_qpc.h dictates: the name and the type of the structure.)
//   The numbers below were MEASURED. They are properties of what a SuperFox transmitter sends -
//   any receiver of the mode must use exactly these, and anyone can read them off a transmission:
//     1. ONE transmission was made with WSJT-X 3.0.2's own SuperFox transmit tool, sftx: five
//        messages, nine Hounds, signed (sfox_1.dat, probe_args.txt). sftx writes the 151 tones
//        it would send to a file; nothing went on the air.
//     2. The 50 message symbols of the same transmission were printed by WSJT-X's own packer,
//        stopped before the encoder (packprobe.f90, linked against WSJT-X's library).
//        The transmission was chosen so that these 50 symbols are all different and none is
//        zero (measure.py find - with the packer alone, no run of sftx).
//     3. The 24 sync tones dropped, tone - 1 = symbol value, the untransmitted symbol 0 put back
//        as zero, and the polar transform applied, which is its own inverse: u(0:127).
//        u is non-zero at exactly 50 positions: the information positions. Message symbol k
//        sits where u equals it. Every other position is zero: the frozen values.
//   The full record - messages, tones, message symbols, u - is observation.txt, copied below.
//   The numbers are, necessarily, the same as WSJT-X's: they define the code it transmits.
//
// HOW TO MAKE IT AGAIN FROM NOTHING          (in lib/superfox/qpc/measured/)
//   python3 measure.py bootstrap --wsjtx <DIR> [--fresh]
//       <DIR> = the directory WSJT-X 3.0.2 was built in from its sources (it holds sftx and
//       libwsjt_fort.a). Builds packprobe, finds the transmission (only if sfox_1.dat is not
//       there, or with --fresh), runs sftx once, writes this file, and checks it as below.
//
// HOW TO CHECK IT          (in lib/superfox/qpc/measured/)
//   python3 measure.py check
//       the copies below are the files, and the tables follow from observation.txt. No WSJT-X
//       is needed for this.
//   python3 measure.py check --wsjtx <DIR>          (after: sh build_packprobe.sh <DIR>)
//       Makes the measurement again (one run of sftx), and CONFIRMS the result on 24 further
//       random transmissions it was not made from - reports, RR73, CQ, free text, signed and
//       unsigned: the message symbols put at the measured positions and transformed must be,
//       tone for tone, what sftx sends.
//   A receiver built on this file decodes what WSJT-X's own receiver decodes, line for line,
//   from WSJT-X's and from MSHV's encoders (the project's regression tests, which are kept
//   outside this source tree).
//
// THE FILES THIS CAME FROM are kept twice: in the source tree, directory
//     lib/superfox/qpc/measured/
//   (README.md, measure.py, packprobe.f90, build_packprobe.sh,
//    sfox_1.dat, probe_args.txt, observation.txt)
//   and, line for line, inside the "#if 0" at the end of this file.
// ------------------------------------------------------------------------------
#include "np_qpc.h"
qpccode_ds qpccode = {
  .n  = 128,          // inputs of the polar transform
  .np = 127,          // symbols transmitted (codeword symbol 0 is always zero)
  .k  = 50,           // message symbols
  .q  = 128,          // values of a symbol
  .xpos = {           // message symbol k -> position; entries 50..127 are never read
    1,   2,   3,   4,   5,   6,   8,   9,  10,  12,  16,  32,  17,  18,  64,  20,
   33,  34,  24,   7,  11,  36,  13,  19,  14,  65,  40,  21,  66,  22,  35,  68,
   25,  48,  37,  26,  72,  15,  38,  28,  41,  67,  23,  80,  42,  69,  49,  96,
   44,  27,   0,  29,  30,  31,  39,  43,  45,  46,  47,  50,  51,  52,  53,  54,
   55,  56,  57,  58,  59,  60,  61,  62,  63,  70,  71,  73,  74,  75,  76,  77,
   78,  79,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,
   95,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
  112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127
  },
  .f = {              // all zero
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
  },
  .fsize = {          // 1 = carries a message symbol
    0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,
    1,   1,   1,   1,   1,   1,   1,   0,   1,   1,   1,   0,   1,   0,   0,   0,
    1,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    1,   1,   1,   1,   1,   1,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,
    1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
  }
};

#if 0
// From here to the #endif: the part of this file for which authorship is claimed (GPL v3 or later, see the top).
// Copies of the files of lib/superfox/qpc/measured/, each inside a comment so that no compiler reads them.
/* ===== BEGIN FILE: lib/superfox/qpc/measured/README.md =====
# The SuperFox code definition, measured

`../qpc_measured.c` tells the polar encoder/decoder (`../np_qpc.c`, GPL v3) WHICH code SuperFox
uses: the positions of the 50 message symbols among the 128 inputs of the polar transform, in
message order. WSJT-X ships the same information in `qpc_n127k50q128.c`, a file whose author
reserves its tables; that file is not part of this program. The numbers here were measured from
the tone sequence of ONE transmission of WSJT-X 3.0.2's own SuperFox transmit tool (written to a
file; nothing went on the air) - they are properties of what a SuperFox transmitter sends, and
any receiver of the mode must use exactly these.

| file | what |
| --- | --- |
| `measure.py` | `bootstrap`: all of the following, from nothing. `find`: look for one transmission whose 50 message symbols are all different and non-zero (packer only). `measure`: ONE run of WSJT-X's `sftx` + one of `packprobe`, the arithmetic, `observation.txt` and `../qpc_measured.c`. `regen`: write `../qpc_measured.c` again from `observation.txt` and the files here, no WSJT-X needed (after editing a text of this directory, which the C file copies). `check`: the generated file, the record and the copies agree; with `--wsjtx DIR` the measurement is made again and confirmed on 24 further transmissions. The method is in its opening text |
| `packprobe.f90` | WSJT-X's `sftx.f90` up to its call of `sfox_pack`, stopped before the encoder: prints the 50 message symbols |
| `build_packprobe.sh DIR` | builds it against WSJT-X's own library (`DIR` = a WSJT-X 3.0.2 build from source) |
| `sfox_1.dat`, `probe_args.txt` | the transmission: five old-style Fox messages (nine Hounds), the Fox call, the key, and how many single-field changes the guided search of `find` tried. The call signs are random strings. (`sftx` 3.0.2 takes the Fox's call from the messages; its second argument is read and not used) |
| `observation.txt` | the record: the 151 tones, the 50 message symbols, the transform's output, the result |

Every file of this table is also copied, line for line, into the `#if 0` at the end of
`../qpc_measured.c`. `.gitignore` keeps `packprobe_build/` out of a repository: a binary built
there names the paths of the machine it was built on.

**No claim is made on the table.** The numbers were read off a transmitted signal; neither CE3TSK
nor JTDX_CONTEST claims a copyright or any other right in them. What authorship is claimed for is
the measuring programs and their record - the files of this directory, which are also the part of
`../qpc_measured.c` between its `#if 0` and `#endif`: `measure.py`, `build_packprobe.sh` and the
texts under the GNU GPL, version 3 or later; `packprobe.f90`, derived from WSJT-X's `sftx.f90` by
Joseph Taylor K1JT, under the GPL version 3 as WSJT-X is.

To make `../qpc_measured.c` again FROM NOTHING: build WSJT-X 3.0.2 from its sources, then

    python3 measure.py bootstrap --wsjtx <the directory WSJT-X was built in>

It builds `packprobe` (into `./packprobe_build`), finds the transmission if `sfox_1.dat` is not
there (`--fresh`: even if it is), runs `sftx` once, writes `observation.txt` and
`../qpc_measured.c`, and checks the result - including on 24 further transmissions it was not
made from. `check` alone verifies a file that EXISTS (without `--wsjtx` it needs no WSJT-X);
`measure` is the step that writes it.
===== END FILE: lib/superfox/qpc/measured/README.md ===== */
/* ===== BEGIN FILE: lib/superfox/qpc/measured/measure.py =====
#!/usr/bin/env python3
"""CE3TSK 2026-09-20: the definition of the SuperFox (127,50) polar code, MEASURED from ONE
transmission of WSJT-X's own transmitter, and the generator of ../qpc_measured.c.

What a SuperFox receiver has to know about the code, beyond the polar transform itself (Arikan's,
y[hi] ^= y[lo] at each of seven stages, 128 seven-bit symbols - np_qpc.c, GPL):
  - which 50 of the 128 positions of the transform's input carry the message (the rest are frozen),
  - which message symbol sits at which of them,
  - the frozen values.
These are properties of the transmitted signal, so they are read off the signal here:

  tones   = sftx      <messages> <foxcall> <key>    WSJT-X 3.0.2's SuperFox transmit tool, ONE run:
                                                    the 151 channel symbols it sends (sfox_2.dat)
  payload = packprobe <messages> <foxcall> <key>    WSJT-X's packer stopped before the encoder
                                                    (packprobe.f90): the 50 message symbols

Drop the 24 sync symbols, tone - 1 = symbol value, put the untransmitted codeword symbol 0 back as
zero, apply the transform (it is its own inverse): u(0:127). Then
  - the positions where u is non-zero are the information positions,
  - message symbol k sits at the position p where u[p] == payload[k].
ONE transmission answers both questions if its 50 message symbols are all DIFFERENT and all
NON-ZERO. "find" looks for such a transmission with the packer alone (no run of sftx): call
signs for the Fox and for nine Hounds random in every character, random reports, a signed message
(the 20-bit signature field is zero otherwise). Drawing whole transmissions at random does not get
there: while this script was written a blind search of that kind was stopped after 490000 draws
(call signs from fifteen prefixes) and again after 90000 (random in every character) without one
that qualified - a few symbols carry report and flag bits and take a handful of values. So the
search is guided: one field at a time (a call, a report, the key) is drawn again, and the change is
kept when the number of different non-zero symbols does not drop. 138 changes do it.

  python3 measure.py bootstrap --wsjtx DIR    FROM NOTHING: builds packprobe if it is not there,
                                              finds the transmission if there is none (--fresh:
                                              even if there is), measures, checks. The one command
                                              that makes ../qpc_measured.c where there is none
  python3 measure.py find    --wsjtx DIR      writes sfox_1.dat and probe_args.txt (done once)
  python3 measure.py measure --wsjtx DIR      ONE run of sftx, one of packprobe: observation.txt
                                              and ../qpc_measured.c (tables, record, copies of
                                              every file of this directory)
  python3 measure.py regen                    writes ../qpc_measured.c again from observation.txt and
                                              the files here - no WSJT-X needed (after an edit of a
                                              text in this directory, which the C file copies)
  python3 measure.py check  [--wsjtx DIR]     the copies inside ../qpc_measured.c are these files,
                                              its tables follow from observation.txt; with --wsjtx
                                              the measurement is made again and must agree, and the
                                              result is CONFIRMED on transmissions it never saw: for
                                              24 further random ones (reports, RR73, CQ, free text,
                                              signed and unsigned) the message symbols put at the
                                              measured positions and transformed must be, tone for
                                              tone, what WSJT-X's sftx sends
DIR is the directory of a WSJT-X 3.0.2 build from source (the one that holds sftx and
libwsjt_fort.a); packprobe is built into ./packprobe_build by build_packprobe.sh DIR (bootstrap
does it, every time). That directory is NOT for a repository - a binary built there names the
paths of the machine it was built on; .gitignore keeps it out. The call signs in the probe are
random strings. (sftx 3.0.2 takes the Fox's call from the messages; its second argument is read
and not used.)
"""
import argparse, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
TREEPATH = 'lib/superfox/qpc/measured'                 # this directory, as the source tree names it
FILES = ['README.md', 'measure.py', 'packprobe.f90', 'build_packprobe.sh', 'sfox_1.dat', 'probe_args.txt', 'observation.txt']
ISYNC = [1, 2, 4, 7, 11, 16, 22, 29, 37, 39, 42, 43, 45, 48, 52, 57, 63, 70, 78, 80, 83, 84, 86, 89]   # sftx.f90
CO, CC = '/' + '*', '*' + '/'                           # a C comment's ends, never spelled out: this file is copied into one


def polar(v):
    """y[hi] ^= y[lo] at every stage; its own inverse"""
    y = list(v); k = 1
    while k < 128:
        for base in range(0, 128, 2 * k):
            for m in range(k): y[base + m + k] ^= y[base + m]
        k *= 2
    return y


def wsjtx_dir(a):
    d = a.wsjtx or os.environ.get('WSJTX_BUILD', '')
    if not d: sys.exit('give --wsjtx <WSJT-X 3.0.2 build directory> (or WSJTX_BUILD)')
    return os.path.abspath(d)          # the tools are run from a temporary directory


def tools(a):
    d = wsjtx_dir(a)
    sftx = os.path.join(d, 'sftx'); pack = os.path.abspath(a.packprobe) if a.packprobe else os.path.join(HERE, 'packprobe_build', 'packprobe')
    if not os.access(sftx, os.X_OK): sys.exit('no sftx in %s - --wsjtx must name the directory WSJT-X 3.0.2 was BUILT in (sftx, libwsjt_fort.a)' % d)
    if not os.access(pack, os.X_OK): sys.exit('no %s - build it: sh build_packprobe.sh %s   (or: measure.py bootstrap --wsjtx %s)' % (pack, d, d))
    return sftx, pack


def put(path, text):
    with open(path, 'w', encoding='utf-8') as f: f.write(text)


def payload_of(pack, W, fox, key):
    p = subprocess.run([pack, 'sfox_1.dat', fox, key], cwd=W, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try: x = [int(v) & 127 for v in p.stdout.split()]
    except ValueError: x = []
    if p.returncode != 0 or len(x) != 50: sys.exit('packprobe gave %d symbols, exit %d: %s' % (len(x), p.returncode, p.stderr.strip()[-300:]))
    return x


def derive(tones, payload):
    """the arithmetic of the module text; returns (order, info, u) or stops with the reason"""
    if len(tones) != 151 or any(tones[n - 1] != 0 for n in ISYNC): sys.exit('not a SuperFox tone sequence (151 tones, sync = tone 0)')
    data = [tones[n - 1] - 1 for n in range(1, 152) if n not in ISYNC]
    if len(data) != 127 or min(data) < 0 or max(data) > 127: sys.exit('data symbols out of range')
    u = polar([0] + data)
    info = [p for p in range(128) if u[p] != 0]
    if len(info) != 50: sys.exit('%d non-zero positions, not 50: the probe has a zero message symbol' % len(info))
    if len(set(payload)) != 50 or 0 in payload: sys.exit('the probe\'s 50 message symbols are not all different and non-zero')
    order = []
    for k in range(50):
        c = [p for p in info if u[p] == payload[k]]
        if len(c) != 1: sys.exit('message symbol %d matches %d positions' % (k, len(c)))
        order.append(c[0])
    if sorted(order) != info: sys.exit('the order does not cover the information positions')
    return order, info, u


def read_probe():
    msgs = open(os.path.join(HERE, 'sfox_1.dat')).read().split('\n')
    msgs = [m for m in msgs if m.strip()]
    args = dict(l.split('=', 1) for l in open(os.path.join(HERE, 'probe_args.txt')).read().split('\n') if '=' in l)
    return msgs, args['foxcall'], args['key']


def cmd_find(a):
    import random
    _, pack = tools(a); rnd = random.Random(20260920); tries = 0
    L = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
    def call():       # a standard call sign in form, random in every character: one or two letters, a digit, one to three letters
        return ''.join(rnd.choice(L) for _ in range(rnd.randrange(1, 3))) + str(rnd.randrange(10)) + ''.join(rnd.choice(L) for _ in range(rnd.randrange(1, 4)))
    def rpt(): return '%+03d' % rnd.randrange(-24, 20)
    # the fields: the Fox, five Hounds that get RR73, four that get a report, the four reports, the key
    f = {'fox': call(), 'r': [call() for _ in range(5)], 'h': [call() for _ in range(4)], 'p': [rpt() for _ in range(4)], 'key': rnd.randrange(1, 1000000)}
    def text(f):
        l = ['%s RR73; %s <%s> %s' % (f['r'][i], f['h'][i], f['fox'], f['p'][i]) for i in range(4)] + ['%s %s RR73' % (f['r'][4], f['fox'])]
        return [v[:40] for v in l], 'OTP:%06d' % f['key']     # sftx reads a40 and pads: no trailing blanks to lose
    def score(f, W):
        lines, key = text(f); put(os.path.join(W, 'sfox_1.dat'), '\n'.join(lines) + '\n')
        return len(set(payload_of(pack, W, f['fox'], key)) - {0})
    with tempfile.TemporaryDirectory() as W:
        best = score(f, W)
        while best < 50:
            tries += 1
            if tries > 200000: sys.exit('no transmission with 50 different non-zero symbols after 200000 changes (%d of 50) - is this WSJT-X 3.0.2?' % best)
            g = {k: (list(v) if isinstance(v, list) else v) for k, v in f.items()}
            w = rnd.randrange(15)
            if w == 0: g['fox'] = call()
            elif w <= 5: g['r'][w - 1] = call()
            elif w <= 9: g['h'][w - 6] = call()
            elif w <= 13: g['p'][w - 10] = rpt()
            else: g['key'] = rnd.randrange(1, 1000000)
            if len({g['fox']} | set(g['r']) | set(g['h'])) < 10: continue          # ten different stations
            sc = score(g, W)
            if sc >= best: f, best = g, sc
            if tries % 1000 == 0: print('  %d changes tried, %d of 50 symbols different and non-zero' % (tries, best), flush=True)
        lines, key = text(f); fox = f['fox']
    put(os.path.join(HERE, 'sfox_1.dat'), '\n'.join(lines) + '\n')
    put(os.path.join(HERE, 'probe_args.txt'), 'foxcall=%s\nkey=%s\nchanges_tried=%d\n' % (fox, key, tries))
    print('after %d changes the transmission has 50 different non-zero message symbols: Fox %s, key %s' % (tries, fox, key))
    for l in lines: print('  ' + l.rstrip())


def observe(a, probe=None):
    """ONE run of sftx and one of packprobe - on the stored probe, or on the one given"""
    sftx, pack = tools(a); msgs, fox, key = probe or read_probe()
    with tempfile.TemporaryDirectory() as W:
        put(os.path.join(W, 'sfox_1.dat'), '\n'.join(msgs) + '\n')
        r = subprocess.run([sftx, 'sfox_1.dat', fox, key], cwd=W, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try: tones = [int(v) for v in open(os.path.join(W, 'sfox_2.dat')).read().split()]
        except OSError: sys.exit('sftx wrote no sfox_2.dat (exit %d)' % r.returncode)
        payload = payload_of(pack, W, fox, key)
    return msgs, fox, key, tones, payload


def rows(v, n=16, w=3): return ['  ' + ', '.join('%*d' % (w, x) for x in v[i:i + n]) for i in range(0, len(v), n)]


def observation_text(msgs, fox, key, tones, payload, order, info, u):
    t = ['# One SuperFox transmission of WSJT-X 3.0.2 and what follows from it (measure.py measure).',
         '# messages (sfox_1.dat), Fox call and key as given to sftx and to packprobe:']
    t += ['#   ' + m.rstrip() for m in msgs] + ['foxcall=%s' % fox, 'key=%s' % key,
         '# the 151 tones sftx wrote to sfox_2.dat (tone 0 = sync, tone v+1 = symbol value v):', 'tones=' + ' '.join(str(v) for v in tones),
         '# the 50 message symbols packprobe printed (WSJT-X\'s sfox_pack, before the encoder):', 'payload=' + ' '.join(str(v) for v in payload),
         '# u = transform of (0, the 127 data symbols); non-zero at exactly 50 positions:', 'u=' + ' '.join(str(v) for v in u),
         '# information positions, ascending:', 'info=' + ' '.join(str(v) for v in info),
         '# message symbol k -> position (u[position] == payload[k]):', 'order=' + ' '.join(str(v) for v in order)]
    return '\n'.join(t) + '\n'


def parse_observation():
    d = {}
    for l in open(os.path.join(HERE, 'observation.txt')).read().split('\n'):
        if '=' in l and not l.startswith('#'): k, v = l.split('=', 1); d[k] = v
    return [int(v) for v in d['tones'].split()], [int(v) for v in d['payload'].split()], [int(v) for v in d['order'].split()]


def c_file(order, info):
    frozen = [p for p in range(128) if p not in info]
    xpos = order + frozen; fsize = [1 if p in info else 0 for p in range(128)]
    h = '''// ------------------------------------------------------------------------------
// qpc_measured.c
//
// The definition of the SuperFox q-ary polar code (127,50), Q = 128, as MEASURED from the
// tone sequence of one transmission of WSJT-X 3.0.2.      GENERATED - do not edit:
//                                      %(tree)s/measure.py
// ------------------------------------------------------------------------------
// CE3TSK 2026-09-20, for JTDX_CONTEST.                  Contact: jtdx_contest@ce3tsk.com
// ------------------------------------------------------------------------------
//
// NO CLAIM IS MADE ON THE TABLE
//   The numbers in the structure below were not composed by anybody here. They were read off
//   the tone sequence WSJT-X's own transmit tool produces, and they are what every receiver of
//   the mode has to use. Neither CE3TSK nor JTDX_CONTEST claims a copyright or any other right
//   in them, and nothing in this file is meant to say who, if anybody, holds one.
//   The ONLY part of this file for which authorship is claimed is what stands between the
//   "#if 0" and the "#endif" at its end: the programs the measurement was made with, their
//   description and their record. (This very text is part of it: it is written by measure.py,
//   which is copied there.) That part is free software: measure.py, build_packprobe.sh and the
//   texts under the GNU General Public License, version 3 or later; packprobe.f90, which is
//   derived from WSJT-X's sftx.f90 by Joseph Taylor K1JT, under the GPL version 3 as WSJT-X is.
//
// WHAT THIS FILE IS
//   np_qpc.c (Nico Palermo IV3NWV, GPL v3, from WSJT-X) encodes and decodes a polar code over
//   seven-bit symbols; which code, it takes from one structure, qpccode (np_qpc.h):
//     xpos[k]   the position, among the 128 inputs of the polar transform, of message symbol k.
//               Only xpos[0..49] is ever read (qpc_encode, qpc_decode). The other 78 entries
//               are filled here with the remaining positions in ascending order.
//     f[p]      the value of input p before encoding: zero everywhere. For a frozen position
//               this is its frozen value; f[0] = 0 makes codeword symbol 0 zero, which is why
//               it is not transmitted (127 symbols on the air).
//     fsize[p]  1 = position p carries a message symbol, 0 = frozen. The same set as xpos[0..49].
//   So the whole content is ONE list: the positions of the 50 message symbols, in message order.
//   This program uses it to RECEIVE SuperFox, and in its test tool sfoxsim to write test audio.
//   It does not transmit the mode.
//
// WHERE THE NUMBERS COME FROM
//   WSJT-X ships this structure in lib/superfox/qpc/qpc_n127k50q128.c, a file whose author
//   reserves its tables: "licensed only for use with WSJT-X", published "so that the transmitted
//   messages can be decoded by anybody", written authorisation asked for before the tables are
//   used in a derived work. He was asked on 2026-09-10 and again later; no answer came. That file
//   is NOT part of this program, and the tables below were not copied from it. (What this file
//   shares with it is what np_qpc.h dictates: the name and the type of the structure.)
//   The numbers below were MEASURED. They are properties of what a SuperFox transmitter sends -
//   any receiver of the mode must use exactly these, and anyone can read them off a transmission:
//     1. ONE transmission was made with WSJT-X 3.0.2's own SuperFox transmit tool, sftx: five
//        messages, nine Hounds, signed (sfox_1.dat, probe_args.txt). sftx writes the 151 tones
//        it would send to a file; nothing went on the air.
//     2. The 50 message symbols of the same transmission were printed by WSJT-X's own packer,
//        stopped before the encoder (packprobe.f90, linked against WSJT-X's library).
//        The transmission was chosen so that these 50 symbols are all different and none is
//        zero (measure.py find - with the packer alone, no run of sftx).
//     3. The 24 sync tones dropped, tone - 1 = symbol value, the untransmitted symbol 0 put back
//        as zero, and the polar transform applied, which is its own inverse: u(0:127).
//        u is non-zero at exactly 50 positions: the information positions. Message symbol k
//        sits where u equals it. Every other position is zero: the frozen values.
//   The full record - messages, tones, message symbols, u - is observation.txt, copied below.
//   The numbers are, necessarily, the same as WSJT-X's: they define the code it transmits.
//
// HOW TO MAKE IT AGAIN FROM NOTHING          (in %(tree)s/)
//   python3 measure.py bootstrap --wsjtx <DIR> [--fresh]
//       <DIR> = the directory WSJT-X 3.0.2 was built in from its sources (it holds sftx and
//       libwsjt_fort.a). Builds packprobe, finds the transmission (only if sfox_1.dat is not
//       there, or with --fresh), runs sftx once, writes this file, and checks it as below.
//
// HOW TO CHECK IT          (in %(tree)s/)
//   python3 measure.py check
//       the copies below are the files, and the tables follow from observation.txt. No WSJT-X
//       is needed for this.
//   python3 measure.py check --wsjtx <DIR>          (after: sh build_packprobe.sh <DIR>)
//       Makes the measurement again (one run of sftx), and CONFIRMS the result on 24 further
//       random transmissions it was not made from - reports, RR73, CQ, free text, signed and
//       unsigned: the message symbols put at the measured positions and transformed must be,
//       tone for tone, what sftx sends.
//   A receiver built on this file decodes what WSJT-X's own receiver decodes, line for line,
//   from WSJT-X's and from MSHV's encoders (the project's regression tests, which are kept
//   outside this source tree).
//
// THE FILES THIS CAME FROM are kept twice: in the source tree, directory
//     %(tree)s/
//   (%(names1)s
//    %(names2)s)
//   and, line for line, inside the "#if 0" at the end of this file.
// ------------------------------------------------------------------------------
#include "np_qpc.h"
qpccode_ds qpccode = {
  .n  = 128,          // inputs of the polar transform
  .np = 127,          // symbols transmitted (codeword symbol 0 is always zero)
  .k  = 50,           // message symbols
  .q  = 128,          // values of a symbol
  .xpos = {           // message symbol k -> position; entries 50..127 are never read
%(xpos)s
  },
  .f = {              // all zero
%(f)s
  },
  .fsize = {          // 1 = carries a message symbol
%(fsize)s
  }
};
''' % dict(tree=TREEPATH, names1=', '.join(FILES[:4]) + ',', names2=', '.join(FILES[4:]), xpos=',\n'.join(rows(xpos)), f=',\n'.join(rows([0] * 128)), fsize=',\n'.join(rows(fsize)))
    t = [h, '#if 0', '// From here to the #endif: the part of this file for which authorship is claimed (GPL v3 or later, see the top).',
         '// Copies of the files of %s/, each inside a comment so that no compiler reads them.' % TREEPATH]
    for n in FILES:
        body = open(os.path.join(HERE, n), encoding='utf-8').read()
        if CO in body or CC in body: sys.exit('%s holds a C comment delimiter and cannot be copied into a C comment' % n)
        t += [CO + ' ===== BEGIN FILE: %s/%s =====' % (TREEPATH, n), body.rstrip('\n'), '===== END FILE: %s/%s ===== ' % (TREEPATH, n) + CC]
    t += ['#endif', '']
    return '\n'.join(t)


def cmd_measure(a):
    msgs, fox, key, tones, payload = observe(a)
    order, info, u = derive(tones, payload)
    put(os.path.join(HERE, 'observation.txt'), observation_text(msgs, fox, key, tones, payload, order, info, u))
    out = os.path.join(HERE, '..', 'qpc_measured.c'); put(out, c_file(order, info))
    print('one run of sftx: 50 information positions, the order of the 50 message symbols unique')
    print('  order: ' + ' '.join(str(v) for v in order)); print('wrote observation.txt and ' + os.path.normpath(out))


def cmd_regen(a):
    tones, payload, _ = parse_observation()
    order, info, _ = derive(tones, payload)
    out = os.path.join(HERE, '..', 'qpc_measured.c'); put(out, c_file(order, info))
    print('wrote ' + os.path.normpath(out) + ' from observation.txt and the files here')


def cmd_bootstrap(a):
    d = wsjtx_dir(a)
    if not a.packprobe:       # built every time (a second): a packprobe left from another WSJT-X or another packprobe.f90 is not reused
        r = subprocess.run(['sh', os.path.join(HERE, 'build_packprobe.sh'), d, os.path.join(HERE, 'packprobe_build')])
        if r.returncode != 0: sys.exit('build_packprobe.sh failed')
    if a.fresh or not (os.path.exists(os.path.join(HERE, 'sfox_1.dat')) and os.path.exists(os.path.join(HERE, 'probe_args.txt'))): cmd_find(a)
    else: print('the transmission of sfox_1.dat / probe_args.txt is used (--fresh looks for it again)')
    a.online = True
    cmd_measure(a); cmd_check(a)


def cmd_check(a):
    bad = 0
    if not os.path.exists(os.path.join(HERE, '..', 'qpc_measured.c')):
        sys.exit('there is no ../qpc_measured.c to check. "check" verifies an existing file; to MAKE it:\n'
                 '    python3 measure.py bootstrap --wsjtx <WSJT-X 3.0.2 build directory>     (everything, from nothing)\n'
                 ' or python3 measure.py measure   --wsjtx <WSJT-X 3.0.2 build directory>     (packprobe built, transmission present)')
    if not os.path.exists(os.path.join(HERE, 'observation.txt')): sys.exit('there is no observation.txt - run "measure" (or "bootstrap")')
    tones, payload, order_rec = parse_observation()
    order, info, _ = derive(tones, payload)
    if order != order_rec: print('** observation.txt: its order line does not follow from its tones and payload'); bad += 1
    src = open(os.path.join(HERE, '..', 'qpc_measured.c'), encoding='utf-8').read()
    if src != c_file(order, info): print('** ../qpc_measured.c is not what measure.py makes of observation.txt and these files'); bad += 1
    blocks = [[int(v) for v in b.replace('\n', ' ').split(',') if v.strip()] for b in re.findall(r'\{[^\n{}]*\n([0-9,\s]+)\}', src.split('\n#if 0\n')[0])]   # the directive, not the words in the text above
    if len(blocks) != 3 or blocks[0][:50] != order or any(blocks[1]) or [p for p in range(128) if blocks[2][p]] != info:
        print('** the tables of ../qpc_measured.c are not the measured ones'); bad += 1
    for n in FILES:
        m = re.search(r'===== BEGIN FILE: %s/%s =====\n(.*?)\n===== END FILE: %s/%s =====' % (TREEPATH, re.escape(n), TREEPATH, re.escape(n)), src, re.S)
        body = open(os.path.join(HERE, n), encoding='utf-8').read().rstrip('\n')
        if not m or m.group(1) != body: print('** the copy of %s inside ../qpc_measured.c differs from the file' % n); bad += 1
    if a.wsjtx or a.online:   # only when ASKED for: a WSJTX_BUILD left in the environment must not turn the offline check into the online one
        _, _, _, tones2, payload2 = observe(a)
        if tones2 != tones or payload2 != payload: print('** measured again: WSJT-X\'s tones or message symbols differ from observation.txt'); bad += 1
        else: print('measured again with one run of sftx: the same 151 tones and 50 message symbols')
        # the confirmation: transmissions the measurement never saw, encoded with the measured definition
        import random
        rnd = random.Random(7); L = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'; nok = 0; N = 24
        def call(): return ''.join(rnd.choice(L) for _ in range(rnd.randrange(1, 3))) + str(rnd.randrange(10)) + ''.join(rnd.choice(L) for _ in range(rnd.randrange(1, 4)))
        def rpt(): return '%+03d' % rnd.randrange(-24, 20)
        for i in range(N):
            fox = call(); m = []
            if i % 4 == 3:
                m = ['%s %s %s' % (call(), fox, rpt())] * (i % 8 == 7) + ['%-38s1 ' % ''.join(rnd.choice(L + '0123456789 /.?+-') for _ in range(rnd.randrange(5, 27))).strip()[:26]]
            else:
                for _ in range(rnd.randrange(1, 6)):
                    m.append(rnd.choice(['%s %s RR73' % (call(), fox), '%s %s %s' % (call(), fox, rpt()),
                                         '%s RR73; %s <%s> %s' % (call(), call(), fox, rpt()), 'CQ %s %s' % (fox, rnd.choice(['FF46', 'FN20', 'JD15', 'QH29']))]))
            m = [('%-40s' % v)[:40] for v in m]
            if i % 3 == 0: m[0] = m[0][:39] + '1'
            _, _, _, t3, x3 = observe(a, (m, fox, 'OTP:%06d' % (rnd.randrange(1, 1000000) if i % 2 else 0)))
            v = [0] * 128
            for k in range(50): v[order[k]] = x3[k]
            y = polar(v)
            sent = [t3[n - 1] - 1 for n in range(1, 152) if n not in ISYNC]
            nok += (y[0] == 0 and y[1:] == sent)
        if nok != N: print('** confirmation: only %d of %d further transmissions are reproduced by the measured definition' % (nok, N)); bad += 1
        else: print('confirmed on %d further transmissions: encoded with the measured definition they are WSJT-X\'s tones' % N)
    print('qpc_measured.c: tables, record and the %d copies %s' % (len(FILES), 'agree' if not bad else 'DO NOT agree'))
    sys.exit(1 if bad else 0)


ap = argparse.ArgumentParser()
ap.add_argument('command', choices=['bootstrap', 'find', 'measure', 'regen', 'check'])
ap.add_argument('--wsjtx', default='', help='the directory WSJT-X 3.0.2 was built in (holds sftx and libwsjt_fort.a)')
ap.add_argument('--packprobe', default='', help='the packprobe binary (default ./packprobe_build/packprobe)')
ap.add_argument('--fresh', action='store_true', help='bootstrap: look for the transmission again even if sfox_1.dat is there')
a = ap.parse_args(); a.online = False
{'bootstrap': cmd_bootstrap, 'find': cmd_find, 'measure': cmd_measure, 'regen': cmd_regen, 'check': cmd_check}[a.command](a)
===== END FILE: lib/superfox/qpc/measured/measure.py ===== */
/* ===== BEGIN FILE: lib/superfox/qpc/measured/packprobe.f90 =====
program packprobe

! CE3TSK 2026-09-20: the PAYLOAD of a SuperFox transmission, from WSJT-X's own packer.
!
! This is wsjtx-3.0.2/lib/superfox/sftx.f90 (K1JT, GPL v3) up to and including its call of
! sfox_pack - the same input file, the same arguments - and it stops there: it prints the 50
! seven-bit message symbols xin(0:49) and never calls the encoder. WSJT-X's sftx, run on the same
! input, gives the 151 tones of the transmission. measure.py puts the two side by side.
! It is linked against WSJT-X's library (build_packprobe.sh), not against this program's.
!
!   packprobe <message_file_name> <foxcall> <ckey>

  character*120 fname
  character*120 line
  character*40 cmsg(5)
  character*26 freeTextMsg
  character*10 ckey
  character*11 foxcall0,foxcall
  logical*1 bMoreCQs,bSendMsg
  integer*1 xin(0:49)

  if(iargc().ne.3) stop 1
  call getarg(1,fname)
  call getarg(2,foxcall0)
  call getarg(3,ckey)
  open(25,file=trim(fname),status='old')
  do i=1,5
     read(25,1000,end=10) cmsg(i)
1000 format(a40)
  enddo
  i=6
10 close(25)
  nslots=i-1
  freeTextMsg='                          '
  bMoreCQs=cmsg(1)(40:40).eq.'1'
  bSendMsg=cmsg(nslots)(39:39).eq.'1'
  if(bSendMsg) then
     freeTextMsg=cmsg(nslots)(1:26)
     if(nslots.gt.2) nslots=2
  endif
  call foxgen2(nslots,cmsg,line,foxcall)
  call sfox_pack(line,ckey,bMoreCQs,bSendMsg,freeTextMsg,xin)
  write(*,'(50i4)') xin
end program packprobe
===== END FILE: lib/superfox/qpc/measured/packprobe.f90 ===== */
/* ===== BEGIN FILE: lib/superfox/qpc/measured/build_packprobe.sh =====
#!/bin/sh
# CE3TSK 2026-09-20: builds packprobe against WSJT-X's OWN library.
#   sh build_packprobe.sh <WSJT-X 3.0.2 build directory> [output directory, default ./packprobe_build]
# The WSJT-X build directory is the one that holds libwsjt_fort.a, libwsjt_cxx.a and sftx after
# "cmake --build" of WSJT-X 3.0.2 from its sources. gfortran alone links it: packprobe needs the
# packer (libwsjt_fort.a) and the hash behind its CRC (nhash2, in libwsjt_cxx.a), nothing else.
# The output directory is NOT for a repository: the binary names the paths of the machine it was
# built on (the Fortran run-time's messages). .gitignore here keeps ./packprobe_build out.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
[ -n "$1" ] || { echo "usage: sh build_packprobe.sh <WSJT-X build directory> [output directory]"; exit 1; }
D=$(cd "$1" && pwd); O=${2:-$HERE/packprobe_build}; mkdir -p "$O"
for x in libwsjt_fort.a libwsjt_cxx.a sftx; do [ -e "$D/$x" ] || { echo "no $x in $D - build WSJT-X 3.0.2 there first"; exit 1; }; done
gfortran -O2 -fno-second-underscore -c "$HERE/packprobe.f90" -o "$O/packprobe.o"
gfortran "$O/packprobe.o" -o "$O/packprobe" "$D/libwsjt_fort.a" "$D/libwsjt_cxx.a"
echo "built $O/packprobe"
===== END FILE: lib/superfox/qpc/measured/build_packprobe.sh ===== */
/* ===== BEGIN FILE: lib/superfox/qpc/measured/sfox_1.dat =====
SZ4HY RR73; UO1CC <UX7WF> +04
HP8PS RR73; B6F <UX7WF> -23
PR3WU RR73; BW1AP <UX7WF> +14
LO3NTG RR73; NO1VOR <UX7WF> -17
VA1R UX7WF RR73
===== END FILE: lib/superfox/qpc/measured/sfox_1.dat ===== */
/* ===== BEGIN FILE: lib/superfox/qpc/measured/probe_args.txt =====
foxcall=UX7WF
key=OTP:327031
changes_tried=138
===== END FILE: lib/superfox/qpc/measured/probe_args.txt ===== */
/* ===== BEGIN FILE: lib/superfox/qpc/measured/observation.txt =====
# One SuperFox transmission of WSJT-X 3.0.2 and what follows from it (measure.py measure).
# messages (sfox_1.dat), Fox call and key as given to sftx and to packprobe:
#   SZ4HY RR73; UO1CC <UX7WF> +04
#   HP8PS RR73; B6F <UX7WF> -23
#   PR3WU RR73; BW1AP <UX7WF> +14
#   LO3NTG RR73; NO1VOR <UX7WF> -17
#   VA1R UX7WF RR73
foxcall=UX7WF
key=OTP:327031
# the 151 tones sftx wrote to sfox_2.dat (tone 0 = sync, tone v+1 = symbol value v):
tones=0 0 69 0 108 35 0 57 44 77 0 28 2 72 102 0 40 98 80 101 91 0 41 65 22 47 82 56 0 4 111 116 85 22 7 101 0 115 0 20 53 0 0 25 0 26 94 0 100 37 20 0 82 17 33 63 0 80 104 55 34 24 0 35 128 119 109 5 3 0 101 82 15 30 71 113 46 0 125 0 120 48 0 0 40 0 53 30 0 89 63 47 57 94 78 7 46 102 9 69 112 71 64 128 123 70 82 37 72 114 92 24 94 119 75 35 50 83 51 88 60 22 5 74 124 60 62 93 48 53 52 105 123 79 61 84 56 72 1 13 112 89 65 3 53 105 27 66 80 22 91
# the 50 message symbols packprobe printed (WSJT-X's sfox_pack, before the encoder):
payload=68 107 13 56 87 31 1 2 15 88 40 24 44 86 52 65 69 46 90 73 9 4 63 94 126 109 57 89 7 112 50 34 77 79 97 84 51 93 47 54 29 66 17 99 37 104 36 123 114 110
# u = transform of (0, the 127 data symbols); non-zero at exactly 50 positions:
u=0 68 107 13 56 87 31 73 1 2 15 9 88 63 126 93 40 44 86 94 65 89 112 17 90 77 84 110 54 0 0 0 24 69 46 50 4 97 47 0 57 29 37 0 114 0 0 0 79 36 0 0 0 0 0 0 0 0 0 0 0 0 0 0 52 109 7 66 34 104 0 0 51 0 0 0 0 0 0 0 99 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 123 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
# information positions, ascending:
info=1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 32 33 34 35 36 37 38 40 41 42 44 48 49 64 65 66 67 68 69 72 80 96
# message symbol k -> position (u[position] == payload[k]):
order=1 2 3 4 5 6 8 9 10 12 16 32 17 18 64 20 33 34 24 7 11 36 13 19 14 65 40 21 66 22 35 68 25 48 37 26 72 15 38 28 41 67 23 80 42 69 49 96 44 27
===== END FILE: lib/superfox/qpc/measured/observation.txt ===== */
#endif
