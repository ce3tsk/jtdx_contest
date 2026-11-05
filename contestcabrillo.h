// -*- Mode: C++ -*-
/* CE3TSK 2026-11-05 (TODO.md 3.-235): a contest's own ADIF log as a Cabrillo file. There is no second store of QSOs: the log the score
   and the rank read is what the file says. Nothing here needs the program, so it is tested on its own.

   A line: QSO: freq DG date time own-call own-grid his-call his-grid [t]
     freq   what was logged in whole kHz, cut; 6 m is "50"; with no frequency logged, the band's lower edge, which the Cabrillo spec takes too
     time   the END of the QSO, as WSJT-X writes it and WW Digi's rule X.1 asks ("logged at the time they are completed"), minutes cut
     grids  the first 4 characters of the record's own, never the report fields: in a grid exchange those are made up
     t      the transmitter, in the templates that have it

   The file is the log, whole. The organisers check the dates, the bands and the modes, and a QSO taken out here can cost the other station his (the
   operator 2026-11-05; WW Digi's FAQ: "please do not remove any QSOs"): a QSO outside the contest's weekend or on another band stays, and so does
   a real repeat, with its own line. A QSO with no grid goes in with ZZ00 (the same FAQ: "Log the grid as ZZ00").
   Three things cannot go in, and none silently (Selection, Log::damaged): a damaged record, which has no line to write; a QSO under another own call,
   which belongs in that call's file; and the second copy of the same QSO logged twice (same call, band and start), the QSO itself staying.

   Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software: you may
   redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 or (at your option) any later version. It is distributed WITHOUT ANY WARRANTY. */
#ifndef CONTESTCABRILLO_H
#define CONTESTCABRILLO_H

#include <algorithm>
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTime>
#include "contestscore.h"
#include "logfields.h"

namespace cabrillo
{
  struct Contest
  {
    QString name;                     // the CONTEST: tag it starts with
    QString assisted;                 // the CATEGORY-ASSISTED it starts with
    bool transmitterColumn = false;   // its QSO template ends in the transmitter: 0, a single one
    bool callNamesFile = false;       // its rules name the file: your_callsign.log
    bool locationIsGrid = false;      // its guide asks the grid for LOCATION, not DX or a section
    QStringList bands;                // the CATEGORY-BAND it takes
    QStringList powers;               // the CATEGORY-POWER it takes, the one offered first at the head
    QStringList transmitters;         // the CATEGORY-TRANSMITTER it takes: this log has no way to say which of two transmitters made a QSO
  };

  // ww-digi.com/cabrillo.htm
  inline Contest ww_digi ()
  {
    Contest c;
    c.name = "WW-DIGI";
    c.transmitterColumn = true;
    c.bands = QStringList {"ALL", "160M", "80M", "40M", "20M", "15M", "10M"};
    c.powers = QStringList {"LOW", "HIGH", "QRP"};
    c.transmitters = QStringList {"ONE", "UNLIMITED"};
    return c;
  }

  // the rules name no tag: WWROF's list has this one, and the public logs of 2026 use it. No band categories and no high power (rules 3).
  inline Contest arrl_digi ()
  {
    Contest c;
    c.name = "ARRL-DIGI";
    c.bands = QStringList {"ALL"};
    c.powers = QStringList {"LOW", "QRP"};
    c.transmitters = QStringList {"ONE"};
    return c;
  }

  /* Its rules: "in Cabrillo Format from WSJT", and "Incomplete content" goes to the check log. Its guide to WSJT-X's export: "Contest name YBDXPI", "Your 4 Grid Locator"
     for the location, "Fill with Assisted". Its definition file for N1MM says YBDXPIFT8, so the tag can be edited. All band only. The file named by its rules. */
  inline Contest ybdxpi ()
  {
    Contest c;
    c.name = "YBDXPI";
    c.assisted = "ASSISTED";
    c.callNamesFile = true;
    c.locationIsGrid = true;
    c.bands = QStringList {"ALL"};
    c.powers = QStringList {"LOW", "HIGH", "QRP"};
    c.transmitters = QStringList {"ONE"};
    return c;
  }

  struct Record
  {
    int line = 0;                     // where it starts in the log file
    QString call, band, frequency, grid, ownCall, ownGrid;   // grid: empty, or 4 characters
    QDateTime start, end;
  };

  // read's answer: the records whole, and the lines of what cannot be read as a QSO
  struct Log
  {
    QList<Record> records;
    QList<int> damaged;
  };

  inline bool all_digits (QString const& text)
  {
    for (auto const c : text) if (c < '0' || c > '9') return false;
    return !text.isEmpty ();
  }

  // a 4 character Maidenhead square. RR73 has that form and is a sign-off: the square is in the Arctic Ocean
  inline bool is_grid (QString const& g)
  {
    return 4 == g.size () && g[0] >= 'A' && g[0] <= 'R' && g[1] >= 'A' && g[1] <= 'R' && g[2] >= '0' && g[2] <= '9' && g[3] >= '0' && g[3] <= '9' && g != "RR73";
  }

  // from the digits (memory: a format parse fails on a summer-time start day in some Qt versions); invalid when they are no date and time
  inline QDateTime when (QString const& date, QString const& time)
  {
    if (date.size () != 8 || !all_digits (date) || (time.size () != 4 && time.size () != 6) || !all_digits (time)) return QDateTime {};
    QDate const d {date.mid (0, 4).toInt (), date.mid (4, 2).toInt (), date.mid (6, 2).toInt ()};
    QTime const t {time.mid (0, 2).toInt (), time.mid (2, 2).toInt (), time.size () >= 6 ? time.mid (4, 2).toInt () : 0};
    if (!d.isValid () || !t.isValid ()) return QDateTime {};
    return QDateTime {d, t, Qt::UTC};
  }

  // the freq column, from ADIF's FREQ in MHz. By its digits, so that no rounding can reach the kHz. Empty: neither frequency nor a band known.
  inline QString frequency_of (QString const& mhz, QString const& band)
  {
    static QRegularExpression const number {"^([0-9]{1,5})(?:\\.([0-9]*))?$"};
    auto const m = number.match (mhz);
    int const whole = m.captured (1).toInt ();
    if (m.hasMatch () && whole > 0)
      {
        if (whole >= 50 && whole < 54) return QString {"50"};
        return QString::number (whole * 1000 + (m.captured (2) + "000").left (3).toInt ());
      }
    static QMap<QString, QString> const edges {{"160m", "1800"}, {"80m", "3500"}, {"40m", "7000"}, {"20m", "14000"}, {"15m", "21000"}, {"10m", "28000"}, {"6m", "50"}};
    return edges.value (band);
  }

  inline bool parse (QString const& part, Record& r)
  {
    r.call = adif_field (part, "CALL:").trimmed ().toUpper ();
    r.band = adif_field (part, "BAND:").trimmed ().toLower ();
    r.frequency = frequency_of (adif_field (part, "FREQ:").trimmed (), r.band);
    r.ownCall = adif_field (part, "STATION_CALLSIGN:").trimmed ().toUpper ();
    r.ownGrid = adif_field (part, "MY_GRIDSQUARE:").trimmed ().left (4).toUpper ();
    r.grid = adif_field (part, "GRIDSQUARE:").trimmed ().left (4).toUpper ();
    if (!is_grid (r.grid)) r.grid.clear ();   // no exchange: R-12 or 1234 typed in the box is none either
    QString const dateOn = adif_field (part, "QSO_DATE:").trimmed ();
    QString const dateOff = adif_field (part, "QSO_DATE_OFF:").trimmed ();
    QString const timeOff = adif_field (part, "TIME_OFF:").trimmed ();
    r.start = when (dateOn, adif_field (part, "TIME_ON:").trimmed ());
    r.end = timeOff.isEmpty () ? r.start : when (dateOff.isEmpty () ? dateOn : dateOff, timeOff);
    if (!r.end.isValid ()) r.end = r.start;
    if (dateOff.isEmpty () && r.end < r.start) r.end = r.end.addDays (1);   // no end date, and the QSO crossed midnight
    return !r.call.isEmpty () && !r.band.isEmpty () && !r.frequency.isEmpty () && !r.ownCall.isEmpty () && is_grid (r.ownGrid) && r.start.isValid ();
  }

  /* The log file's text. By <eor> and not by line, for a record another program wrapped; by searching for <call: and not by the field
     lengths, because a record cut inside a field would swallow the next: a crash leaves a part with no <eor>, the next QSO written after
     it, so every <call: but the last in a part started a record that was cut. ADIF::load () would read the two as one and lose the second. */
  inline Log read (QString const& text)
  {
    Log log;
    int line = 1, counted = 0;
    auto lineOf = [&] (int at) {
      for (; counted < at; ++counted) if ('\n' == text.at (counted)) ++line;
      return line;
    };
    auto firstCharacter = [&] (int at, int end) {
      while (at < end && text.at (at).isSpace ()) ++at;
      return at;
    };
    int const header = text.indexOf ("<eoh>", 0, Qt::CaseInsensitive);
    int pos = header < 0 ? 0 : header + 5;
    while (pos < text.size ())
      {
        int const eor = text.indexOf ("<eor>", pos, Qt::CaseInsensitive);
        int const end = eor < 0 ? text.size () : eor;
        int calls = 0, last = pos;
        for (int at = text.indexOf ("<call:", pos, Qt::CaseInsensitive); at >= 0 && at < end; at = text.indexOf ("<call:", at + 1, Qt::CaseInsensitive))
          {
            if (calls++) log.damaged << lineOf (last);
            last = at;
          }
        // one <call: and the fields before it are this record's own; of several, only the last is whole
        int const first = firstCharacter (calls > 1 ? last : pos, end);
        if (first < end)
          {
            Record r;
            r.line = lineOf (first);
            if (eor >= 0 && parse (text.mid (first, end - first), r)) log.records << r;
            else log.damaged << r.line;
          }
        if (eor < 0) break;
        pos = eor + 5;
      }
    return log;
  }

  // the own calls in the log, the one with the most QSOs first
  inline QStringList own_calls (QList<Record> const& records)
  {
    QMap<QString, int> counts;
    for (auto const& r : records) counts[r.ownCall]++;
    QStringList calls = counts.keys ();
    std::stable_sort (calls.begin (), calls.end (), [&] (QString const& a, QString const& b) {return counts[a] > counts[b];});
    return calls;
  }

  // what goes in the file, in order of time, and the lines of what does not. noGrid are among the qsos.
  struct Selection
  {
    QList<Record> qsos;
    QList<int> otherCall, twice, noGrid;
  };

  inline Selection select (QList<Record> const& records, QString const& call)
  {
    Selection s;
    QMap<QString, int> seen;   // a QSO's key, its place in qsos
    for (auto const& r : records)
      {
        if (r.ownCall != call.toUpper ()) {s.otherCall << r.line; continue;}
        QString const key = r.call + ' ' + r.band + ' ' + QString::number (r.start.toMSecsSinceEpoch ());
        auto const first = seen.constFind (key);
        if (first != seen.constEnd ())
          {
            s.twice << r.line;
            if (s.qsos[*first].grid.isEmpty ()) s.qsos[*first].grid = r.grid;   // the same QSO: only the later copy has his grid
            continue;
          }
        seen.insert (key, s.qsos.size ());
        s.qsos << r;
      }
    for (auto const& q : s.qsos) if (q.grid.isEmpty ()) s.noGrid << q.line;
    std::stable_sort (s.qsos.begin (), s.qsos.end (), [] (Record const& a, Record const& b) {return a.end < b.end;});
    return s;
  }

  // what the status bar's sum takes (contest_log_score ()): oldest first for each call, as the file is
  inline QList<contest_score::Qso> score_qsos (QList<Record> const& qsos)
  {
    QList<contest_score::Qso> list;
    for (auto const& r : qsos) list << contest_score::Qso {r.call, r.band, r.grid};
    return list;
  }

  // the own grid most of the QSOs were made from, for the header and for the distances of the claimed score
  inline QString own_grid (QList<Record> const& qsos)
  {
    QMap<QString, int> counts;
    QString most;
    for (auto const& r : qsos) if (++counts[r.ownGrid] > counts.value (most)) most = r.ownGrid;
    return most;
  }

  // digits by hand: QDate::toString () follows the system's locale in Qt 5, and this file leaves the machine
  inline QString digits (int value, int width) {return QString::number (value).rightJustified (width, '0');}

  inline QString qso_line (Record const& r, Contest const& c)
  {
    QDate const d = r.end.date ();
    QTime const t = r.end.time ();
    QString const received = r.grid.isEmpty () ? QString {"ZZ00"} : r.grid;
    QString line = "QSO: " + r.frequency.rightJustified (5) + " DG "
      + digits (d.year (), 4) + '-' + digits (d.month (), 2) + '-' + digits (d.day (), 2) + ' ' + digits (t.hour (), 2) + digits (t.minute (), 2) + ' '
      + r.ownCall.leftJustified (13) + ' ' + r.ownGrid.leftJustified (8) + ' ' + r.call.leftJustified (13) + ' ';
    return line + (c.transmitterColumn ? received.leftJustified (12) + " 0" : received);
  }

  struct Header
  {
    QString contest, call, location, categoryOperator, categoryAssisted, categoryBand, categoryPower, categoryTransmitter, categoryStation;
    QString claimedScore, gridLocator, operators, name, address1, address2, email, club, soapbox, createdBy;
  };

  // the spec: a line of 75 characters at most, and as many OPERATORS: lines as it takes, the calls split by spaces or commas
  inline QStringList operator_lines (QString const& operators)
  {
    int const room = 75 - QString {"OPERATORS: "}.size ();
    QStringList lines;
    QString line;
    for (auto const& call : QString {operators}.replace (',', ' ').simplified ().split (' '))
      {
        if (!line.isEmpty () && line.size () + 1 + call.size () > room) {lines << line; line.clear ();}
        line += (line.isEmpty () ? QString {} : QString {" "}) + call;
      }
    return lines << line;
  }

  // an empty tag is left out: the spec requires only the first and the last line
  inline QString document (Header const& h, QList<Record> const& qsos, Contest const& c)
  {
    QString t {"START-OF-LOG: 3.0\n"};
    auto tag = [&t] (char const* name, QString const& value) {
      if (!value.trimmed ().isEmpty ()) t += QString {name} + ": " + value.trimmed () + '\n';
    };
    tag ("CONTEST", h.contest);
    tag ("CALLSIGN", h.call);
    tag ("LOCATION", h.location);
    tag ("CATEGORY-OPERATOR", h.categoryOperator);
    tag ("CATEGORY-ASSISTED", h.categoryAssisted);
    tag ("CATEGORY-BAND", h.categoryBand);
    tag ("CATEGORY-POWER", h.categoryPower);
    tag ("CATEGORY-MODE", "DIGI");
    tag ("CATEGORY-TRANSMITTER", h.categoryTransmitter);
    tag ("CATEGORY-STATION", h.categoryStation);
    tag ("CLAIMED-SCORE", h.claimedScore);
    tag ("GRID-LOCATOR", h.gridLocator);
    for (auto const& line : operator_lines (h.operators)) tag ("OPERATORS", line);
    tag ("NAME", h.name);
    tag ("ADDRESS", h.address1);
    tag ("ADDRESS", h.address2);
    tag ("EMAIL", h.email);
    tag ("CLUB", h.club);
    tag ("SOAPBOX", h.soapbox);
    tag ("CREATED-BY", h.createdBy);
    for (auto const& r : qsos) t += qso_line (r, c) + '\n';
    t += "END-OF-LOG:\n";
    return t;
  }

  // a call's / is no character for a file name
  inline QString file_name (Contest const& c, QString const& slug, int year, QString const& call)
  {
    QString safe {call};
    safe.replace ('/', '-');
    if (c.callNamesFile) return safe + ".log";
    return safe + '_' + slug + '-' + QString::number (year) + ".cbr";
  }

  // the year in contest_log_filename ()'s name for a contest's log; 0 for any other name
  inline int log_year (QString const& fileName, QString const& slug)
  {
    QString const before = "wsjtx_" + slug + '-', after {"_log.adi"};
    if (!fileName.startsWith (before) || !fileName.endsWith (after) || fileName.size () != before.size () + 4 + after.size ()) return 0;
    bool ok = false;
    int const year = fileName.mid (before.size (), 4).toInt (&ok);
    return ok ? year : 0;
  }

  // lines of the log for the dialog's summary: the first dozen say what to look for
  inline QString line_list (QList<int> const& lines)
  {
    QStringList shown;
    for (int i = 0; i < lines.size () && i < 12; ++i) shown << QString::number (lines[i]);
    return shown.join (", ") + (lines.size () > 12 ? ", ..." : "");
  }
}

#endif
