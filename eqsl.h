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
   The address and the password are never shown: an answer's text has neither (eqsl::scrubbed). No retry, no queue
   file: a QSO not sent is named, for the operator to upload on eQSL's website. test_qsoupload has the rules and a
   stand-in on 127.0.0.1 (JTDX_EQSL_UPLOAD). */

#include <QList>
#include <QMetaType>
#include <QObject>
#include <QString>
#include "FileDownload.hpp"
#include "Radio.hpp"

class QNetworkAccessManager;

namespace eqsl
{
  constexpr char const * url {"https://www.eqsl.cc/qslcard/importADIF.cfm"};
  constexpr char const * api_env {"JTDX_EQSL_UPLOAD"};   // tests: a stand-in's address in place of eQSL's
  QString upload_url ();                                 // the variable's, else url

  struct Answer
  {
    enum Kind {Added, Duplicate, Refused, Failed, Unclear};
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
}

Q_DECLARE_METATYPE (eqsl::Answer)

class EQSL : public QObject
{
  Q_OBJECT;

public:
  explicit EQSL(QNetworkAccessManager *, QObject *parent = nullptr);
    void upload(QString const& eqsl_username, QString const& eqsl_passwd, QString const& eqsl_nickname
                  , QString const& call, QString const& mode
                  , QDateTime const& QSO_date_on
                  , QString const& rpt_sent, QString const& band
                  , QString const& eqslcomments
                  /* CE3TSK 2026-11-13 (TODO.md 3.-247, its review): a satellite QSO's PROP_MODE and SAT_NAME */
                  , QString const& prop_mode = QString {}, QString const& sat_name = QString {});
    /* CE3TSK 2026-11-13: the QSO's part of the upload, <CALL> to <EOR> - apart, so a test can read it (no network) */
    static QString record (QString const& call, QString const& mode, QDateTime const& QSO_date_on
                           , QString const& rpt_sent, QString const& band, QString const& eqslcomments
                           , QString const& prop_mode, QString const& sat_name);
    int waiting () const {return queue_.size ();}   // (3.-284) QSOs not answered yet, the one under way among them

    // (3.-284) eQSL's answer for a QSO's call, each QSO's once, in the order they were logged
    Q_SIGNAL void answered (QString const& call, eqsl::Answer const& answer) const;

private:
    struct Pending
    {
      QString call;
      QString url;        // holds the password: never shown, never logged
      QString password;   // to scrub the answer's text with
    };
    void next ();
    void finish (eqsl::Answer const& answer);

    QNetworkAccessManager *networkManager;
    FileDownload request_;
    QList<Pending> queue_;   // the first is the one under way while busy_
    bool busy_ {false};
};

#endif // EQSL_H
