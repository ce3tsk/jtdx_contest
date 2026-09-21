# JTDX\_CONTEST by CE3TSK

A fork of **JTDX v2.2.159** (itself derived from **WSJT-X**) with an enhanced FT8
and rebuilt FT4 decoder, and built-in support for the **WW Digi DX Contest**.
The GUI has been repaired throughout and the dark style now works.

Designed, built and measured by **Tihomir Sokcevic, CE3TSK** — Santiago de Chile,
2025–2026 · [https://ce3tsk.com](https://ce3tsk.com) · source code: [https://github.com/ce3tsk/jtdx\_contest](https://github.com/ce3tsk/jtdx_contest)

Version string: `v3.0.0-rc07` · derivative work of JTDX by UA3DJY, ES1JA and the
HF community, WSJT-X by K1JT.

Support this work: https://ko-fi.com/ce3tsk

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

### SuperFox DXpeditions (DXpedition → SuperFox mode)

DXpeditions running WSJT-X 2.7 or later, or MSHV, increasingly transmit **SuperFox**: one
constant-envelope signal, 1512 Hz wide, that carries reports or RR73s for up to nine Hounds at
once. JTDX could not read it - the Fox's slot simply stayed empty. JTDX\_CONTEST receives it
(*SuperHound*): with Hound mode on and **DXpedition → SuperFox mode (S-Hound)** ticked - or a
right click on the Hound button - the Fox's slot, the even one (00 and 30 s), goes to the
SuperFox receiver and the odd slots to the FT8 decoder as before. The Hound button then reads
**S-Hound**.

The receiver is WSJT-X 3.0.2's (`lib/superfox`, by K1JT, K9AN and IV3NWV), ported and tested
to print the same decodes as WSJT-X's own `sfrx`, file by file: half of the
transmissions decode near −16.7 dB on a quiet channel and near −15.4 dB on a mid-latitude
fading one. What to know about it:

- **Tone bins that hold QRM are normalised - the one step of the receiver that is not WSJT-X's.**
  The receiver weighs all 128 tone bins with one noise level. An FT8 caller at +10 dB is two
  thousand times the noise in its own bin, where the Fox is five to twenty times - that bin wins
  every symbol it is keyed in - and the symbol FFT leaks it over the whole band, so every other
  bin's noise level is wrong as well. A bin whose mean power over the transmission is more than
  twice the noise's is therefore divided by that mean. On a band without QRM no bin qualifies and
  nothing changes: 360 test transmissions around the threshold, the same ones decoded as by
  WSJT-X's receiver, to the last one; and on 78 bands without a Fox nothing is printed. With QRM
  it is the difference in the figures below, and the Fox under three callers is printed after
  0.06 s instead of 0.7. `JTDX_SFOX_NORM=0` in the environment switches the step off.
- **A Fox whose call is known is decoded a little deeper.** WSJT-X's receiver throws a decode
  away, CRC or not, when its estimated SNR is under −16.5 dB - that is what keeps the chance CRC
  passes of an empty slot off the screen. A decode that carries the call you have in DX Call, or
  the call of the Fox this receiver last decoded above that floor, is kept: in a slot without
  that Fox a chance pass would have to hit the call's 28 bits as well. (With that Fox on the air
  just under the threshold a wrong word can carry the right call - the decoder settles the call
  early - and pass the CRC: 2 to 3 in ten million such periods, the order WSJT-X's receiver has
  above its floor too; such a line would show a code that does not verify.) It is worth little -
  7 more decodes in 120 transmissions between −16.5 and −17.5 dB, about 0.15 dB; a Fox that is
  not known is treated exactly as before. `JTDX_SFOX_KNOWN=0` switches it off.
- **A message that can be foreseen is decoded up to 4 dB deeper, and marked `*`.** When the search
  has found nothing, the receiver tries - for a Fox it knows, as above - the messages it can state
  almost whole, as FT8's a-priori decoding states `MyCall DxCall`: the Fox's **CQ** (with its grid,
  from DX Grid or remembered from the last CQ this receiver decoded: half of them decode at
  −21 dB instead of −17; without the grid −17.5), its answer to **you and nobody else** - your
  report while you are calling, RR73 once you have sent your R+report, −20.7 and −21.4 dB - your
  report or RR73 beside up to four reports to others (−18.4 to −19.4 dB), and messages of
  **nothing but reports** (−18.5 to −19.2 dB); on a mid-latitude fading channel, where the plain
  receiver stands at −15.4 dB, the CQ and the answer to you alone reach −20. As in FT8, what
  concerns your own call is tried only while a transmission of yours is no more than two minutes
  old and the QSO expects that answer - and only from the Fox in DX Call. It has to be whole
  messages: the code's decoder settles its least reliable symbols first, the Hound slots hold
  eight of the nine worst, and with one of them left open the rest of what is known is worth half
  a dB - so **a busy Fox, with RR73s to others or a full message, gains nothing here**; that takes
  the Hounds' calls, which the receiver does not collect yet. The pass is at most 44 decoder calls
  against the search's 796, in each of the one to three looks the receiver takes at a slot.
  Against false lines every stated bit must stand in the decoded word and its SNR estimate must
  reach a floor of its own; measured, a false line would take 2·10⁹ periods of noise, 5·10⁷ under
  FT8 signals - and about 2 million periods of *that Fox on the air and not decoded*, because a
  nearly right guess makes a nearly right word and only the 21-bit CRC parts them (FT8's a-priori
  decodes stand on 14 bits). Such a line would show the right Fox with a Hound, a report or your
  own call that it did not send; the one that would matter - a false RR73 to you after your
  R+report, which logs a QSO that was not completed - about once in a million periods spent
  waiting for the RR73 of a Fox too weak to decode. A Fox this receiver remembers is forgotten
  after four of its periods without a decode of it, and at once on a band or mode change, as the
  FT8 hint lists are.
  **None of this is final.** The figures come from simulated transmissions and two on-air
  recordings, so until there is more on-air material the recipe is a set of settings, read from
  the environment when the decoder starts (a value that cannot be read stops it, by name):
  `JTDX_SFOX_AP=0` the pass off; `JTDX_SFOX_APFAM=cmor` the families tried (**c** the CQ, **m** the
  answer to me alone, **o** me beside others, **r** reports only); `JTDX_SFOX_APFLOOR1=-20.5` and
  `JTDX_SFOX_APFLOOR2=-18.7` the floors of whole and of partial messages; `JTDX_SFOX_APLOOKS=2`
  how many forms of the spectra are tried; `JTDX_SFOX_APAGE=4` how many Fox periods a Fox is
  remembered; and for the bin normalisation above `JTDX_SFOX_NORMX=2.0`, its factor.
- **A busy Fox is decoded about 1.5 dB deeper when you have heard its Hounds (also marked `*`).**
  The pass above has nothing for a Fox whose message is full of calls it cannot foresee. But in
  S-Hound mode the odd periods go to this program's FT8 decoder, so it hears who calls and who
  answers the Fox in DX Call - and those are the stations the Fox's next messages name. Their
  calls form a *pool*. When the search and the pass above have found nothing, the receiver runs
  a **list decoder** of its own (CRC-aided successive-cancellation list decoding: the 16 most
  probable paths are kept where WSJT-X's decoder keeps one) with the Fox told and every Hound slot
  of the message held to *one of the pool, your own call while your QSO runs, or empty*. It needs
  no order, no placement and no guess at the message. Measured with a Fox working six Hounds, all
  of them among 16 calls heard: half of the transmissions decode at −18.3 dB instead of −16.7, and
  on a mid-latitude fading channel at −17.3 instead of −15.3. A Hound the Fox works that you did
  *not* hear: the pass finds nothing and costs nothing - the ordinary search stands as before.
  Two list decodes, about 10 ms, each time the receiver has looked at a period and found nothing
  (up to three times a period when FT8 signals are taken out, see below). Against false lines -
  which here would show calls that *were* heard - the CRC is tried on the best path only (with
  the slots held to candidates every decode on record had the right path first), the message must
  be a standard one, and its SNR estimate must reach −19.5 dB; nothing was printed in 300 periods
  of noise and 200 of another Fox. Your own call is a candidate by the QSO's rule, slot by slot:
  for a *report* once you have called, for *RR73* only after your R+report - "RR73 to me" while
  you are only calling is a line no Fox can send, and it is not decoded. A Hound leaves the pool
  when it has been absent from four odd periods *that were listened to* - while you call in every
  odd period you hear nobody, and the pool keeps what it had; decoding a period again (the Decode
  button) counts as nothing. The pool is emptied on a band or mode change, when DX Call changes
  and when S-Hound mode is left. Settings, as provisional as the others: `JTDX_SFOX_POOL=0` the
  pass off, `JTDX_SFOX_POOLL=16` the list size, `JTDX_SFOX_POOLLOOKS=2` the forms of the spectra
  tried, `JTDX_SFOX_POOLCRC=1` the paths tried against the CRC, `JTDX_SFOX_POOLFLOOR=-19.5`,
  `JTDX_SFOX_POOLAGE=4` - every figure here comes from simulated transmissions, and the defaults
  will be looked at again with the first recordings of a working SuperFox. A value that cannot
  be read - not a plain number, out of range, longer than 32 characters - stops the decoder with
  a message that names it; it is never skipped.
- **FT8 signals on top of the Fox are taken out - by this program's own FT8 decoder.** A
  DXpedition works on a frequency of its own, so what lies on top of a SuperFox is mostly Hounds
  calling in the Fox's period - and three of those at the Fox's own strength are enough to make
  it unreadable. WSJT-X and MSHV decode and subtract such FT8 signals before they look for the
  Fox. JTDX\_CONTEST looks for the Fox in the band as received first (0.02 s when it is there).
  Only when that finds nothing does its own FT8 decoder go over the Fox's 1.5 kHz, **silently** -
  nothing of it is printed: first the plain recipe, three cycles, which takes a few tenths of a
  second, so that a Fox rescued this way is still reported **before the period ends and your
  reply goes out on time**; and, if the Fox is still not there, SWL mode with five cycles and the
  alternate pass. After each round every FT8 signal decoded is subtracted from the band as
  received - its timing refined by what the subtraction leaves, as WSJT-X does - and the SuperFox
  receiver runs again. The four-level hint memory, which these decodes keep filled from one Fox
  slot to the next, serves both rounds. Measured with simulated Fox transmissions at −10 and
  −13 dB under three and ten callers at 0 and +10 dB in the Fox's period (80 of them): WSJT-X's
  receiver, which removes FT8 first, decodes 67; this receiver as WSJT-X wrote it and without the
  FT8 step, none; with the bin normalisation alone 31; **with both 79**. Under nineteen different
  recorded FT8 bands (114): WSJT-X's 83; ours 67, 80 with the normalisation alone, **89 with
  both**. The two steps are complements: the normalisation deals with a few callers at once and
  with what the subtraction leaves behind, the FT8 step with a band full of them. (The second,
  deeper round runs only if it can be expected to end inside the RX budget - a DXpedition's
  frequency passes, a crowded band does not. `JTDX_SFQRM=1` in the environment drops it,
  `JTDX_SFQRM=0` the whole step.)
- **A transmission that decodes takes about 0.02 s.** A slot with nothing to decode used to cost
  up to 2 s of searching; it costs at most 0.4 s now (the search is WSJT-X's, unchanged in what it
  tries and in what order - 62 % of its time was a sort that only ever served to find a median).

- **The receiver finds the Fox by itself.** It looks for the Fox's lowest tone at 750 Hz, 50 Hz
  either side, and then follows where it last decoded it (`SuperFoxFreq` and `SuperFoxTol` in
  the ini file, for a Fox that announces another frequency). After two of the Fox's periods
  without a decode it looks at 750 Hz and at the last place in turn, and when you move the dial
  by more than those 50 Hz it starts over at 750 Hz. The RX frequency box plays no part
  in this: it is put on the Fox when the mode comes on so that the marker shows where the signal
  is, and it may be moved, or dragged away by the end of a QSO, without the Fox being lost.
  **Lock Tx=Rx** is ignored while the mode is on - a double click on a Fox line would otherwise
  put your transmitter on top of the Fox.
- **Call by double-clicking a decoded Fox line.** You stay on your own TX frequency for the
  whole QSO - anywhere from 200 to 3000 Hz, not necessarily above 1000 Hz - and the program
  never moves you: the Hound TX frequency control is off in this mode, and split operation is
  not required.
- **Your TX period is the odd one (15 and 45 s) and cannot be changed**: a SuperFox transmits in
  the even periods and nowhere else. The TX minute button shows `TX 15/45` greyed while the mode
  is on; what you had chosen before is put back when you leave it, and is what the settings keep.
- **A Fox that has not been decoded cannot be called.** The transmission is halted and the
  status bar says why. This is SuperFox's own operating rule: a Hound that cannot hear the Fox
  has no chance of a QSO and only adds QRM. "Decoded" means within the last five minutes and on
  this dial frequency: a Fox heard this morning, or before you moved, does not count.
- **Is it really them?** A DXpedition can sign its transmissions with a one-time code (keys are
  issued by the NCDXF). A SuperFox carries the code in every transmission, an old-style
  multi-stream Fox sends it now and then as the free text `CALL.123456`. In Hound mode
  JTDX\_CONTEST asks the verification server `www.9dx.cc` and shows the answer as a line at the
  transmission's time and frequency - **`VP8PJ verified`** on green or **`VP8PJ invalid`** on red,
  and the Hound button turns red for a minute on *invalid*. An unsigned Fox (code 000000) is not
  asked about and gets no line; a callsign the server does not know, or no connection, is said
  once in the status bar. What is sent: the Fox's callsign, the time and the code - switch it off
  with **DXpedition → Verify Fox online (OTP)**. The lines that carry the code are hidden unless
  `ShowOTP=true` in the ini file; they, and the verdict, are written to ALL.TXT like every other
  decode, that is when *write decoded messages* is on. A Fox is asked about again if the question
  failed (no network, no answer in time); a replayed recording is asked about only if its file
  name says when it was made (`yymmdd_hhmmss.wav`).
- **More than one verification server.** `www.9dx.cc` holds the keys the NCDXF issues. A station
  may be registered with another provider instead - `hamdx.org` describes itself as an alternate
  one and speaks the same protocol (MSHV ships both addresses). To have it asked too, put a list
  into the `[Common]` section of the ini file, program closed, best server first:
  `OTPUrl=https://www.9dx.cc, https://hamdx.org`. The list is a **ranking by trust, not a pool**:
  a server is asked only when every server before it answered that it *does not know the
  callsign*. An earlier server's verdict is final either way, and so is its failure to answer -
  otherwise registering somebody else's callsign at the weakest provider on the list would buy a
  "verified" whenever the better one objected or could not be reached. A verdict from any but the
  first server names its server: **`VP8PJ verified (hamdx.org)`**. A later server sees only the
  callsigns, times and codes of Foxes the earlier ones do not know. The default is `www.9dx.cc`
  alone; who runs `hamdx.org`, and how it checks who may register a callsign, its site does not
  say - what its verdict is worth to you is yours to judge.
- **A Fox with a compound callsign** (`VP2X/K1JT`) sends only a hash of its call, and a busy one
  never sends the one transmission that carries it in full - so its lines read `K1ABC <...> RR73`.
  **Type the callsign into the DX Call box**: from the next Fox transmission on the lines read
  `K1ABC <VP2X/K1JT> RR73`, and from then on the Fox can be called (the rule above counts a Fox
  as decoded once it has been read under its callsign).
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

The project welcomes contributions from those with programming,
documentation or other relevant skills interested in supporting amateur radio development.

73 de CE3TSK
