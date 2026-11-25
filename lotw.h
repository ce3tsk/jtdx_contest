#ifndef LOTW_H
#define LOTW_H

/* CE3TSK 2026-11-25 (TODO.md 3.-299, LOTW_PLAN.md, the operator: "Let's plan the LoTW confirmations download first and for
   later the upload", then "let's start with the first part of LotW"): the user's LoTW confirmations from LoTW itself,
   added up with Club Log's (clublog.h) and QRZ.com's (qrzlogbook.h) - both only have what they have fetched from LoTW
   (on 2026-11-21 Club Log held 8,290 of the operator's 31,965 LoTW QSLs).

   LoTW's report (lotw.arrl.org/lotw-help/developer-query-qsos-qsls/): https://lotw.arrl.org/lotwuser/lotwreport.adi,
   GET only, the website's username and password in the query - so the address is never logged, shown or written, no
   redirect is followed, and a shown text is scrubbed of the password (scrubbed ()). qso_qslsince is ALWAYS sent: left out,
   LoTW takes the account's own marker on the server, which the user's other programs move too. A failure is an HTML page
   without the ADIF end of header - a wrong password is not told apart from LoTW being busy, so a failure waits
   retry_hours before the next try. A good report has PROGRAMID LoTW and as many records as its APP_LoTW_NUMREC; anything
   else changes nothing.

   The operator's decisions (LOTW_PLAN.md 6): only My Call's QSLs (qso_owncall, LD3); every 12 hours the QSLs since the
   last report's APP_LoTW_LASTQSL, merged into the file - a full report the first time, for another login or My Call, a
   missing file and every full_days (LD2); a failure tried again after retry_hours (LD4); a DATA-mode QSL stays DATA (LD7).
   Kept, as lotw_qsl.adi in the data directory: only what the log's reader and the count use - not the partner's
   location details beyond the grid, state and county. One QSO a key (call, band, mode, submode, date, time, own call);
   of two, the newer QSLRDATE stays. */

#include <QByteArray>
#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QString>

#include "FileDownload.hpp"

class QNetworkAccessManager;

namespace lotw
{
  constexpr char const * real_api {"https://lotw.arrl.org/lotwuser/lotwreport.adi"};
  constexpr char const * file_name {"lotw_qsl.adi"};
  constexpr char const * api_env {"JTDX_LOTW_API"};
  constexpr char const * first_day {"2000-01-01"};   // a full report: every QSL since before LoTW began (2003)
  constexpr int refresh_hours {12};
  constexpr int full_days {30};
  constexpr int retry_hours {6};
  constexpr qint64 max_size {128 * 1024 * 1024};      // the operator's whole report is 20.5 MB
  constexpr int timeout_ms {120000};

  /* LoTW's address. JTDX_LOTW_API is for tests and may only name a server on this machine, http://127.0.0.1:<port> or
     http://localhost:<port> (a path after it is kept); any other value gives an empty string, which switches the download
     off - as qrz_logbook::api_base () */
  QString api_base ();

  // the login's file beside the settings file: JTDX.ini -> JTDX_lotw.login (loginfile.h: the username, the password)
  QString login_path (QString const& settings_file);

  /* the report's address: login, password, qso_query=1, qso_qsl=yes, qso_qslsince (never left out: an empty `since` is
     first_day), qso_owncall, qso_withown=yes, qso_qsldetail=yes - every value percent-encoded */
  QString request_url (QString const& base, QString const& user, QString const& password, QString const& since,
                       QString const& own_call);

  // a text for the user with the password - as typed and percent-encoded - and any address in it taken out
  QString scrubbed (QString text, QString const& password);

  /* One report. Good: the end of header, PROGRAMID LoTW, APP_LoTW_NUMREC there and as many records; then last_qsl is its
     APP_LoTW_LASTQSL (empty when it has none: no QSL in it) and records the QSLs as kept, by key. Else reason says why. */
  struct Report
  {
    bool good {false};
    QString reason;
    QString last_qsl;
    int numrec {-1};
    QMap<QByteArray, QByteArray> records;
  };
  Report read_report (QByteArray const& body);

  // the kept file: its records by key; written back with a header naming its origin and the time
  QMap<QByteArray, QByteArray> read_kept (QByteArray const& file);
  QByteArray write_kept (QMap<QByteArray, QByteArray> const& kept, QDateTime const& downloaded);
  // a report's QSLs added to what is kept: a new QSO added, of the same QSO the newer QSLRDATE kept
  void merge (QMap<QByteArray, QByteArray>& kept, QMap<QByteArray, QByteArray> const& more);
  int entities (QMap<QByteArray, QByteArray> const& kept);   // DXCC entities among them, by LoTW's numbers (0 not one)

  bool due (QDateTime const& last_success, QDateTime const& now);   // none yet, refresh_hours on, or in the future
  bool full_due (QDateTime const& last_full, QDateTime const& now); // none yet, full_days on, or in the future

  /* One report: a FileDownload with a header of its own (so no redirect is followed), checked in full before anything is
     written, merged into the file there (or replacing it, for a full report) through QSaveFile. Exactly one finished ()
     per start (). */
  class Download
    : public QObject
  {
    Q_OBJECT

  public:
    explicit Download (QObject * parent = nullptr);
    /* since: the last report's APP_LoTW_LASTQSL - empty for a full report (from first_day, the file replaced) */
    void start (QNetworkAccessManager * manager, QString const& user, QString const& password, QString const& own_call,
                QString const& since, QString const& file, QString const& user_agent);
    bool running () const;
    bool full () const {return full_;}
    QDateTime started () const {return started_;}
    QString last_qsl () const {return last_qsl_;}   // after a good one: the next report's qso_qslsince
    int confirmed () const {return confirmed_;}     // after a good one: the QSLs in the file
    int entities () const {return entities_;}
    int received () const {return received_;}       // after a good one: the QSLs in this report

    Q_SIGNAL void finished (bool ok, FileDownload::Failure failure, QString const& detail);

  private:
    FileDownload download_;
    QString file_;
    QString password_;
    QString since_;
    QString last_qsl_;
    QDateTime started_;
    Report report_;
    int confirmed_ {0};
    int entities_ {0};
    int received_ {0};
    bool full_ {true};
  };
}

#endif
