#ifndef WAITREPLY_H
#define WAITREPLY_H

/* CE3TSK 2026-10-21: Wait and Reply (WAIT_AND_REPLY_PLAN.md, TODO.md 3.-170) - WSJT-X's wait feature of that name,
   alone. While Enable Tx is off, the station in DX Call sending us a message switches Enable Tx on, and the QSO goes on
   as if Enable Tx had stayed on: the sequencer has followed it with Enable Tx off and chooses the answer as always.
   Eight T/R periods later Enable Tx goes off again, unless something else switched it since.

   What it takes (the operator, 2026-10-21, "as recommended"):
   - the setting, Settings > Sequencing > Wait and Reply, off by default (WSJT-X's is on);
   - AutoSeq on - without it the QSO would not move on from the first answer;
   - DX Call filled, Enable Tx off, a mode with AutoSeq (all but WSPR);
   - not held: the operator's Halt Tx or Stop holds it for the DX call of the moment, until that call changes or Enable
     Tx is switched on (WSJT-X never holds it - after Halt Tx the DX's next message switches Enable Tx on again).
   Enable Tx is yellow exactly while all of this holds: the colour and the trigger are one condition (in WSJT-X they are
   two, so a contest mode can switch Enable Tx on with no yellow).

   The line that fires it is to us (its first call our base call) from the DX call's base call, not his 73 or RR73,
   not a decode asked for by hand and not a late line of the band or mode before (decodeorigin.h). In Hound a line of
   the second period, the Hounds', where the Fox never transmits, is not the Fox (WSJT-X's rule for seconds 15 and 45).
   The Fox's two-part message needs nothing here: the decoder prints its halves as two lines (lib/msgparser.f90).

   The decisions are here; MainWindow carries them out (waitReplyArmed (), waitReplyLine (), waitReplyHold (),
   waitReplyTimeout (), setEnableTxButtonStyle ()), test_waitreply holds them. */

#include <QString>

namespace wait_reply
{
  // the modes with AutoSeq: every one but WSPR
  inline bool mode_has_it (QString const& mode)
  {
    return mode == "FT8" || mode == "FT4" || mode == "FT2" || mode == "JT65" || mode == "JT9" || mode == "T10"
        || mode == "JT9+JT65";
  }

  struct State
  {
    bool setting = false;      // Settings > Sequencing > Wait and Reply
    bool enableTx = false;
    bool autoseq = false;
    QString mode;
    QString hisCall;           // DX Call (MainWindow::m_hisCall)
    QString held;              // the DX call the operator's Halt Tx or Stop held it for
  };

  // it may switch Enable Tx on - and Enable Tx is yellow
  inline bool armed (State const& s)
  {
    return s.setting && !s.enableTx && s.autoseq && !s.hisCall.isEmpty () && mode_has_it (s.mode)
        && s.held != s.hisCall;
  }

  // his 73 or RR73: the QSO is over and there is nothing to go on with (rx_step::on_decode's test of his line)
  inline bool ends_qso (QString const& msg)
  {
    return msg.contains (" 73") || msg.contains ("RR73");
  }

  struct Line
  {
    bool toUs = false;         // its first call is our base call (readFromStdout ()'s mycallinmsg)
    bool fromHim = false;      // its sender is the DX call's base call
    bool manualDecode = false;
    bool late = false;         // of the band or mode before (decode_origin::late ())
    QString msg;               // DecodedText::message ()
    bool hound = false;
    bool secondPeriod = false; // in the second period of the pair, the Hounds' (pick_step::tx_first_for_line ())
  };

  // the line switches Enable Tx on
  inline bool fires (bool armedNow, Line const& l)
  {
    return armedNow && l.toUs && l.fromHim && !l.manualDecode && !l.late && !(l.hound && l.secondPeriod)
        && !ends_qso (l.msg);
  }

  // how long Enable Tx stays on, in ms: eight T/R periods (WSJT-X's stopWRTimer)
  inline int cap_ms (double trPeriod)
  {
    return int (8000.0 * trPeriod);
  }

  // the hold after DX Call changed: it lasts while the call is the one it was taken for
  inline QString held_after_change (QString const& held, QString const& hisCall)
  {
    return held == hisCall ? held : QString {};
  }
}

#endif
