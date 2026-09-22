# JTDX\_CONTEST by CE3TSK

A fork of **JTDX v2.2.159** (itself derived from **WSJT-X**) with an enhanced FT8
and rebuilt FT4 decoder, and built-in support for the **WW Digi DX Contest**.
The GUI has been repaired throughout and the dark style now works.

Designed, built and measured by **Tihomir Sokcevic, CE3TSK** — Santiago de Chile,
2025–2026 · [https://ce3tsk.com](https://ce3tsk.com) · source code: [https://github.com/ce3tsk/jtdx\_contest](https://github.com/ce3tsk/jtdx_contest)

Version string: `v3.0.0-rc08` · derivative work of JTDX by UA3DJY, ES1JA and the
HF community, WSJT-X by K1JT.

Support this work: https://ko-fi.com/ce3tsk

New versions: **Help → Check for updates...** shows whether a newer version is published and what changed, and
opens the download page. In the background the program asks the same thing once after starting and every 12
hours, and only shows a small update icon beside the Ko-fi cup when there is one. All it fetches is
`https://ce3tsk.com/download/latest.json`; `UpdateCheck=false` in the ini file stops the background check.

---

## The one thing no other FT8 program does

**It keeps decoding through the next period.**

Every FT8 program decodes once, when the period's audio is complete, and then leaves the
processor idle for roughly **twelve of the next fifteen seconds** — whether you are transmitting
or listening. WSJT-X and JTDX trigger one decode and the decoder process then waits; MSHV splits
its FT8 decode into three passes, but all three fall inside the receive period. In every case the
whole decode has to finish before the auto-sequencer picks your reply, so the depth a decoder can
afford is capped by that deadline rather than by the period.

JTDX\_CONTEST cuts the runtime budget **at** the deadline instead of fitting the work inside it:

- an **RX phase**, deliberately cheap — five cycles, sensitivity 2, one ensemble member — which
  finishes about **1.1 s** into the period and hands the sequencer what it needs, exactly as an
  unsplit decoder would;
- a **background phase** on the retained band, running what the first phase could never afford:
  the alternate pass, the ensemble members, a residual pass with every known message subtracted,
  and a plain classic decode of the pristine audio.

Bounded by the deadline a decoder may spend about a second; bounded by the period, about thirteen.
**Listening, the background phase has one period. Transmitting, it has two** — a period you
transmitted in has nothing new to decode, so its decode is skipped and the phase simply keeps
going, close to **29 s** without a break. The deadline never moves and the reply is unaffected
either way. Each unit re-reads the decode-request counter and the lock file before it starts and
aborts *within* a pass when the next decode is due.

Then it pays forward: everything the background finds enters the four-period hint memory, so a
station it recovers late is pinned as 77-bit a-priori knowledge for the RX phases that follow —
the next deadline-bound second goes to stations still unknown instead of re-deriving one already
heard.

Measured on the 104-message reference capture, the split alone carries the decoder from **78 to
102** messages: **+30.8 %**, the largest gain of any single mechanism in this fork. In FT4 the TX
background is worth **+8.3 %** over a baseline that already carries the hint memory. Available in
every preset whose name carries **pipeline**, on FT8 and FT4 alike; off in the plain presets, so a
slow machine is never asked for it.

---

## What it does better

Measured on **240 consecutive on-air 15 s periods** (2026-08-27 16:57–17:57 UTC,
20 m, 100–3100 Hz, 12 threads of an AMD Ryzen 7 5800H), the same recorded audio decoded
by every engine with its own best settings:

| **engine** | **unique messages** | **vs stock JTDX** |
| --- | ---: | ---: |
| **JTDX\_CONTEST · Pipeline ensemble full** | **6 208** | **+27.5 %** |
| JTDX\_CONTEST · Pipeline ensemble | 6 194 | +27.2 % |
| JTDX\_CONTEST · recommended preset (*Pipeline max decodes light*, 5.3 s CPU/period) | 6 091 | +25.1 % |
| JTDX\_CONTEST at JTDX's own recipe (9 cycles, sensitivity 2) | 5 249 | +7.8 % |
| WSJT-X 3.0.2 (multi-thread FT8 decoder, 9 passes, sensitivity 3, AP, 12 threads) | 5 171 | +6.2 % |
| WSJT-X improved 3.2.0 (same settings) | 5 163 | +6.0 % |
| **Stock JTDX v2.2.159 without its ALLCALL7.TXT** (9 cycles, sensitivity 2, 12 threads - its honest best) | 4 869 | — |
| Stock JTDX v2.2.159 as shipped (the same with the ALLCALL7.TXT lookup, which rejects 67 real decodes) | 4 802 | −1.4 % |
| WSJT-X 3.0.2 standard decoder (depth 3, AP) | 4 782 | −1.8 % |

The best preset against the three references a reader is likely to run: **+27.5 %** over stock
JTDX v2.2.159 with its ALLCALL7.TXT lookup switched off (the baseline here - stock at its honest
best), **+29.3 %** over stock as shipped, **+20.1 %** over WSJT-X 3.0.2's multi-thread decoder.

FT4, the same test on 240 recorded 7.5 s periods (14.080 MHz, 100–3100 Hz, WW Digi 2026):

| **engine** | **unique messages** | **vs stock JTDX** |
| --- | ---: | ---: |
| **JTDX\_CONTEST · max effort** (0.59 s at reply time) | **1 888** | **+23.8 %** |
| JTDX\_CONTEST · recommended preset (0.10 s at reply time) | 1 881 | +23.3 % |
| WSJT-X improved 3.2.0 (FT4 decoder, depth 3, AP) | 1 688 | +10.7 % |
| WSJT-X 3.0.2 (FT4 decoder, depth 3, AP) | 1 561 | +2.4 % |
| JTDX\_CONTEST without its hint memory (JTDX's FT4 recipe) | 1 552 | +1.8 % |
| **Stock JTDX v2.2.159 without its ALLCALL7.TXT** (single-threaded, 1.40 s a period) | 1 525 | — |
| Stock JTDX v2.2.159 as shipped (the lookup rejects 169 real decodes here) | 1 356 | −11.1 % |

In FT4, max effort is **+23.8 %** over stock without ALLCALL7.TXT, **+39.2 %** over stock as
shipped, **+20.9 %** over WSJT-X 3.0.2 and **+11.8 %** over WSJT-X improved 3.2.0; the busier
73-period day hour gives +10.5 % over the same baseline.

Live, two receivers on one antenna for 56 minutes: **+16.8 %** over the previous
JTDX build. Every number comes from recorded audio and a script that re-runs it;
the suites, the recorded periods and the measurement harness are not part of this
repository - they will be published for download (see **What is not in this repository** below).

### The decoder

- **Two new decoding approaches.** The **alternate pass** re-decodes the residual audio with

the opposite recipe (SWL after plain, plain after SWL) once the normal passes have
subtracted what they can. The **ensemble** re-decodes the same audio through fixed
perturbations (delay, dither, tone shift) and keeps the union — deterministic on
any machine with 2 threads or more.

- **The pipeline** — see *The one thing no other FT8 program does* above. A fast **RX phase**

(5 cycles, sensitivity 2, one ensemble member — 1.1 s per period) decides the answer, and a
**TX background** keeps working on the same audio through the idle time that follows, whether
you are transmitting or listening: one period when listening, two after your own transmission,
because the period you transmitted in has nothing new to decode. Its decodes arrive in time to
be pinned as hints for the next reply. Background units abort cleanly on a band change and never
carry over between periods.

- **Four-period hint memory** (+6 % messages at no measurable cost), the **classic unit**

on pristine audio in the background, **100–3100 Hz** anchored to the full band, the
candidate cap raised 450 → 2000, OSD bit-packing in 64-bit words.

- **Fifteen data races** found and fixed - nine inherited in the FT8 path, four more in FT4
when it was given the same threading in August 2026, and two the fork introduced in its own
pipeline (the emission-order chain of 2026-09-01): the same audio gives the
same decodes run after run, SNRs and provenance markers included. Three of the FT4
four were a slice index passed as a literal `1` to code that keeps per-slice state
(the OSD enumeration, the callsign hash window), and one left every thread re-scrambling
the shared a-priori bit patterns; fixing them recovered decodes as well as making
the output reproducible.

- **FT4 is threaded too** (August 2026), on the same anchored slice grid as FT8, which took its

ensemble from unusable to comfortable: six members cost 39 % of FT4's 1360 ms decoding
budget where they had overrun it at 131 %, and the plain decoder now costs 7 %.

- **16-bit audio, as WSJT-X and JTDX** (2026-09-03). The fork spent its first year on a 32-bit

sample chain inherited from JTDX's `jtdx_32a` branch; 73 on-air periods decode identically
at 16 bit, the codec delivers no more, and the wider chain cost double shared memory,
double disk per saved period and an audio format other platforms' backends can refuse.
Reverted to upstream's layout; File -> Open reads 16- and 32-bit recordings alike.

- **Cleaner output.** The 2024 `ALLCALL7.TXT` known-call lookup — measured to throw away

\~45 *real* decodes an hour for one or two false ones — is switched off in the source;
reports no transmitter can encode (`CE3TSK JW1GPY/R 216`) are rejected at unpack
time; RTTY Roundup exchanges and telemetry, never used in FT8 on HF, are switched
off behind source constants (`LRTTYRU_DECODE`, `LTELEMETRY_DECODE`, `LALLCALL7_FILTER`).

### Presets (Decode → FT8 decoding → Presets)

Six measured tiers, marked in the menu with a coloured dot; the preset in force shows
on the main window as the **Preset** lamp under the **Contest** lamp (`Preset R`,
`Custom` when a control drifts, greyed outside FT8). Everything else lives under
an *expert* submenu.

| **tier** | **preset** | **gain vs stock JTDX (ALLCALL7 off)** | **CPU / period** |
| --- | --- | ---: | ---: |
| best power | Maximum efficiency | +11.3 % | 0.4 s |
| best value | Maximum decodes | +16.6 % | 1.1 s |
| **recommended: best medium effort** | **Pipeline max decodes light** | **+25.1 %** | **5.3 s** |
| best results | Pipeline ensemble | +27.2 % | 12.7 s |
| max effort | Pipeline ensemble full | +27.5 % | 14.0 s |
| most results | Pipeline run | +28.0 % | 13.3 s |

Applying any preset also switches *early start of decoder* off and *wideband DX Call
search* on — the environment every table was measured in.

### WW Digi contest support (Settings → Contest)

- One switch parks 28 settings and eight main-window controls in their contest state,

names the contest in the window title and lights the Contest lamp; everything comes
back as it was when the contest is switched off.

- The exchange `HISCALL MYCALL R GRID` received and transmitted correctly; the SNR goes to

the log's RST fields; no usable grid, no transmission.

- Points `1 + km/3000` from 4-character grid centres, shown next to every decode

(`CQ XQ3SK FF46 * 1p Chile`, `--` when the distance is unknown) and in the live score
bar (`M × P = S`); multipliers are the 2-character fields, per band.

- Autoselect that knows the rules: contest points rank above every other tier, a new field

outranks a new DXCC, rule XI.12 respected (answer, never initiate); several callers
— the most valuable first, then the strongest (Max distance is forced off: the points
tiers already rank by distance bracket).

- A separate contest ADIF log; worked-before, multipliers, highlighting and the score all

come from it. Contest frequencies kept apart from the everyday band plan.

---

### SuperFox DXpeditions (DXpedition → SuperFox mode) - experimental

DXpeditions running WSJT-X 2.7 or later, or MSHV, increasingly transmit **SuperFox**: one signal,
1500 Hz wide, that answers up to nine Hounds at once. JTDX could not read it - the Fox's period
stayed empty. JTDX\_CONTEST receives it (*SuperHound*): with Hound mode on, tick **DXpedition →
SuperFox mode (S-Hound)** - or right-click the Hound button. The Fox's (even) period then goes to the
SuperFox receiver, the odd periods to the FT8 decoder as before, and the Hound button reads
**S-Hound**.

**SuperFox support is experimental.** It has been tested against WSJT-X's and MSHV's receivers on
simulated transmissions and a few recordings, not yet on enough DXpeditions on the air; until there
are real recordings to measure it on, no figures are given for it. Recordings of a working SuperFox
(Save all, both periods) are very welcome: jtdx_contest@ce3tsk.com. How the receiver works - WSJT-X's
receiver first, then everything this program does differently - is described in the **SuperFox
explainer**, published with the other documents at [https://ce3tsk.com](https://ce3tsk.com).

The decoder starts from **WSJT-X 3.0.2's decoder** (by K1JT, K9AN and IV3NWV), ported and checked to
decode what WSJT-X decodes, file by file, and adds, in the order of the release notes:

- **CRC-aided successive-cancellation list decoding (CA-SCL)**, with a list decoder of its own: it keeps
  the most probable paths at every decision and lets the CRC choose among them, so a wrong early
  decision is no longer final. By the way, the new "CRC-aided successive-cancellation list decoding"
  (CA-SCL) method that JTDX\_CONTEST rc08 now uses for SuperFox decoding is a technique used widely in
  5G cell phone networks, i.e. JTDX\_CONTEST is using current, state-of-the-art technological
  approaches for amateur radio.
- **Tone bins that hold QRM are normalised**, so an FT8 caller inside the Fox's band does not swamp it.
- **A-priori decoding at four levels**: messages it can foresee - the Fox's CQ, its answer to you - are
  decoded with that knowledge; **the Hounds you heard** in the odd period constrain the Fox's message
  (the *pool* pass), and when none were heard **the Fox alone** does (the *list* pass) - lines marked
  `*`.
- **The search and the demodulation run in threads**, with the same result at any thread count.
- **FT8 QRM on top of the Fox is taken out** by this program's own FT8 decoder - silently, and only
  when the Fox is not found without it; a quick round first, a deeper one if time allows.
- **MSHV's three sync windows**, as a last step when nothing else has decoded - it only ever adds
  decodes, never delays one (`JTDX_SFOX_SYNC3=0` switches it off).
- **The Fox's code is checked online** (see *Verification* below).
- **A Fox it knows is decoded deeper** - the Fox in DX Call, or the Fox it last decoded; a Fox it does
  not know is accepted down to MSHV's floor.
- Every one of these is a setting, read from the environment (`JTDX_SFOX_...`, `JTDX_SFQRM`); the
  explainer lists them. A value that cannot be read stops the decoder, naming it.

Operating:

- **Call by double-clicking a decoded Fox line.** You stay on your own TX frequency, anywhere from 200
  to 3000 Hz; the program never moves you, and **Lock Tx=Rx** is ignored in this mode.
- **Your TX period is the odd one** and cannot be changed while the mode is on.
- **A Fox that has not been decoded cannot be called** (recently, on this dial frequency) - SuperFox's
  own rule.
- **The receiver finds the Fox by itself**: it looks near 750 Hz and follows where it last decoded it
  (`SuperFoxFreq`, `SuperFoxTol` in the ini file for a Fox announcing another frequency). The **RX
  frequency** box follows the Fox and is greyed while the mode is on, so the marker and the Rx
  Frequency window always show it; your own RX frequency comes back when you leave the mode.
- **A compound Fox call** (`VP2X/K1JT`) arrives only as a hash: type it into DX Call and its lines read
  `K1ABC <VP2X/K1JT> RR73`.
- **Verification**: a signed Fox is checked online and shown as **`VP8PJ verified`** (green) or
  **`VP8PJ invalid`** (red) - for SuperFox and for an old-style Fox's `CALL.123456`. It sends the Fox's
  call, the time and the code to `www.9dx.cc`; switch it off with **DXpedition → Verify Fox online
  (OTP)**. More servers can be listed, best first, in the ini file (`OTPUrl`); a later one is asked
  only when the earlier ones do not know the call. The code lines are hidden unless `ShowOTP=true`.
- JTDX\_CONTEST does **not** transmit SuperFox.

## Building

Building is the same as for original JTDX and WSJT-X, so follow the already existing
instructions. 

Linux dependencies (Ubuntu/Debian names): `cmake`, `gfortran`, `g++`, Qt 5 (`qtbase5-dev`,
`libqt5serialport5-dev`, `qtmultimedia5-dev`, `libqt5websockets5-dev`, `qttools5-dev`),
`libboost-dev` (≥ 1.63), `libfftw3-dev` (single precision + threads), `libhamlib-dev`,
`libusb-1.0-0-dev`.

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/jtdx-prefix ..
make -j8
```

Two SuperFox tools are built alongside the simulators: `sfoxsim` writes SuperFox test audio
(`sfoxsim 750 0.0 AW VP8PJ 5 1 1 0 1 -15`, run it without arguments for the list; an optional
last argument is the 6-digit one-time code) and `sfrx 750 50 file.wav` decodes wav files. In
the decoder's file mode, `jtdxjt9 -8 -f 750 -o 50 file.wav` does what the GUI does in SuperFox
mode: `-o` is the search range in Hz around `-f`, it switches Hound mode on, and the slot is
taken from the `_hhmmss` in the file name (no time in the name counts as an even slot).

The simulators (`ft8sim`, `ft4sim`, `ft2sim`; run one without arguments for its list) read
three environment switches: `JTDX_SIM_SEED=n` fixes the noise so that a file is the same on every
run, `JTDX_SIM_32BIT=1` writes 32 bit files instead of 16 bit ones, and `JTDX_SIM_GFSK=1` makes
`ft8sim` write the Gaussian-shaped FSK that WSJT-X, JTDX and MSHV transmit. Its default is plain FSK, as it
always was: that makes no difference to a test that decodes one signal, but a test of how well
signals subtract from each other needs the switch (`ft4sim` and `ft2sim` always shape their pulses).

`build/jtdx` (the GUI) and `build/jtdxjt9` (the decoder) must always be deployed
**as a pair** from the same build — the shared-memory parameter block has changed
several times in this fork (and the sample buffer's width once, 2026-09-03) and a
mismatched pair decodes wrongly or not at all. `make install` puts them under the
prefix; `jtdx -r NAME` runs a separate instance with its own `JTDX - NAME.ini`.

**Windows (JTSDK64, MinGW-w64 gfortran 8.1).** One toolchain difference matters for the
threaded decoder: MinGW gcc implements `!$omp threadprivate` with *emulated* TLS, so every
threadprivate variable lives in an exactly sized heap block instead of the static TLS segment
Linux uses. An array overrun that was silent on Linux and in the original JTDX (where the
array was plain static data) becomes heap corruption on Windows. That is what the FT4 crash of
2026-09-05 was: `ft4b.f90` filled the 64-element columns of its threadprivate twiddle table
`ctwk2` through `twkfreq1`, whose loop runs `0..npts` inclusive, with `npts=64` — one element
too many, 8 bytes past the end of the last column — and `jtdxjt9.exe` died with
`STATUS_HEAP_CORRUPTION` (0xC0000374) at the next `malloc`, inside `ft4_downsample`, on the
first FT4 candidate of the first period. gfortran's `-fcheck=bounds` cannot see this class of
bug: the dummy `cb(nbot:ntop)` is explicit-shape and takes its extent from the caller's own
arguments. Fixed by passing the last index, `2*NSS-1`; the 64 stored values are unchanged.
The routine's contract is still the trap (`npts` is a last index, the loop ignores `nbot`, the
dummies are sized by the caller): the header of `lib/twkfreq1.f90` records it, with the deeper
change that closes it and the one way to get that change wrong.

To chase a Windows heap fault in the decoder: reproduce it in file mode (`jtdxjt9 -4 file.wav`,
no GUI needed), run under gdb with `_NO_DEBUG_HEAP=1` in the environment (otherwise the Windows
debug heap changes the layout and the crash disappears), and check the heap at breakpoints with
the CRT's own `_heapchk()` — msvcrt's `malloc` uses a separate CRT heap, so `HeapValidate` on
the process heap reports OK while the CRT heap is already broken.


## What is not in this repository

This repository carries the program: the source, the build files and this README.
Deliberately left out, because they belong to the author's working environment rather
than to the released program:

- the test suites and their recorded audio (the on-air hours, the FT4 night set, the benchmark

captures — several hundred megabytes of it);

- the measurement harness that reproduces the tables above and the design and measurement notes

behind them - to be published for download at [ce3tsk.com](https://ce3tsk.com);

- the packaging and document-building scripts, and the built PDFs, slides and web pages.

The measured claims in this README stand on those runs; the numbers are quoted here,
the apparatus is not shipped with the source.

---

## Documents

| **file** | **what it is** |
| --- | --- |
| `README.md` | this file |

The fork's longer documents — how the FT8 decoder works, every decoder and contest
change with its measurement, the preset ini keys, the contest design, the presentation
and the long-form decoder explainer — are published separately at [https://ce3tsk.com](https://ce3tsk.com)
rather than kept in the source tree.

## Benchmark data

The recorded air behind every decoder figure in those documents is published too, with the
script that replays it: two hours off the band, 240 FT8 and 240 FT4 periods, plus the two
crowded-band files and their truth manifests, at
[https://ce3tsk.com/download/wav/](https://ce3tsk.com/download/wav/). The script
(`jtdxbench.py`, one file, Python 3.6+, Linux/macOS/Windows) drives the standalone `jtdxjt9`
over a suite preset by preset and prints the decodes, the per-period seconds and how many
periods finished inside the mode's reply deadline, beside the reference machine's numbers —
so a claim can be checked, and a machine can be measured before choosing a preset for it.

---

## Upstream: JTDX v2.2.159

JTDX is derived from WSJT-X and facilitates amateur radio communication using extremely
weak signals; it runs on Windows, macOS and Linux. The upstream fork this tree started
from carries these recent updates:

Resources: upstream project files [https://sourceforge.net/projects/jtdx/files/](https://sourceforge.net/projects/jtdx/files/)

---

If JTDX\_CONTEST gave you a QSO you would otherwise have missed, a coffee keeps the
benchmarks running. Support this work: https://ko-fi.com/ce3tsk

---

## Thanks

The first release candidates were put on the air by a small group of operators who found the
problems a benchmark cannot. Thank you:

- **Willy XQ3SK**
- **Eduardo CA3EAP**
- **Miguel CA7FKZ**
- **Cristian CA8CEU**

## Copyright and license

- Copyright © 2001–2026 Joe Taylor, K1JT (WSJT-X)
- Copyright © 2016–2026 Igor Chernikov, UA3DJY and Arvo Järve, ES1JA (JTDX), and the HF

community

- Copyright © 2025–2026 Tihomir Sokcevic, CE3TSK (this fork: the decoder work, the contest

support, the benchmark measurements and the documents)

Open source under the **GNU General Public License, version 3** (see `COPYING`),
as JTDX and WSJT-X.

**The SuperFox code's definition is measured, not copied.** `lib/superfox` is WSJT-X's, GPL v3,
with one exception that is NOT in this program: WSJT-X's `lib/superfox/qpc/qpc_n127k50q128.c`,
60 lines that say which 50 of the polar transform's 128 positions carry the message and in what
order. Its author, Nico Palermo IV3NWV, reserves those tables - published so that SuperFox
transmissions "can be decoded by anybody", written authorization asked for before they are used
in a derived work. He was asked on 2026-09-10 and again afterwards and has not answered either
way. JTDX\_CONTEST therefore does not ship that file. Its `lib/superfox/qpc/qpc_measured.c`
holds the same definition as MEASURED from the tone sequence of one transmission of WSJT-X
3.0.2's own transmit tool (written to a file; nothing went on the air): the numbers are
properties of what a SuperFox transmitter sends - every receiver of the mode must use exactly
these - and the file says how they were read off it, with the programs and the record kept beside it
(`lib/superfox/qpc/measured/`) and again inside it. No claim of any kind is made on the numbers
themselves. JTDX\_CONTEST uses them to receive SuperFox and, in its test tool `sfoxsim`, to
write test audio; it does not
transmit the mode.

**Patent notice for the SuperFox list decoder (CA-SCL).** The source code is available under the GNU
GPL v3 and is intended for educational, simulation and research purposes. CRC-aided
successive-cancellation list (CA-SCL) decoding of polar codes (`lib/superfox/qpc/qpc_scl.c`, called
from `lib/superfox/qpc_decode2.f90`) is used in 5G; deploying it in commercial telecom networks may
require Standard Essential Patent (SEP) licensing from the appropriate 3GPP patent pools. This notice
is information only and adds no restriction to the GPL.

The project welcomes contributions from those with programming,
documentation or other relevant skills interested in supporting amateur radio development.

73 de CE3TSK
