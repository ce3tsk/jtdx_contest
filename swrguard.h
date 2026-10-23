// -*- Mode: C++ -*-
/* CE3TSK 2026-10-23 (TODO.md 3.-196, section 9): Settings > Radio "Halt Tx when SWR > 2.5", as WSJT-X 3.0 has it. With
   Settings' Output power the rig's SWR is read while the program keys it (HamlibTransceiver and TCI poll it only while
   their PTT is on): one reading above 2.5 halts the transmission - a message or the Tune carrier - as Halt Tx pressed, and
   says so. And the SWR label's colour, WSJT-X's: yellow above 1.5, red above 2.0 - here following each reading.

   The SWR as the rig state carries it: hundredths (2.50 is 250), 0 for no reading. The radio's own antenna tuner cycle is
   not read: the program's PTT is off then (atutune.h). Qt-free: test_swrguard drives it. */

#ifndef SWRGUARD_H
#define SWRGUARD_H

namespace swr_guard
{
  unsigned const haltAbove {250};   // 2.5, fixed (WSJT-X)

  enum class Shade {Plain, Yellow, Red};

  inline Shade shade (unsigned swr)
  {
    return swr > 200 ? Shade::Red : swr > 150 ? Shade::Yellow : Shade::Plain;
  }

  // a reading halts: the switch on, above 2.5, the program transmitting, not halted for this transmission already (the
  // readings go on until the PTT drops)
  inline bool halts (bool on, unsigned swr, bool transmitting, bool tripped)
  {
    return on && swr > haltAbove && transmitting && !tripped;
  }
}

#endif
