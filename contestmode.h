#ifndef CONTESTMODE_H
#define CONTESTMODE_H

// CE3TSK 2026-09-26: the modes a contest allows, in one place.
//
// A running contest (WW Digi, ARRL Digi) is worked in FT8 and FT4 only, the YBDXPI one in FT8 alone (ft8Only). Two things follow from
// that and used to spell it out separately: entering the contest parks any other mode and forces
// FT8, and while it runs the Mode menu greys every other mode. The greying listed its modes by
// hand and FT2, which came later, was left live - one click put the station in FT2 mid-contest
// with nothing parked to undo it. Both now ask the same function, and test_uilock tests it
// rather than a copy of it.

#include <QString>

inline bool contest_allows_mode (QString const& mode, bool ft8Only = false)
{
  return mode == "FT8" || (mode == "FT4" && !ft8Only);
}

// the mode to park on entering a contest: nothing when the contest allows the current one (the
// empty string then means "nothing to restore", and leaving keeps whichever FT mode is current)
inline QString contest_parked_mode (QString const& mode, bool ft8Only = false)
{
  return contest_allows_mode (mode, ft8Only) ? QString {} : mode;
}

// (TODO.md 3.-232) from one contest directly to another: what is parked stays, and a mode the new one refuses joins it, or leaving
// would not give it back (WW Digi in FT4, nothing parked, to the contest in FT8 alone, then None)
inline QString contest_parked_on_switch (QString const& parked, QString const& mode, bool ft8Only = false)
{
  return parked.isEmpty () ? contest_parked_mode (mode, ft8Only) : parked;
}

// the mode to be in on leaving: the parked one, or - nothing parked - the one the contest ended in
inline QString contest_mode_on_leaving (QString const& parked, QString const& current)
{
  return parked.isEmpty () ? current : parked;
}

#endif
