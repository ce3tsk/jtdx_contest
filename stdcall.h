/* CE3TSK 2026-11-13 (TODO.md 3.-251): the one test of a standard callsign, as WSJT-X's message generator makes it: K1ABC,
   and K1ABC/P or K1ABC/R. A compound call such as V5/G0RIB or G0RIB/MM gets messages for nonstandard calls, and in the
   normal exchange (Tx1 with the grid, then reports) none of his carries his grid; a grid exchange is another matter (JTDX
   sends "K1ABC <V5/G0RIB> GRID" there).

   standard () is WSJT-X's expression, from MainWindow::stdCall (), which asks it here - the message generator's rule,
   left as it is. grid_can_come () is DisplayText's question for the FT Challenge's points on a line with no grid (1 point
   only where his grid cannot come, the operator): the generator's rule for his call and ours together, and the 77-bit
   packer's own view of a standard call (3.-251's review).

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

  // a call as the 77-bit packer takes it (packjt77.f90, pack28): its work-arounds for Eswatini's 3DA0 and Guinea's 3X
  // prefixes, which the expression above does not know - 3DA0RU is 3D0RU, 3XA1B is QA1B, both standard to it
  inline QString packed_form (QString const& call)
  {
    if (call.startsWith ("3DA0")) return "3D0" + call.mid (4, 3);
    if (call.startsWith ("3X") && call.size () > 2 && call[2] >= 'A' && call[2] <= 'Z') return "Q" + call.mid (2, 4);
    return call;
  }

  // can the grid of a station, his, reach us, mine, in the normal exchange (the FT Challenge's)? Only when his call is
  // standard (as the packer takes it), and not in the pairs MainWindow::genStdMsgs () sends without a grid whatever the
  // calls' shapes: a /P call calling a station whose call is not standard, and a /R call with a /P one (mixedPR). A decode
  // line names no hashed call in brackets (DecodedText takes them off), so neither call carries any.
  inline bool grid_can_come (QString const& his, QString const& mine)
  {
    if (!standard (packed_form (his))) return false;
    bool const hisP {his.endsWith ("/P")}, hisR {his.endsWith ("/R")};
    if (hisP && !mine.isEmpty () && !standard (packed_form (mine))) return false;
    if ((hisR && mine.endsWith ("/P")) || (hisP && mine.endsWith ("/R"))) return false;
    return true;
  }
}

#endif
