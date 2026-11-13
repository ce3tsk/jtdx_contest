/* CE3TSK 2026-11-13 (TODO.md 3.-251): the one test of a standard callsign - one that a 77-bit FT8, FT4 or FT2 message
   carries whole, with a grid beside it: K1ABC, and K1ABC/P or K1ABC/R. A compound call such as V5/G0RIB or G0RIB/MM goes
   in the messages for nonstandard calls, which have no room for a grid, so such a station can never send his.

   WSJT-X's expression, from MainWindow::stdCall (), which now asks it here; DisplayText asks it too (the FT Challenge's
   points on a line with no grid: 1 point only where the grid cannot come).

   Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software: you may
   redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 or (at your option) any later version. It is distributed WITHOUT ANY WARRANTY. */
#ifndef STDCALL_H
#define STDCALL_H

#include <QRegularExpression>
#include <QString>

namespace std_call
{
  inline bool standard (QString const& w)
  {
    static QRegularExpression const standard_call_re {
      R"(
          ^\s*				# optional leading spaces
          ( [A-Z]{0,2} | [A-Z][0-9] | [0-9][A-Z] )  # part 1
          ( [0-9][A-Z]{0,3} )                       # part 2
          (/R | /P)?			# optional suffix
          \s*$				# optional trailing spaces
      )", QRegularExpression::CaseInsensitiveOption | QRegularExpression::ExtendedPatternSyntaxOption};
    return standard_call_re.match (w).hasMatch ();
  }

  // the call of a station, as a decode line names it - hashed calls in angle brackets - whatever the message's form
  inline bool standard_station (QString call)
  {
    if (call.startsWith ('<') && call.endsWith ('>')) call = call.mid (1, call.size () - 2);
    return standard (call);
  }
}

#endif
