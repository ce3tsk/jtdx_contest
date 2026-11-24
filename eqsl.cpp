// This source code file was last time modified by Arvo ES1JA on 20181229
// CE3TSK 2026-11-24 (TODO.md 3.-284): eQSL's answer kept, https through FileDownload, one request at a time - see eqsl.h

#include "eqsl.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <QUrl>

#include "satlog.h"   // CE3TSK 2026-11-13 (TODO.md 3.-247): the satellite's tags

#include "moc_eqsl.cpp"

namespace eqsl
{
  QString upload_url ()
  {
    QByteArray const set {qgetenv (api_env)};
    return set.isEmpty () ? QString {url} : QString::fromLatin1 (set);
  }

  namespace
  {
    // the page's text, a line a tag or a line end: what eQSL puts between <BR>s, without the HTML around it
    QStringList lines_of (QByteArray const& page)
    {
      QString t {QString::fromUtf8 (page)};
      t.replace (QRegularExpression {QStringLiteral ("<[^>]*>")}, QStringLiteral ("\n"));
      t.replace (QLatin1String {"&nbsp;"}, QLatin1String {" "});
      t.replace (QLatin1String {"&lt;"}, QLatin1String {"<"});
      t.replace (QLatin1String {"&gt;"}, QLatin1String {">"});
      t.replace (QLatin1String {"&quot;"}, QLatin1String {"\""});
      t.replace (QLatin1String {"&#39;"}, QLatin1String {"'"});
      t.replace (QLatin1String {"&amp;"}, QLatin1String {"&"});
      QStringList lines;
      for (auto const& l : t.split ('\n'))
        {
          auto const s = l.simplified ();
          if (!s.isEmpty ()) lines << s;
        }
      return lines;
    }

    QString first_starting (QStringList const& lines, QString const& start)
    {
      for (auto const& l : lines) if (l.startsWith (start, Qt::CaseInsensitive)) return l;
      return {};
    }
  }

  Answer answer (int http_status, QByteArray const& page)
  {
    if (200 != http_status) return Answer {Answer::Failed, QString {}, http_status};
    auto const lines = lines_of (page);
    for (auto const& l : lines)
      if (l.startsWith (QLatin1String {"Warning:"}, Qt::CaseInsensitive) && l.contains (QLatin1String {"Duplicate"}, Qt::CaseInsensitive))
        return Answer {Answer::Duplicate, l, http_status};
    auto const error = first_starting (lines, QStringLiteral ("Error:"));
    if (!error.isEmpty ())
      return Answer {error.contains (QLatin1String {"down"}, Qt::CaseInsensitive) ? Answer::Failed : Answer::Refused, error, http_status};
    auto const warning = first_starting (lines, QStringLiteral ("Warning:"));
    if (!warning.isEmpty ()) return Answer {Answer::Refused, warning, http_status};
    auto const result = first_starting (lines, QStringLiteral ("Result:"));
    auto const m = QRegularExpression {QStringLiteral ("^Result:\\s*(\\d+)\\s+out\\s+of\\s+(\\d+)\\s+records?\\s+added"),
                                       QRegularExpression::CaseInsensitiveOption}.match (result);
    if (m.hasMatch ()) return Answer {m.captured (1).toInt () > 0 ? Answer::Added : Answer::Refused, result, http_status};
    return Answer {Answer::Unclear, lines.value (0).left (120), http_status};
  }

  QString scrubbed (QString text, QString const& password)
  {
    text.replace (QRegularExpression {QStringLiteral ("[A-Za-z][A-Za-z0-9+.-]*://\\S*")}, QStringLiteral ("eQSL"));
    if (!password.isEmpty ())
      {
        text.replace (password, QStringLiteral ("***"));
        text.replace (QString::fromLatin1 (QUrl::toPercentEncoding (password)), QStringLiteral ("***"));
      }
    return text;
  }

  QString request_url (QString const& base, QString const& adif)
  {
    return base + QStringLiteral ("?ADIFdata=") + QString::fromLatin1 (QUrl::toPercentEncoding (adif));
  }
}

EQSL::EQSL(QNetworkAccessManager * manager, QObject *parent)
  : QObject{parent}
  , networkManager {manager}
{
  qRegisterMetaType<eqsl::Answer> ("eqsl::Answer");
  // (3.-284) this request's own answer alone - never the shared manager's finished ()
  connect (&request_, &FileDownload::replied, this, [this] (int status, QByteArray const& page) {finish (eqsl::answer (status, page));});
  connect (&request_, &FileDownload::error, this, [this] (FileDownload::Failure failure, QString const& detail) {
      finish (eqsl::Answer {eqsl::Answer::Failed, FileDownload::Failure::TooLarge == failure ? QStringLiteral ("an answer too large") : detail, 0});
    });
}

void EQSL::upload(QString const& eqsl_username, QString const& eqsl_passwd, QString const& eqsl_nickname
                  , QString const& call, QString const& mode
                  , QDateTime const& QSO_date_on
                  , QString const& rpt_sent, QString const& band
                  , QString const& eqslcomments
                  , QString const& prop_mode, QString const& sat_name)
{
    QString myadif="<ADIF_VER:5>2.1.9";
    myadif+="<EQSL_USER:" + QString::number(eqsl_username.length()) + ">" + eqsl_username;
    myadif+="<EQSL_PSWD:" + QString::number(eqsl_passwd.length()) + ">" + eqsl_passwd;
    myadif+="<PROGRAMID:4>JTDX<EOH>";
    /* CE3TSK 2026-11-24 (TODO.md 3.-285): the QTH nickname only when there is one - without it eQSL files the QSO by its
       date into the account whose dates cover it (ImportADIF.txt); an empty tag would name no profile at all */
    if (!eqsl_nickname.isEmpty ()) myadif+="<APP_EQSL_QTH_NICKNAME:" + QString::number(eqsl_nickname.length()) + ">" + eqsl_nickname;
    myadif+=record (call, mode, QSO_date_on, rpt_sent, band, eqslcomments, prop_mode, sat_name);

    // (3.-284) queued behind one under way - the inherited code cut that one off
    queue_ << Pending {call, eqsl::request_url (eqsl::upload_url (), myadif), eqsl_passwd};
    next ();
}

QString EQSL::record (QString const& call, QString const& mode, QDateTime const& QSO_date_on
                      , QString const& rpt_sent, QString const& band, QString const& eqslcomments
                      , QString const& prop_mode, QString const& sat_name)
{
    QString myadif;
    myadif+="<CALL:" + QString::number(call.length()) + ">" + call;
    myadif+="<MODE:"  + QString::number(mode.length()) + ">" + mode;
    myadif+="<QSO_DATE:8>" + QSO_date_on.date().toString("yyyyMMdd");
    myadif+="<TIME_ON:4>" + QSO_date_on.time().toString("hhmm");
    myadif+="<RST_SENT:" + QString::number(rpt_sent.length()) + ">" + rpt_sent;
    myadif+="<BAND:" + QString::number(band.length()) + ">" + band;
    /* CE3TSK 2026-11-13 (TODO.md 3.-247, its review): a satellite QSO as the log has it - eQSL's card names the satellite */
    myadif+=sat_log::tag ("PROP_MODE", prop_mode) + sat_log::tag ("SAT_NAME", sat_name);
    if(eqslcomments!="") myadif+="<QSLMSG:" + QString::number(eqslcomments.length()) + ">" + eqslcomments;
//    myadif+="<QSLMSG:19>TNX For QSO TU 73!.";
    myadif+="<EOR>";
    return myadif;
}

void EQSL::next ()
{
  if (busy_ || queue_.isEmpty ()) return;
  if (!networkManager)
    {
      busy_ = true;
      finish (eqsl::Answer {eqsl::Answer::Failed, QStringLiteral ("no network"), 0});
      return;
    }
  FileDownload::Request r;
  r.reply_only = true;              // the answer page, whatever its status, nothing written
  r.max_body_size = 256 * 1024;     // a page of a few lines
  r.timeout_ms = 30000;             // with no byte moving
  request_.configure (networkManager, queue_.first ().url, QString {},
                      "JTDX_contest/" + QCoreApplication::applicationVersion () + " (https://github.com/ce3tsk/jtdx_contest)",
                      FileDownload::Validator {}, r);
  busy_ = true;
  request_.start_download ();       // may answer at once (no SSL): finish () runs before this returns
}

void EQSL::finish (eqsl::Answer const& answer)
{
  if (!busy_ || queue_.isEmpty ()) return;
  Pending const p {queue_.takeFirst ()};
  busy_ = false;
  eqsl::Answer a {answer};
  a.text = eqsl::scrubbed (a.text, p.password);
  Q_EMIT answered (p.call, a);
  next ();
}
