// -*- Mode: C++ -*-
#ifndef FT2PRESET_H
#define FT2PRESET_H

/* CE3TSK 2026-09-16, step 5 of the FT2 port: FT2's decoding tiers.
 *
 * FT2 is decoded by the FT4 chain and fills the same parameter fields, so a tier is an FT4Recipe -
 * but FT4's SIX tiers do not transfer, because the clock is half as long. FT4 answers inside a
 * 1360 ms reply deadline and its "most decodes" tier spends 0.55 s of it at reply time; FT2's
 * period is 3.75 s, its reply deadline about 580 ms and its RX budget 5 tenths, so a tier costing
 * 0.55 s in the period would be permanently late. FT2 therefore has FOUR tiers, and the effort
 * that FT4 can afford at reply time goes into FT2's TX background instead, which has the whole
 * transmission - about 3.0 s of a 3.75 s period - to spend.
 *
 * The figures in the comments come from test/decode/ft2_presets_sweep.py on the 240-period set
 * (wav_save/ft2_night, 1575 signals) and seven noise realisations of the crowded period, with the
 * RX budget lifted so each tier's true cost is visible. RX mean is the <DecodeFinished><rxs> mean.
 */

#include "decodepreset.h"   // FT4Recipe and the member ladders: FT2 shares both

enum class FT2Preset { Fast, Default, Recommended, MaxEffort, Custom };

constexpr int FT2_RX_BUDGET_DEFAULT = 5;    // tenths: the reply deadline is about 0.58 s
constexpr int FT2_BG_MARGIN_DEFAULT = 5;    // tenths: the background stops this long before the window ends

/* MEASURED 2026-09-16, test/decode/ft2_presets_sweep.py, twelve recipes on both sets: the
   240-period night set (1575 signals, decoded as one chain so the hint memory works as it does on
   the air) and seven noise realisations of the crowded period (350 signals). RX is the
   <DecodeFinished><rxs> mean, against a reply deadline of about 0.58 s.

       recipe                     night   crowded   RX mean night/crowded   background
       fast                        1261      124       0.025 / 0.079            -
       default                     1457      296       0.104 / 0.224            -
       background 3                1464      302       0.104 / 0.229        0.30 / 0.46 s
       background 3 + extras       1465      318       0.104 / 0.223        1.46 / 1.82 s
       background 6                1466      302       0.104 / 0.229        0.50 / 0.79 s
       background 6 + extras       1465      321       0.104 / 0.233        2.32 / 2.82 s
       background 6 + extras+sens  1466      333       0.103 / 0.223        4.47 / 4.42 s
       3 members at reply time     1464      301       0.295 / 0.577            -
       3 members + background 6    1467      321       0.295 / 0.561        1.42 / 1.76 s
       6 members + background 6    1467      317       0.484 / 0.909        0.58 / 0.79 s
       budget members + bg 6       1467      321       0.459 / 0.341        0.69 / 2.43 s
       max effort (the tier)       1467      333       0.559 / 0.411        2.69 / 3.79 s
       6 members + OSD + alt       1466      333       4.255 / 4.631        1.01 / 0.94 s

   WHAT THAT SETTLES. Spending members AT REPLY TIME costs 0.3-0.9 s of a 0.58 s deadline and buys
   what the background gives for nothing: three members alone reach 301 on the crowded band, three
   in the TX window with the extras reach 318 at an unchanged 0.10 s. So FT4's two reply-time tiers
   do not transfer, and FT2 has four tiers, not six.

   WHY RECOMMENDED CARRIES THREE BACKGROUND MEMBERS AND NOT SIX. FT2 transmits 2.52 s of a 3.75 s
   period, so the idle window is about 3 s and the margin takes half a second of it. Six members
   with the extras cost 2.82 s on a crowded band and would be cut off there; three cost 1.82 s and
   fit, for three decodes' difference (318 against 321). Six is available in the menu for anyone
   whose machine has the headroom.

   THE PRICE OF THE EXTRAS, stated plainly: they roughly triple the false decodes on the night set,
   from 2 to 7 in 240 periods (max effort 10). That is the same trade FT4's recommended tier makes.

   Field order is FT4Recipe's: depth, alt, members, deeposd, bg, bgdepth, bgdeeposd, bgalt,
   bgtwopass, sens, bgresidual, bgsens, twopass, bgon, rxf, bgrxf */
inline FT4Recipe ft2_preset_recipe (FT2Preset p)
{
  switch (p)
    {
    // 1261 night / 124 crowded, 0.025 / 0.079 s - the shallow decoder, for a machine with nothing to spare
    case FT2Preset::Fast:
      return {1, false, 0, false, 0, 3, false, false, true, 0, false, 0, true, false, RXF_MEDIUM, RXF_MEDIUM};
    // 1457 / 296, 0.104 / 0.224 s - deep, everything inside the period, no background at all
    case FT2Preset::Default:
      return {3, false, 0, false, 0, 3, false, false, true, 0, false, 0, true, false, RXF_MEDIUM, RXF_MEDIUM};
    /* 1465 / 318, 0.104 / 0.223 s at reply time, 1.46 / 1.82 s of idle CPU: +0.5 % on the night
       set and +7.4 % on the crowded band FOR NOTHING at reply time - the tier the mode is built
       around, since FT2's whole margin is in the TX window rather than in the period */
    case FT2Preset::Recommended:
      return {3, false, 0, false, 3, 3, true, true, true, 0, true, 0, true, true, RXF_MEDIUM, RXF_MEDIUM};
    /* 1467 / 333, 0.559 / 0.411 s mean and 1.16 s worst - over the deadline on a busy period, which
       is the same bargain FT4's max effort offers and is labelled the same way. The budget keeps
       the members honest: six fixed members with deep OSD and the alternate pass cost 4.6 s for
       the same 333. */
    case FT2Preset::MaxEffort:
      return {3, false, FT4_ENSEMBLE_BUDGET, false, 6, 3, true, true, true, 1, true, 1, true, true, RXF_MEDIUM, RXF_MEDIUM};
    default:
      return {3, false, 0, false, 0, 3, false, false, true, 0, false, 0, true, false, RXF_MEDIUM, RXF_MEDIUM};
    }
}

// which tier a set of controls describes, at this thread count - "auto" resolves on both sides,
// exactly as ft4_preset_of does
inline FT2Preset ft2_preset_of (FT4Recipe const& r, int threads)
{
  auto const same = [threads] (FT4Recipe const& a, FT4Recipe const& b)
  {
    return ft4_effort_members (a.members, threads) == ft4_effort_members (b.members, threads)
        && ft4_bg_effort_members (a.bg, threads) == ft4_bg_effort_members (b.bg, threads)
        && a.depth == b.depth && a.alt == b.alt && a.deeposd == b.deeposd
        && a.bgdepth == b.bgdepth && a.bgdeeposd == b.bgdeeposd && a.bgalt == b.bgalt
        && a.bgtwopass == b.bgtwopass && a.sens == b.sens && a.bgresidual == b.bgresidual
        && a.bgsens == b.bgsens && a.twopass == b.twopass && a.bgon == b.bgon
        && a.rxf == b.rxf && a.bgrxf == b.bgrxf;
  };
  for (auto p : {FT2Preset::Fast, FT2Preset::Default, FT2Preset::Recommended, FT2Preset::MaxEffort})
    if (same (r, ft2_preset_recipe (p))) return p;
  return FT2Preset::Custom;
}

/* the menu dot and the lamp's colour, FT4's table read across: green is the recommended tier in
   every mode, red the one that spends without a ceiling. FT2 has no "best power" or "most at reply
   time" tier - at a 0.58 s deadline neither exists - so it uses two of the four colours. */
inline char const* ft2_preset_colour (FT2Preset p)
{
  switch (p) {
    case FT2Preset::Fast:        return "#8d99ae";   // grey, FT8's own grey for a lesser tier
    case FT2Preset::Default:     return "#8d99ae";   // grey - deep, but nothing added
    case FT2Preset::Recommended: return "#3cb043";   // green - the tier the mode is built around
    case FT2Preset::MaxEffort:   return "#d32f2f";   // red - no budget at reply time
    /* Custom returns nullptr, which is what FT8's and FT4's Custom returns: the lamp then wears
       the plain bordered style instead of a colour, and all three modes say "Custom" alike. */
    default: return nullptr;
  }
}

// the lamp's letter, as FT8's and FT4's have one
inline char ft2_preset_letter (FT2Preset p)
{
  switch (p)
    {
    case FT2Preset::Fast:        return 'F';
    case FT2Preset::Default:     return 'D';
    case FT2Preset::Recommended: return 'R';
    case FT2Preset::MaxEffort:   return 'M';   // M for max, as the operator reads it
    default:                     return 'C';
    }
}

#endif
