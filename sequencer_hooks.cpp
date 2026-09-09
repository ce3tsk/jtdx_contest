/* CE3TSK: the sequencer's post-QSO hooks.

   process_Auto() decides, once a QSO has run its course, what happens next: whether the
   sequencer is released early, whether a station it has just worked may be picked again, and
   what gets written to the debug log about it. Those decisions are gathered here rather than
   spelled out inline, so that mainwindow.cpp carries the call sites and this file carries the
   policy - a build with a different policy replaces this file and nothing else.

   The bodies below are the plain behaviour: no early release, no veto, nothing logged. Every
   one of them is called from process_Auto() (or from the two places that invalidate its state)
   and every one is free to do nothing at all, which is what they do here.

   m_finishedCall and m_finishedTime are the state these hooks own. They stay unset in this
   build. */

#include "mainwindow.h"
#include "ui_mainwindow.h"

/* CE3TSK: three seams in the sequencer, each gathering a decision that used to be repeated at
   every call site.

   endOfQsoStopTx - what a finished QSO does to the transmitter. Eight places used to call
   autoStopTx() directly with their own reason string; they all go through here now.

   replyOtherOverridesCounters - whether replying to another station lifts the answer counters.

   haltTxWhenFrequencyTaken - whether another station appearing on our transmit frequency should
   stop the transmission. */
void MainWindow::endOfQsoStopTx(QString const& reason) { autoStopTx (reason); }

bool MainWindow::replyOtherOverridesCounters() const { return m_reply_other; }

bool MainWindow::haltTxWhenFrequencyTaken() const
{
  return m_enableTx && !m_reply_me && !m_houndMode
      && (abs (m_used_freq - ui->TxFreqSpinBox->value ()) < m_nguardfreq || m_config.halttxreplyother ());
}

/* Called in process_Auto()'s status chain, before the ordinary "AutoSeq QSO finished" branch.
   Returning true means this pass has been dealt with and the chain must not continue; the
   arguments are process_Auto()'s own working values, to be rewritten if it does. */
bool MainWindow::afterQsoFinished (QString&, QString&, QString&, int&, int&, bool&, QStringList const&)
{
  return false;
}

/* Called once autoselect has a candidate, before it is accepted - clear hisCall to reject it. */
void MainWindow::beforeAutoselectPick (QString&)
{
}

/* Called after autoselect has settled, with whatever afterQsoFinished returned for this pass. */
void MainWindow::afterAutoselectPick (bool, QString const&, int, int, QStringList const&)
{
}

/* The QSO these hooks may be holding state about is being abandoned - a mode or contest change. */
void MainWindow::onQsoAbandoned ()
{
}

/* The operator picked this station by hand, which overrides anything these hooks decided. */
void MainWindow::onCallPickedByHand (QString const&)
{
}

/* Called from readSettings(), for whatever stored state the policy keeps. */
void MainWindow::readSequencerSettings ()
{
}
