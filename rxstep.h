// -*- Mode: C++ -*-
#ifndef RXSTEP_H
#define RXSTEP_H

/* CE3TSK 2026-10-19 (TODO.md section 7, 1f; 3.-141): the sequencer's decisions in MainWindow::readFromStdout (), which
 * takes every line the decoder prints, as pure functions of their inputs.
 *
 * They were written inline in readFromStdout (), and MainWindow cannot be built without a GUI, so nothing but reading
 * ever checked them: when the sequencer is asked at the end of a decode or at a line of the station worked, what his
 * line says about whom he answers (the flags the "reply to other" rule reads), when the Rx frequency follows him, which
 * lines go to the Rx frequency window, and when his line tells that he works another station on our frequency - the
 * seam haltTxWhenFrequencyTaken () decides what then happens (sequencer_hooks.cpp). readFromStdout () now asks these
 * functions and carries out what they return; test/harness/test_rxstep pins them, and compares readFromStdout () with
 * its body as it was before (commit 2091f46) over random sequences of decoder lines.
 *
 * Nothing here changes what the program does - the conditions, their order and their words (" 73" with its blank,
 * "RR73" without in one place and with it in another) are the old ones, character for character.
 */

#include <QString>
#include <QStringList>
#include <QtGlobal>
#include "qsohistory.h"

namespace rx_step
{
  /* ---- the end of a decode ----
   * The sequencer is asked once a decode ends, unless a line asked it already or the operator pressed Decode. */
  inline bool auto_at_decode_end (bool autoseq, bool processAutoDone, bool manualDecode)
  {
    return autoseq && !processAutoDone && !manualDecode;
  }

  /* ---- a line of the station worked ----
   * fromHim: the line's sender (deCallAndGrid) is the DX call's base call. Until he has answered us, each of his lines
   * says whom he answers: us (me - the flags are settled then), a CQ or a 73 of his (cq73: whatever came before, he is
   * between QSOs), anyone else (other - unless a CQ or 73 of his was seen). */
  struct ReplyFlags   // an aggregate, built from MainWindow's three flags (C++11: no member defaults)
  {
    bool me;
    bool other;
    bool cq73;
  };

  inline ReplyFlags reply_flags (ReplyFlags f, bool fromHim, bool myCallInMsg, QString const& msg)
  {
    if (!fromHim || f.me) return f;
    if (myCallInMsg)
      {
        f.me = true;
        f.other = false;
      }
    else if (msg.contains ("73") || msg.left (2) == "CQ")
      {
        f.other = false;
        f.cq73 = true;
      }
    else if (!f.cq73) f.other = true;
    return f;
  }

  /* the Rx frequency follows his lines (with a DX call) */
  inline bool rx_follows (bool hisCallKnown, bool fromHim)
  {
    return hisCallKnown && fromHim;
  }

  /* ---- the Rx frequency window ----
   * A line goes there when it is within 10 Hz of the Rx frequency, or carries our call (with that option), or a call
   * with one of the "content" suffixes (/P, /QRP ... - items of 3 to 6 characters), or - with Enable Tx off - the DX
   * call's 73 or RR73 as the third of four words, or a wanted call (with that option; notified bit 8). It is shown
   * there without its filters when they are bypassed or the line comes from a decode again (nagain, nagainfil). */
  inline bool content_line (bool enabled, QString const& tcut, QString const& items)
  {
    if (!enabled || !tcut.contains ("/")) return false;
    for (auto const& item : items.split (","))
      if (item.length () > 2 && item.length () < 7 && tcut.contains ("/" + item)) return true;
    return false;
  }

  inline bool dx_call_73 (QStringList const& messageWords, bool enableTx, QString const& dxBaseCall)
  {
    return messageWords.length () == 4 && !enableTx && !dxBaseCall.isEmpty () && messageWords.at (2).contains (dxBaseCall)
           && messageWords.at (3).contains ("73");
  }

  inline bool to_rx_window (int freq, int rxFreq, bool showMyCall, bool myCallInMsg, bool content, bool dxCall73,
                            bool showWanted, int notified)
  {
    return qAbs (freq - rxFreq) <= 10 || (showMyCall && myCallInMsg) || content || dxCall73 || (showWanted && notified & 8);
  }

  inline bool bypass_rx_filters (bool bypassRxfFilters, int nagain, int nagainfil)
  {
    return bypassRxfFilters || nagain == 1 || nagainfil == 1;
  }

  /* ---- what a decoded line asks of the sequencer ----
   * A line with our call, not from the Decode button:
   *   - from the station worked: the sequencer is asked at once, unless a line asked it already, when it is no 73 and no
   *     RR73 - or the 73 that answers our RRR (status RRREPORT, "RRR" set), the RR73 that answers our report (RREPORT) -,
   *     or in call mode 0, Single shot or Hound mode; a 73 or RR73 otherwise only notes, in call modes 0 and 1, that his
   *     final came first (m_callFirst73: the start of our period asks the sequencer);
   *   - from anyone else, in call mode 1 ("call first") with no DX call: the sequencer is asked at once (the first
   *     caller is answered).
   * Any other line from the station worked - not a CQ, DE or QRZ, no " 73", " RR73" or " RRR" - is him working
   * another station on that frequency: HisOtherQso (the frequency is recorded, the seam decides on a halt). A line with
   * our call that came from the Decode button counts as that too, kept as written. */
  enum class OnDecode {None, ProcessAuto, CallFirst73, HisOtherQso};

  struct DecodeInputs
  {
    bool myCallInMsg = false;
    bool manualDecode = false;
    bool fromHim = false;
    bool hisCallEmpty = true;
    bool processAutoDone = false;
    bool autoseq = false;
    bool rrr = false;
    bool singleshot = false;
    bool hound = false;
    int callMode = 0;
    QsoHistory::Status status = QsoHistory::NONE;
    QString msg;                 // DecodedText::message ()
  };

  inline OnDecode on_decode (DecodeInputs const& in)
  {
    if (in.myCallInMsg && !in.manualDecode)
      {
        if (in.fromHim)
          {
            bool const s73 = in.msg.contains (" 73"), rr73 = in.msg.contains ("RR73");
            if (!in.processAutoDone && in.autoseq
                && ((!s73 && !rr73) || (s73 && in.status == QsoHistory::RRREPORT && in.rrr)
                    || (rr73 && in.status == QsoHistory::RREPORT) || in.callMode == 0 || in.singleshot || in.hound))
              return OnDecode::ProcessAuto;
            if ((s73 || rr73) && in.callMode <= 1) return OnDecode::CallFirst73;
            return OnDecode::None;
          }
        if (in.callMode == 1 && !in.processAutoDone && in.autoseq && in.hisCallEmpty) return OnDecode::ProcessAuto;
        return OnDecode::None;
      }
    if (in.fromHim && in.msg.left (3) != "CQ " && in.msg.left (3) != "DE " && in.msg.left (4) != "QRZ "
        && !in.msg.contains (" 73") && !in.msg.contains (" RR73") && !in.msg.contains (" RRR"))
      return OnDecode::HisOtherQso;
    return OnDecode::None;
  }
}

#endif
