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

   The decisions are here, and since 2026-10-22 the state machine too (TODO.md 3.-184, WAIT_AND_CALL_PLAN.md section 10):
   what it is (Run), what happens to it (Event), and step (), which moves it and says what the window must do (Do).
   MainWindow gathers the window's state into the events (waitCallArm (), waitCallAnswer (), waitCallLine (),
   waitCallCounted (), waitCallCheck (), waitCallGivenUp (), waitCallEnd ()) and carries out the actions in Do's order
   (waitCallDo ()); test_waitcall holds them. */

#include <QString>
#include <QStringList>
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

  /* ---- the state machine (2026-10-22, TODO.md 3.-184): the moves MainWindow made itself until then, unchanged */

  // what it is: the state, what it was armed with, the burst's calls - MainWindow keeps one (m_waitCall)
  struct Run
  {
    State state = State::Off;
    Arm at;              // what it was armed with: the DX call, the mode, the band
    int calls = 0;       // the burst's calls made
    bool skip = false;   // the transmission under way at the call, or the one Settings OK's pause cuts: none of them
  };

  // what happened
  enum class Kind
  {
    Arm,        // the operator arms it (the menu entry, the right click on Enable Tx, Ctrl+W): now
    Answer,     // a decoded line, before the sequencer's pass on it - his answer to us: line
    Line,       // ... after the pass - his line to anyone else calls him: line
    Taken,      // his line handled as a double click, while it still waits: txMsg, hisCall
    Switched,   // Enable Tx switched on for the call: enableTx, transmitting, restart, decoded
    Counted,    // a transmission ended (stopTx ())
    Listened,   // the listening after the sixth call is over
    Check,      // what it was armed with may have changed - AutoSeq, DX Call, the mode, the band...: now
    GivenUp,    // the sequencer gives up on him (process_Auto (), the release): why
    End,        // an end - the operator's, or the window's: why, leaveTx
    EnableTx,   // Enable Tx being switched (on_enableTxButton_clicked ()): own, checked, enableTx
    Pause       // Settings OK's pause begins or ends (settingsOkPause ()): begins
  };

  // ... with what the window knows of it
  struct Event
  {
    explicit Event (Kind k) : kind {k} {}
    Kind kind;
    Arm now;                    // Arm, Check: what arming asks of the window now
    Line line;                  // Answer, Line
    QString txMsg;              // Taken: the message it would send now (as guiUpdate () picks it)
    QString hisCall;            // Taken: DX Call now
    QString decoded;            // Switched: his line as decoded (the debug line)
    bool enableTx = false;      // Switched: Enable Tx now; EnableTx: before the change
    bool transmitting = false;  // Switched: a transmission under way
    bool restart = false;       // Switched: the double click sent the transmission under way again (the same period)
    QString why;                // GivenUp, End: the debug line's reason
    bool leaveTx = false;       // End: Enable Tx left as it is (stops_calls ())
    bool own = false;           // EnableTx: the change is Wait and Call's own, or Settings OK's pause's while it calls
    bool checked = false;       // EnableTx: what Enable Tx becomes
    bool begins = false;        // Pause
  };

  // its own change of Enable Tx (enableTx_mode () with m_waitCallOwn set). On and Off switch it whatever it is: Enable
  // Tx's slot runs even when nothing changes, and these are kept apart as the code had them
  enum class Tx {Leave, On, OnIfOff, Off, OffIfOn};

  // what MainWindow does after a step - in this order (waitCallDo ())
  struct Do
  {
    bool refuse = false;           // arming refused: the menu entry unticked unheard, the reason in the status bar
    bool doubleClick = false;      // his line handled as a double click - a Taken follows (waitCallLine ())
    bool periodFromLine = false;   // our period from his line ...
    bool lineTxFirst = false;      // ... which is this
    QStringList debugFirst;        // the lines ALL.TXT gets with "write decoded debug" before its Enable Tx change (a
                                   // call counted - the sixth's comes before Enable Tx off)
    bool watchdog = false;         // the Tx watchdog afresh
    Tx enableTx = Tx::Leave;
    bool listen = false;           // the listening timer started (listen_ms ())
    bool ended = false;            // the listening timer stopped, the menu entry unticked unheard
    QStringList debug;             // the lines ALL.TXT gets with "write decoded debug"
    bool stopCalls = false;        // the calls stopped: Settings OK's pause stopped, Enable Tx off if on - not its own
                                   // change, nor need it be: it is Off by then
    bool show = false;             // the status bar's line, Enable Tx repainted
    bool halt = false;             // the transmission under way or keyed halted, if there is one
    bool own = false;              // Pause: Settings OK's halt and Enable Tx changes are the burst's own
  };

  // the end: Off, the listening timer stopped, the entry unticked; ended while it calls, the calls stop with it - but
  // where Enable Tx is left (stops_calls ())
  inline Do end (Run& r, QString const& why, bool leaveTx)
  {
    Do d;
    if (r.state == State::Off) return d;
    d.stopCalls = stops_calls (r.state, leaveTx);
    r.state = State::Off;
    d.ended = true;
    d.debug << "Wait and Call: ended - " + why;
    d.show = true;
    return d;
  }

  // the move an event makes, and what the window must do for it
  inline Do step (Run& r, Event const& e)
  {
    Do d;
    switch (e.kind)
      {
      case Kind::Arm:   // armed: Enable Tx off - waiting is not transmitting; a transmission under way ends normally
        if (r.state != State::Off) return d;
        if (!may_arm (e.now))
          {
            d.refuse = true;
            return d;
          }
        r.state = State::Waiting;
        r.at = e.now;
        r.calls = 0;
        d.enableTx = Tx::OffIfOn;
        d.debug << "Wait and Call: armed for " + e.now.hisCall;
        d.show = true;
        return d;

      case Kind::Answer:   // his answer to us ends it - while it waits or listens with Enable Tx on first, in the period
        if (!answered (r.state, e.line)) return d;   // after his line's, so that the sequencer's pass answers him
        if (may_switch_on (r.state))
          {
            d.periodFromLine = true;
            d.lineTxFirst = e.line.secondPeriod;
            d.watchdog = true;
            d.enableTx = Tx::On;
          }
        {
          Do const x = end (r, "his answer to us", true);
          d.ended = x.ended;
          d.debug = x.debug;
          d.stopCalls = x.stopCalls;
          d.show = x.show;
        }
        return d;

      case Kind::Line:   // while it waits, his line to anyone else calls him - as a double click on the line
        d.doubleClick = fires (r.state, e.line);
        return d;

      case Kind::Taken:   // the double click done, still waiting: the message it would send must call him (3.-180, 8)
        if (r.state != State::Waiting) return d;   // ended in the double click: what it was armed with changed
        if (!to_him (e.txMsg, e.hisCall)) return end (r, "his line not taken, the message is \"" + e.txMsg.trimmed () + "\"", false);
        d.watchdog = true;
        d.enableTx = Tx::OnIfOff;
        return d;

      case Kind::Switched:   // Enable Tx on: the burst's calls start - a transmission under way at the call is none of
        if (r.state != State::Waiting) return d;   // them, but one the double click sent again with the call is
        if (!e.enableTx) return end (r, "Enable Tx not switched on", false);
        r.state = State::Calling;
        r.calls = 0;
        r.skip = e.transmitting && !e.restart;
        d.debug << "Wait and Call: " + r.at.hisCall + " heard, calling: " + e.decoded.trimmed ();
        d.show = true;
        return d;

      case Kind::Counted:   // while calling, one of the six - after the sixth Enable Tx off, and it listens for his answer
        if (r.state != State::Calling) return d;   // to the end of the period after the next one (3.-180, 2 B)
        if (r.skip)
          {
            r.skip = false;
            return d;
          }
        ++r.calls;
        d.debugFirst << QString {"Wait and Call: call %1 of %2"}.arg (r.calls).arg (calls);
        d.show = true;
        if (!burst_over (r.calls)) return d;
        r.state = State::Listening;
        d.enableTx = Tx::Off;
        d.listen = true;
        d.debug << QString {"Wait and Call: %1 calls made, listening for his answer"}.arg (calls);
        return d;

      case Kind::Listened:
        return end (r, QString::number (calls) + " calls made, no answer", false);

      case Kind::Check:   // it ends when what it was armed with no longer holds - Enable Tx left only for another station
        if (r.state == State::Off || holds (r.at, e.now)) return d;
        return end (r, broken (r.at, e.now), another_station (r.at, e.now));

      case Kind::GivenUp:   // while it calls: ended, Enable Tx off, and the transmission halted - before the release's
        if (r.state != State::Calling) return d;   // clearDX () and the CQ that would follow with Enable Tx on (3.-178)
        d = end (r, "the sequencer gave up on him:" + e.why, false);
        d.halt = true;
        return d;

      case Kind::End:
        return end (r, e.why, e.leaveTx);

      case Kind::EnableTx:   // any change of Enable Tx not its own ends it - and Enable Tx follows the change
        if (e.own || e.checked == e.enableTx) return d;
        return end (r, e.checked ? "Enable Tx on" : "Enable Tx off", true);

      case Kind::Pause:   // its halt and Enable Tx off and on again are the burst's own while it calls; the call it cuts
        d.own = r.state == State::Calling;   // is none of the six - the one sent again is (3.-178, 3.-180)
        if (e.begins && d.own) r.skip = true;
        return d;
      }
    return d;
  }

  // the status bar's line: hidden when off; "waiting" while Enable Tx is off but it may switch it on (waiting,
  // listening); "calling n/6" while it calls, n the call under way
  enum class Text {Hidden, Waiting, Calling};
  inline Text text (Run const& r)
  {
    return r.state == State::Off ? Text::Hidden : may_switch_on (r.state) ? Text::Waiting : Text::Calling;
  }

  inline int call_shown (Run const& r)
  {
    return qMin (r.calls + 1, calls);
  }
}

#endif
