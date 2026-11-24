#ifndef GRIDFILL_H
#define GRIDFILL_H

/* CE3TSK 2026-11-23 (TODO.md 3.-283, GRID_FILL_PLAN.md): a grid missing from a QSO, filled before the QSO is logged - the
   rules, free of the program's windows. The operator: "Sometimes we do not get a grid for a QSO, and the idea is to fill
   the grid before logging the QSO to the ADIF file, but only when not in contest mode, and have that grid filed before
   eventually uploading the log to logbooks". Decided (G1-G8, "all as recommended, comment form ok"):

   - only while the QSO gave none - never over a grid heard or typed (the caller asks only then), never in a contest, and
     only with Settings > Reporting's "Fill a missing grid before logging" (off by default);
   - from the sources in their order (G2): what the station sent earlier, heard by this program; the locator it reports
     to PSK Reporter (the Reception map open); later stages add eQSL's member list, Club Log's users file and QRZ.com;
   - a source's grid only when it is what G3 asks: its first four characters (the operator: "use the first 4 characters
     of that station's reported grid"), a locator, not a software's default, inside the call's DXCC entity as the
     program's own grid data has it (geodata: the square itself, no neighbour; one administration's squares all, as a
     KL7 call in Ohio) - and never for a call that moves (/MM, /AM) or one of no country: nothing to judge its place by;
   - the QSO's comment says where it came from (G5, the operator: "document it in the comment field so that it can be
     inferred afterwards"): "[grid: <source>]" after the operator's own words - English, never translated, data that
     programs read back - and only while the grid logged is the one filled: cleared or changed in Log QSO, it is the
     operator's own.

   test_gridfill has the rules. */

#include <initializer_list>
#include <utility>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include "geodata/geodata.h"

namespace grid_fill
{
  enum class Source {None, HeardEarlier, PskReporter, Eqsl, ClubLog, Qrz,
                     HeardViaPskReporter};   // (3.-289) the square others heard it send, as their spots say: after PskReporter

  struct Fill
  {
    Fill () {}
    Fill (QString const& g, Source s) : grid {g}, source {s} {}
    QString grid;                  // four characters, upper case
    Source source {Source::None};
    bool filled () const {return Source::None != source && !grid.isEmpty ();}
  };

  // the source as the comment names it - data, never translated
  inline QString name (Source s)
  {
    switch (s)
      {
      case Source::HeardEarlier: return QStringLiteral ("heard earlier");
      case Source::PskReporter: return QStringLiteral ("PSK Reporter");
      case Source::HeardViaPskReporter: return QStringLiteral ("heard via PSK Reporter");   // (3.-289)
      case Source::Eqsl: return QStringLiteral ("eQSL");
      case Source::ClubLog: return QStringLiteral ("Club Log");
      case Source::Qrz: return QStringLiteral ("QRZ.com");
      default: return {};
      }
  }

  /* (the review, 2026-11-23) how long ago what a call sent may have been heard to be its place now: a day - but a land
     mobile (/M) moves, so half an hour, as long as PSK Reporter's locator is taken. A prefix M (England: M/K1ABC) is no
     mobile; /MM and /AM are never filled at all (usable ()). */
  long long const heardSeconds {86400};
  long long const mobileHeardSeconds {1800};
  inline long long heard_seconds (QString const& call)
  {
    for (auto const& part : call.trimmed ().toUpper ().split (QLatin1Char ('/')).mid (1)) if (QLatin1String {"M"} == part) return mobileHeardSeconds;
    return heardSeconds;
  }

  // "[grid: <source>]", as the comment gets it
  inline QString note (Source s)
  {
    return Source::None == s ? QString {} : QStringLiteral ("[grid: ") + name (s) + QLatin1Char (']');
  }

  /* A source's grid as the log may take it (G3), or "". entityOf is the cty.dat lookup - LogBook::getDXCC's answer
     "continent,prefix,name,...", the prefix "?" for a call of no country - or "" while cty.dat is not read. */
  template<typename EntityOf> QString usable (QString const& call, QString const& candidate, EntityOf entityOf)
  {
    QString const c = call.trimmed ().toUpper ();
    QString const square = candidate.trimmed ().left (4).toUpper ();
    static QRegularExpression const locator {QStringLiteral ("^[A-R]{2}[0-9]{2}$")};
    if (c.isEmpty () || !locator.match (square).hasMatch ()) return {};
    // a software's default, never a station's place: AA00 (the corner of the grid) and JJ00 (0 degrees, 0 degrees)
    if (QLatin1String {"AA00"} == square || QLatin1String {"JJ00"} == square) return {};
    for (auto const& part : c.split (QLatin1Char ('/')).mid (1))
      if (QLatin1String {"MM"} == part || QLatin1String {"AM"} == part) return {};   // at sea or in the air: no place kept
    QString const prefix = entityOf (c).section (QLatin1Char (','), 1, 1).trimmed ();
    if (prefix.isEmpty () || QLatin1String {"?"} == prefix) return {};   // of no country, or cty.dat not read
    if (!geodata::gridFitsEntity (prefix.toLatin1 ().constData (), square.toLatin1 ().constData (), false, true)) return {};
    return square;
  }

  // the first source whose grid the log may take, in the order given (G2); none: nothing filled
  template<typename EntityOf> Fill choose (QString const& call, std::initializer_list<std::pair<Source, QString>> candidates,
                                           EntityOf entityOf)
  {
    for (auto const& c : candidates)
      {
        QString const g = usable (call, c.second, entityOf);
        if (!g.isEmpty ()) return Fill {g, c.first};
      }
    return Fill {};
  }

  /* The comment logged (G5): the operator's own words and, while the grid logged is the one filled, "[grid: <source>]"
     after them. A grid cleared or changed in Log QSO is the operator's own: no note. */
  inline QString commented (QString const& comment, Fill const& fill, QString const& gridLogged)
  {
    if (!fill.filled () || gridLogged.trimmed ().toUpper () != fill.grid) return comment;
    QString const n = note (fill.source);
    return comment.trimmed ().isEmpty () ? n : comment + QLatin1Char (' ') + n;
  }

  /* CE3TSK 2026-11-24 (TODO.md 3.-291) a comment without the fill's note: eQSL's card takes the QSO's comment when Log
     QSO's eQSL message is empty, and "[grid: <source>]" is the log's bookkeeping, not a word to the other station */
  inline QString without_note (QString comment)
  {
    static QRegularExpression const fillNote {QStringLiteral ("\\s*\\[grid: [^\\]]*\\]")};
    return comment.remove (fillNote).trimmed ();
  }
}

#endif
