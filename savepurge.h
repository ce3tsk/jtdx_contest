// -*- Mode: C++ -*-
/* CE3TSK 2026-10-23 (TODO.md 3.-192): Save > Remove saved files after 30 days, as WSJT-X 3.0 has it (its
   MainWindow::remove_old_files ()) - at each start, the recordings in the save directory older than 30 days are deleted.

   Narrower than WSJT-X's, which deletes every file of the directory: only what this program saves there, by the names
   it gives them (ownName ()) - a *.wav filter alone took any old recording in the folder, someone else's too (the review
   of 3.-192, 1: File > Delete all *.wav && *.c2 files in SaveDir asks first, this does not); only files - never a
   subdirectory, nor anything in one (the station keeps samples/ there), nor a symbolic link; and only what may be
   written. A file's age is its last modification against the computer's clock, both in UTC - not the program's clock,
   which a time sync moves.

   Fast with 30 days of Save all (the review of 3.-192's fixes, 2): the names come first, from the directory alone, and a
   file's details only for a name whose day is old enough (expiredFiles ()). Reading every file's details took 2.3 s more
   at a start with 178 560 recordings (FT8, 30 days of Save all), before the window appeared.

   Qt Core only: test_savepurge drives it on a directory of its own. */

#ifndef SAVEPURGE_H
#define SAVEPURGE_H

#include <QDateTime>
#include <QDate>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace save_purge
{
  int const days {30};

  // a name this program gives what it saves (MainWindow's m_fnameWE): the period's start in UTC, yyMMdd_hhmmss in the FT
  // modes, yyMMdd_hhmm in the others, then .wav - and WSPR's .c2 beside it. Exactly so: no other name, no other case
  inline bool ownName (QString const& name)
  {
    static QRegularExpression const own {"^[0-9]{6}_[0-9]{4}([0-9]{2})?\\.(wav|c2)$"};
    return own.match (name).hasMatch ();
  }

  // the day in a name of the program's: its period's start, UTC; not valid for a day that does not exist (201399_...)
  inline QDate nameDay (QString const& name)
  {
    return QDate {2000 + name.left (2).toInt (), name.mid (2, 2).toInt (), name.mid (4, 2).toInt ()};
  }

  // last changed at `modified`, it is past its time at `now`: more than `days` days before it
  inline bool expired (QDateTime const& modified, QDateTime const& now, int days = save_purge::days)
  {
    return modified.isValid () && now.isValid () && modified.toUTC () < now.toUTC ().addDays (-days);
  }

  /* the names of the files to remove, in name order. A file is looked at only when its name is the program's and its day
     at least `days` - 1 days back - its last change, read then, decides. A younger name cannot be past its time: the file
     is written after its period began; the day's margin is for the program's clock, which names it, being ahead of the
     computer's. A day that does not exist counts no days (QDate::daysTo () of an invalid date): never looked at. */
  inline QStringList expiredFiles (QDir const& dir, QDateTime const& now, int days = save_purge::days)
  {
    QStringList out;
    if (!dir.exists () || !now.isValid ()) return out;
    QDate const today {now.toUTC ().date ()};
    QDirIterator it {dir.absolutePath (), QDir::Files};
    while (it.hasNext ())
      {
        QString const path {it.next ()};
        QString const name {it.fileName ()};
        if (!ownName (name) || nameDay (name).daysTo (today) < days - 1) continue;
        QFileInfo const f {path};
        if (!f.isSymLink () && f.isWritable () && expired (f.lastModified (), now, days)) out << name;
      }
    out.sort ();
    return out;
  }

  // removes them: how many were removed
  inline int purge (QDir dir, QDateTime const& now, int days = save_purge::days)
  {
    int removed {0};
    for (auto const& name : expiredFiles (dir, now, days)) if (dir.remove (name)) ++removed;
    return removed;
  }
}

#endif
