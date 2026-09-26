#ifndef SAVEDMODE_H
#define SAVEDMODE_H

// CE3TSK 2026-09-26: the saved Mode and ModeTx made safe to start from.
//
// readSettings used to accept any mode whose name began with FT, JT, T10 or WSPR, so a damaged or
// foreign ini ("JT4", "WSPR-15") reached a start-up dispatch with no case for it: m_mode named a
// mode no slot had set while FT8 stayed checked from the .ui. And ModeTx was taken as saved - with
// Mode=JT65 and ModeTx=FT8 the JT65 slot's toggle turned FT8 into JT9, so the station ran JT65 and
// transmitted JT9. Every single mode transmits itself, and JT9+JT65 starts on JT65 - its slot sets
// that whatever was saved (the operator switches the dual mode's TX with the Tx mode button), so a
// saved JT9 is not kept here either: keeping it only put "Tx JT9" on the button until the slot ran.
//
// `known` says whether a name is one of the Mode menu's modes (MainWindow passes its mode table),
// so this header holds no list of its own to drift from it; `allowed` says whether the mode may be
// run now (with a contest running, contestmode.h's rule).

#include <QString>

// A saved mode name as one of the Mode menu's: kept when it is one, an older "WSPR..." name as
// WSPR-2, anything else FT8. One rule for the saved Mode and for the mode a contest parked
// (ContestUiMode) - review 2026-09-26: the two had drifted, Mode sending "WSPR-15" to FT8 while
// the contest restore sent it to WSPR-2, as the old restore always had.
template <class Known>
inline QString saved_mode_name (QString const& name, Known known)
{
  if (known (name)) return name;
  if (name.startsWith ("WSPR") && known (QString {"WSPR-2"})) return "WSPR-2";
  return "FT8";
}

template <class Known, class Allowed>
inline void normalise_saved_mode (QString& mode, QString& modeTx, Known known, Allowed allowed)
{
  mode = saved_mode_name (mode, known);
  if (!allowed (mode)) mode = "FT8";
  modeTx = mode == "JT9+JT65" ? QString {"JT65"} : mode;
}

#endif
