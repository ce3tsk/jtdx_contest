// -*- Mode: C++ -*-
/* CE3TSK 2026-10-23 (TODO.md 3.-198, the review of 3.-196, item 1): after a command (PTT, frequency, mode, split)
   PollingTransceiver holds the rig's updates back until the rig is as asked, for up to polls_to_stabilize polls. The
   readings - S meter, power, SWR - are never asked for and change at every poll: compared with the rest, they kept
   every update back for all those polls whenever Settings' S meter or Output power is on. Halt Tx when SWR > 2.5
   (swrguard.h) came 2.5 s after the PTT in FT8 and FT4, 2.8 s in FT2 (near the end of its ~3 s transmission; never in a
   late-started, shorter one). WSJT-X 3.0.2 has the same comparison. Here the rig is as asked when every other field
   is. test_pollexpect drives it. */

#ifndef POLLEXPECT_H
#define POLLEXPECT_H

#include "Transceiver.hpp"

namespace poll_expect
{
  // the rig as asked: equal but for the readings
  inline bool as_asked (Transceiver::TransceiverState actual, Transceiver::TransceiverState const& asked)
  {
    actual.level (asked.level ());
    actual.power (asked.power ());
    actual.swr (asked.swr ());
    return actual == asked;
  }
}

#endif
