#ifndef WAITCALL_H
#define WAITCALL_H

/* CE3TSK 2026-10-21: Wait and Call (WAIT_AND_CALL_PLAN.md, TODO.md 3.-178) - WSJT-X's wait feature of that name, with
   the trigger of pull request #1's "Chase DX" and six calls in place of three; decided W1-W11 "as recommended".

   The operator arms it for the station in DX Call (a right click on Enable Tx, or DXpedition > Wait and Call, Ctrl+W):
   Enable Tx goes off and it waits. The first line FROM that station - any message: its CQ, its report or RR73 to
   someone else - calls it: what a double click on the line does (the calling message, Tx1 or with Skip Tx1 Tx2, the
   report, our period), Enable Tx on. Each of our transmissions that ends is one call; after the sixth Enable Tx goes off
   - one burst, as WSJT-X (W1): with this trigger waiting again would call without end, the DX being heard every period
   in a pileup. Hearing him again during the burst changes nothing (W2). His answer to the sixth call comes a period
   later, so it listens on to the end of the period after the next one (3.-180, the review 3.-179 2 B), and then it is
   over. His answer to us ends it - while it waits or listens it switches Enable Tx on first, as Wait and Reply, and the
   QSO goes on under AutoSeq as any QSO. His 73 or RR73 to us is no answer (3.-180, 1), nor is a line of a recording
   played (File > Open), which never calls him either (3.-180, 4).

   Any manual intervention ends it, waiting or calling: Halt Tx (the button, Alt+H, Esc in both windows, a UDP client's
   halt), Stop, Enable Tx clicked either way (W3), Tune, Log QSO, another DX call, band or mode, AutoSeq off; also a band
   changed at the rig, SuperFox switched on, band hopping started (3.-180, 7). Ended while it calls, the calls stop with
   it - Enable Tx off, a transmission under way ends normally ("stopped by any manual intervention", 3.-180, 3) - but at
   his answer, and with another station put in DX Call (the operator steers to it). So does any Enable Tx off of the
   program while calling - the Tx watchdog, the "1 QSO" stop, the answer counters, a halt on "reply to other" (W8) -
   but Settings OK's pause, which the burst runs on through as Wait and Reply's eight periods. The sequencer giving up
   on him while it calls halts the transmission too, before its CQ (3.-178, 3.-180 5).
   It is never remembered: the program starts with it off. Enable Tx is orange while it waits or listens (W9). Normal
   operation and Hound, not a contest activity (W6); SuperFox Hound not yet, its two-part lines unchecked as triggers.

   The decisions are here; MainWindow carries them out (waitCallArm (), waitCallAnswer (), waitCallLine (),
   waitCallCounted (), waitCallCheck (), waitCallGivenUp (), waitCallEnd ()), test_waitcall holds them. */

#include <QString>
#include <QtGlobal>
#include "Radio.hpp"
#include "waitreply.h"   // ends_qso (): his 73 or RR73 is no answer, as for Wait and Reply

namespace wait_call
{
  // Listening: after the sixth call, Enable Tx off, his answer to it still taken (3.-180)
  enum class State {Off, Waiting, Calling, Listening};

  // W1, W2: one burst of six calls - six of our transmissions
  int const calls = 6;

  // W5: the fast modes - six calls are 3 min in FT8, 90 s in FT4, 45 s in FT2 (12 min in JT65 or JT9)
  inline bool mode_has_it (QString const& mode)
  {
    return mode == "FT8" || mode == "FT4" || mode == "FT2";
  }

  // what arming asks, and what must still hold while armed
  struct Arm
  {
    bool autoseq = false;
    QString hisCall;           // DX Call (MainWindow::m_hisCall)
    QString mode;
    QString band;              // the dial's band
    bool contest = false;      // a special operating activity (W6: normal operation and Hound only)
    bool hopping = false;      // band hopping, which changes the band by itself
    bool superFox = false;     // SuperFox Hound: not yet - the Fox's two-part lines unchecked as triggers (W6)
  };

  inline bool may_arm (Arm const& a)
  {
    return a.autoseq && !a.hisCall.isEmpty () && mode_has_it (a.mode) && !a.contest && !a.hopping && !a.superFox;
  }

  // armed with `at`, it still holds `now`: arming would still be allowed, and the station (its base call), the mode and
  // the band are the ones it was armed for - a double click on his line may put his full call /P in DX Call
  inline bool holds (Arm const& at, Arm const& now)
  {
    return may_arm (now) && Radio::base_callsign (now.hisCall) == Radio::base_callsign (at.hisCall) && now.mode == at.mode
        && now.band == at.band;
  }

  // what no longer holds - the debug line's reason; empty while it holds
  inline QString broken (Arm const& at, Arm const& now)
  {
    if (!now.autoseq) return "AutoSeq off";
    if (now.hisCall.isEmpty ()) return "DX Call empty";
    if (Radio::base_callsign (now.hisCall) != Radio::base_callsign (at.hisCall)) return "another DX call: " + now.hisCall;
    if (now.mode != at.mode) return "another mode: " + now.mode;
    if (now.band != at.band) return "another band: " + now.band;
    if (now.contest) return "a contest activity";
    if (now.hopping) return "band hopping";
    if (now.superFox) return "SuperFox mode";
    return {};
  }

  /* (TODO.md 3.-180, the review 3.-179 3, 5) another station put in DX Call at once - a double click on another line, a
     paste: the operator steers to it, and the calls go on to it. A call typed over his passes through a DX Call under
     three characters, empty for the program, and is no such change */
  inline bool another_station (Arm const& at, Arm const& now)
  {
    return !now.hisCall.isEmpty () && Radio::base_callsign (now.hisCall) != Radio::base_callsign (at.hisCall);
  }

  /* (3.-180, the review 3.-179 3) it ends while it calls: the calls stop with it - Enable Tx off, a transmission under way
     ends normally, Settings OK's pause does not switch Enable Tx on again - but where Enable Tx is left as it is: at his
     answer, with another station in DX Call, and when Enable Tx itself is being switched */
  inline bool stops_calls (State was, bool leaveTx)
  {
    return was == State::Calling && !leaveTx;
  }

  // Enable Tx off, and the program may switch it on - waiting, or listening for his answer to the sixth call: orange,
  // and Settings OK's pause does not switch it on again (3.-180, the review 3.-179 6)
  inline bool may_switch_on (State s)
  {
    return s == State::Waiting || s == State::Listening;
  }

  struct Line
  {
    bool fromHim = false;      // its sender is the DX call's base call
    bool toUs = false;         // its first call is our base call (readFromStdout ()'s mycallinmsg)
    bool manualDecode = false;
    bool late = false;         // of the band or mode before (decode_origin::late ())
    bool replay = false;       // (3.-180) of a recording played (File > Open), not of the air
    QString msg;               // DecodedText::message () - his answer's 73 or RR73 (3.-180)
    bool hound = false;
    bool secondPeriod = false; // in the second period of the pair, the Hounds' (pick_step::tx_first_for_line ())
  };

  // a line of his that counts: not from the Decode button, not late, not of a replay, and in Hound not of the Hounds'
  // period
  inline bool his (Line const& l)
  {
    return l.fromHim && !l.manualDecode && !l.late && !l.replay && !(l.hound && l.secondPeriod);
  }

  // waiting, a line of his to anyone but us calls him - the Chase DX trigger: any message
  inline bool fires (State s, Line const& l)
  {
    return s == State::Waiting && his (l) && !l.toUs;
  }

  // his answer to us ends it; while waiting or listening it switches Enable Tx on first. His 73 or RR73 to us is no
  // answer - a repeat of an old QSO's (3.-180, the review 3.-179 1), as Wait and Reply never fires on it
  inline bool answered (State s, Line const& l)
  {
    return s != State::Off && his (l) && l.toUs && !wait_reply::ends_qso (l.msg);
  }

  /* (3.-180, the review 3.-179 8) the message it would send calls him: its first word his call (a hashed one in <>) -
     or the double click did not take his line (processMessage ()'s early returns: a transmission under way with the Tx
     frequency box greyed, a sender that is no callsign) and what is selected is another's: a CQ, another QSO's report */
  inline bool to_him (QString const& txMessage, QString const& hisCall)
  {
    QString first = txMessage.trimmed ().section (' ', 0, 0);
    first.remove ('<').remove ('>');
    return !first.isEmpty () && !hisCall.isEmpty () && Radio::base_callsign (first) == Radio::base_callsign (hisCall);
  }

  // the burst is over after its sixth call
  inline bool burst_over (int callsMade)
  {
    return callsMade >= calls;
  }

  /* (3.-180, the review 3.-179 2 B) after the sixth call his answer is taken to the end of the period after the next
     one: the sixth call's period, his - with the answer - and ours, in which his is decoded. msOfDay: the clock's
     milliseconds of the day, to which the periods are aligned (FT2's 3.75 s too) */
  inline int listen_ms (qint64 msOfDay, double trPeriod)
  {
    qint64 const per = qRound64 (1000.0 * trPeriod);
    return int (per - msOfDay % per + 2 * per);
  }
}

#endif
