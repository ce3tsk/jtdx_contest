// -*- Mode: C++ -*-
/* CE3TSK 2026-10-02: the US state or states a decoded station's grid square lies in, for the country the
   decode windows append - "USA, CA", "K, CA", and for a square several states share every one of them,
   the most populous share first: "USA, NY/MA" (the operator's format since 2026-10-03; it was "U.S.A.-CA").
   Settings > General > Display > Show US states.

   The states come from the grid square table compiled into the program (geodata::statesOfGrid, built from
   the US Census boundaries and population, DC counted as Maryland, the territories left out). Only a call of
   one of the United States' DXCC entities is placed - K, KL, KH6, and the territories, whose calls are often
   held by stations on the mainland: geodata's administration group "US". A Canadian in a square shared with
   New York is in Canada.

   Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software: you may
   redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free
   Software Foundation, either version 3 or (at your option) any later version. It is distributed WITHOUT ANY
   WARRANTY. */
#ifndef JTDX_USSTATES_H
#define JTDX_USSTATES_H

#include <QString>
#include <cstring>
#include "geodata/geodata.h"

namespace us_states
{
  // a cty.dat master prefix ("K", "KL", "KH6", "KP4") of one of the United States' entities; any case
  inline bool usEntity (QString const& masterPrefix)
  {
    return 0 == std::strcmp (geodata::entityGroup (geodata::entityIndex (masterPrefix.toLatin1 ().constData ())), "US");
  }

  // ", CA", ", NY/MA/VT/NH/CT" - every state of the square, in the table's order - or "" when the call is not of
  // a US entity, the grid is not an upper-case square (RR73 is one, in the Arctic Ocean, and lies in no state),
  // or the square lies in no state: the sea, Canada, Mexico, a territory. CE3TSK 2026-10-03 (the operator): a shared
  // square that the call's licence resolves - its state is one of the square's (clearState's second layer) - shows
  // that state alone, ", MA", as Worked All States counts it.
  inline QString suffix (QString const& masterPrefix, QString const& grid, QString const& licenseState = {})
  {
    if (grid.size () < 4) return {};
    // the square first: one binary search, which finds nothing for nearly every grid outside the United States;
    // the entity - a case-insensitive scan for many prefixes - only when the square names a state (review)
    geodata::StateShare s[16];   // the busiest square today holds five (FN32)
    std::size_t const n = geodata::statesOfGrid (grid.toLatin1 ().constData (), s, sizeof s / sizeof s[0]);
    if (!n || !usEntity (masterPrefix)) return {};
    if (n > 1 && !licenseState.isEmpty ())
      for (std::size_t i = 0; i < n; ++i)
        if (licenseState == QLatin1String (s[i].code)) return QStringLiteral (", ") + licenseState;
    QString r;
    for (std::size_t i = 0; i < n; ++i) r += (i ? QStringLiteral ("/") : QStringLiteral (", ")) + QString::fromLatin1 (s[i].code);
    return r;
  }

  /* CE3TSK 2026-10-03, Worked All States: the state a decode or a log entry counts for, when the call is of a US entity -
     the square's one state, when it lies in a single state; in a square several states share, the state of the call's
     licence (licenseState, LogBook::licenseState) when that is one of the square's - the second layer, the operator's -
     and otherwise none: which state the station is in cannot be told. A licence state outside the square - a traveller
     - resolves nothing; it never moves a square of a single state either. Every square the table gives two states or
     more is shared, however few people live in the second: EM64, EM65 and FM17 were counted for their one peopled state
     for a day, until the licence file showed active licensees of the other state there too (the operator, 2026-10-04).
     Any case of square. */
  inline QString clearState (QString const& masterPrefix, QString const& grid, QString const& licenseState = {})
  {
    if (grid.size () < 4) return {};
    QString const square = grid.left (4).toUpper ();
    geodata::StateShare s[16];   // the busiest square today holds five (FN32)
    std::size_t const n = geodata::statesOfGrid (square.toLatin1 ().constData (), s, sizeof s / sizeof s[0]);
    if (!n || !usEntity (masterPrefix)) return {};
    if (n == 1) return QString::fromLatin1 (s[0].code);
    for (std::size_t i = 0; i < n && !licenseState.isEmpty (); ++i)
      if (licenseState == QLatin1String (s[i].code)) return licenseState;
    return {};
  }
}

#endif
