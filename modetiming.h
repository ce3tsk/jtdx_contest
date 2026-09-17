// -*- Mode: C++ -*-
#ifndef MODETIMING_H
#define MODETIMING_H

/* CE3TSK 2026-09-16: the per-mode timing constants, in one place, with a test.
 *
 * These four numbers were open-coded where they were used, and FT2 was missing from three of them
 * for a whole deployment: band_changed() re-derived the T/R period with its own if/else chain and
 * fell back to 60 s, so every band change zeroed the decoder's buffer; the delay-to-half-symbol
 * factor assumed m_nsps 6912, so FT2's monitor-start delay came out half what it should be; and
 * the position inside the period was taken as tenths modulo 37, though 3.75 s is 37.5 tenths, so
 * it drifted a tenth every second period. None of that was visible to any test, because the
 * arithmetic lived inside GUI functions that need a running JTDX. It lives here now, as the
 * decode budget, label and range rules already do.
 *
 * FT8 and FT4 keep the numbers they have always had, to the digit - see delay_blocks().
 */

#include <cmath>
#include <QString>

// the T/R period in seconds. 60 is the answer for JT9, JT65, T10 and WSPR, and the safe answer
// for a mode this does not know: it is the longest, so a guard derived from it never under-waits.
inline double tr_period_of (QString const& mode)
{
  if (mode == "FT8") return 15.0;
  if (mode == "FT4") return 7.5;
  if (mode == "FT2") return 3.75;
  return 60.0;
}

/* Monitoring started this many tenths of a second into the period, so the decode trigger has to
 * move by the same amount, counted in the half-symbol blocks ihsym counts. A block is nsps/2
 * samples at 12000 S/s, so a tenth of a second is 2400/nsps of them: 0.3472 at FT8's and FT4's
 * 6912, 0.6944 at FT2's 3456.
 *
 * FT8 and FT4 KEEP THE LEGACY 0.345, which is 0.6 % low. Replacing it with the exact value moves
 * their trigger by one block at delays 13, 36, 39, 42, 62, 65, 68, 85 and 88 (measured), and this
 * is a refactor: it is not the place to change when FT8 decodes. FT2 has no legacy to preserve,
 * so it gets the exact figure. The rounding rule - up above 0.49, down below - is the original's.
 */
inline int delay_blocks (int tenths, int nsps)
{
  if (tenths <= 0) return 0;
  double const per_tenth = (nsps == 3456) ? 2400.0 / 3456.0 : 0.345;
  double const f = double (tenths) * per_tenth;
  return std::fmod (f, 1.0) > 0.49 ? int (std::ceil (f)) : int (std::floor (f));
}

/* How far into the current period we are, in tenths of a second, from a millisecond clock. Every
 * T/R period divides a minute (15, 7.5, 3.75, 60 s), so it does not matter whether the caller
 * passes milliseconds of the minute or of the day. Taking the remainder in MILLISECONDS is the
 * whole point: FT2's period is 37.5 tenths, so a remainder taken in tenths cannot be right.
 */
inline int period_position_tenths (qint64 ms, double period)
{
  double const p = period * 1000.0;
  if (p <= 0.0) return 0;
  return int (std::fmod (double (ms), p) / 100.0);
}

/* The UTC clock label alternates colour once per period, so it has to be repainted at every period
 * boundary. The caller ticks with the whole second, so a mode whose period is not a whole number of
 * seconds is refreshed at the first whole second after each boundary: FT4's 7.5 s falls at 8, 23,
 * 38 and 53, and FT2's 3.75 s at 4, 8 and 12 past each quarter minute.
 */
inline bool clock_refresh_due (QString const& mode, int isecond)
{
  if (mode == "FT8") return isecond % 15 == 0;
  if (mode == "FT4") return isecond % 15 == 0 || isecond == 8 || isecond == 23
                         || isecond == 38 || isecond == 53;
  if (mode == "FT2") { int const s = isecond % 15; return s == 0 || s == 4 || s == 8 || s == 12; }
  return isecond == 0;   // the 60 s modes
}

#endif
