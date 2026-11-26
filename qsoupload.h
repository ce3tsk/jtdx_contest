#ifndef QSOUPLOAD_H
#define QSOUPLOAD_H
/* CE3TSK 2026-11-23 (TODO.md 3.-282, QSO_UPLOAD_PLAN.md): each QSO logged sent to Club Log and QRZ.com - the other way
   round from the LoTW confirmations (clublog.h, qrzlogbook.h), with the same token and key (the operator: "the
   credentials, aka token and API key are already used for the LotW confirmations"), off by default. The aim: no other
   program running beside JTDX for it (the operator: "to replace or reduce the need of running extra tools like
   gridtracker, jtalert, ham radio delux").

   Club Log (REST API v1): POST /qso with the log's call and the QSO as ONE ADIF record, the token as for the downloads;
   a run of QSOs waiting goes as POST /qso/batch, up to 100 in a request ("A run of contacts belongs on the batch").
   201 created, 200 a duplicate or an update; 422 the QSO refused - "Do not send the same body again"; 401 the token,
   403 the call not on the token's account, 404 a call Club Log never registered; 429 wait for Retry-After; 500 "may or
   may not have been applied. Do not immediately repeat a write". A test-mode token checks and stores nothing.

   QRZ.com (Logbook API): KEY, ACTION=INSERT and ADIF, form-encoded. RESULT=OK with LOGID, FAIL with a REASON (a
   duplicate among them), AUTH when the key may not do it (INSERT needs a subscription). OPTION=REPLACE would overwrite
   confirmed QSOs with this one, and is NEVER sent - nor any other action but INSERT.

   The operator's decisions (2026-11-23): a QSO that cannot be sent is kept and tried again (U1) - a queue file a site in
   the data directory, QSO records alone, never a token or key; a QSO the site refuses is dropped with the site's reason;
   every QSO logged, in a contest too (U2); a QSO not sent or refused noted in the main window's status bar (U3). A second
   copy of a QSO is never harm: both sites answer it as a duplicate and keep what they have. */
#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QProcess>
#include "FileDownload.hpp"
#include "tqsl.h"   // CE3TSK 2026-11-25 (TODO.md 3.-303)

class QNetworkAccessManager;

namespace qso_upload
{
  enum class Site {ClubLog, Qrz, Lotw};   // (3.-295, the review 12) eQSL is none of them: its queue file names it by write_queue (name)
  // (3.-303) Lotw: TQSL signs the QSOs and uploads them - a program run where the others send a request (LOTW_PLAN.md 5)
  constexpr char const * clublog_queue_file {"clublog_upload.adi"};
  constexpr char const * qrz_queue_file {"qrz_upload.adi"};
  constexpr char const * lotw_queue_file {"lotw_upload.adi"};      // (3.-303)
  constexpr char const * lotw_signing_file {"lotw_signing.adi"};  // (3.-303) the QSOs of one run, for TQSL - removed after it
  constexpr int lotw_batch_max {100};                            // (3.-303) QSOs a TQSL run
  constexpr int clublog_batch_max {100};      // Club Log: "One to 100 items"
  constexpr int timeout_ms {60000};
  constexpr qint64 max_reply_size {1024 * 1024};
  constexpr int gap_ms {2000};                // between two requests of a run waiting: never a burst
  constexpr int max_wait_hours {24};          // the longest a Retry-After may hold the uploads (Club Log's daily limit)

  QString site_name (Site site);              // "Club Log", "QRZ.com" - names, not translated
  // an ADIF field's value in one record ("" when it has none): STATION_CALLSIGN, CALL ...
  QString field (QByteArray const& record, QString const& name);
  // (3.-296) the record without that field, its tag and its value; walked as field () walks it, so a value holding '<' is
  // passed over whole - the record as it was when it has none
  QByteArray without_field (QByteArray const& record, QString const& name);
  // one line: a line break inside the record (none should be there) becomes a space, so that the queue keeps a record a line
  QByteArray one_line (QByteArray const& record);

  // Club Log: the URL (base + /qso, or /qso/batch for more than one), and the JSON - each record with its own call,
  // STATION_CALLSIGN, or own_call when the record has none
  QString clublog_url (QString const& base, int records);
  QByteArray clublog_body (QList<QByteArray> const& records, QString const& own_call);
  // QRZ.com: KEY, ACTION=INSERT and ADIF, URL-encoded - nothing else
  QByteArray qrz_body (QString const& key, QByteArray const& record);

  /* What an answer means for one record. Sent: the site has it now (id: Club Log's qsoid, QRZ.com's logid; test: a Club
     Log test-mode token - checked, nothing stored). Duplicate: it had it already. Rejected: the site refuses this QSO -
     reason in its own words - and it is dropped. Refused: nothing more is sent until the user acts - scope Token (the
     token or key), Call (Club Log: the call with this token - that call's QSOs alone) or Request (another refusal of the
     request itself, until the next start or OK). Wait: until Retry-After. Retry: later - no answer, a server failing, an
     answer not understood (review: Club Log's answer is understood only with the status created, updated, duplicate or
     rejected - a second copy of a QSO comes back a duplicate, so asking again never harms). OneByOne: Club Log refused a
     batch as a whole - its records go one at a time. */
  struct Result
  {
    enum Kind {Sent, Duplicate, Rejected, Refused, Wait, Retry, OneByOne};
    enum Scope {Token, Call, Request};
    Kind kind {Retry};
    Scope scope {Request};
    int http_status {0};
    QString reason;
    QString id;
    bool test {false};
    int code {-1};   // (3.-304) LoTW: TQSL's final status (or its exit code); -1 none
  };
  // a request's answer, one Result a record sent (records: how many the request carried)
  QList<Result> clublog_results (int http_status, QByteArray const& body, int records);
  Result qrz_result (int http_status, QByteArray const& body);
  /* (3.-303) a TQSL run (started: it ran; finished: it ended by itself, not killed or crashed): its final status - (3.-304)
     or its exit code, the same (TQSL's exitNow) - 0 Sent; (3.-304) 8 Rejected (none signed: -a all skips a QSO TQSL cannot
     sign - a call or grid not the Station Location's), 9 Sent (some skipped - its words on the line), 14 Duplicate (signed
     before); 1 (cancelled - its password window closed), 2 (rejected by LoTW), 4, 5 (TQSL errors: the Station Location,
     the certificate), 10, or not started: Refused - kept, nothing more until OK; else (3, 6, 7, 11, 13 its database busy,
     a crash, killed) Retry. The reason: TQSL's own words; code: the status */
  Result tqsl_result (bool started, bool finished, int exit_code, QByteArray const& output);

  // the queue file: a header naming it, then one record a line; written whole, removed when nothing waits
  QList<QByteArray> read_queue (QString const& path);
  bool write_queue (QString const& path, QList<QByteArray> const& records, Site site);
  // (3.-295) the same, the header naming `name` - eQSL's queue (eqsl.h), which is no Site of the uploaders here
  bool write_queue (QString const& path, QList<QByteArray> const& records, QString const& name);

  int retry_minutes (int failures);   // after the n-th failure in a row: 2, 5, 10, 30, then 60 minutes
  // a Retry-After honoured: never sooner than a minute, never later than max_wait_hours; none named: five minutes
  QDateTime wait_until (QDateTime const& retry_after, QDateTime const& now);

  /* One site's uploads: the queue in its file, one request at a time, the oldest first. A QSO added is sent at once - also
     while a failure's back-off runs (U1: "at the next QSO") - but never before a Retry-After, never while blocked
     (Configuration keeps the refusals), never without a credential or with the uploads off.
     (review, 2026-11-23) A request carries the QSOs of one station call alone, and the QSOs of a call Club Log refused
     with this token are set aside - kept, never asked again until that refusal is lifted - while the others go on: Club
     Log's rule is "On 401, 403, 404, 409, and 422, stop and change the request", and one QSO of a call not on the account
     held back every QSO logged after it. */
  class Uploader
    : public QObject
  {
    Q_OBJECT
  public:
    explicit Uploader (Site site, QObject * parent = nullptr);
    ~Uploader () override;   // (3.-304) a TQSL run under way killed, its end never answered into members going away
    void configure (QNetworkAccessManager * manager, QString const& queue_file, QString const& user_agent);
    void set_credential (QString const& credential);   // the token or the key; none: nothing sent, the queue kept
    void set_own_call (QString const& call) {own_call_ = call;}   // Club Log's call for a record without STATION_CALLSIGN
    void set_enabled (bool on);           // off: nothing taken, nothing sent (the queue stays for a later tick)
    void set_blocked (bool blocked) {blocked_ = blocked;}   // a refusal on record (Configuration's): nothing sent
    // (review) the station calls refused on record (Club Log, with this token): their QSOs kept, not sent; upper case
    void set_refused_calls (QStringList const& calls);
    void resume () {stopped_ = false;}    // a refusal of this run lifted: the user pressed OK
    /* (review) the token proven good - a good Club Log download: a stop for the token's refusal lifted (a 401 to an
       upload while that download was under way), never one for a refusal of the request (it holds until OK or the next
       start). The token's refusal on record still holds every QSO through set_blocked () */
    void token_good ();
    void set_not_before (QDateTime const& when) {not_before_ = when;}   // a Retry-After kept from an earlier run
    void set_program (QString const& program) {program_ = program;}    // (3.-303) LoTW: the TQSL program; none: nothing sent
    void set_process_timeout (int ms) {process_timeout_ms_ = ms;}      // (3.-303) a TQSL run killed after it (tests: shorter)
    void add (QByteArray const& record);  // a QSO logged: into the queue file, and sent as soon as it may be
    void send_now ();                     // the next request now, if it may go
    int waiting () const {return queue_.size ();}
    int held () const {return queue_.size () - in_flight_;}   // (review) waiting, and not in the request now asked
    bool unsaved () const {return unsaved_;}   // (3.-295, the review 7) the queue's file could not be written: kept in memory alone
    bool running () const {return in_flight_ > 0;}
    bool enabled () const {return enabled_;}
    bool stopped () const {return blocked_ || stopped_;}      // a refusal holds every QSO (the token or key, the request)
    QStringList station_calls () const;   // (review) the station calls of the QSOs waiting, upper case, each once
    QDateTime not_before () const {return not_before_;}
    QDateTime next_try () const {return next_try_;}   // the back-off's or Retry-After's next try; invalid: none planned
    QString base () const;                // the API's address; empty: the uploads are switched off (the env variable)
    Site site () const {return site_;}

    // one record's end: Sent, Duplicate or Rejected (call: the station worked)
    Q_SIGNAL void done (qso_upload::Result const& result, QString const& call, int waiting);
    /* Refused (station_call: the call the request was for, call: the station worked in the first QSO of it): scope Token
       or Request - nothing more is sent; scope Call - that call's QSOs set aside, the others go on */
    Q_SIGNAL void refused (qso_upload::Result const& result, QString const& station_call, QString const& call, int waiting);
    // Wait or Retry: the record kept, tried at next
    Q_SIGNAL void delayed (qso_upload::Result const& result, QString const& call, QDateTime const& next, int waiting);
    /* (review) a QSO just added that is not sent now: refused - held by a refusal (the token or key, the request, its
       call); else waiting - no token or key, the uploads switched off, a Retry-After. Not when a request is under way:
       it goes with the next */
    Q_SIGNAL void kept (bool refused, QString const& call, int waiting);

  private:
    bool may_send () const;
    QString station_of (QByteArray const& record) const;   // STATION_CALLSIGN, or own_call_; upper case
    int first_sendable () const;          // the oldest QSO not set aside; -1: none
    void answered (int http_status, QByteArray const& body);
    void failed (FileDownload::Failure failure, QString const& detail);
    void apply (QList<Result> const& results);
    void save ();
    void tqsl_ended (bool started, bool finished, int exit_code);   // (3.-303)

    Site site_;
    QNetworkAccessManager * manager_ {nullptr};
    QString queue_file_;
    QString user_agent_;
    QString credential_;
    QString request_credential_;  // (review) the token or key the request under way was made with
    QString own_call_;
    bool enabled_ {false};
    bool blocked_ {false};
    bool stopped_ {false};       // a Refused of this run
    Result::Scope stop_scope_ {Result::Request};   // (review) what it was for: the token, or the request itself
    bool one_by_one_ {false};    // Club Log refused a batch: one record a request from then on
    QStringList refused_calls_;  // (review) station calls set aside
    QList<QByteArray> queue_;
    QList<int> in_flight_at_;    // (review) where in the queue the records of the request under way are
    int in_flight_ {0};
    int answers_ {0};            // answers taken (apply), counted: add () tells whether one came at once
    int failures_ {0};
    bool unsaved_ {false};       // (3.-295) the last save failed
    QDateTime not_before_;       // Retry-After: nothing before it
    QDateTime next_try_;
    QTimer timer_;
    FileDownload request_;
    QProcess process_;            // (3.-303) LoTW: the TQSL run under way
    QTimer process_timer_;
    QString program_;
    QString signing_file_;
    int process_timeout_ms_ {tqsl::timeout_ms};
    bool timed_out_ {false};
  };
}

#endif
