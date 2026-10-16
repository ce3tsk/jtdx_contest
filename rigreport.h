#ifndef RIGREPORT_H
#define RIGREPORT_H
// CE3TSK 2026-10-15 (TODO 3.-107, item 9 of the review 3.-104): what the main window makes of the rig's frequency
// reports, and of a QSY it could not send - the decisions alone, so that they can be tested (test_rigreport); the main
// window does what they say (displayDialFrequency (), band_changed (), setRig (), stopTx2 ()).
//
// (a) A band changed AT THE RIG is taken as one changed in the program (3.-93): a transmission halted, Enable Tx off,
//     the period not spotted, the decoder told. A late report of the program's OWN band change is not one: two quick
//     band clicks (20 m, 40 m, 20 m) gave a report of 40 m after the selector showed 20 m again, then one of 20 m after
//     the branch had put 40 m back - two band changes "at the rig", the decoder's blanking restarted late each time. A
//     report of another band within echoMs of the program's own band change is that change's echo: nothing is done,
//     not even the last band remembered, so that a rig that stays on another band is taken as changed once echoMs are
//     over.
// (b) The decoder is told of a band changed at the rig whether or not the rig was known to be online: the first report
//     after a reconnect comes before the main window marks the rig online (m_rigOk), and that one is the band change.
// (c) A band click during a transmission retuned the rig while it was still keyed, with "Allow Tx frequency changes
//     while transmitting" off: the click halts the transmission (band_changed ()), which ends m_transmitting at once, and
//     the QSY went out at once - the receive dial (setRig ()) and in split the transmit dial (setXIT ()) - while the PTT
//     dropped some 0.3 s later (stopTx (), stopTx2 ()). Seen on Hamlib's dummy rig and on the operator's IC-7300 through
//     flrig (2026-10-15): PTT on at 14.074 MHz for 0.3 s after a click from 40 m - with audio, a piece of the transmission
//     on the new band. (The review 3.-104 read it as never sent; the running program showed otherwise.) Now, with the
//     setting off and the PTT up, the QSY is kept, the latest one, and sent once the PTT is lowered (stopTx2 () runs
//     band_changed () again); asking for the frequency the rig is on drops it. Only the program's own band changes
//     (band_changed (): the selector, the band buttons, the dial, the scheduler) are kept. Stock JTDX the same. Not the
//     re-send 3.-95 removed: that was the same frequency at every return to receive.
//     2026-10-16 (TODO 3.-113, the review 3.-111): the 3.-107 record above was taken with Monitor OFF at the start
//     (make_test_ini.py), the only setup where band_changed () switches monitoring on before setRig (); with Monitor on
//     (the operator's station) a transmission switches monitoring off and the halt ends m_transmitting, so setRig () and
//     setXIT () sent nothing: before 3.-107 the click was never sent there - the review 3.-104's reading was right for
//     Monitor on. And it is held now also (G1) in the 200 ms from stopTx () to stopTx2 () - m_transmitting and g_iptt are
//     down there, the PTT is not - and (G2) with "Allow Tx frequency changes while transmitting" ON once the click has
//     halted the transmission: nothing sent it then with Monitor on, and with Monitor off it went out keyed. A band
//     changed at the rig drops a held QSY (G4); the held QSY is taken when stopTx2 ()'s queued call runs, after WSPR's
//     band hop, which drops it (G3); a close drops it (G9).
// (d) 2026-10-16 (TODO 3.-115, found on the way in 3.-113): a band changed AT THE RIG during a SPLIT transmission was not
//     seen while the PTT was up - the main window judges the Tx dial of a split transmission, "Fake It"
//     (EmulateSplitTransceiver) reported the Tx dial it had asked for whatever the rig showed, and "Rig" split did not
//     read the receive VFO while keyed (HamlibTransceiver::do_poll ()) - and with "Fake It" the PTT's drop put the rig
//     back on the old band (the transmission ran to its end, Enable Tx stayed on). Now "Fake It" reports the rig's one
//     VFO as the Tx dial, and "Rig" split reads the receive VFO by its name where the rig addresses its VFOs: the band
//     left shows on ONE dial, the other stays on the old band. receiveDialJudged (): the receive dial is the one judged
//     when it alone has left the last band. adoptMovedDial (): a band change seen on the Tx dial while the receive dial
//     the program holds is still on the old band ("Fake It": the one remembered) - the program takes the moved dial as
//     its receive dial before the halt, or the PTT's drop sends the old one (nothing goes to the rig while keyed: "Fake
//     It" keeps it on the Tx dial until then). txDialFollows (): a receive dial reported while the PTT is up moves the
//     Tx dial (setXIT ()) only where a QSY may go then - not while one is held (the halt's PTT still up: the dial just
//     taken), nor for a band changed at the rig, which halts the transmission; the next transmission sets it again.
//     2026-10-16 (TODO 3.-117, the review 3.-116 3): adoptMovedDial () for "Fake It" alone - under "Rig" split a Tx
//     dial that left the band alone is the Tx VFO moved by hand, and taking it retuned the receive VFO while keyed.
#include <QString>
#include <QtGlobal>
#include "Radio.hpp"

namespace rig_report
{
  constexpr qint64 echoMs {3000};

  enum class Action
  {
    Keep,      // the band the program knows (the last report's, or the selector's): the last band is this one
    Show,      // nothing to compare with (the first report, or a dial between bands): the selector shows it
    Echo,      // a late report of the program's own band change: nothing at all
    Changed    // a band changed at the rig: what band_changed () does, and the selector shows it
  };

  // reported: the band of the rig's report; last: the last report's band; shown: the selector's band;
  // msSinceOwnChange: since the program's own last band change (band_changed ()), -1 when there was none
  inline Action decide (QString const& reported, QString const& last, QString const& shown, qint64 msSinceOwnChange)
  {
    if (last == reported || shown == reported) return Action::Keep;
    if (last.isEmpty () || reported.isEmpty ()) return Action::Show;
    if (msSinceOwnChange >= 0 && msSinceOwnChange < echoMs) return Action::Echo;
    return Action::Changed;
  }

  // (b) whether the decoder is told of a band changed at the rig: not while transmitting (as band_changed ()), and only
  // with a rig - not whether the main window has marked it online yet
  inline bool tellDecoder (bool transmitting, QString const& rigName)
  {
    return !transmitting && !rigName.startsWith ("None");
  }

  // (c) whether a QSY waits for the PTT to drop: the PTT up - transmitting, a halted transmission whose PTT is still up
  // (pttRaised: g_iptt), or the 200 ms from stopTx () to stopTx2 () (pttDropping: ptt0Timer), where both are already down
  // and the PTT is not (3.-113, G1) - unless a transmission goes on and "Allow Tx frequency changes while transmitting"
  // is on: once a click has halted it, the QSY waits whatever the setting (3.-113, G2)
  inline bool holdQsy (bool transmitting, bool pttRaised, bool pttDropping, bool txQsyAllowed)
  {
    return (transmitting || pttRaised || pttDropping) && !(transmitting && txQsyAllowed);
  }

  // (d) splitTx: the PTT up in split; txBand, rxBand: the bands of the two dials; last: the last report's band
  inline bool receiveDialJudged (bool splitTx, QString const& txBand, QString const& rxBand, QString const& last)
  {
    return splitTx && txBand == last && !rxBand.isEmpty () && rxBand != last;
  }

  // (d) a band changed at the rig (band: its band) seen during a split transmission, the receive dial on rxBand;
  // emulated: the split is "Fake It"'s
  inline bool adoptMovedDial (bool splitTx, bool emulated, QString const& rxBand, QString const& band)
  {
    return splitTx && emulated && rxBand != band;
  }

  // (d) whether a receive dial reported with the PTT up moves the Tx dial: qsyHeld - holdQsy (); report - the decision
  // on the receive dial's band
  inline bool txDialFollows (bool pttUp, bool qsyHeld, Action report)
  {
    return !pttUp || (!qsyHeld && report != Action::Changed);
  }

  // (c) a QSY held back while the PTT is up
  class PendingQsy
  {
  public:
    // the QSY held back: kept when it is one (a frequency the rig is not on), the one kept before dropped when it is not
    void refused (Radio::Frequency wanted, Radio::Frequency rigOn)
    {
      m_pending = wanted != rigOn;
      m_frequency = wanted;
    }
    // a QSY sent, or none held back: nothing is kept
    void drop () {m_pending = false;}
    bool pending () const {return m_pending;}
    // the transmission is over: the QSY to send, once
    bool take (Radio::Frequency * frequency)
    {
      if (!m_pending) return false;
      m_pending = false;
      *frequency = m_frequency;
      return true;
    }
  private:
    bool m_pending {false};
    Radio::Frequency m_frequency {0};
  };
}
#endif
