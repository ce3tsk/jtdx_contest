#ifndef EMULATE_SPLIT_TRANSCEIVER_HPP__
#define EMULATE_SPLIT_TRANSCEIVER_HPP__

#include <memory>

#include "Transceiver.hpp"

// Emulate Split Transceiver
//
// Helper decorator class that encapsulates  the emulation of split TX
// operation.
//
// Responsibilities
//
//  Delegates  all  but setting  of  other  (split) frequency  to  the
//  wrapped Transceiver instance. Also routes failure signals from the
//  wrapped Transceiver instance to this instances failure signal.
//
//  Intercepts status  updates from  the wrapped  Transceiver instance
//  and re-signals it with the emulated status.
//
//  Generates a status update signal if the other (split) frequency is
//  changed, this is necessary  since the wrapped transceiver instance
//  never receives other frequency changes.
//
class EmulateSplitTransceiver final
  : public Transceiver
{
  Q_OBJECT

public:
  // takes ownership of wrapped Transceiver
  explicit EmulateSplitTransceiver (std::unique_ptr<Transceiver> wrapped,
                                    QObject * parent = nullptr);

  void set (TransceiverState const&,
            unsigned sequence_number) noexcept override;

  // forward everything else to wrapped Transceiver
  void start (unsigned sequence_number, JTDXDateTime* jtdxtime) noexcept override {wrapped_->start (sequence_number,jtdxtime);}
  void stop () noexcept override;

private:
  void handle_update (TransceiverState const&, unsigned seqeunce_number);

  std::unique_ptr<Transceiver> wrapped_;
  Frequency rx_frequency_;        // requested Rx frequency
  Frequency tx_frequency_;        // requested Tx frequency
  bool split_; // requested split state
  // CE3TSK 2026-10-16 (TODO 3.-115): keyed in split, the rig moved off the Tx dial (by hand): where it went - not driven
  // back while the PTT is up
  bool moved_ {false};
  Frequency moved_to_ {0};
  // CE3TSK 2026-10-21 (TODO.md 3.-173): the last state asked for and its number - stop () unkeys with them - and
  // whether the wrapped rig failed (it is offline then: a state sent would start it again)
  TransceiverState requested_;
  unsigned sequence_number_ {0};
  bool failed_ {false};
};

#endif
