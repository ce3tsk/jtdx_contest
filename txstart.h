#ifndef TXSTART_H
#define TXSTART_H
// CE3TSK 2026-10-15 (TODO 3.-92): where a transmission starts in its period - the sound card's Modulator and the TCI
// modulator (TCITransceiver.cpp) both ask here.
//
// Both computed the silence before the first symbol as frameRate / (1000 / delay_ms): exact for 500 and 1000 ms, but
// 1000 / 150 is 6 in integers, so FT2 started 8000 frames - 166.7 ms - into its period instead of 150 (17 ms late). And
// the TCI modulator had no FT2 case at all: FT2 fell through to 1000 ms and started some 850 ms late. FT2's 150 ms is
// where WSJT-X improved's transmitter starts it, and where our simulator and decoder put the frame.
#include <QtGlobal>

namespace tx_start
{
  // ms into the period at which the first symbol goes out, by samples per symbol
  inline unsigned delay_ms (unsigned nsps)
  {
    if (1920 == nsps) return 500;   // FT8
    if (576 == nsps) return 500;    // FT4
    if (288 == nsps) return 150;    // FT2
    if (6 == nsps) return 0;        // MSK144 (2026-11-17): from the period's start, see repeats_frame ()
    return 1000;
  }

  /* CE3TSK 2026-11-17 (MSK144_PLAN.md step 2): MSK144 - 6 samples a symbol at 12 kHz, which no other mode has - sends
     its frame of 144 symbols (72 ms; 40 symbols, 20 ms, for a short message) over and over for the whole
     transmission, with no start delay, and stops half a second before the period's end: WSJT-X's "fast mode" in
     Modulator.cpp (`isym=isym%m_symbolsLength`, `i1=m_TRperiod*48000.0 - 24000.0`, `i0=i1-816`, tuning
     `999999*m_nsps`). Both modulators ask here.
     Not WSJT-X's: asked for late, WSJT-X begins with the frame's first symbol and sends for the period less half a
     second FROM THEN (its main window cuts that a quarter second before the period's end); here a late start begins
     where an on-time one would be by then - as every other mode's does - and so ends half a second before the
     period's end whenever it began. */
  inline bool repeats_frame (unsigned nsps) { return 6 == nsps; }

  // the frame's symbols: genmsk_128_90 marks a short message with -40 in the tone after its 40
  inline unsigned frame_symbols (int tone40) { return tone40 < 0 ? 40u : 144u; }

  // the frame of the period after which nothing more is sent, and how many frames before it the fade begins
  inline unsigned repeat_end_frame (double trPeriod, unsigned frameRate)
  {
    return static_cast<unsigned> (trPeriod * frameRate - frameRate / 2.0);
  }
  constexpr unsigned repeat_fade_frames = 816;

  // Tune's tone ends by itself after this many frames (the main window ends it long before): 9999 symbols' worth -
  // which at MSK144's 6 would be a second and a quarter, so WSJT-X gives its fast mode a hundred times as many
  inline unsigned tune_end_frame (unsigned nsps)
  {
    return (repeats_frame (nsps) ? 999999u : 9999u) * nsps;
  }

  // the frames of silence to send when the transmission is asked for mstr ms into the period, up to its start; none
  // once the start has passed (the caller then begins inside the message)
  inline qint64 silent_frames (unsigned frameRate, unsigned delay_ms, unsigned mstr)
  {
    if (mstr > delay_ms) return 0;
    return qint64 (frameRate) * delay_ms / 1000 - qint64 (mstr) * (frameRate / 1000);
  }
}
#endif
