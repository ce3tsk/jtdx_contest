// -*- Mode: C++ -*-
#ifndef AUTOSEQSTEP_H
#define AUTOSEQSTEP_H

/* CE3TSK 2026-10-17 (TODO.md section 7, step 1): the decisions of MainWindow::process_Auto (), the auto
 * sequencer's step after every decode, as pure functions of their inputs.
 *
 * They were written inline in process_Auto (), and MainWindow cannot be built without a GUI, so nothing but
 * reading ever checked them: which counter gives up on a station and with what reason, Hound's drop after three
 * R+report, the flag word autoselect is handed, which message answers which QSO step, and when the transmitter
 * stops. process_Auto () now asks these functions and carries out what they return; test/harness/test_autoseqstep
 * pins them, and compares process_Auto () with the chain as it was written before over a million random states.
 *
 * Nothing here changes what the sequencer does - the conditions, their order and the reason texts are the old
 * ones, character for character. The seams of sequencer_hooks.cpp stay where they were, in process_Auto ().
 */

#include <cmath>
#include <cstdlib>
#include <QString>
#include "qsohistory.h"
#include "priorities.h"
#include "modetiming.h"

namespace autoseq_step
{

  /* ---- the station being worked: Hound mode ----
   * A WSJT-X Fox drops a Hound whose R+report it has not decoded after three tries, so the Hound gives up too:
   * drop - halt, his count back to RCQ, Tx to the frequency we called on. Otherwise, with "Tx frequency jumps"
   * on, the Tx frequency follows the Fox's answer: on it the first time, 300 Hz away after that. */
  struct HoundStep
  {
    bool drop = false;
    int txFreq = -1;   // the Tx frequency to set, -1 for none
  };

  inline HoundStep hound (QsoHistory::Status status, int count, bool txFreqJumps, int rx)
  {
    HoundStep s;
    if (status != QsoHistory::SRREPORT && status != QsoHistory::RREPORT) return s;
    if (count > 3) { s.drop = true; return s; }
    if (txFreqJumps && rx > 199 && rx < 1000)
      {
        if (count == 1) s.txFreq = rx;
        else if (rx < 600) s.txFreq = rx + 300;
        else s.txFreq = rx - 300;
      }
    return s;
  }

  /* ---- the station being worked: the rules that let him go ----
   * Asked outside Hound mode only (the else of process_Auto ()'s `if (m_houndMode)`) and when the post-QSO seam
   * (afterQsoFinished) did not deal with the pass - so Hound mode is no term of these rules (3.-135, the review 3.-124
   * item 4: the old condition's `!hound` was always true here). The first rule that holds wins:
   *   QsoFinished        the QSO is over and logged and we are not keeping the frequency
   *   AnswerCQCount      "answer CQ" counter: he did not come back after we called him (RCQ, SCALL; SREPORT when
   *                      Tx1 is skipped)
   *   AnswerInCallCount  "answer in call" counter: he called us, we sent the report and heard nothing (RCALL;
   *                      SREPORT when Tx1 is not skipped)
   *   SentRReportCount   "sent R+report" counter
   *   SentRR7373Count    "sent RRR/RR73/73" counter
   * Replying to another station (replyOther) lifts the first two counters before they are reached. */
  enum class Rule { None, QsoFinished, AnswerCQCount, AnswerInCallCount, SentRReportCount, SentRR7373Count };

  struct Worked
  {
    QsoHistory::Status status = QsoHistory::NONE;
    int count = 0;             // autoseq ()'s count, one less while transmitting
    int prio = 0;
    bool singleshot = false;
    bool autolog = false;
    bool loggedHim = false;    // the last QSO logged is with him
    bool lockTxFreq = false;
    int rx = 0;                // autoseq ()'s rx and tx
    int tx = 0;
    int txFreq = 0;            // the Tx frequency box
    int guardFreq = 0;         // m_nguardfreq
    bool skipTx1 = false;
    bool strictdirCQ = false;
    bool replyOther = false;   // replyOtherOverridesCounters ()
    bool answerCQCount = false;      int answerCQCounter = 0;
    bool answerInCallCount = false;  int answerInCallCounter = 0;
    bool sentRReportCount = false;   int sentRReportCounter = 0;
    bool sentRR7373Count = false;    int sentRR7373Counter = 0;
  };

  struct Release
  {
    Rule rule = Rule::None;
    QString reason;              // clearDX ()'s reason, as the debug record writes it
    bool armCounter = false;     // m_counter = the "answer CQ" counter
    bool callList = false;       // he goes on the call list (QsoHistory::calllist)
    bool resetCount = false;     // his count is reset (QsoHistory::reset_count)
    bool counters2Off = false;   // no CQ priority in this pass's autoselect
    bool countersOff = false;    // no autoselect in this pass: Single shot ends here
  };

  inline Release release (Worked const& w)
  {
    using S = QsoHistory::Status;
    Release r;
    if ((w.status == S::SRR73 || w.status >= S::S73) && !w.singleshot && !w.autolog && w.loggedHim && !w.lockTxFreq
        && (w.tx == 1 || std::abs (w.rx - w.txFreq) > w.guardFreq))
      {
        r.rule = Rule::QsoFinished;
        r.reason = " cleared, AutoSeq QSO finished";
      }
    else if ((w.status == S::RCQ || w.status == S::SCALL || (w.status == S::SREPORT && w.skipTx1))
             && w.answerCQCount && (decode_priority::counterGivesUp (w.prio) || w.strictdirCQ)
             && (w.answerCQCounter <= w.count || w.replyOther))
      {
        r.rule = Rule::AnswerCQCount;
        /* CE3TSK 2026-10-05: say which rule cleared it - "count reached" was written when he answered another
           station after one call, which read as a miscount (W7JDB, count 1 of 4) */
        r.reason = w.answerCQCounter <= w.count ? QString {" cleared, RCQ/SCALL/SREPORT count reached"}
                 : " cleared, answering another station (RCQ/SCALL/SREPORT count " + QString::number (w.count) + " of "
                   + QString::number (w.answerCQCounter) + " not reached)";
        if (w.replyOther) r.counters2Off = true;
        else { r.armCounter = true; r.callList = true; }
        r.resetCount = true;
        r.countersOff = w.singleshot;
      }
    else if ((w.status == S::RCALL || (w.status == S::SREPORT && !w.skipTx1)) && w.answerInCallCount
             && (w.answerInCallCounter <= w.count || w.replyOther))
      {
        r.rule = Rule::AnswerInCallCount;
        r.reason = w.answerInCallCounter <= w.count ? QString {" cleared, RCALL/SREPORT count reached"}
                 : " cleared, answering another station (RCALL/SREPORT count " + QString::number (w.count) + " of "
                   + QString::number (w.answerInCallCounter) + " not reached)";
        r.callList = true;
        r.resetCount = true;
        r.counters2Off = true;
        r.countersOff = w.singleshot;
      }
    else if ((w.status == S::RREPORT || w.status == S::SRREPORT) && w.sentRReportCount
             && w.sentRReportCounter <= w.count)
      {
        r.rule = Rule::SentRReportCount;
        r.reason = " cleared, RREPORT/SRREPORT count reached";
        r.resetCount = true;
        r.counters2Off = true;
        r.countersOff = w.singleshot;
      }
    else if ((w.status == S::RRR || w.status == S::RRR73 || w.status == S::R73 || w.status == S::SRR73
              || w.status == S::S73) && w.sentRR7373Count && w.sentRR7373Counter <= w.count)
      {
        r.rule = Rule::SentRR7373Count;
        r.reason = " cleared, RRR|RR73|R73 count reached";
        r.resetCount = true;
        r.counters2Off = true;
        r.countersOff = w.singleshot;
      }
    return r;
  }

  /* ---- autoselect: the flag word handed to QsoHistory::autoseq () for a new pick ----
   * time 1 asks it to look at the answers to our CQ first; that is done when "CQ priority" is on, the Tx frequency
   * is not locked, the pass has not just let a station go by a counter, no "answer CQ" count-down runs (m_counter),
   * and the period decoded is the one the other side transmits in. Otherwise time is 0 and the count-down steps.
   * The period is told from the decode's start: whole seconds plus one, modulo two periods - 0 for the second
   * period of the pair. 128 answer worked-before, 64 call worked-before (256 a higher new call instead), 16 the
   * report decides, 32 the distance decides. With no new-one tier chosen at all, worked-before is answered and
   * called. */
  inline int decoded_period_mod (qint64 msDecStarted, double trPeriod)
  {
    auto ms = msDecStarted % 86400000;
    auto secs = std::round (ms / 1000.0) + 1;
    int nmod = std::fmod (double (secs), 2.0 * trPeriod);
    return nmod;
  }

  struct PickInputs
  {
    qint64 msDecStarted = 0;
    double trPeriod = 15.0;
    bool callPrioCQ = false;
    bool lockTxFreq = false;
    bool counters2 = true;
    int counter = 0;           // m_counter
    bool txFirst = false;
    bool anyNewTierChosen = false;
    bool answerWorkedB4 = false;
    bool callWorkedB4 = false;
    bool callHigherNewCall = false;
    bool rprtPriority = false;
    bool maxDistance = false;
  };

  struct Pick
  {
    unsigned time = 0;
    int counter = 0;           // m_counter after this pass
  };

  inline Pick pick_flags (PickInputs const& in)
  {
    Pick p;
    p.counter = in.counter;
    int const nmod = decoded_period_mod (in.msDecStarted, in.trPeriod);
    if (in.callPrioCQ && !in.lockTxFreq && in.counters2 && in.counter == 0 && in.txFirst != (nmod != 0)) p.time = 1;
    else { if (p.counter > 0) p.counter -= 1; p.time = 0; }
    if (!in.anyNewTierChosen || in.answerWorkedB4) p.time |= 128;
    if (!in.anyNewTierChosen || in.callWorkedB4) p.time |= 64;
    else if (in.callHigherNewCall) p.time |= 256;
    if (in.rprtPriority) p.time |= 16;
    if (in.maxDistance) p.time |= 32;
    return p;
  }

  /* ---- the answer: which message the QSO step sends ----
   * A report heard from him that differs from ours is taken first (new_report): it regenerates the messages, and
   * an SREPORT step then becomes RCQ (Tx1 skipped, not a Hound) or RREPORT. In the WW Digi contest the report field
   * carries his grid, which is not a report. */
  inline bool new_report (QString const& rpt, QString const& current, bool wwDigi)
  {
    bool rptIsExchange = false;
    if (wwDigi && !rpt.isEmpty ()) { bool numeric = false; rpt.toInt (&numeric); rptIsExchange = !numeric; }
    return !rpt.isEmpty () && rpt != current && !rptIsExchange;
  }

  inline QsoHistory::Status after_new_report (QsoHistory::Status status, bool skipTx1, bool hound)
  {
    if (status != QsoHistory::SREPORT) return status;
    return (skipTx1 && !hound) ? QsoHistory::RCQ : QsoHistory::RREPORT;
  }

  struct ReplyInputs
  {
    QsoHistory::Status status = QsoHistory::NONE;
    bool skipTx1 = false;
    bool hound = false;
    bool wwDigi = false;
    bool hisGridKnown = false;   // m_hisGrid, as it stands after this pass's grid
    bool enableTx = false;
    bool autoseq = true;
    int callMode = 0;
    bool autofilter = false;     // the auto filter setting
    bool filterOn = false;       // the filter as it is
    bool singleshot = false;
    bool autolog = false;
    bool loggedHim = false;      // the last QSO logged is with him
    bool houndLogDue = false;    // Hound: not logged with him within the guard (hound_log_due ())
    bool txActive = false;       // Enable Tx, transmitting, Tx ok or the PTT
  };

  struct Reply
  {
    int tx = 0;                  // Tx1..Tx5 to select, 0 for none
    bool tab2Ntx7 = false;       // on the second tab m_ntx becomes 7 after the message (RFIN and RCQ only)
    bool watchdogReset = false;  // txwatchdog (false)
    bool autoFilter = false;     // autoFilter (true)
    bool filterFirst = false;    // ... before the message is chosen (RREPORT's order; RCALL's is after it)
    bool houndLog = false;       // log the DXpedition QSO now
    bool houndHalt = false;      // haltTx ("end of DXpedition QSO ")
    QString stopReason;          // endOfQsoStopTx (stopReason) unless empty
  };

  /* Hound: the DXpedition QSO is logged at RRR73 unless it was logged with him within seven periods. */
  inline bool hound_log_due (bool loggedHim, qint64 msSinceLogged, double trPeriod)
  {
    return !loggedHim || qAbs (msSinceLogged) > relog_guard_ms (trPeriod, 7);
  }

  inline Reply reply (ReplyInputs const& in)
  {
    using S = QsoHistory::Status;
    Reply r;
    switch (in.status)
      {
      case S::RFIN:
      case S::RCQ:
        r.tx = (in.skipTx1 && !in.hound) ? 2 : 1;
        r.tab2Ntx7 = true;
        break;
      case S::RCALL:
        r.watchdogReset = in.enableTx && in.autoseq && in.callMode > 0;
        /* CE3TSK: WW Digi contest - his call carries his exchange already, so acknowledge it with Tx3 "HISCALL MYCALL
           R MYGRID" instead of repeating a bare exchange with Tx2. That produces the published 4 message contest
           sequence. Only when his grid is known, otherwise there is nothing to roger yet. */
        r.tx = (in.wwDigi && in.hisGridKnown) ? 3 : 2;
        r.autoFilter = in.autofilter && in.enableTx && !in.filterOn;
        break;
      case S::RREPORT:
        r.watchdogReset = in.enableTx && in.autoseq && in.callMode > 0;
        r.autoFilter = in.autofilter && in.enableTx && !in.filterOn;
        r.filterFirst = true;
        r.tx = 3;
        break;
      case S::RRREPORT:
        r.tx = 4;
        break;
      case S::RRR:
      case S::R73:
        r.tx = 5;
        break;
      case S::RRR73:
        if (!in.hound) r.tx = 5;
        else { r.houndLog = in.houndLogDue; r.houndHalt = in.txActive; }
        break;
      /* CE3TSK: the SRR73/S73/FIN cases below are the fallback to CQ. They are reached only when the post-QSO hook
         did not deal with the pass (sequencer_hooks.cpp); otherwise the QSO was cleared there and the step is NONE. */
      case S::SRR73:
        if (!in.singleshot && !in.autolog && in.loggedHim) r.stopReason = "SRR73, none received ";
        break;
      case S::S73:
        r.stopReason = "S73, none received ";
        break;
      case S::FIN:
        if (in.singleshot) r.stopReason = "FIN, end of QSO, Singleshot ";
        else if (in.autolog) r.stopReason = "FIN, end of QSO, Autolog ";
        else if (!in.loggedHim) r.stopReason = "FIN, end of QSO, Call not logged ";
        else r.stopReason = "FIN, end of QSO, not owner of the frequency ";
        break;
      default:
        break;
      }
    return r;
  }

  /* No QSO step, and a counter ended the pass's search (countersOff): Single shot and Hound stop there.
     The Hound line cannot be reached from process_Auto (): it switches the counters off only with Single shot on
     (release ()'s countersOff, the post-QSO seam), and Single shot is asked first. Kept as the original had it - the
     sequencer is not changed in this refactor (TODO.md 3.-124 item 5, section 7). */
  inline QString counters_stop_reason (bool counters, bool singleshot, bool hound)
  {
    if (counters) return {};
    if (singleshot) return "m_singleshot, counter triggered ";
    if (hound) return "m_houndMode, counter triggered ";
    return {};
  }

}

#endif
