// -*- Mode: C++ -*-
#ifndef PICKSTEP_H
#define PICKSTEP_H

/* CE3TSK 2026-10-19 (TODO.md section 7, 1f; 3.-142): the sequencer's decisions in MainWindow::processMessage () - what a
 * double click on a decoded line (and a UDP "reply", which comes the same way) sets up - as pure functions of their
 * inputs.
 *
 * They were written inline in processMessage (), and MainWindow cannot be built without a GUI, so nothing but reading
 * ever checked them: which period our transmission takes from the line, whether the line is taken at all, whether the
 * Tx frequency moves to it, when the DX call changes, and the answer - which Tx and QSO progress for a line to us, his
 * "DE ... 73" or anything else -, the auto filter and the restart of a transmission under way. processMessage () now
 * asks these functions and carries out what they return; test/harness/test_pickstep pins them, and compares
 * processMessage () with its body as it was before (commit 2091f46) over random clicks on random lines.
 *
 * Nothing here changes what the program does - the conditions and their order are the old ones, character for
 * character.
 */

#include <cmath>
#include <QString>
#include <QStringList>
#include "contestreply.h"
#include "txstep.h"

namespace pick_step
{
  using namespace tx_step;   // Progress: MainWindow::QSOProgress's order

  /* ---- our period ----
   * We transmit in the period the line was not sent in: the first of the pair for a line of the second. The line's time
   * is whole seconds, and the remainder modulo two periods is cut to whole seconds again. */
  inline bool tx_first_for_line (int lineSeconds, double trPeriod)
  {
    int const nmod = std::fmod (double (lineSeconds), 2.0 * trPeriod);
    return nmod != 0;
  }

  /* the period changes when the line asks for the other one - not for a line of our own transmission (a refusal is
     setTxFirst ()'s, asked after this) */
  inline bool period_change (bool txFirst, bool wanted, bool txMessage)
  {
    return txFirst != wanted && !txMessage;
  }

  /* ---- the line ----
   * Taken when its sender is a call - or it is "<our call> 73" to a compound call of ours (t4: the line's words, its
   * fifth and sixth being the message's first two). */
  inline bool takes_line (bool senderIsCall, QStringList const& t4, QString const& baseCall)
  {
    return senderIsCall
           || (t4.size () == 7
               && (t4.at (5) == baseCall || t4.at (5).startsWith (baseCall + '/') || t4.at (5).endsWith ('/' + baseCall))
               && t4.at (6) == "73");
  }

  /* the Tx frequency moves to the line's with the Tx frequency locked to Rx, Ctrl held, or a Tx mode changed by it */
  inline bool tx_freq_follows (bool lockInEffect, bool ctrl, bool txModeChanged)
  {
    return lockInEffect or ctrl or txModeChanged;
  }

  /* the DX call becomes the line's sender when its base call differs from the DX call's, or the sender is a compound
     call (a more qualified form of the same base call) */
  inline bool call_changes (QString const& senderBase, QString const& hisBase, QString const& sender)
  {
    return senderBase != hisBase || senderBase != sender;
  }

  /* ---- the answer ----
   * ToMe: the line carries our base call as a word (or beside a slash), or is a "DE" line not ending in 73 - answered
   * with contestreply.h's rule, or Tx5 for a 73 back to a compound call of ours. De73: "DE <call> 73" - from the
   * station worked, our 73; from another, as a CQ. Other: as a CQ - Tx1, or Tx2 with Tx1 skipped (not as a Hound), the
   * QSO history of a newly picked call then forgotten (its old steps would break the sequence).
   * The two "DE" rules never decide in processMessage (): its firstcall is DecodedText::call (), which takes no word
   * without a digit - "DE" is never one, so a DE line is answered as a CQ. Kept as written (3.-142). */
  enum class Kind {ToMe, De73, Other};

  inline Kind kind_of (QString const& dtext, QString const& baseCall, QString const& firstcall, QStringList const& t4)
  {
    if (dtext.contains (" " + baseCall + " ") || dtext.contains ("/" + baseCall + " ") || dtext.contains (" " + baseCall + "/")
        || (firstcall == "DE" && ((t4.size () > 7 && t4.at (7) != "73") || t4.size () <= 7)))
      return Kind::ToMe;
    if (firstcall == "DE" && t4.size () == 8 && t4.at (7) == "73") return Kind::De73;
    return Kind::Other;
  }

  inline int tx_to_me (QStringList const& t4, bool wwDigi, bool lastIsGrid)
  {
    if (t4.size () == 7 && t4.at (6) == "73") return 5;   // 73 back to compound call holder
    return reply_tx_to_me (wwDigi, t4.size () > 7, t4.size () > 7 ? t4.at (7) : QString {}, lastIsGrid);
  }

  inline int progress_to_me (int tx)
  {
    switch (tx)
      {
      case 5: return SIGNOFF;
      case 4: return ROGERS;
      case 3: return ROGER_REPORT;
      default: return REPORT;
      }
  }

  inline int first_tx (bool skipTx1, bool hound)
  {
    return (skipTx1 && !hound) ? 2 : 1;
  }

  inline int progress_of_first (int tx)
  {
    return 2 == tx ? REPORT : REPLYING;
  }

  /* ---- after the answer ----
   * The auto filter comes on for a call or a report of ours (AutoSeq, the option, not on yet); a transmission under way
   * restarts with the new message unless the period changed. */
  inline bool auto_filter_on_pick (bool autofilter, bool autoseq, bool filter, int progress)
  {
    return autofilter && autoseq && !filter && (progress == REPORT || progress == REPLYING);
  }

  inline bool restart_on_pick (bool transmitting, bool periodChanged)
  {
    return transmitting && !periodChanged;
  }
}

#endif
