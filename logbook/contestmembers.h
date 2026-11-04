// -*- Mode: C++ -*-
/* CE3TSK 2026-11-04 (TODO.md 3.-230): the stations a contest gives bonus points for, by base call: the YBDXPI FT8 contest's member
   list, ybdxpi.txt from https://contest.ybdxpi.net/ft8/software/udc-files/. Bundled with each release; a copy in the data directory
   wins, so a download only has to put one there (CountryDat::fileToUse).

   The file is N1MM's support file: an opening line, then "CALL," a line, CRLF. A member stays one whatever he signs (the rules: "5
   points regardless the member's country"), so both sides are cut to the base call: the list holds YB1EIG/7, and YC2GBS only as
   YC2GBS-1. This decides the rank and the score shown; the organisers decide the real score.

   (TODO.md 3.-232) A file that is not the list - a browser's error page saved under its name, an empty file - would cost every
   member the 5 points without a word, so it is refused whole and the list bundled with the release stands in.

   Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software: you may
   redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 or (at your option) any later version. It is distributed WITHOUT ANY WARRANTY. */
#ifndef JTDX_CONTESTMEMBERS_H
#define JTDX_CONTESTMEMBERS_H

#include <QByteArray>
#include <QDate>
#include <QDebug>
#include <QFile>
#include <QList>
#include <QSet>
#include <QString>
#include "Radio.hpp"

class ContestMembers
{
public:
  // the file carries no date: the copy in the data directory wins when there is one
  static QDate version (QByteArray const&) {return QDate {};}
  // the call as the list compares it: upper case, no "-1", the base call
  static QString key (QString const& call)
  {
    QString c = call.trimmed ().toUpper ();
    int const dash = c.indexOf ('-');
    if (dash > 0) c = c.left (dash);
    return Radio::base_callsign (c);
  }
  // letters, digits, / and -, at least a letter and a digit: every entry of the real lists, and no word of an error page
  static bool looksLikeCall (QByteArray const& entry)
  {
    bool letter = false, digit = false;
    for (char const c : entry)
      {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) letter = true;
        else if (c >= '0' && c <= '9') digit = true;
        else if (c != '/' && c != '-') return false;
      }
    return letter && digit;
  }
  // false, and nobody on the list, for anything but the list: the opening line may only be the first, every other a call
  bool parse (QByteArray const& content)
  {
    _members.clear ();
    QSet<QString> members;
    bool first = true;
    for (auto const& raw : content.split ('\n'))
      {
        QByteArray line = raw.trimmed ();
        if (first && line.startsWith ("\xEF\xBB\xBF")) line = line.mid (3);   // an editor's byte order mark
        if (line.isEmpty ()) continue;
        bool const opening = first && line.startsWith ('<');
        first = false;
        if (opening || line.startsWith ('#')) continue;
        int const comma = line.indexOf (',');
        if (comma >= 0) line = line.left (comma).trimmed ();
        if (!looksLikeCall (line)) return false;
        members.insert (key (QString::fromLatin1 (line)));
      }
    if (members.isEmpty ()) return false;
    _members = members;
    return true;
  }
  void load (QString const& filename, QString const& bundled = QStringLiteral (":/ybdxpi.txt"))
  {
    QFile f {filename};
    if (f.open (QIODevice::ReadOnly) && parse (f.readAll ())) return;
    _members.clear ();
    qWarning () << "the contest's member list" << filename << "is not the list, using the bundled one";
    QFile b {bundled};
    if (b.open (QIODevice::ReadOnly)) parse (b.readAll ());
  }
  bool contains (QString const& call) const {return _members.contains (key (call));}
  int size () const {return _members.size ();}
private:
  QSet<QString> _members;   // implicitly shared: a copy of the object that a window takes the logbook by value shares it
};

#endif
