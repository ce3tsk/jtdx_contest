/* CE3TSK 2026-11-13 (TODO.md 3.-253): contest log sessions - a contest's log is the file of a session, named by the profile and
   the UTC date the session began: wsjtx_ww-digi-2026-10-17_log.adi, a second one begun the same day -2, then -3. Any number of
   them per profile: a profile may be used for another contest of the same scoring (the operator). The session is kept in the
   settings and reused at every start, so the clock and a QSO's date never choose the file - a month's end or a midnight
   inside a contest moves nothing.

   When a contest is switched on, back on, or changed to another, the newest session of that profile begun within the last
   week, if its file holds QSOs, is offered: continue it, or start a new one - unless a session begun after it, with nothing
   logged yet, is the stored one: the operator began that one ("Start a new log"), it is kept. At a start the stored session
   is used without a question, unless it is older than a week and holds QSOs: then a new log is the default answer.
   A file that cannot be read may hold QSOs: it counts as one that does (unreadable), never passed over unseen.

   The names of before stay readable here: the year (wsjtx_ww-digi-2026_log.adi, until 2026-11-13) and the year and month
   (3.-252, the same day) - the Cabrillo export offers those logs as they are; they are never a session to continue.

   No window, no file system: the caller lists the files and counts their QSOs (qso_count () reads a file's text).

   Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software: you may
   redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 or (at your option) any later version. It is distributed WITHOUT ANY WARRANTY. */
#ifndef CONTESTSESSION_H
#define CONTESTSESSION_H

#include <QByteArray>
#include <QDate>
#include <QMap>
#include <QRegularExpression>
#include <QString>

namespace contest_session
{
  int const reuse_days {7};   // a session begun fewer days than this before today is offered again
  int const unreadable {-1};   // (review 4) the QSOs of a file that could not be read: not known

  // what a file's count says: QSOs, or perhaps (unreadable) - offered as a file with QSOs is
  inline bool holds_qsos (int count) { return count != 0; }

  inline QString prefix (QString const& slug) { return "wsjtx_" + slug + '-'; }
  inline QString suffix () { return QStringLiteral ("_log.adi"); }

  // a session's file: the profile, the day it began, and from the second of a day its number
  inline QString file_name (QString const& slug, QDate const& date, int n = 1)
  {
    QString name = prefix (slug) + QString {"%1-%2-%3"}.arg (date.year (), 4, 10, QChar {'0'}).arg (date.month (), 2, 10, QChar {'0'})
                                                       .arg (date.day (), 2, 10, QChar {'0'});
    if (n > 1) name += '-' + QString::number (n);
    return name + suffix ();
  }

  // what stands between the profile and "_log.adi" in a contest log's name: a session's day (and number), or the year, or the
  // year and month, of the names before; empty for any other file - a backup, another profile's, a day that is none
  inline QString period_of (QString const& fileName, QString const& slug)
  {
    static QRegularExpression const form {R"(^(\d{4})(-(0[1-9]|1[0-2])(-(\d{2})(-([2-9]|[1-9]\d+))?)?)?$)"};
    QString const before {prefix (slug)}, after {suffix ()};
    if (!fileName.startsWith (before) || !fileName.endsWith (after)) return QString {};
    QString const p {fileName.mid (before.size (), fileName.size () - before.size () - after.size ())};
    auto const m = form.match (p);
    if (!m.hasMatch ()) return QString {};
    if (m.capturedLength (5) && !QDate {m.captured (1).toInt (), m.captured (3).toInt (), m.captured (5).toInt ()}.isValid ()) return QString {};
    return p;
  }

  // a session's day and number from its file's name; false for any other name, the year- and month-named logs too
  inline bool parse (QString const& fileName, QString const& slug, QDate* date, int* n)
  {
    QString const p {period_of (fileName, slug)};
    if (p.size () < 10) return false;
    *date = QDate {p.left (4).toInt (), p.mid (5, 2).toInt (), p.mid (8, 2).toInt ()};
    *n = p.size () > 10 ? p.mid (11).toInt () : 1;
    return date->isValid ();
  }

  // the QSOs of an ADIF file's text: its records' ends, in any case; the header has none
  inline int qso_count (QByteArray const& adif)
  {
    return adif.toLower ().count ("<eor>");
  }

  enum class Choice { Use, New, Ask };

  struct Decision
  {
    Choice choice;
    QString name;    // Use, New: the session; Ask: the one offered
    QString fresh;   // Ask: the one a new log would be
    bool newFirst;   // Ask: a new log the default answer - the one offered was begun a week or more ago (TODO.md 3.-253's review 3)
  };

  /* the session of a profile, slug, today (UTC): stored is the one the settings hold for it; files the profile's contest logs in
     the data directory, each with the QSOs it holds (unreadable for one that could not be read); atStart for the program's start, else a contest switched on, back on or
     changed to this one. */
  inline Decision decide (QString const& slug, QDate const& today, QString const& stored, QMap<QString, int> const& files, bool atStart)
  {
    // a new session: today's, numbered after any of today's already there or stored
    int last {0};
    QDate d; int n {0};
    for (auto i = files.constBegin (); i != files.constEnd (); ++i)
      if (parse (i.key (), slug, &d, &n) && d == today) last = qMax (last, n);
    if (parse (stored, slug, &d, &n) && d == today) last = qMax (last, n);
    QString const fresh {file_name (slug, today, last + 1)};
    auto const recent = [&today] (QDate const& when) { return qAbs (when.daysTo (today)) < reuse_days; };

    QDate storedDate; int storedN {0};
    bool const storedValid {parse (stored, slug, &storedDate, &storedN)};
    if (atStart && storedValid)
      {
        if (recent (storedDate)) return {Choice::Use, stored, QString {}, false};
        // contest mode left on since (review 3): Enter begins a new log
        if (holds_qsos (files.value (stored))) return {Choice::Ask, stored, fresh, true};
        return {Choice::New, fresh, QString {}, false};
      }
    // the newest session of the profile that holds QSOs (the stored one among them when its file does)
    QString newest; QDate newestDate; int newestN {0};
    for (auto i = files.constBegin (); i != files.constEnd (); ++i)
      if (holds_qsos (i.value ()) && parse (i.key (), slug, &d, &n)
          && (newest.isEmpty () || d > newestDate || (d == newestDate && n > newestN)))
        { newest = i.key (); newestDate = d; newestN = n; }
    /* a session begun this week with nothing logged yet: the same one, there is nothing to ask about - while it is newer than the
       one that would be offered, too (review 2): the operator began it after that one, and switching the contest off and on again
       must not offer the older one back */
    bool const storedEmpty {storedValid && recent (storedDate) && files.value (stored) == 0};
    bool const storedNewer {newest.isEmpty () || storedDate > newestDate || (storedDate == newestDate && storedN > newestN)};
    if (storedEmpty && (storedNewer || !recent (newestDate))) return {Choice::Use, stored, QString {}, false};
    if (!newest.isEmpty () && recent (newestDate)) return {Choice::Ask, newest, fresh, false};
    return {Choice::New, fresh, QString {}, false};
  }
}

#endif
