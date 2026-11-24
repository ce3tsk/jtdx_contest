#ifndef CLUBLOGUSERS_H
#define CLUBLOGUSERS_H

/* CE3TSK 2026-11-24 (TODO.md 3.-283, GRID_FILL_PLAN.md stage 4): Club Log's users file - each call's locator and last
   upload - for a QSO that gave no grid (gridfill.h, source 4, after eQSL's member list, before QRZ.com).

   GET /api/v1/reference/clublog-users with the token (one general request of Club Log's 500 a day): "A zip of JSON,
   large. Callsigns known to Club Log, with first QSO, last QSO, last upload, locator, OQRS, and LoTW fields where Club
   Log has them ... Download on a schedule." As downloaded once by the operator (2026-11-23): a ZIP of 5.6 MB holding
   clublog_users.json, 37 MB - one JSON object, the call the key, each value some of firstqso, lastqso, lastupload,
   locator, oqrs, last-lotw, dates "YYYY-MM-DD HH:MM:SS"; 275,287 keys, 1,085 placeholders (an underscore or a hyphen);
   91,720 with a locator, 33,417 of them uploaded within 12 months, 41,130 within 24.

   - The ZIP read here: its central directory, the one entry inflated with zlib (a raw deflate stream), its CRC checked,
     no more than max_json bytes. No ZIP64. zlib 1.4 is built in (contrib/zlib/, Z_PREFIX: its JTDX_CONTEST.txt).
   - The JSON read by a scanner of this one shape - never a 37 MB document in memory - keeping what a grid may come from:
     a real call (no placeholder), a locator, an upload within keep_days. Written as a small file at the download
     (clublog_grids.csv in the log directory: "CALL,GRID,YYYY-MM-DD"), read at the start.
   - A grid only while its call uploaded within recent_days (G3, as eQSL's list) - asked again by date at each lookup.

   test_gridfill has the rules, with a ZIP made by the test. */

#include <QByteArray>
#include <QDate>
#include <QHash>
#include <QString>

namespace club_log_users
{
  char const * const path {"/reference/clublog-users"};     // under club_log::api_base ()
  char const * const entry_name {"clublog_users.json"};
  char const * const file_name {"clublog_grids.csv"};       // the small file, in the log directory
  qint64 const max_json {256LL * 1024 * 1024};               // inflated, at most: a ZIP is no bomb here
  int const keep_days {730};                                 // kept in the small file: the last 24 months
  int const recent_days {365};                               // taken: the last 12 months (G3)
  int const minimum_members {1000};                          // fewer: not Club Log's file

  // the entry `name` of a ZIP, inflated, or empty with `error` saying why
  QByteArray zip_entry (QByteArray const& zip, char const * name, qint64 max_size, QString * error);

  /* The small file's text from the downloaded ZIP, or empty with `error` (not a ZIP, no entry, not the JSON expected,
     fewer than minimum_members kept): "# Club Log users, YYYY-MM-DD: call,locator,last upload" then a line a call kept. */
  QByteArray small_file (QByteArray const& zip, QDate const& today, QString * error, int * kept = nullptr);

  struct Member
  {
    QString grid;     // its first four characters, upper case
    QDate uploaded;
  };
  QDate date_of (QByteArray const& d);   // "YYYY-MM-DD..." from its digits, or invalid
  // the small file read: by call in upper case
  QHash<QString, Member> read (QByteArray const& small);
  // a call's grid, or "": while its last upload is within recent_days before `today`
  QString grid (QHash<QString, Member> const& members, QString const& call, QDate const& today);
}

#endif
