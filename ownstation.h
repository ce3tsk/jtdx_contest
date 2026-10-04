// -*- Mode: C++ -*-
/* CE3TSK 2026-10-04: the operator's own call and grid, judged as a decoded station's are (phantomdecodes.h) - a call
   whose prefix belongs to no DXCC country in cty.dat, and a grid that does not lie in the call's country, with the
   same tolerances: the eight neighbouring squares, and one administration counted as one country (a KL7 call in Ohio
   is fine). Distances, PSK Reporter spots and the WW Digi exchange are all worked out from these two fields.

   Settings > General turns the box in doubt red while it is (as the value is typed) and, on OK, asks once - a warning,
   never a hard error: a special-event call, a prefix cty.dat does not know yet, an operator abroad who has not changed
   My Call yet are all real (the operator). A grid must also be a whole locator of 4 to 12 characters.

   Never judged against cty.dat: an empty call, /MM and /AM (they have no country), and anything while cty.dat is not
   read - the lookup then answers "".

   Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software: you may
   redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 or (at your option) any later version. It is distributed WITHOUT ANY WARRANTY. */
#ifndef JTDX_OWNSTATION_H
#define JTDX_OWNSTATION_H

#include <QRegularExpression>
#include <QString>
#include "phantomdecodes.h"
#include "geodata/geodata.h"

namespace own_station
{
  /* a whole Maidenhead locator: 4, 6, 8, 10 or 12 characters - field, square, subsquare, extended square and on, letters
     and digits in turn (IO91, IO91wm, IO91wm99, IO91wm99aa, IO91wm99aa00); FT8 sends the first four, and the program
     uses the first eight at most (Configuration::my_grid) */
  inline bool wellFormedGrid (QString const& grid)
  {
    static QRegularExpression const re {"^[A-R]{2}[0-9]{2}(?:[A-X]{2}(?:[0-9]{2}(?:[A-X]{2}(?:[0-9]{2})?)?)?)?$",
                                        QRegularExpression::CaseInsensitiveOption};
    return re.match (grid.trimmed ()).hasMatch ();
  }

  enum class Call {Fine, NoCountry};
  enum class Grid {Fine, Malformed, Outside};

  struct Verdict
  {
    Call call = Call::Fine;
    Grid grid = Grid::Fine;
    QString country;   // the call's DXCC country as cty.dat names it, for Grid::Outside
    QString square;    // the grid's first four characters, for Grid::Outside
    bool fine () const {return Call::Fine == call && Grid::Fine == grid;}
  };

  /* the two judged on their own, so both can be in doubt: a grid that is not a whole locator is malformed whatever the
     call (and with or without cty.dat); a call of no country leaves the grid's country unjudged.
     entityOf: the cty.dat lookup - LogBook::getDXCC's answer "continent,prefix,name,..." - or "" while cty.dat is not read */
  template<typename EntityOf> Verdict judge (QString const& call, QString const& grid, EntityOf entityOf)
  {
    Verdict v;
    QString const g = grid.trimmed ();
    if (!g.isEmpty () && !wellFormedGrid (g)) v.grid = Grid::Malformed;
    QString const c = call.trimmed ().toUpper ();
    if (c.isEmpty ()) return v;
    for (auto const& part : c.split ('/').mid (1)) if ("MM" == part || "AM" == part) return v;
    QString const entity = entityOf (c);
    if (entity.isEmpty ()) return v;
    if (phantom_decodes::unknownCountry (c, entityOf)) {v.call = Call::NoCountry; return v;}
    QString const prefix = entity.section (',', 1, 1).trimmed ();
    if (prefix.isEmpty () || Grid::Fine != v.grid || g.isEmpty ()) return v;
    QString const square = g.left (4).toUpper ();
    if (!geodata::gridFitsEntity (prefix.toLatin1 ().constData (), square.toLatin1 ().constData ()))
      {
        v.grid = Grid::Outside;
        v.country = entity.section (',', 2, 2).trimmed ();
        v.square = square;
      }
    return v;
  }
}

#endif
