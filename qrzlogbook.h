#ifndef QRZLOGBOOK_H
#define QRZLOGBOOK_H

/* CE3TSK 2026-11-22 (TODO.md 3.-280, QSL_CONFIRMATIONS_PLAN.md step 4): the user's LoTW confirmations from QRZ.com, beside
   Club Log's (clublog.h) - the two add up (the operator: "so that we get the best sample in case something is missing
   on one side").

   QRZ's Logbook API (https://logbook.qrz.com/api, docs QRZLogbookAPI.html) answers a POST of KEY, ACTION and OPTION
   with name=value pairs; FETCH gives QSOs as ADIF in that answer, escaped as HTML (&lt;call:6&gt;...), NOT URL-encoded,
   so the answer is never split on every '&'. The key is the logbook's own, from the user's QRZ.com Logbook, and needs a
   subscription (XML level or higher). A record carries QRZ's confirmation (APP_QRZLOG_STATUS C) - LoTW's credit and
   QRZ's own match alike - and apart from it LOTW_QSL_RCVD Y with LOTW_QSLRDATE: only those count here (the operator:
   "LoTW confirmations only"; QRZ's own confirmations may be another feature one day).

   Checked against the operator's logbook 2026-11-22: a page of 250 QSOs is 0.3 MB in about a second; a page past the
   last record answers RESULT=FAIL&COUNT=0 with no REASON; a wrong key HTTP 401 with RESULT=AUTH; MODSINCE returns the
   QSOs modified since a day, among them old QSOs whose LoTW confirmation came since. So the first download, a new key
   and once a week fetch every confirmed QSO in pages (STATUS:CONFIRMED, MAX:250, AFTERLOGID); in between, every 12 hours,
   only what changed (MODSINCE, a day of overlap) - a QSO that lost its LoTW confirmation is dropped, a new one added.

   Only what the log's reader uses is kept, as qrz_qsl.adi in the data directory - QRZ's records carry the partner's
   name, address and e-mail, which are no business of this file. The key is a password with full read and write access
   to the logbook: only FETCH is ever sent, the key lives in a file of its own beside the settings file, readable by the
   user alone (club_log::write_token, D6), never in the ini, a log, ALL.TXT or an error message. A refused key (401, 403
   or RESULT=AUTH) is never tried again until another is saved. */

#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QString>

#include "FileDownload.hpp"

class QNetworkAccessManager;

namespace qrz_logbook
{
  constexpr char const * real_api {"https://logbook.qrz.com/api"};
  constexpr char const * file_name {"qrz_qsl.adi"};
  constexpr char const * api_env {"JTDX_QRZ_API"};
  constexpr int refresh_hours {12};
  constexpr int full_days {7};              // a full download once a week: what MODSINCE cannot show (a QSO deleted)
  constexpr int page_size {250};            // QRZ's own advice
  constexpr int max_pages {2000};           // 500,000 QSOs: past it the pages are taken not to end
  constexpr qint64 max_page_size {32 * 1024 * 1024};
  constexpr int timeout_ms {120000};
  constexpr int page_gap_ms {500};          // between two pages: one request at a time, never a burst

  /* The API's address. JTDX_QRZ_API is for tests and may only name a server on this machine, http://127.0.0.1:<port> or
     http://localhost:<port> (a path after it is kept); any other value gives an empty string, which switches the
     download off - as club_log::api_base () */
  QString api_base ();

  // four groups of four letters and digits, with or without the dashes (QRZ: "leave them out" where dashes are refused)
  bool plausible_key (QString const& key);
  // the key's file beside the settings file: JTDX.ini -> JTDX_qrz.key (written with club_log::write_token)
  QString key_path (QString const& settings_file);
  // the key in the file, or none: a byte-order mark dropped, what is not a key (plausible_key) none
  QString read_key (QString const& path);

  // the form of one FETCH: KEY, ACTION and OPTION, URL-encoded
  QByteArray request_body (QString const& key, QString const& option);
  QString full_option (qint64 after_logid);                       // STATUS:CONFIRMED,MAX:250,AFTERLOGID:n
  QString changes_option (QDate const& since, qint64 after_logid);  // MODSINCE:yyyy-MM-dd,MAX:250,AFTERLOGID:n

  /* One answer. Records: lotw holds the LoTW-confirmed QSOs as kept (by their logid), others the logids of the rest
     (a change fetch drops them), last_logid the highest of all. End: no QSO (RESULT=FAIL, COUNT=0, no REASON). Refused:
     RESULT=AUTH. Error: anything else, reason saying what. */
  struct Page
  {
    enum Kind {Records, End, Refused, Error};
    Kind kind {Error};
    QString reason;
    QMap<qint64, QByteArray> lotw;
    QList<qint64> others;
    int records {0};
    qint64 last_logid {-1};
  };
  Page read_page (QByteArray const& body);

  // the kept file: its records by logid, and written back with its header (its origin and the time)
  QMap<qint64, QByteArray> read_kept (QByteArray const& file);
  QByteArray write_kept (QMap<qint64, QByteArray> const& kept, QDateTime const& downloaded);
  int entities (QMap<qint64, QByteArray> const& kept);   // DXCC entities among them, by QRZ's numbers (0 not one)

  // a download is due: none yet, the last good one refresh_hours or more ago, or in the future
  bool due (QDateTime const& last_success, QDateTime const& now);
  // a full one is due: none yet, the last full_days or more ago, or in the future
  bool full_due (QDateTime const& last_full, QDateTime const& now);

  /* The paged download: one FileDownload a page (its transport, size cap, timeouts and no-redirect rule for a request with
     headers), one page at a time, the records held in memory until the last page, and only then qrz_qsl.adi replaced at
     once. Exactly one finished () per start (). */
  class Fetch
    : public QObject
  {
    Q_OBJECT

  public:
    explicit Fetch (QObject * parent = nullptr);
    /* full: every confirmed QSO; else the changes since `since` applied to the file there now. user_agent names the
       program (QRZ: "MUST provide an identifiable user agent") */
    void start (QNetworkAccessManager * manager, QString const& key, bool full, QDate const& since,
                QString const& file, QString const& user_agent);
    bool running () const {return running_;}
    void abort ();
    bool full () const {return full_;}
    bool refused () const {return refused_;}       // the key was refused (RESULT=AUTH, or HTTP 401 or 403)
    QDateTime retry_after () const {return page_.retry_after ();}
    int confirmed () const {return kept_.size ();}    // after a good one: the LoTW-confirmed QSOs kept
    int entities () const {return entities_;}
    int pages () const {return pages_;}
    QDateTime started () const {return started_;}

    Q_SIGNAL void finished (bool ok, FileDownload::Failure failure, QString const& detail);

  private:
    void request ();
    void page_done ();
    void fail (FileDownload::Failure failure, QString const& detail);

    FileDownload page_;
    QNetworkAccessManager * manager_ {nullptr};
    QString key_;
    QString file_;
    QString user_agent_;
    QDate since_;
    QDateTime started_;
    QMap<qint64, QByteArray> kept_;
    Page last_;
    qint64 after_ {0};
    int pages_ {0};
    int entities_ {0};
    bool full_ {true};
    bool running_ {false};
    bool refused_ {false};
  };
}

#endif
