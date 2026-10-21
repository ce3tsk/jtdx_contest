#include "EmulateSplitTransceiver.hpp"

#include "moc_EmulateSplitTransceiver.cpp"

EmulateSplitTransceiver::EmulateSplitTransceiver (std::unique_ptr<Transceiver> wrapped, QObject * parent)
  : Transceiver {parent}
  , wrapped_ {std::move (wrapped)}
  , rx_frequency_ {0}
  , tx_frequency_ {0}
  , split_ {false}
{
  // Connect update signal of wrapped Transceiver object instance to ours.
  connect (wrapped_.get (), &Transceiver::update, this, &EmulateSplitTransceiver::handle_update);

  // Connect other signals of wrapped Transceiver object to our
  // parent matching signals.
  connect (wrapped_.get (), &Transceiver::resolution, this, &Transceiver::resolution);
  connect (wrapped_.get (), &Transceiver::tciframeswritten, this, &Transceiver::tciframeswritten);
  connect (wrapped_.get (), &Transceiver::tci_mod_active, this, &Transceiver::tci_mod_active);
  connect (wrapped_.get (), &Transceiver::finished, this, &Transceiver::finished);
  connect (wrapped_.get (), &Transceiver::failure, this, &Transceiver::failure);
  connect (wrapped_.get (), &Transceiver::failure, this, [this] {failed_ = true;});   // CE3TSK 2026-10-21 (3.-173)
}

/* CE3TSK 2026-10-21 (TODO.md 3.-173): stopped while keyed in split - the program closed while transmitting - the rig
   is on the Tx dial, and the wrapped rig's shutdown drops the PTT without knowing the receive dial: the rig stayed on
   the Tx dial (14.075 for 14.074 and the Tx audio at 2790 Hz). Unkeyed first, as the PTT's drop does it, which brings
   it back to the receive dial - not on a rig that failed, which a state would start again */
void EmulateSplitTransceiver::stop () noexcept
{
  if (requested_.ptt () && split_ && !failed_)
    {
      TransceiverState unkeyed {requested_};
      unkeyed.ptt (false);
      set (unkeyed, sequence_number_);
    }
  wrapped_->stop ();
}

void EmulateSplitTransceiver::set (TransceiverState const& s, unsigned sequence_number) noexcept
{
#if WSJT_TRACE_CAT
  qDebug () << "EmulateSplitTransceiver::set: state:" << s << "#:" << sequence_number;
#endif
  /* CE3TSK 2026-10-16 (TODO 3.-115): a rig moved off the Tx dial while keyed (by hand) stays where it was put until the
     PTT drops - each state sent while keyed put it back on the Tx dial, keyed (TransceiverBase takes a frequency the
     rig reports for the one asked for, so it differed). (3.-117, the review 3.-116 2) A Tx dial asked for anew while
     keyed ends it - the program's own QSY while transmitting (Tx frequency changes allowed): it is the program's word,
     and a rig that rounds the Tx dial (a calibrated one, 10 Hz steps) set it at every transmission */
  if (!s.ptt () || s.tx_frequency () != tx_frequency_) moved_ = false;

  // save for use in updates
  requested_ = s;   // CE3TSK 2026-10-21 (3.-173): and for stop ()
  sequence_number_ = sequence_number;
  rx_frequency_ = s.frequency ();
  tx_frequency_ = s.tx_frequency ();
  split_ = s.split ();

  TransceiverState emulated_state {s};
  if (s.ptt () && split_) emulated_state.frequency (moved_ ? moved_to_ : s.tx_frequency ());
  emulated_state.split (false);
  emulated_state.tx_frequency (0);
  wrapped_->set (emulated_state, sequence_number);
}

void EmulateSplitTransceiver::handle_update (TransceiverState const& state,
                                             unsigned sequence_number)
{
#if WSJT_TRACE_CAT
  qDebug () << "EmulateSplitTransceiver::handle_update: from wrapped:" << state;
#endif

  if (state.split ())
    {
      Q_EMIT failure (tr ("Emulated split mode requires rig to be in simplex mode"));
    }
  else
    {
      TransceiverState new_state {state};

      // These are always what was requested in prior set state operation
      new_state.tx_frequency (tx_frequency_);
      new_state.split (split_);

      /* CE3TSK 2026-10-16 (TODO 3.-115): keyed in split, the rig's one VFO is the Tx dial - reported as the rig shows it,
         so that a band changed at the rig shows while the PTT is up (the main window judges the Tx dial of a split
         transmission); it reported the Tx dial asked for, whatever the rig showed, and the PTT's drop then put the rig
         back on the old band. The receive dial is the one remembered, as before. Keyed and not in split the VFO is the
         receive dial, reported as it is - as without "Fake It" (it reported the one remembered) */
      if (state.ptt () && split_)
        {
          new_state.frequency (rx_frequency_);   // Follow the rig if in RX mode.
          if (state.frequency ()) new_state.tx_frequency (state.frequency ());
          if (state.frequency () && (moved_ || state.frequency () != tx_frequency_))
            {
              moved_ = true;
              moved_to_ = state.frequency ();
            }
        }

#if WSJT_TRACE_CAT
      qDebug () << "EmulateSplitTransceiver::handle_update: signalling:" << state;
#endif

      // signal emulated state
      Q_EMIT update (new_state, sequence_number);
    }
}
