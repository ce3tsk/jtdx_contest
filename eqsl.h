// This source code file was last time modified by Arvo ES1JA on 20181229

#ifndef EQSL_H
#define EQSL_H

/* CE3TSK 2026-11-24 (TODO.md 3.-284, the operator: "keep eQSL's answer and show it like the others", then "Switch to
   https"): each QSO logged to eQSL.cc as before - one GET to importADIF.cfm with the record and the login in ADIFdata -
   and now:
   - eQSL's answer read (eqsl::answer) and given as answered (): Settings > Reporting's line and the main window's note
     show it, as the Club Log and QRZ.com uploads' (Configuration::eqsl_answered). The inherited class read the answer
     page and dropped it: a refusal - a QTH nickname that is no profile's (2026-11-24, 9A6T) - left no trace at all.
   - over https, through FileDownload (WinHTTP on Windows, where Qt has no OpenSSL): eQSL's interface description
     (ImportADIF.txt, 2026-02-19): "PLEASE USE https instead of http, since you will be sending the user's password in
     plain text!"
   - only its own reply listened to. The inherited class took the shared network manager's finished () - every reply
     of the program's, each deleteLater ()'d under its owner and an error of another's printed as eQSL's.
   - one request at a time, the oldest first: a QSO logged while one was under way cut that one off (abort ()), its
     answer lost.
   - ADIFdata percent-encoded whole: a '+' left bare in an address is read as a space - a report "+05" reached eQSL as
     " 05".
   (3.-285, the operator: "Let's relax the requirement to what eQSL really needs") the QTH nickname optional: sent only
   when there is one; without it eQSL picks the account by the QSO's date - "less reliable", in its own words, when
   QTH profiles' dates overlap. The user and the password are what eQSL needs.
   (3.-290, the operator: "Let's do the recommended Send Comments as COMMENT too") the QSO's own comment - the log's
   COMMENT, the one Club Log and QRZ.com get - sent as COMMENT when there is one: eQSL's ADIF content page, "The field
   called 'COMMENT' is normally used only for private comments and is NOT printed on the eQSLs". The card's message is
   still Log QSO's eQSL comments field (QSLMSG).
   (3.-291, the operator: eQSL's web upload "has an option for it to use the FT8 comments", then "Comment when field
   empty") the card's message: Log QSO's eQSL comments field, else the QSO's comment without the grid fill's note - what
   that option does for a file; the real-time interface has no such option, so the upload says it itself.
   (3.-296, the operator: "Let's always move the comment from comments to that other field that appears on the QSL card
   (removing the eventual grid tag), and leave the comment field empty", then "let's do both") the card: Log QSO's eQSL
   comments field, then " - " and the QSO's comment without the fill's note, either alone when the other is empty; COMMENT
   is no longer sent - also not from a record an earlier build queued with it.
   The address and the password are never shown: an answer's text has neither (eqsl::scrubbed).
   (3.-294, the operator: "yes, give eQSL the same queue and retries") a QSO not sent is kept and tried again as Club Log's
   and QRZ.com's are (qsoupload.h): its record in a queue file in the data directory (eqsl_upload.adi, the login never in
   it - read from the Settings at each request), 2, 5, 10, 30, then 60 minutes after failures in a row, at once at the next
   QSO logged, 75 s after the start; a login eQSL refuses (or a QTH nickname no profile has) holds every QSO until OK; a
   record eQSL refuses is dropped with its words. A second copy is harmless: eQSL answers it as a duplicate.
   test_qsoupload has the rules and a stand-in on 127.0.0.1 (JTDX_EQSL_UPLOAD). */

#include <functional>
#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QTimer>
#include "FileDownload.hpp"
#include "Radio.hpp"

class QNetworkAccessManager;

namespace eqsl
{
  constexpr char const * url {"https://www.eqsl.cc/qslcard/importADIF.cfm"};
  constexpr char const * api_env {"JTDX_EQSL_UPLOAD"};   // tests: a stand-in's address in place of eQSL's
  int const card_max {240};   // (3.-293) eQSL keeps so much of the card's message: "Truncated after 240 characters"
  constexpr char const * queue_file {"eqsl_upload.adi"};   // (3.-294) in the data directory, as Club Log's and QRZ.com's queues
  struct Login                                              // (3.-294) as the Settings have it at a request's time
  {
    QString user;
    QString password;
    QString nickname;
  };
  inline bool operator== (Login const& a, Login const& b) {return a.user == b.user && a.password == b.password && a.nickname == b.nickname;}
  inline bool operator!= (Login const& a, Login const& b) {return !(a == b);}
  // (3.-294) a request's header: the login, the program, the QTH nickname when there is one
  QString header (Login const& login);
  QString upload_url ();                                 // the variable's, else url

  struct Answer
  {
    enum Kind {Added, Duplicate, Refused, Failed, Unclear,
               LoginRefused};   // (3.-294) the login (or a QTH nickname no profile has): every QSO kept until OK
    Kind kind;
    QString text;      // eQSL's own line (Result:, Error:, Warning:), or the transport's words; never the address or the password
    int http_status;   // 0: no answer at all
  };

  /* eQSL's answer page, read as ImportADIF.txt (revised 2026-03-01) describes it - "designed to be parsed electronically":
     a status other than 200 is a failure (500 or 503: eQSL down for maintenance); "Warning: ... Bad record: Duplicate"
     is a QSO eQSL has already; an "Error:" line refuses it (one with "down" in it is eQSL down: a failure); another
     "Warning:" line is a record eQSL did not import; "Result: x out of y records added" with x at least 1 is the QSO
     added, with 0 refused; anything else is not understood. Tags are line ends, the usual entities decoded. */
  Answer answer (int http_status, QByteArray const& page);
  // a text for the user: every address in it replaced, and the password, plain or percent-encoded
  QString scrubbed (QString text, QString const& password);
  // the request: base + "?ADIFdata=" + adif percent-encoded whole, nothing left for a server to read otherwise
  QString request_url (QString const& base, QString const& adif);
  // (3.-291) the card's message (QSLMSG): (3.-296) the eQSL comments field, then " - " and the QSO's comment without the
  // fill's note - either alone when the other is empty ("" none); eQSL's 240 characters at most
  QString card_message (QString const& eqslcomments, QString const& comment);
  /* (3.-293, the review) a transport failure in words for the user: never the request's address - it holds the password,
     and decoded for display a password with a space got past the scrub - the transport's own words only when they name
     no address, else what kind of failure it was */
  QString transport_text (FileDownload::Failure failure, QString const& detail);
}

Q_DECLARE_METATYPE (eqsl::Answer)

class EQSL : public QObject
{
  Q_OBJECT;

public:
  explicit EQSL(QNetworkAccessManager *, QObject *parent = nullptr);
    /* (3.-294) the queue's file - the QSOs not sent yet, their records alone, kept across a restart and read here - and
       what the program has at each request: the login (never in the file) and whether the box is ticked. Not configured
       (a test): in memory only, with the login the last upload () was given */
    void configure (QString const& queue_file, std::function<eqsl::Login ()> login, std::function<bool ()> enabled);
    void upload(QString const& eqsl_username, QString const& eqsl_passwd, QString const& eqsl_nickname
                  , QString const& call, QString const& mode
                  , QDateTime const& QSO_date_on
                  , QString const& rpt_sent, QString const& band
                  , QString const& eqslcomments
                  /* CE3TSK 2026-11-13 (TODO.md 3.-247, its review): a satellite QSO's PROP_MODE and SAT_NAME */
                  , QString const& prop_mode = QString {}, QString const& sat_name = QString {}
                  /* (3.-290) the QSO's comment, as logged: (3.-296) on the card after the eQSL message, never as COMMENT */
                  , QString const& comment = QString {});
    /* CE3TSK 2026-11-13: the QSO's part of the upload, <CALL> to <EOR> - apart, so a test can read it (no network) */
    static QString record (QString const& call, QString const& mode, QDateTime const& QSO_date_on
                           , QString const& rpt_sent, QString const& band, QString const& eqslcomments
                           , QString const& prop_mode, QString const& sat_name);
    int waiting () const {return queue_.size ();}   // (3.-284) QSOs not answered yet, the one under way among them; (3.-294) kept ones too
    void send_now ();      // (3.-294) the oldest now, if it may go: a start (75 s after it), OK - a back-off's wait does not hold it
    void resume ();        // (3.-294) a refused login lifted: OK pressed - send_now () then sends what waits
    QDateTime next_try () const {return next_try_;}   // (3.-294) the back-off's next try; invalid: none planned
    bool stopped () const {return stopped_;}           // (3.-294) a refused login holds every QSO until OK
    bool unsaved () const {return unsaved_;}           // (3.-295, the review 7) the queue's file could not be written

    // (3.-284) eQSL's answer for a QSO's call, in the order they were logged; (3.-294) a failure keeps the QSO (waiting ())
    Q_SIGNAL void answered (QString const& call, eqsl::Answer const& answer) const;
    // (3.-294) a QSO just logged and not sent now: refused - held by a refused login - or waiting (no login; another
    // user's QSOs first - 3.-295)
    Q_SIGNAL void kept (bool refused, QString const& call, int waiting) const;

private:
    struct Pending
    {
      QString call;
      QByteArray record;   // <CALL> to <EOR>: no login, no password - the queue's file holds these alone
      /* (3.-295, the review 4) the eQSL user and QTH nickname it was logged under - in the file with it, never the
         password: sent with its own nickname, and held while the Settings name another user (whose password it needs) */
      QString user;
      QString nickname;
    };
    int first_sendable (eqsl::Login const& settings) const;   // (3.-295) the oldest a request may carry now; -1: none
    void next ();
    void finish (eqsl::Answer const& answer);
    void save ();

    QNetworkAccessManager *networkManager;
    FileDownload request_;
    QList<Pending> queue_;   // the first is the one under way while busy_
    bool busy_ {false};
    // (3.-294) the queue's file, and the login and the box as the program has them at each request
    QString queue_file_;
    std::function<eqsl::Login ()> login_;
    std::function<bool ()> enabled_;
    eqsl::Login last_login_;      // not configured: the last upload ()'s
    QString request_password_;    // the request under way's: its answer scrubbed with it
    // (3.-295, the review 2, 3) the request under way's QSO and the Settings' login it was made with; the login a refusal
    // stopped at, and OK since then (the first answer after it may show the QSO at fault)
    int in_flight_ {-1};
    eqsl::Login request_login_;
    eqsl::Login stop_login_;
    bool resumed_ {false};
    bool unsaved_ {false};
    int failures_ {0};            // in a row: the back-off's step
    int answers_ {0};             // answers taken: upload () tells whether one came at once
    bool stopped_ {false};
    QDateTime next_try_;
    QTimer timer_;
};

#endif // EQSL_H
