// -*- Mode: C++ -*-
/* CE3TSK 2026-10-03: the state of a US amateur licence's mailing address - the second layer of Worked All States, for a
   grid square several states share: such a square counts for the state of the call's licence when that state is one of
   the square's (us_states::clearState). The table is us-license-states.txt, built from the FCC's weekly licence file by
   tools/make_license_states.py, bundled with each release and downloadable from ce3tsk.com (Settings > General > Data
   files); the newer copy is read (CountryDat::fileToUse).

   Kept as one sorted array of 64-bit entries - the call packed in base 37 (a US call has at most six characters) above
   its two state letters - so 900 000 calls take about 7 MB, a lookup is a binary search, and a copy of the object (the
   window takes the logbook by value) shares the array instead of copying it.

   Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software: you may
   redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 or (at your option) any later version. It is distributed WITHOUT ANY WARRANTY. */
#ifndef JTDX_LICENSESTATES_H
#define JTDX_LICENSESTATES_H

#include <QByteArray>
#include <QDate>
#include <QString>
#include <QVector>

class LicenseStates
{
public:
  /* the version the file carries ("# version yyyy-mm-dd"), or an invalid date when the content is not such a file: it
     must open with the file's own first line, and its "# calls N" must count its call lines exactly - an error page, a
     truncated download or another file is refused */
  static QDate version (QByteArray const& content);
  // the table from a file (a ":/" resource too); empty when it cannot be read or is not such a file
  void load (QString const& path);
  void load (QByteArray const& content);
  // the state of the licence of this call's US base call ("K1ABC" of "W1/K1ABC/P"), or "" when it has none in the table
  QString state (QString const& call) const;
  int size () const {return entries_.size ();}
  // the part of a call that has the shape of a US call - "K1ABC" of "KH6/K1ABC" or "K1ABC/P" - or "" when none has
  static QString usBaseCall (QString const& call);

private:
  static quint64 pack (QByteArray const& call);   // 0 when it cannot be packed
  QVector<quint64> entries_;
};

#endif
