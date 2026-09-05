# JTDX\_CONTEST by CE3TSK

A fork of **JTDX v2.2.159** (itself derived from **WSJT-X**) with an enhanced FT8
and rebuilt FT4 decoder, and built-in support for the **WW Digi DX Contest**.

Designed, built and measured by **Tihomir Sokcevic, CE3TSK** — Santiago de Chile,
2025–2026 · [https://ce3tsk.com](https://ce3tsk.com) · source code: [https://github.com/ce3tsk/jtdx\_contest](https://github.com/ce3tsk/jtdx_contest)

Version string: `v3.0.0-rc02` · derivative work of JTDX by UA3DJY, ES1JA and the
HF community, WSJT-X by K1JT.

Support this work: https://ko-fi.com/ce3tsk

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

- **The pipeline.** A contest reply must be decided before the transmitter keys, so the

decoder is split in two: a fast **RX phase** (5 cycles, sensitivity 2, one ensemble
member — 1.1 s per period) that decides the answer, and a **TX background** that
keeps working on the same audio through your own transmission, where the CPU used
to sit idle. Its decodes arrive while you transmit and are ready for the next reply.
Background units abort cleanly on a band change and never carry over between periods.

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

`build/jtdx` (the GUI) and `build/jtdxjt9` (the decoder) must always be deployed
**as a pair** from the same build — the shared-memory parameter block has changed
several times in this fork (and the sample buffer's width once, 2026-09-03) and a
mismatched pair decodes wrongly or not at all. `make install` puts them under the
prefix; `jtdx -r NAME` runs a separate instance with its own `JTDX - NAME.ini`.

## Rig control: use flrig rather than hamlib

JTDX can drive the transceiver directly through hamlib (Settings → Radio → Rig), and that
still works here. For contest operating the recommendation is to let **flrig** own the CAT
link and point JTDX at it instead: Settings → Radio → Rig **FLRig FLRig**, PTT method **CAT**,
split operation **Rig** or **Fake It** as you use today, with flrig started first. Reasons:

- one CAT connection for everything - flrig talks to the radio, and JTDX, the logger and
  any other program share it over its XML-RPC interface, so nothing fights over the serial
  port;
- the link survives a JTDX restart or a rig power cycle without a reconfiguration - flrig
  reconnects, JTDX only asks it for frequency and PTT;
- PTT every 7.5 s in FT4 and the frequency polling behind the waterfall go through flrig's
  own rig driver, which is maintained per model and is easier to tune (poll intervals, PTT
  line, band-change behaviour) than hamlib's generic backend settings inside JTDX;
- a CAT problem shows up in flrig's window, separately from the decoder and the contest
  logic, which is where you want it during a run.

hamlib remains a build dependency (`libhamlib-dev`) and the direct rig entries stay in the
menu; nothing in the fork depends on which of the two you choose.

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

## Copyright and license

- Copyright © 2001–2026 Joe Taylor, K1JT (WSJT-X)
- Copyright © 2016–2026 Igor Chernikov, UA3DJY and Arvo Järve, ES1JA (JTDX), and the HF

community

- Copyright © 2025–2026 Tihomir Sokcevic, CE3TSK (this fork: the decoder work, the contest

support, the measurements and the documents)

Open source under the **GNU General Public License, version 3** (see `COPYING`),
as JTDX and WSJT-X. The project welcomes contributions from those with programming,
documentation or other relevant skills interested in supporting amateur radio development.

73 de CE3TSK
