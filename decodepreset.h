#ifndef DECODEPRESET_H
#define DECODEPRESET_H

/* CE3TSK: FT8 decoding presets - named recipes over the settings that were measured to
   matter (DECODER_BENCHMARK_PLAN.md steps 6-13, ENSEMBLE_DECODE_PLAN.md,
   PIPELINED_DECODE_PLAN.md, DECODE_RECIPE_PLAN.md; re-swept after each decoder change). A
   recipe has two phases: the RX phase (Decode -> FT8 decoding -> RX: the period's own
   decode, whose result is on the table when the reply is chosen) and the TX background
   phase (-> TX background: the same controls again, applied to the retained band in the
   decoder's idle time after <DecodeFinished>, until the next decode is due). Measured on
   the benchmark capture at 100-3100 Hz (DECODE_RECIPE_PLAN.md, 2026-08-26), 12 threads,
   deterministic, the same message set at every thread count; file-mode times, the GUI's
   persistent decoder is about 0.5 s faster:

     Default        3 cycles                                   70 messages, 4 of 17 weakest, 0.95 s
     MaxEfficiency  5 cycles, sensitivity 2 - the knee of the  76 messages, 7 of 17 weakest, 1.25 s
                    decodes-per-second curve: +6 for 0.3 s,    (43 -> 51 on the -3 dB capture);
                    everything beyond costs far more per       no member, no passes
                    message
     MaxDecodes     5 cycles, sensitivity 2 (low thresholds +  87 messages, 10 of 17 weakest, 2.4 s
                    subpass), 1 ensemble member from 8 threads (76 / 7 in 1.25 s without the member)
     PipelineMaxDecodesLight  RX: Max decodes (87 / 10 at 2.4 s); TX background: the plain
                    6-cycle pass (the classic decode, ~0.5 s), members 2 and 3, the
                    residual pass - 94 in all, ~6.3 s of background (P10, DECODE_RECIPE_PLAN.md
                    section 12): the light pipeline for a 15 s cycle with TX
     Ensemble       SWL-5 + alternate pass + ensemble members  97 messages with 3 members, 13 weakest
                    by the thread count (3 / 2 / 1 from 12 /   (9.1 s) - no time budget: max effort RX only, no TX background
                    6 / 3 threads)                             without TX
     PipelineEnsemble  RX: SWL-4 + 1 member from 8 threads (86 / 8 at 2.8 s); TX background:
                    SWL-5 + alternate pass + background effort 5 (with the RX member and the
                    member table's order, every unit that ever found anything) + residual
                    pass: 100 in all - every message any recipe finds on the range - in
                    ~16.5 s of background
     PipelineEnsembleFull  the same RX phase with the background at effort auto: every
                    member sample including the two that never found a message, ~18.6 s of
                    background for the same result on every capture measured
     PipelineRun    RX: 5 cycles, sensitivity 2 (76 / 7 at 1.25 s, the reply decided before
                    TX keys); the same TX background: 97 in all
     (both phases of every preset, with times: test/decode/preset_data.tsv)
     The pipeline backgrounds also run the *classic unit* (P9, DECODE_RECIPE_PLAN.md section
     11): the plain 6-cycle non-SWL decode, the pass sequence no SWL unit runs. Measured on
     207 on-air periods beside the old plain decoder: the pipeline finds 270 messages the
     6/9-cycle decoder does not, the 6-cycle decoder 11 the pipeline does not; as a unit in
     the pipeline background it adds 6 (5 real, one false decode) in 6 periods for 0.3-0.5 s
     and loses nothing. The old decoder's other extra finds were JTDX's DX-call search on a
     clicked station.

   The sweep behind this (140 runs): sensitivity 2 is the cheapest gain in the menu (+6 for
   0.2 s; inert under SWL mode, whose decoder sets its own thresholds); OSD order 2 is no
   weak-signal lever on a crowded band (beaten at equal cost, behind on a -3 dB capture);
   one member (a 128-sample delay, decoded SWL-5) adds 9-11 messages for 1.2-1.7 s to any
   base and is the best second second; on a sparse band every recipe finds the same
   messages. So the old MaxDecodes (SWL-6, 81 in 7.4 s), WeakSignals (5 cycles + OSD, 79 in
   4.9 s) and Exhaustive (SWL-5 + alt, 84 in 3.1 s) presets are all dominated by one recipe
   and were retired. The pipeline keeps an SWL RX phase: that is what reaches the range's
   ceiling with the background (a plain-cycles RX phase ends at 97). Since the OSD speedups
   of step 13 every preset fits the 15 s period from 2 threads up, so the extra passes are
   included from TWO_PASS_MIN_THREADS / ALT_PASS_MIN_THREADS = 2; the RX member from
   RX_MEMBER_MIN_THREADS = 8 (2.4 s at 8 threads, 2.9 s at 6, 4.3 s at 3); the Ensemble
   preset's members follow ensemble_members(). The sensitivity (1 = low thresholds) and the
   RX frequency sensitivity (2 = medium) are JTDX's defaults where not stated; "Aggressive"
   is not part of any recipe, the FT8 decoder does not read it.

   A preset is applied by setting the ordinary controls of both phases, so what persists
   is the controls' own settings; the preset shown is derived from them, Custom when they
   match none. */
enum class DecodePreset { Default, MaxEfficiency, MaxDecodes, PipelineMaxDecodesLight, Ensemble, PipelineEnsemble, PipelineEnsembleFull, PipelineRun, Custom };

/* one phase's recipe: the controls of the RX submenu, or of the TX background submenu */
struct DecodePhase
{
  bool swl;          // SWL mode (the 6 pass decoder), uses swl_cycles
  int cycles;        // FT8 decoding cycles 3-9, used when swl is off
  int swl_cycles;    // SWL decoding cycles 3-9, used when swl is on
  int sensitivity;   // decoder sensitivity: 0 normal, 1 low thresholds, 2 plus subpass (no effect while swl is on)
  int rxf_sens;      // QSO RX frequency sensitivity 1-3
  bool deep_osd;     // OSD order 2 for every candidate
  bool two_pass;     // second slicing pass (threads > 1)
  bool alt_pass;     // alternate-approach pass, 7 cycles + OSD order 2 on the subtracted band (threads > 1)
  int ensemble;      // ensemble members: RX phase the resolved count 0-5; TX background -1 auto (as the budget allows), 0-5
  bool operator== (DecodePhase const& o) const
  {
    return swl == o.swl && (swl ? swl_cycles == o.swl_cycles : cycles == o.cycles)   // only the cycle count in use
      && sensitivity == o.sensitivity && rxf_sens == o.rxf_sens && deep_osd == o.deep_osd
      && two_pass == o.two_pass && alt_pass == o.alt_pass && ensemble == o.ensemble;
  }
};

struct DecodeRecipe
{
  DecodePhase rx;    // the period's decode
  bool background;   // the TX background phase runs (pipeline ensemble)
  DecodePhase bg;    // its recipe (compared only while background is on)
  int bg_classic;    // P9: the classic background unit - a plain non-SWL decode with this many cycles, 0 off (compared only while background is on)
};

constexpr int TWO_PASS_MIN_THREADS = 2;
constexpr int ALT_PASS_MIN_THREADS = 2;
constexpr int ENSEMBLE_MAX_MEMBERS = 5;
/* P12: what applying any preset sets besides its recipe. Early start of the decoder off -
   the presets are measured on the whole period's audio (decode trigger at half-symbol 49,
   not 48), and the pipeline timing (DECODE_RECIPE_PLAN.md) assumes it; wideband DX Call
   search on - the DX-call AP search over the whole decode range, the section 11 finding
   (the -24 dB CQ caller the side-by-side instance kept "finding"). Neither is part of the
   recipe, so neither takes part in recipe_preset()'s identification: a hand change of
   either leaves the preset radio where it is. */
constexpr bool PRESET_EARLY_START = false;
constexpr bool PRESET_WIDE_DXCALL_SEARCH = true;

/* P13: the preset lamp under the Contest lamp (labelPreset) reads "Preset X" - one letter per
   preset, in the submenu's order: 3 default (3 cycles), P best power (max efficiency), V best
   value (max decodes), R recommended (pipeline max decodes light), E ensemble, B best results
   (pipeline ensemble), M max effort (pipeline ensemble full), O most results (pipeline run);
   the C of the table is not shown - the lamp reads "Custom" then. The lamp's background is the dot colour of the marked presets - the same table
   markRecommendedPresets() paints the dots from - and plain bold text in a grey box, no
   background and not greyed, for the unmarked ones (3, E, Custom). Outside FT8 the lamp is
   greyed whatever the controls say - the presets are FT8 decoding presets. */
constexpr char PRESET_LETTERS[] = "3PVREBMOC";
inline char preset_letter (DecodePreset p) { return PRESET_LETTERS[static_cast<int> (p)]; }
inline char const* preset_colour (DecodePreset p)
{
  switch (p) {
    case DecodePreset::MaxEfficiency:           return "#8d99ae";   // grey
    case DecodePreset::MaxDecodes:              return "#f57c00";   // orange
    case DecodePreset::PipelineMaxDecodesLight: return "#3cb043";   // green
    case DecodePreset::PipelineEnsemble:        return "#2f6fd6";   // blue
    case DecodePreset::PipelineEnsembleFull:    return "#d32f2f";   // red
    case DecodePreset::PipelineRun:             return "#8e44ad";   // purple
    default: return nullptr;
  }
}
constexpr int ENSEMBLE_AUTO = -1;
constexpr int ENSEMBLE_BUDGET = -2;      // P8: RX members while the next one's learned cost fits the RX budget (FT8RXBudget)
constexpr int SENS_LOW_THRESHOLDS = 1;
constexpr int SENS_SUBPASS = 2;          // low thresholds + subpass: the plain-cycles recipes' setting
constexpr int RX_MEMBER_MIN_THREADS = 8;
constexpr int RXF_MEDIUM = 2;
constexpr int CLASSIC_CYCLES = 6;        // P9: the classic background unit's cycles - measured on air: +6 messages in 207 periods for ~0.4 s (alone, 6 cycles beat 9)

/* the decoder's own rule for "FT8 threads = auto" (decoder.f90), from the core count */
inline int auto_ft8_threads (int cores)
{
  if (cores <= 1) return 1;
  if (cores < 5) return cores - 1;
  if (cores < 9) return cores - 2;
  if (cores < 16) return cores - 3;
  if (cores < 21) return cores - 4;
  if (cores < 30) return cores - 5;
  return 24;
}

/* effective decoder thread count from the FT8 threads setting (0 = auto) and the core count */
inline int effective_ft8_threads (int setting, int cores)
{
  if (setting > 0) return setting < cores ? setting : cores;
  return auto_ft8_threads (cores);
}

/* RX-phase ensemble members the thread count allows while the decode stays well inside the
   period (measured at 12 threads: 2.6 s base + 1.2 + 1.1 + 2.6 s for the three members) */
inline int ensemble_members (int threads)
{
  if (threads >= 12) return 3;
  if (threads >= 6) return 2;
  if (threads >= 3) return 1;
  return 0;
}

/* the stored classic-unit cycle counts readSettings accepts (key FT8BgClassicCycles): off, or a
   decoder cycle count 3-9 - anything else falls back to CLASSIC_CYCLES */
inline bool valid_classic_cycles (int n)
{
  return n == 0 || (n >= 3 && n <= 9);
}

/* the stored RX ensemble effort values readSettings accepts: budget auto, auto, off, or a
   member count - anything else (an old or corrupt ini) falls back to off. The rule lives
   here so the harness can pin it: a validation predating ENSEMBLE_BUDGET once silently
   reset a stored budget auto to off at startup. */
inline bool valid_ensemble_effort (int e)
{
  return e == ENSEMBLE_BUDGET || (e >= ENSEMBLE_AUTO && e <= ENSEMBLE_MAX_MEMBERS);
}

/* CE3TSK: FT4 counts ensemble members exactly as FT8 does - same concept, same names. Key
   FT4EnsembleEffort, 0 = off, 1..FT4_ENSEMBLE_MAX_MEMBERS members on a candidate the ordinary
   passes gave up on: 1 dither, 2 and 3 the frequency kinds, 4 and 5 the delays, 6 a second
   dither draw (DECODER_IMPROVEMENTS item 49). FT4 has no "auto" or "budget": its period is
   7.5 s and the members are cheap enough to be a plain count. `FT4Dither`, the boolean of the
   days when the dither was the only member, migrates to one member. */
constexpr int FT4_ENSEMBLE_MAX_MEMBERS = 6;

inline bool valid_ft4_ensemble (int n) { return n >= 0 && n <= FT4_ENSEMBLE_MAX_MEMBERS; }

/* CE3TSK item 73: FT4 ensemble members by thread count - the "auto" entry of both ensemble
   menus (FT8's ensemble_members: 3 from 12, 2 from 6, 1 from 3). The FT4 counts were calibrated
   at 12 threads (6 members at reply time = 0.57 s mean / 1.29 s worst against the 1360 ms
   deadline, item 67) and a member's cost scales with the thread count, so the ladder halves
   with it; the background is idle time and keeps at least 3 from 3 threads - its clock cuts
   what does not fit. thread_ladder.f90 carries the Fortran copy (file mode's -M auto / -V auto);
   params_layout.sh pins the two against each other. */
constexpr int FT4_ENSEMBLE_AUTO = -1;
constexpr int FT4_ENSEMBLE_BUDGET = -2;   /* item 80: FT8's P8 for FT4 - as many members as fit the RX budget (FT4RXBudget, 1.3 s), from the band's learned cost; passed to the decoder as it is */
inline int ft4_members_auto (int threads)
{
  if (threads >= 12) return 6;
  if (threads >= 8) return 4;
  if (threads >= 6) return 3;
  if (threads >= 4) return 2;
  if (threads >= 3) return 1;
  return 0;
}
inline int ft4_bg_auto (int threads) { return threads >= 3 ? (ft4_members_auto (threads) > 3 ? ft4_members_auto (threads) : 3) : 0; }
inline int ft4_effort_members (int effort, int threads) { return effort == FT4_ENSEMBLE_BUDGET ? FT4_ENSEMBLE_BUDGET : effort == FT4_ENSEMBLE_AUTO ? ft4_members_auto (threads) : (valid_ft4_ensemble (effort) ? effort : 0); }
inline int ft4_bg_effort_members (int effort, int threads) { return effort == FT4_ENSEMBLE_AUTO ? ft4_bg_auto (threads) : (valid_ft4_ensemble (effort) ? effort : 0); }

/* CE3TSK: FT4's presets, laid out as FT8's are - a preset sets the ordinary controls and the
   menu shows which one the controls currently match, "custom" when they match none. FT4 has
   five knobs: the decoding effort (1 fast, 2 medium, 3 deep - the depth passed as nft4depth),
   the alternate pass over the residual, the RX ensemble members (spent at reply time), deep
   OSD (item 58) and the TX background members (item 59: the members the RX phase did not run,
   decoded in the TX window over the retained band, so they cost nothing at reply time). The
   tiers mirror FT8's preset menu (DECODER_IMPROVEMENTS item 67). */
enum class FT4Preset { Fast, Default, BestPower, Recommended, MaxDecodes, MaxEffort, Custom };

struct FT4Recipe
{
  int depth;      // 1 fast, 2 medium, 3 deep
  bool alt;       // the alternate pass on the residual
  int members;    // RX ensemble members at reply time, 0..FT4_ENSEMBLE_MAX_MEMBERS
  bool deeposd;   // OSD order 2 for every candidate (item 58)
  int bg;         // TX background members - the TOTAL member count reached (item 59); the members above `members` run (item 78: none left = the extras alone)
  int bgdepth;    // item 69: the background phase's effort 1-3 (0 = the RX phase's)
  bool bgdeeposd; // item 69: deep OSD in the background phase
  bool bgalt;     // item 69: the alternate pass in the background phase
  bool bgtwopass; // item 69: the second slicing pass in the background phase
  int sens;       // item 72: decoder sensitivity at reply time, 0 JTDX's thresholds, 1 sync minimum 1.0 + sync quality 16
  bool bgresidual;// item 72: the residual unit in the background phase
  int bgsens;     // item 73: decoder sensitivity in the background phase (the residual scales from it)
  bool twopass;   // item 74: the RX phase's second slicing pass (FT4 needs it on; every preset sets it, as FT8's set theirs)
  bool bgon;      // item 74: TX background decoding on - FT8's explicit switch; item 78: it alone decides whether the phase runs, as FT8's does
  int rxf;        // item 75: QSO RX frequency sensitivity 0-3 (the virtual candidate at the QSO frequency), FT8's RXF_MEDIUM in every preset
  int bgrxf;      // item 75: the same for the background phase
};

inline FT4Recipe ft4_preset_recipe (FT4Preset p)
{
  switch (p)
    {
    /* CE3TSK: measured 2026-09-02 (DECODER_IMPROVEMENTS item 67) on both recorded WW Digi hours
       - 242 periods of the night hour (14.080 MHz, 23:38 UTC on) and 73 of the day hour - idle
       machine, 12 threads, the four-pass default of item 66, file mode's full 0-5000 Hz (the
       GUI's range is narrower, so its times are lower). Messages on the night hour, gained/lost
       (period, message) pairs against the default, and the RX phase's mean / worst wall time
       against FT4's 1360 ms reply deadline:

           recipe                          night   +/-        day    reply time mean / max
           default (deep, 4 passes)        1748     -          546   0.12 / 0.22 s
           background 3                    1826  +82 / -4      557   0.12 / 0.22 s
           background 6                    1856  +114 / -6     559   0.12 / 0.22 s
           deep OSD + background 6         1862  +126 / -12    565   0.27 / 0.65 s
           3 members + background 6        1855  +118 / -11    560   0.34 / 0.75 s
           3 members alone                 1811  +74 / -11     556   0.34 / 0.75 s
           6 members alone                 1842  +108 / -14    560   0.57 / 1.29 s
           6 members + OSD + alt (no bg)   1858  +123 / -13    563   2.47 / 6.02 s  <- over the deadline

       The background does the members' work for free: 6 members in the TX window beat 6 at
       reply time (1856 against 1842) at 0.12 s instead of 0.57. So every tier but the last
       two keeps the RX phase light and spends in the background; "most at reply time" is the
       one that spends members in the period (the message the QSO is waiting for is decoded
       there, the background only ever adds late). The old presets' RX-member ladder (item 49:
       3 the knee, 5 everything worth taking) is superseded by this; the members are still
       there, in the background. "max effort" is the RX-only maximum, kept as FT8 keeps its
       "ensemble" preset - no time budget - and labelled with its cost: on 12 threads it
       overruns the reply deadline every period.

       Item 68, the crowded-band check (item 64's file, seven noise realisations): deep OSD
       adds nothing there at reply time or in total and doubles the reply time, and its gain
       on the sparse hours is 6 of 1856 - so "recommended" is background 6 alone, the recipe
       that is never worse at reply time on either file, and it carries the "best value" mark;
       and six reply-time members are the most at reply time on both files (+2.8 crowded, 1842
       against 1811 on the night hour, 0.57 s mean / 1.29 s worst, one false decode in seven
       crowded runs where the background recipes had none). */
    /* item 69 measured the background phase's own settings (both recorded hours, reply time
       unchanged in every row): against 6 plain background members (1856 night / 559 day),
       + the alternate pass 1864 / 560 at 1.7x the background's CPU, + deep OSD 1866 / 564 at
       2.3x, + both 1875 / 565 at 3.8x - the best figures of the day for nothing at reply
       time; medium effort in the background 1757 (the members' decodes come through OSD) and
       no second slicing 1837 - never. Item 70: "recommended" takes both extras in its
       background phase (about 0.9 s of idle CPU per 7.5 s period on 12 threads); "best power"
       stays plain, its point being the least CPU (3 members + both extras only equal 6 plain
       members at 2x). The bg-phase fields only count when the background is active. */
    /* item 72: the residual unit joins the recommended background (+13/-0 on the night hour,
       ~0.12 s of idle time); the sensitivity switch stays out of every preset - +1.2 % on a
       sparse band at +20 % reply time, -1 on a crowded one - the operator's call in the RX menu */
    /* item 79 (2026-09-04, the re-sweep of TODO.md 2.9 over all sixteen fields - 24 recipes on
       seven noise realisations of the crowded file at 0-5000 Hz and on the 240-period night hour
       at 100-3100 Hz, test/decode/ft4_presets_sweep.md): with item 78 the background can run its
       extras with no member left, and that is what the two reply-time tiers were missing.
       "most at reply time" = six members in the period AND the background's extras (deep OSD,
       alternate pass, residual) in the TX window: 1886 total / 1845 at reply time against the
       old 1842 / 1842, same 0.55 s mean and 1.3 s worst period (crowded: 505 / 498 of 721 =
       498 / 498). "max effort" = everything: six members, deep OSD, alternate pass and low
       thresholds at reply time, the extras and low thresholds in the background: 1891 / 1878
       at 3.1 s mean - above recommended at last (it was 1858, below it), still no budget and
       labelled with its overrun. Recommended stays: its own candidates were +6 (RX low
       thresholds: -8 at reply time on the crowded band), +8 (background low thresholds, +29 %
       idle CPU) and +12 at reply time (RX alternate pass, -2 crowded) - operator options, not
       defaults. Best power stays the least idle CPU per decode (bg 3 plain: 78 gained for
       0.32 s; the extras alone with no members gave 72 for 0.58 s). */
    /* item 73: the member counts that scale with the machine are "auto" (FT4_ENSEMBLE_AUTO) -
       recommended's background (6 here) and most-at-reply-time's members (6 here); best power
       stays a fixed 3 (its point is the least CPU) and max effort a fixed 6 (no budget). The
       background phase's sensitivity stays 0 in every preset until measured. */
    case FT4Preset::Fast:        return {1, false, 0, false, 0, 3, false, false, true, 0, false, 0, true, false, RXF_MEDIUM, RXF_MEDIUM};   // shallow, nothing added
    case FT4Preset::Default:     return {3, false, 0, false, 0, 3, false, false, true, 0, false, 0, true, false, RXF_MEDIUM, RXF_MEDIUM};   // 1748, 0.12 s
    case FT4Preset::BestPower:   return {3, false, 0, false, 3, 3, false, false, true, 0, false, 0, true, true, RXF_MEDIUM, RXF_MEDIUM};    // +4.5 %, reply time unchanged, 3 members of idle CPU
    case FT4Preset::Recommended: return {3, false, 0, false, FT4_ENSEMBLE_AUTO, 3, true, true, true, 0, true, 0, true, true, RXF_MEDIUM, RXF_MEDIUM};   // items 70/72: 1888 night (+8.0 %), 565 day (+3.5 %), reply time unchanged - best value
    case FT4Preset::MaxDecodes:  return {3, false, FT4_ENSEMBLE_AUTO, false, FT4_ENSEMBLE_AUTO, 3, true, true, true, 0, true, 0, true, true, RXF_MEDIUM, RXF_MEDIUM};   // item 79: all six members at reply time (12 threads) + the background's extras with no member left: 1886 / 1845 at reply time, 0.55 s mean
    case FT4Preset::MaxEffort:   return {3, false, FT4_ENSEMBLE_BUDGET, false, 6, 3, true, true, true, 1, true, 1, true, true, RXF_MEDIUM, RXF_MEDIUM};   // item 80: as many members as fit the 1.3 s RX budget + low thresholds at reply time (the RX alternate pass with six members costs 1.5 s on a crowded band, 2.8 s worst on the night: out), everything + low thresholds in the background until 0.5 s before the window ends (the preset sets the margin): 1894 / 1859 at reply time, 0.69 s mean, 1.42 s worst on the night hour
    default:                     return {3, false, 0, false, 0, 3, false, false, true, 0, false, 0, true, false, RXF_MEDIUM, RXF_MEDIUM};
    }
}

/* the recipe the controls describe, at this thread count: "auto" member counts resolve by
   the ladder on both sides (item 73). Item 78: the switch decides whether the phase runs, as
   FT8's does; what it runs is the members above the RX count - none left is a real recipe
   (the phase's extras alone), so the comparison is switch, members run, then the phase's fields */
inline FT4Preset ft4_preset_of (FT4Recipe const& r, int threads)
{
  auto const members_of = [threads] (FT4Recipe const& x) { return ft4_effort_members (x.members, threads); };
  auto const bgm_of = [threads, &members_of] (FT4Recipe const& x) {   // item 78: -1 off, else the members the phase runs; item 80: under budget auto the RX count varies, so the target itself is compared
    if (!x.bgon) return -1;
    int const m = members_of (x), b = ft4_bg_effort_members (x.bg, threads);
    if (m == FT4_ENSEMBLE_BUDGET) return 100 + b;
    return b > m ? b - m : 0; };
  for (auto p : {FT4Preset::Default, FT4Preset::Fast, FT4Preset::BestPower,
                 FT4Preset::Recommended, FT4Preset::MaxDecodes, FT4Preset::MaxEffort})
    {
      auto const t = ft4_preset_recipe (p);
      if (r.depth == t.depth && r.alt == t.alt && members_of (r) == members_of (t) && r.deeposd == t.deeposd && r.sens == t.sens && r.twopass == t.twopass && r.rxf == t.rxf
          && bgm_of (r) == bgm_of (t)
          && (!t.bgon || (r.bgdepth == t.bgdepth && r.bgdeeposd == t.bgdeeposd && r.bgalt == t.bgalt && r.bgtwopass == t.bgtwopass
                                 && r.bgresidual == t.bgresidual && r.bgsens == t.bgsens && r.bgrxf == t.bgrxf)))
        return p;
    }
  return FT4Preset::Custom;
}

/* item 73: the preset lamp in FT4 mode - one letter per preset in the submenu's order: F fast,
   3 default, P best power, R recommended (best value), O most at reply time, M max effort;
   Custom shows as the word (the C is never shown) */
constexpr char FT4_PRESET_LETTERS[] = "F4PROMC";   // 4 = the default (FT4), where FT8's lamp says 3
inline char ft4_preset_letter (FT4Preset p) { return FT4_PRESET_LETTERS[static_cast<int> (p)]; }

/* the marked tiers' colours, FT8's palette (preset_colour): the same meaning, the same dot */
inline char const* ft4_preset_colour (FT4Preset p)
{
  switch (p) {
    case FT4Preset::BestPower:   return "#8d99ae";   // grey
    case FT4Preset::Recommended: return "#3cb043";   // green - the best-value recipe carries the recommended mark
    case FT4Preset::MaxDecodes:  return "#2f6fd6";   // blue
    case FT4Preset::MaxEffort:   return "#d32f2f";   // red
    default: return nullptr;
  }
}



constexpr int FT4_ENSEMBLE_NO_KEY = -99;   /* item 73: readSettings' "no key yet" default, since -1 is "auto" now */
inline int ft4_ensemble_from_settings (int stored, bool oldDitherKey)
{
  if (stored == FT4_ENSEMBLE_NO_KEY) return oldDitherKey ? 1 : 0;   /* no new key yet: follow the old boolean */
  if (stored == FT4_ENSEMBLE_AUTO) return FT4_ENSEMBLE_AUTO;
  if (stored == FT4_ENSEMBLE_BUDGET) return FT4_ENSEMBLE_BUDGET;   /* item 80 */
  return valid_ft4_ensemble (stored) ? stored : 0;
}

/* the RX "ensemble effort" setting (key FT8EnsembleEffort, file mode -M): -1 = auto, the
   thread-count gate above; 0 = off; 1..ENSEMBLE_MAX_MEMBERS = that many members whatever
   the machine - the knob for testing and for tuning to a faster or slower computer. The TX
   background's "ensemble effort" (key FT8BgEnsembleEffort, file mode -V) is passed to the
   decoder as it is: -1 = as many members as fit before the next decode. */
inline int ensemble_effort_members (int effort, int threads)
{
  if (effort == ENSEMBLE_BUDGET) return ENSEMBLE_BUDGET;   // P8: passed to the decoder as it is - the budget decides there
  if (effort < 0) return ensemble_members (threads);
  return effort < ENSEMBLE_MAX_MEMBERS ? effort : ENSEMBLE_MAX_MEMBERS;
}

/* the ensemble member of the RX phase in the budgeted presets (MaxDecodes, PipelineEnsemble):
   one from 8 threads, none below - the member costs 1.2-1.7 s on top of the base recipe */
inline int rx_member (int threads)
{
  return threads >= RX_MEMBER_MIN_THREADS ? 1 : 0;
}

/* the TX background the pipeline presets use, and the default of the TX background controls:
   SWL-5 + alternate pass + as many members as fit, JTDX's sensitivities */
inline DecodePhase background_default (int threads)
{
  return {true, 3, 5, SENS_LOW_THRESHOLDS, RXF_MEDIUM, false, false, threads >= ALT_PASS_MIN_THREADS, ENSEMBLE_AUTO};
}

inline DecodeRecipe preset_recipe (DecodePreset p, int threads)
{
  bool const pass = threads >= TWO_PASS_MIN_THREADS;
  bool const alt = threads >= ALT_PASS_MIN_THREADS;
  int const s = SENS_LOW_THRESHOLDS, s2 = SENS_SUBPASS, f = RXF_MEDIUM, c = CLASSIC_CYCLES;   // every preset carries the classic unit for its background, on or off
  DecodePhase const bg = background_default (threads);
  switch (p)
    {
    case DecodePreset::MaxEfficiency:    return {{false, 5, 3, s2, f, false, false, false, 0}, false, bg, c};
    case DecodePreset::MaxDecodes:       return {{false, 5, 3, s2, f, false, false, false, rx_member (threads)}, false, bg, c};
    case DecodePreset::PipelineMaxDecodesLight:   // P10: Max decodes' RX phase and a fixed ~6 s background - the plain 6-cycle pass
      return {{false, 5, 3, s2, f, false, false, false, rx_member (threads)}, pass,   // (the classic decode, first), members 2 and 3,
              {false, 6, 5, s, f, false, false, false, 2}, 0};                        // the residual pass; no classic unit (it would repeat the pass)
    case DecodePreset::Ensemble:         return {{true, 3, 5, s, f, false, false, alt, ensemble_members (threads)}, false, bg, c};
    case DecodePreset::PipelineEnsemble:
      {
        DecodePhase b = bg; b.ensemble = 5;   // the sweet spot: with the RX member and the reordered table this is every unit that ever
        return {{true, 3, 4, s, f, false, false, false, rx_member (threads)}, pass, b, c};   // found anything (members 2-6), 16.5 s instead of 18.6 - still 100, 8, 58 on the three captures
      }
    case DecodePreset::PipelineEnsembleFull:   // the same RX phase with the whole member list in the background (effort auto)
      return {{true, 3, 4, s, f, false, false, false, rx_member (threads)}, pass, bg, c};
    case DecodePreset::PipelineRun:      return {{false, 5, 3, s2, f, false, false, false, 0}, pass, bg, c};
    case DecodePreset::Default:
    case DecodePreset::Custom:
    default:                             return {{false, 3, 3, s, f, false, false, false, 0}, false, bg, c};
    }
}

/* the preset the current controls amount to. Pipeline ensemble (background effort 5) is
   tried before pipeline ensemble full (auto); with one thread neither has a background
   phase, they coincide, and the trimmed name is reported. Max efficiency is tried before max decodes:
   below 8 threads max decodes has no member and the two coincide, so the cheaper name is
   reported. With a single thread the pipeline presets have no background phase: pipeline
   run then coincides with max efficiency and is reported as that; pipeline ensemble (SWL-4
   alone) still reads as itself. */
inline DecodePreset recipe_preset (DecodeRecipe const& r, int threads)
{
  for (auto p : {DecodePreset::Default, DecodePreset::MaxEfficiency, DecodePreset::MaxDecodes, DecodePreset::PipelineMaxDecodesLight, DecodePreset::Ensemble,
                 DecodePreset::PipelineEnsemble, DecodePreset::PipelineEnsembleFull, DecodePreset::PipelineRun})
    {
      auto const t = preset_recipe (p, threads);
      if (r.rx == t.rx && r.background == t.background && (!r.background || (r.bg == t.bg && r.bg_classic == t.bg_classic)))
        return p;
    }
  return DecodePreset::Custom;
}

#endif
