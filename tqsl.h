#ifndef TQSL_H
#define TQSL_H
/* CE3TSK 2026-11-25 (TODO.md 3.-303, LOTW_PLAN.md section 5, the operator: "Let's add the LotW QSO log upload via TQSL, so
   we would need a way to select automatically the tqsl executable, and if not found there should be a way to select it
   similar to what GridTracker does"): ARRL's TQSL signs each QSO logged with the user's callsign certificate and uploads
   it to LoTW. The program runs TQSL's command line and never touches the certificate, its key or its password (the
   operator's choice: TQSL asks for it in its own window - no -p). Where TQSL is, its Station Locations, the arguments of
   a run and TQSL's final status; what that status means for a QSO is qso_upload::tqsl_result (). */
#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

namespace tqsl
{
  constexpr char const * program_env {"JTDX_TQSL"};   // tests: their stand-in; set and empty: TQSL switched off
  constexpr int timeout_ms {15 * 60 * 1000};          // a run - TQSL's own password window waiting included - then killed
  int test_timeout_ms ();   // the Test button's `tqsl -q -v`: 30 s - (3.-304) JTDX_TQSL_TEST_MS, the tests' own

  // where TQSL is looked for on this system, in this order: GridTracker2's places, Program Files on Windows, the PATH
  QStringList candidates ();
  bool runnable (QString const& path);   // a file that may be run
  bool switched_off ();                  // JTDX_TQSL set and empty
  /* the program to run: JTDX_TQSL when set (empty, or not a program: none), else `chosen` when it may be run (the user's
     Browse), else the first candidate that may be; empty: none */
  QString program (QString const& chosen);
  QString station_file ();               // TQSL's Station Locations (its station_data) on this system - (3.-304) TQSLDIR first
  // (3.-304, the review) the program in what was chosen: a macOS bundle (tqsl.app, a folder) its Contents/MacOS/tqsl
  QString program_in (QString const& chosen);
  // the Station Locations in that file: name and call (upper case), in the file's order
  QList<QPair<QString, QString>> station_locations (QString const& file);
  /* one run: no date range window (-d), upload (-u), every QSO - already uploaded ones too (-a all, the operator) - then
     exit (-x), the Station Location (-l), the file; never -p */
  QStringList arguments (QString const& location, QString const& file);

  // TQSL's last "Final Status: <text> (<code>)" line: code -1 when there is none; detail: its last lines before it
  struct Status
  {
    int code {-1};
    QString text;
    QString detail;
  };
  Status final_status (QByteArray const& output);
  QString version (QByteArray const& output);   // the Test button's answer: TQSL's last line, without its time
}

#endif
