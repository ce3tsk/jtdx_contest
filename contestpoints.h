// -*- Mode: C++ -*-
/* CE3TSK 2026-11-04 (TODO.md 3.-230): the YBDXPI FT8 contest, what a QSO is worth and which multipliers it brings, from the call. The
   other contests score by distance (contest_points () in Configuration.cpp). The rules, for a DX station: a member 5, whatever his
   country; otherwise the same country 1, the same continent 2, another continent 3. Multipliers: the WPX prefix and the DXCC country,
   each once on each band, every station counting, our own too. A country here is CountryDat::find ()'s "continent,prefix,name,...".

   Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software: you may
   redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 or (at your option) any later version. It is distributed WITHOUT ANY WARRANTY. */
#ifndef CONTESTPOINTS_H
#define CONTESTPOINTS_H

#include <QList>
#include <QString>
#include <QStringList>
#include "contestscore.h"
#include "Radio.hpp"

namespace contest_qso
{
  // what the logbook's worked-country index keys on: continent, master prefix, name. Empty for a call cty.dat does not know
  inline QString country_key (QString const& country)
  {
    QStringList const items = country.split (',');
    if (items.size () < 3 || items[1].trimmed () == "?") return QString {};
    return items[0] + ',' + items[1] + ',' + items[2];
  }
  inline QString continent_of (QString const& country) {return country.section (',', 0, 0).trimmed ();}
  // the same as the new-prefix colour asks (ADIF), so that colour and score cannot disagree
  inline QString wpx_prefix (QString const& call) {return Radio::striped_prefix (Radio::effective_prefix (call));}

  inline int ybdxpi_points (bool member, QString const& myCountry, QString const& hisCountry)
  {
    if (member) return 5;
    QString const his = country_key (hisCountry);
    if (his.isEmpty ()) return 0;
    if (his == country_key (myCountry)) return 1;
    return continent_of (hisCountry) == continent_of (myCountry) ? 2 : 3;
  }

  // (TODO.md 3.-232) the points of one QSO: what the score adds and what the log's comment prints
  template <class Countries>
  inline int ybdxpi_points_of (Countries& countries, QString const& call, QString const& myCall)
  {
    return ybdxpi_points (countries.isMember (call), countries.find (myCall), countries.find (call));
  }

  template <class Countries>
  inline contest_score::Score ybdxpi_score (QList<contest_score::Qso> const& qsos, Countries& countries, QString const& myCall)
  {
    return contest_score::sum_by_call (qsos,
      [&] (contest_score::Qso const& q) {return ybdxpi_points_of (countries, q.call, myCall);},
      [&] (contest_score::Qso const& q) {
        QStringList keys;
        QString const prefix = wpx_prefix (q.call);
        if (!prefix.isEmpty ()) keys << "P " + prefix;
        QString const country = country_key (countries.find (q.call));
        if (!country.isEmpty ()) keys << "C " + country;
        return keys;
      });
  }
}

#endif
