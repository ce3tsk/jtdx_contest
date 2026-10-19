// -*- Mode: C++ -*-
#ifndef TXSTEP_H
#define TXSTEP_H

/* CE3TSK 2026-10-19 (TODO.md section 7, 1f; 3.-140): the Tx decisions of MainWindow::guiUpdate (), the timer that runs
 * the transmitter several times a second, as pure functions of their inputs.
 *
 * They were written inline in guiUpdate (), and MainWindow cannot be built without a GUI, so nothing but reading ever
 * checked them: when our period's transmission is due and where it may not go, when the sequencer is asked at the
 * start of a transmission and the PTT goes up, which field's text is sent, which halt stops it before it starts, what
 * a sent 73 does - logging, Enable Tx -, which report the log records as sent, and what the start of a transmission
 * sets. guiUpdate () now asks these functions and carries out what they return - the files, the rig, the widgets and
 * the timers stay there; test/harness/test_txstep pins the functions, and compares guiUpdate () with its body as it
 * was before (commit 2091f46) over random sequences of ticks.
 *
 * Nothing here changes what the program does - the conditions, their order and the reason texts are the old ones,
 * character for character, including terms that never decide (auto_at_tx_start's and assert_ptt's fTR < 99,
 * log_after_73's duplicate guard and its "not tuning"); they are named where they stand.
 */

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include "qsohistory.h"
#include "Radio.hpp"
#include "modetiming.h"

namespace tx_step
{
  using Frequency = Radio::Frequency;

  /* MainWindow::QSOProgress, in its order (private there; guiUpdate () asserts the two agree) */
  enum Progress {CALLING, REPLYING, REPORT, ROGER_REPORT, ROGERS, SIGNOFF};

  /* ---- when our transmission is due ----
   * Our period is the first of the pair with "Tx first" on (WSPR schedules itself), else the second. The window runs
   * from its start for the transmission's length plus the CW ID's (guiUpdate () zeroes the ID in the FT modes before
   * asking), cut at the period's end. t2p is the time within the pair of periods. */
  struct TxWindow
  {
    double tx1 = 0.0;
    double tx2 = 0.0;
  };

  inline TxWindow tx_window (double txDuration, int cwIdDits, double trPeriod, bool txFirst, QString const& mode)
  {
    TxWindow w;
    w.tx2 = txDuration;
    if (cwIdDits > 0) w.tx2 += cwIdDits * 2560.0 / 48000.0;   // full length including CW ID
    if (w.tx2 > trPeriod) w.tx2 = trPeriod;
    if (!txFirst and mode.left (4) != "WSPR")
      {
        w.tx1 += trPeriod;
        w.tx2 += trPeriod;
      }
    return w;
  }

  inline bool in_tx_window (double t2p, TxWindow const& w)
  {
    return (t2p >= w.tx1) and (t2p < w.tx2);
  }

  /* ---- where it may not transmit ----
   * Another mode inside a band's WSPR sub-band (WSPR itself may), and - with Hound "Tx frequency jumps" on - Tx1 below
   * 1000 Hz, where a Fox does not listen. The sub-band of each band by the on-air frequency's whole MHz; on 30 m JT65's
   * begins lower. The warning names the sub-band when the frequency is inside it, the Hound rule otherwise. */
  struct SubBand
  {
    Frequency from = 0;
    Frequency to = 0;
  };

  inline SubBand wspr_sub_band (Frequency onAirFreq, QString const& mode)
  {
    SubBand b;
    switch (onAirFreq / 1000000)
      {
      case 1: b.from = 1837930; b.to = 1838220; break;
      case 3: b.from = 3569930; b.to = 3570220; break;
      case 5: b.from = 5288530; b.to = 5288820; break;
      case 7: b.from = 7039930; b.to = 7040220; break;
      case 10:
        if (mode.contains ("JT65")) b.from = 10139900; else b.from = 10140030;
        b.to = 10140320;
        break;
      case 14: b.from = 14096930; b.to = 14097220; break;
      case 18: b.from = 18105930; b.to = 18106220; break;
      case 21: b.from = 21095930; b.to = 21096220; break;
      case 24: b.from = 24925930; b.to = 24926220; break;
      case 28: b.from = 28125930; b.to = 28126220; break;
      case 50: b.from = 50294370; b.to = 50294620; break;
      case 70: b.from = 70092370; b.to = 70092620; break;
      default: break;
      }
    return b;
  }

  struct Guard
  {
    bool block = false;      // no transmission: m_bTxTime off, Enable Tx off
    bool inSubBand = false;  // the warning is the WSPR sub-band's (else the Hound's)
  };

  inline Guard tx_guard (Frequency onAirFreq, SubBand const& b, QString const& mode, bool houndTxFreqJumps, int txFreq,
                         bool tx1Checked)
  {
    Guard g;
    g.block = (b.from > 0 and (onAirFreq > b.from and onAirFreq < b.to) and mode.left (4) != "WSPR")
              || (houndTxFreqJumps && txFreq < 1000 && tx1Checked);
    g.inSubBand = onAirFreq > b.from and onAirFreq < b.to;
    return g;
  }

  /* ---- the start of our period ----
   * The sequencer is asked at the start of a transmission when no decode asked it yet this period, AutoSeq is on and
   * the call mode wants it: mode 2 ("call decoded till start of Tx interval") always, modes 0 and 1 only once his 73
   * or RR73 was decoded first or the QSO is at RRR73 or later; mode 3 ("call based on end of decoding") never - the
   * decode's end asks it. fTR is the time within the period as a fraction (0 to 1): its "< 99" never decides here or
   * in assert_ptt () (the old "allow late starts"), kept as written. */
  inline bool auto_at_tx_start (bool txTime, int ptt0, float fTR, bool autoseq, bool processAutoDone, int callMode,
                                bool callFirst73, QsoHistory::Status status)
  {
    return txTime && ptt0 == 0 && fTR < 99.0 && autoseq && !processAutoDone
           && (callMode == 2 || (callMode <= 1 && (callFirst73 || status >= QsoHistory::RRR73)));
  }

  /* the PTT goes up in our period, at any point of it, or for Tune */
  inline bool assert_ptt (int iptt, bool txTime, float fTR, bool tune)
  {
    return iptt == 0 and ((txTime and fTR < 99.0) or tune);
  }

  /* Tx ok ends outside our period, unless tuning */
  inline bool period_over (bool txTime, bool tune)
  {
    return !txTime and !tune;
  }

  /* the transmitter stops when Tx ok ended while keyed, or on a halt */
  inline bool stop_tx (bool btxok, bool btxok0, int iptt, bool haltTrans)
  {
    return (!btxok && btxok0 && iptt == 1) || haltTrans;
  }

  /* ---- which text is sent ----
   * m_ntx: 1-6 the first tab's Tx1-Tx6, 7 the second tab's generated message, 8 its free text. Tx5's and the free
   * text's " ^" (the 73 marker of the field) is not sent. Any other number sends nothing. */
  struct TxFields
  {
    QString tx1, tx2, tx3, tx4, tx5, tx6, gen, free;
  };

  inline QString tx_message (int ntx, TxFields const& f)
  {
    QString t;
    if (ntx == 1) t = f.tx1;
    else if (ntx == 2) t = f.tx2;
    else if (ntx == 3) t = f.tx3;
    else if (ntx == 4) t = f.tx4;
    else if (ntx == 5) { t = f.tx5; t.remove (" ^"); }
    else if (ntx == 6) t = f.tx6;
    else if (ntx == 7) t = f.gen;
    else if (ntx == 8) { t = f.free; t.remove (" ^"); }
    return t;
  }

  /* ---- the halts at the start of a transmission ----
   * Not while tuning. The first that holds: nothing to send, no callsign of ours, a SuperFox not decoded yet (the
   * caller works that out: superFoxActive (), a DX call, not heard recently), a CQ in Hound mode (QSO progress CALLING
   * or Tx6). The unencodable message's halt (3.-135) comes later, once the message is packed. */
  enum class Halt {None, EmptyMessage, NoCallsign, SuperFoxNotDecoded, HoundCQ};

  inline Halt start_halt (bool tune, int msgLength, bool noCallsign, bool superFoxBlind, bool hound, bool progressCalling,
                          int ntx)
  {
    if (tune || !(msgLength == 0 || noCallsign || superFoxBlind || (hound && (progressCalling || ntx == 6)))) return Halt::None;
    if (msgLength == 0) return Halt::EmptyMessage;
    if (noCallsign) return Halt::NoCallsign;
    if (superFoxBlind) return Halt::SuperFoxNotDecoded;
    return Halt::HoundCQ;
  }

  /* haltTx ()'s reason, as the debug record writes it */
  inline QString halt_reason (Halt h, QString const& hisCall)
  {
    switch (h)
      {
      case Halt::EmptyMessage: return "Transmission of the empty message is not allowed ";
      case Halt::NoCallsign: return "Transmission halted: user callsign is not configured ";
      case Halt::SuperFoxNotDecoded: return "Transmission halted: SuperFox " + hisCall + " has not been decoded yet ";
      case Halt::HoundCQ: return "Transmission of CQ message is not allowed in the Hound mode ";
      case Halt::None: break;
      }
    return {};
  }

  /* ---- a 73 sent ----
   * A message of type 0-5 or 7 with the word 73 or RR73, or a free text (type 6) with 73 in a word, is a 73 - counted
   * only from QSO progress ROGER_REPORT on or as Tx4/Tx5. The FIRST 73 to a station not logged last, unless the message
   * before was a 73 to him already, logs the QSO (prompt or autolog, not tuning); the duplicate guard behind it never
   * decides - sent_first_73 () already requires another call than the one logged last (3.-124 item 9) - and nor does its
   * "not tuning": while tuning guiUpdate () sends TUNE, of type -1, which is no 73. Both kept as written.
   * Any 73 sent with "disable Tx after 73" on and AutoSeq off switches Enable Tx off. */
  inline bool message_is_73 (int type, QStringList const& msg_parts)   // was mainwindow.cpp's own
  {
    return type >= 0
      && (((type < 6 || 7 == type)
           && (msg_parts.contains ("73") || msg_parts.contains ("RR73")))
          || (type == 6 && !msg_parts.filter ("73").isEmpty ()));
  }

  inline bool is_73 (bool progressRogerReportOrLater, int ntx, int type, QStringList const& msg_parts)
  {
    return (progressRogerReportOrLater || ntx == 4 || ntx == 5) && message_is_73 (type, msg_parts);
  }

  inline bool sent_first_73 (bool is73, QString const& lastLoggedCall, QString const& hisCall, bool lastIs73,
                             QString const& lastMessageSent)
  {
    return is73 && lastLoggedCall != hisCall
           && (!lastIs73 || (lastIs73 && !lastMessageSent.contains (hisCall)));
  }

  inline bool log_after_73 (bool promptToLog, bool autolog, bool tune, QString const& lastLoggedCall,
                            QString const& hisCall, qint64 msSinceLogged, double trPeriod)
  {
    return (promptToLog || autolog) && !tune
           && (lastLoggedCall != hisCall || qAbs (msSinceLogged) > relog_guard_ms (trPeriod, 2));
  }

  inline bool tx_off_after_73 (bool is73, bool disableTxOn73, bool autoseq)
  {
    return is73 && disableTxOn73 && !autoseq;
  }

  /* ---- the report the log records as sent ----
   * From a message of type 0-5 whose second word is our call (or base call): its third word as a report of -50 to
   * +49, or with an R before it; anything else leaves the one recorded. */
  inline QString report_sent (int type, QStringList const& msg_parts, QString const& myCall, QString const& baseCall,
                              QString const& current)
  {
    if (!(type < 6 && msg_parts.length () >= 3 && (msg_parts[1] == myCall || msg_parts[1] == baseCall))) return current;
    bool ok;
    int i1 = msg_parts[2].toInt (&ok);
    if (ok and i1 >= -50 and i1 < 50) return msg_parts[2];
    if (msg_parts[2].mid (0, 1) == "R")
      {
        i1 = msg_parts[2].mid (1).toInt (&ok);
        if (ok and i1 >= -50 and i1 < 50) return msg_parts[2].mid (1);
      }
    return current;
  }

  /* ---- the PTT went up ----
   * A new message restarts the Tx watchdog (not in WSPR); a CQ under AutoSeq empties the DX call; a Hound's Tx1 frequency
   * is kept as the calling frequency; the message sent sets the QSO progress (Tx1-Tx6; the second tab's leaves it);
   * the auto filter goes off at the progress its setting names (2: calling again, 1: sending our 73 - or RRR's, with
   * RR73 sent instead, at ROGERS); Tx5 or the free text switches AutoSeq off when so set. */
  inline bool watchdog_reset_at_tx (int watchdogMinutes, bool wspr, QString const& msg, QString const& lastMsg)
  {
    return watchdogMinutes && !wspr && msg != lastMsg;
  }

  inline bool clear_dx_for_cq (bool autoseq, bool progressCalling, int ntx, bool hisCallEmpty)
  {
    return autoseq && (progressCalling || ntx == 6) && !hisCallEmpty;
  }

  inline bool hound_calling_freq (bool hound, bool progressReplying, int ntx)
  {
    return hound && (progressReplying || ntx == 1);
  }

  inline int progress_of_tx (int ntx, int progress)
  {
    switch (ntx)
      {
      case 1: return REPLYING;
      case 2: return REPORT;
      case 3: return ROGER_REPORT;
      case 4: return ROGERS;
      case 5: return SIGNOFF;
      case 6: return CALLING;
      default: return progress;   // determined elsewhere
      }
  }

  inline bool filter_off_at_tx (bool filter, int filterState, int progress, bool rrr)
  {
    if (!filter) return false;
    if (filterState == 2) return progress == CALLING;                                   // at getting 73
    if (filterState == 1) return progress == SIGNOFF || (!rrr && progress == ROGERS);   // at sending 73
    return false;
  }

  inline bool autoseq_off_at_tx (int tab, bool tx5Checked, bool freeTextChecked, bool autoseq, bool tx5SetsOff,
                                 bool freeTextSetsOff)
  {
    if (0 == tab) return tx5Checked && autoseq && tx5SetsOff;
    if (1 == tab) return freeTextChecked && autoseq && freeTextSetsOff;
    return false;
  }

  /* ---- once a second: the Tx watchdog ----
   * It switches Enable Tx off after its minutes without a new message (not while transmitting, not in WSPR). */
  inline bool watchdog_fires (int watchdogMinutes, bool transmitting, bool wspr, int idleMinutes)
  {
    return watchdogMinutes && !transmitting && !wspr && idleMinutes >= watchdogMinutes;
  }
}

#endif
