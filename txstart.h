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
    return 1000;
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
