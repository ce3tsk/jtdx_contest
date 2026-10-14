#ifndef PTTONLY_H__
#define PTTONLY_H__

#include <QObject>
#include <QString>

#include "TransceiverBase.hpp"

class JTDXDateTime;

// CE3TSK 2026-10-13: the PTT-only object of a rig wrapper, and what it says went wrong.
//
// Ham Radio Deluxe, DX Lab Suite Commander and OmniRig control the rig. When the transmitter is keyed by RTS
// or DTR on a serial port of its own, a second, bare Hamlib object does the keying: TransceiverFactory makes
// it and hands it to the wrapper. It tells of trouble as every transceiver does - failure (reason), and it
// goes offline - and the three wrappers never listened. A PTT port that could not be opened: the rig came
// online, the audio went out, the transmitter was never keyed, and nothing was shown. (Until 2026-10-12 it
// was worse, the program ended there: hamliberror.h.)
//
// Now the wrapper fails with the object:
//   at the start    the rig does not come online, and the reason shown is the object's
//   keying          the same, at the moment the port fails - and when the object is not there to key
//   in between      the object polls its port; when that fails the rig goes offline with its reason
// The reason is the object's own text ("Hamlib error: IO error while opening connection to rig"), which is
// what a Hamlib rig with a PTT port of its own shows for the same fault: no new wording.
//
// The object says failure () from inside its own start () or set (), before it has shut itself down, so
// there nothing is done but to keep the reason. The wrapper acts when the call has returned - or, for a
// failure nobody called for, from the event loop.
namespace ptt_only
{
  class Watch
  {
  public:
    // Listen to the object. wrapper: the rig wrapper, in whose thread the two live. on_its_own: called from
    // the event loop with the reason of every failure, for the one nobody called for; the wrapper sees
    // there whether it is still online (it is not, when start () or key () below has told it already).
    template<typename F>
    void listen (TransceiverBase * object, QObject * wrapper, F on_its_own)
    {
      QObject::connect (object, &Transceiver::failure, wrapper, [this] (QString const& reason) {reason_ = reason;}, Qt::DirectConnection);
      QObject::connect (object, &Transceiver::failure, wrapper, on_its_own, Qt::QueuedConnection);
    }

    // start the object; false when it did not come up, and reason () says why
    bool start (TransceiverBase * object, JTDXDateTime * time)
    {
      reason_.clear ();
      object->start (0, time);
      return object->state ().online ();
    }

    // key, or unkey, through the object; false when it went down doing it - or is not up, and then nothing
    // was done: it takes no orders while it is offline
    bool key (TransceiverBase * object, bool on)
    {
      Transceiver::TransceiverState new_state {object->state ()};
      new_state.ptt (on);
      object->set (new_state, 0);
      return object->state ().online ();
    }

    // Why, for the wrapper to throw. Never empty: to the base class an exception with no text is no failure
    // at all, and the rig would come online after all.
    QString reason () const {return reason_.isEmpty () ? TransceiverBase::tr ("Unexpected rig error") : reason_;}

  private:
    QString reason_;
  };
}

#endif
