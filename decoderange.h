#ifndef DECODERANGE_H
#define DECODERANGE_H

/* CE3TSK: the frequency range handed to the FT8/FT4 decoder - Decode -> FT8 decoding ->
   "decode bandwidth".

   JTDX used to decode exactly what the waterfall window shows - nfa = its start frequency,
   nfb = its Fmax, which is window width x bins/pixel x bin width. Two things follow: anything
   above the window's top is never decoded, and because sync8 takes its noise baseline and
   candidate ordering over the whole range, a narrower window decodes less far below its top
   too (measured: 68 against 71 messages on the benchmark capture, DECODER_BENCHMARK_PLAN.md
   step 7; 100-3100 Hz costs ten at default settings, step 10). The choices: the waterfall's
   width (JTDX's behaviour), twelve fixed ranges (100-3300, 100-3200 ... 200-2400, 300-3300, 50-3650 Hz), and the
   decoder's own ceiling 0-5000 Hz regardless of the window ("testing only" in the menu: it
   decodes most on the benchmark capture, but it is the range no other JTDX offers). The other modes keep the waterfall-derived range, their split
   frequency genuinely being a display setting. */
constexpr int DECODE_BW_WATERFALL = 0;
constexpr int DECODE_BW_DEFAULT = 2;   // 100-3100 Hz: a fresh configuration starts here (DECODE_RECIPE_PLAN.md) - it decodes as the full band does inside it since the fixed baseline span and the anchored grid
constexpr int DECODE_BW_FULL = 10;
constexpr int DECODE_BW_COUNT = 14;   // entries 11 (50-3650 Hz), 12 (300-3300 Hz) and 13 (100-3300 Hz) came later and keep the stored values of 0-10 valid; the menu orders them by range, not by index

struct DecodeBandwidth { int lo; int hi; char const* name; };

inline DecodeBandwidth const& decode_bandwidth (int choice)
{
  static DecodeBandwidth const table[DECODE_BW_COUNT] = {
    {-1, -1, "Waterfall width (default)"},
    {100, 3200, "100-3200 Hz"},
    {100, 3100, "100-3100 Hz"},
    {100, 3000, "100-3000 Hz"},
    {200, 3200, "200-3200 Hz"},
    {200, 3100, "200-3100 Hz"},
    {200, 3000, "200-3000 Hz"},
    {200, 2800, "200-2800 Hz"},
    {200, 2500, "200-2500 Hz"},
    {200, 2400, "200-2400 Hz"},
    {0, 5000, "0-5000 Hz (testing only)"},
    {50, 3650, "50-3650 Hz"},
    {300, 3300, "300-3300 Hz"},
    {100, 3300, "100-3300 Hz"}};
  if (choice < 0 || choice >= DECODE_BW_COUNT) choice = DECODE_BW_WATERFALL;
  return table[choice];
}

/* the range for a decode: an FT mode gets the chosen bandwidth (the waterfall's for choice
   0), every other mode the waterfall's */
inline void decode_range (bool ft_mode, int bandwidth, int waterfall_start, int waterfall_max,
                          int& nfa, int& nfb)
{
  auto const& bw = decode_bandwidth (bandwidth);
  if (ft_mode && bw.lo >= 0)
    {
      nfa = bw.lo;
      nfb = bw.hi;
    }
  else
    {
      nfa = waterfall_start;
      nfb = waterfall_max;
    }
}

/* CE3TSK: the stored FT8DecodeBandwidth setting, made safe. An unreadable value means what a
   missing key means - the documented default - and not DECODE_BW_FULL, the 0-5000 Hz entry
   this used to fall back to and which the menu labels "testing only". Entries 11 to 13 were
   added later and DECODE_BW_COUNT went 11 -> 14, so a configuration storing 13 read back by
   an older build of the pair (a routine rollback here) silently switched decoding to the full
   band, moving the noise baseline and the candidate ordering for every period.
   decode_bandwidth() below keeps its own defensive clamp to the waterfall entry for an
   out-of-range *argument*; with this in readSettings, a stored value can no longer be one. */
inline int decode_bandwidth_stored (int stored)
{
  return (stored < 0 || stored >= DECODE_BW_COUNT) ? DECODE_BW_DEFAULT : stored;
}

/* the earlier boolean setting FT8FullBandDecode, migrated once: true was the full band */
inline int decode_bandwidth_from_full_band (bool full_band)
{
  return full_band ? DECODE_BW_FULL : DECODE_BW_WATERFALL;
}

#endif
