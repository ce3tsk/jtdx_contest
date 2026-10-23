// -*- Mode: C++ -*-
/* CE3TSK 2026-10-23 (TODO.md 3.-194, ATU_TUNE_PLAN.md): a right click on Tune starts the radio's own antenna tuner, as
   WSJT-X 3.0 has it - through Hamlib only. Which Hamlib call does it (way ()), and whether the window asks at all
   (refusal ()).

   way (): Hamlib's tune operation (rig_vfo_op RIG_OP_TUNE - Icom CI-V 1C 01 02, Kenwood AC111: start tuning) where the
   rig has it - for a rig behind rigctld, as the remote rig has it (Hamlib takes its operations from rigctld at the open).
   Through flrig, whose Hamlib back end has no vfo_op, its tuner function (rig_set_func RIG_FUNC_TUNER, Hamlib 4.5.1+):
   that back end sends it as flrig's XML-RPC rig.tune, and flrig calls its rig's tune_rig (2), start tuning (the IC-7300:
   1C 01 02). Only there: on a rig Hamlib drives itself RIG_FUNC_TUNER puts the tuner in or out of the line and starts no
   tuning (Icom 1C 01 00/01, Kenwood AC1x0). Never Hamlib's dummy rig - the transceiver of Rig None, which only keys the
   PTT, is one, and it has the tune operation.

   holdsEnableTx (): after a right click Enable Tx is not switched on for 6 s, while the radio tunes (TODO.md 3.-195; WSJT-X
   3.0 refuses it while its Tune button shows "Tuning", 6 s).

   Qt-free: test_atutune drives it. */

#ifndef ATUTUNE_H
#define ATUTUNE_H

namespace atu_tune
{
  enum class Way {None, TuneOp, FlrigTuner};

  inline Way way (bool dummy, bool hasTuneOp, bool flrig, bool hasTunerFunc)
  {
    if (dummy) return Way::None;
    if (hasTuneOp) return Way::TuneOp;
    if (flrig && hasTunerFunc) return Way::FlrigTuner;
    return Way::None;
  }

  enum class Refusal {None, NoTuner, Transmitting};

  // the window's answer to the right click: a rig control that can (the rig state's tuner (), which HamlibTransceiver
  // reports), and nothing transmitted - neither a message nor the program's Tune carrier
  inline Refusal refusal (bool rigOk, bool canTune, bool transmitting)
  {
    if (!rigOk || !canTune) return Refusal::NoTuner;
    if (transmitting) return Refusal::Transmitting;
    return Refusal::None;
  }

  int const enableTxHoldMs {6000};

  // Enable Tx held: a right click asked the radio to tune `msSince` ms ago (rightClicked: one was ever made)
  inline bool holdsEnableTx (bool rightClicked, long long msSince)
  {
    return rightClicked && msSince >= 0 && msSince < enableTxHoldMs;
  }
}

#endif
