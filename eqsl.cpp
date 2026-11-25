// This source code file was last time modified by Arvo ES1JA on 20181229
// CE3TSK 2026-11-24 (TODO.md 3.-284): eQSL's answer kept, https through FileDownload, one request at a time - see eqsl.h

#include "eqsl.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <QUrl>

#include "satlog.h"   // CE3TSK 2026-11-13 (TODO.md 3.-247): the satellite's tags
#include "qsoupload.h"   // (3.-294) the queue file and the back-off, as Club Log's and QRZ.com's
#include "gridfill.h"   // (3.-291) the fill's note, left off the card

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
    /* (3.-295, the review 1) a 4xx - but 408 (a timeout) and 429 (too many) - is this request's own: refused, never tried
       again (it would hold every QSO behind it); another status a failure, kept */
    if (200 != http_status)
      return http_status >= 400 && http_status < 500 && 408 != http_status && 429 != http_status
        ? Answer {Answer::Refused, QStringLiteral ("HTTP status %1").arg (http_status), http_status} : Answer {Answer::Failed, QString {}, http_status};
    auto const lines = lines_of (page);
    for (auto const& l : lines)
      if (l.startsWith (QLatin1String {"Warning:"}, Qt::CaseInsensitive) && l.contains (QLatin1String {"Duplicate"}, Qt::CaseInsensitive))
        return Answer {Answer::Duplicate, l, http_status};
    auto const error = first_starting (lines, QStringLiteral ("Error:"));
    if (!error.isEmpty ())
      {
        /* (3.-294) the login refused - "No match on eQSL_User/eQSL_Pswd" (a QTH nickname no profile has gives it too),
           "Missing eQSL_User", "Missing eQSL_Pswd": every QSO kept until OK */
        if (error.contains (QLatin1String {"eQSL_User"}, Qt::CaseInsensitive) || error.contains (QLatin1String {"eQSL_Pswd"}, Qt::CaseInsensitive))
          return Answer {Answer::LoginRefused, error, http_status};
      /* (3.-293, the review) "down" as a word: an Error that says "download" refuses, it is no outage; (3.-295, the review 5)
         and eQSL's other words for one - downtime, shutdown, maintenance, try later: kept, tried again */
      return Answer {QRegularExpression {QStringLiteral ("\\b(down|downtime|shutdown|shut down|maintenance|offline|unavailable)\\b|try (again )?later"), QRegularExpression::CaseInsensitiveOption}.match (error).hasMatch () ? Answer::Failed : Answer::Refused, error, http_status};
      }
    auto const result = first_starting (lines, QStringLiteral ("Result:"));
    auto const m = QRegularExpression {QStringLiteral ("^Result:\\s*(\\d+)\\s+out\\s+of\\s+(\\d+)\\s+records?\\s+added"),
                                       QRegularExpression::CaseInsensitiveOption}.match (result);
    // (3.-293, the review) a record added is added, whatever note a Warning line adds to it (one record an upload)
    if (m.hasMatch () && m.captured (1).toInt () > 0) return Answer {Answer::Added, result, http_status};
    auto const warning = first_starting (lines, QStringLiteral ("Warning:"));
    if (!warning.isEmpty ()) return Answer {Answer::Refused, warning, http_status};
    if (m.hasMatch ()) return Answer {m.captured (1).toInt () > 0 ? Answer::Added : Answer::Refused, result, http_status};
    // (3.-293) the whole first line: finish () cuts it to 120 after the scrub - cut first, a password at the edge kept its beginning
    return Answer {Answer::Unclear, lines.value (0).left (4096), http_status};
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

  QString card_message (QString const& eqslcomments, QString const& comment)
  {
    /* (3.-296, the operator: "always move the comment ... to that other field that appears on the QSL card", then "let's
       do both") Log QSO's eQSL message, then the QSO's comment without the fill's note - either alone when the other is
       empty; the comment goes to eQSL nowhere else */
    QString const message {eqslcomments.trimmed ()};
    QString const said {grid_fill::without_note (comment)};
    QString card {(message.isEmpty () ? said : said.isEmpty () ? message : message + QStringLiteral (" - ") + said).left (card_max)};   // (3.-293) eQSL's 240
    if (card.size () == card_max && card.at (card_max - 1).isHighSurrogate ()) card.chop (1);   // (3.-295, the review 10) a character never split
    return card;
  }

  QString transport_text (FileDownload::Failure failure, QString const& detail)
  {
    switch (failure)
      {
      case FileDownload::Failure::NoSsl: return QStringLiteral ("no https support on this computer");
      case FileDownload::Failure::TooLarge: return QStringLiteral ("eQSL's answer too large to read");
      default: return detail.isEmpty () || detail.contains (QLatin1String {"://"}) ? QStringLiteral ("the connection failed") : detail;
      }
  }

  QString header (Login const& login)
  {
    QString h {"<ADIF_VER:5>2.1.9"};
    h += "<EQSL_USER:" + QString::number (login.user.length ()) + ">" + login.user;
    h += "<EQSL_PSWD:" + QString::number (login.password.length ()) + ">" + login.password;
    h += "<PROGRAMID:4>JTDX<EOH>";
    /* CE3TSK 2026-11-24 (TODO.md 3.-285): the QTH nickname only when there is one - without it eQSL files the QSO by its
       date into the account whose dates cover it (ImportADIF.txt); an empty tag would name no profile at all */
    if (!login.nickname.isEmpty ()) h += "<APP_EQSL_QTH_NICKNAME:" + QString::number (login.nickname.length ()) + ">" + login.nickname;
    return h;
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
      // (3.-293, the review) never the address in the words; a page too large was an answer (no "no connection")
      finish (eqsl::Answer {eqsl::Answer::Failed, eqsl::transport_text (failure, detail), FileDownload::Failure::TooLarge == failure ? 200 : 0});
    });
  // (3.-294) the back-off's next try, and a run's next request 2 s after an answer
  timer_.setSingleShot (true);
  connect (&timer_, &QTimer::timeout, this, &EQSL::next);
}

void EQSL::upload(QString const& eqsl_username, QString const& eqsl_passwd, QString const& eqsl_nickname
                  , QString const& call, QString const& mode
                  , QDateTime const& QSO_date_on
                  , QString const& rpt_sent, QString const& band
                  , QString const& eqslcomments
                  , QString const& prop_mode, QString const& sat_name, QString const& comment)
{
    /* (3.-294) the login is read at each request (configure ()) - never kept in the queue or its file; the one given here
       serves only an EQSL that was not configured (a test) */
    last_login_ = eqsl::Login {eqsl_username, eqsl_passwd, eqsl_nickname};
    // (3.-291) the card's message: (3.-296) the eQSL comments field, then the QSO's comment without the fill's note - no COMMENT
    QString const rec {record (call, mode, QSO_date_on, rpt_sent, band, eqsl::card_message (eqslcomments, comment), prop_mode, sat_name)};

    // (3.-284) queued behind one under way - the inherited code cut that one off; (3.-294) and in its file until eQSL has it;
    // (3.-295) with the user and QTH nickname the Settings have as it is logged
    eqsl::Login const as {login_ ? login_ () : last_login_};
    queue_ << Pending {call, rec.toUtf8 (), as.user, as.nickname};
    save ();
    bool const busy {busy_};
    int const answers {answers_};
    send_now ();   // at once - also while a failure's back-off runs, as Club Log's and QRZ.com's
    // (3.-294) not sent now - a refused login, no login, another user's QSOs only - and not answered at once: told, as the others'
    if (!busy && !busy_ && answers == answers_) Q_EMIT kept (stopped_, call, queue_.size ());
}

void EQSL::configure (QString const& queue_file, std::function<eqsl::Login ()> login, std::function<bool ()> enabled)
{
  queue_file_ = queue_file;
  login_ = std::move (login);
  enabled_ = std::move (enabled);
  // (3.-294) what an earlier run kept: sent 75 s after the start (MainWindow), as Club Log's and QRZ.com's queues
  for (auto const& r : qso_upload::read_queue (queue_file_))
    {
      auto const at = r.toUpper ().indexOf ("<CALL:");   // (3.-295) the user and nickname before it, the record from it
      if (at < 0) continue;
      // (3.-296) a record an earlier build queued with the QSO's comment as COMMENT: sent without it
      queue_ << Pending {qso_upload::field (r, QStringLiteral ("CALL")), qso_upload::without_field (r.mid (at), QStringLiteral ("COMMENT")),
                         qso_upload::field (r, QStringLiteral ("APP_JTDX_EQSL_USER")), qso_upload::field (r, QStringLiteral ("APP_JTDX_EQSL_QTH_NICKNAME"))};
    }
}

void EQSL::save ()
{
  if (queue_file_.isEmpty ()) return;
  QList<QByteArray> records;
  // (3.-295, the review 4) each line: the user and QTH nickname it was logged under - never the password - then the record
  for (auto const& p : queue_)
    records << (sat_log::tag ("APP_JTDX_EQSL_USER", p.user) + sat_log::tag ("APP_JTDX_EQSL_QTH_NICKNAME", p.nickname)).toUtf8 () + p.record;
  // (3.-295, the review 7, 12) the file named eQSL's; one that could not be written said on eQSL's line
  unsaved_ = !qso_upload::write_queue (queue_file_, records, QStringLiteral ("eQSL"));
}

int EQSL::first_sendable (eqsl::Login const& settings) const
{
  for (int i = 0; i < queue_.size (); ++i)
    if (queue_[i].user.isEmpty () || 0 == queue_[i].user.compare (settings.user, Qt::CaseInsensitive)) return i;
  return -1;
}

void EQSL::send_now ()
{
  timer_.stop ();
  next ();
}

void EQSL::resume ()
{
  /* (3.-295, the review 2, 4) after a refused login: the first answer may show the QSO at fault (refused again, the login
     unchanged - its date), and the QSOs it held - logged under the refused user - take the login now set: the operator
     corrected it for them */
  if (stopped_)
    {
      resumed_ = true;
      eqsl::Login const l {login_ ? login_ () : last_login_};
      for (auto& p : queue_)
        if (0 == p.user.compare (stop_login_.user, Qt::CaseInsensitive)) {p.user = l.user; p.nickname = l.nickname;}
      save ();
    }
  stopped_ = false;   // (3.-294) OK pressed: the login may be another now
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
    /* CE3TSK 2026-11-25 (TODO.md 3.-296): no COMMENT any more (3.-290 sent the QSO's comment so) - the comment is on the
       card, after the eQSL message, and the length above counted in characters as the log file's records count theirs */
//    myadif+="<QSLMSG:19>TNX For QSO TU 73!.";
    myadif+="<EOR>";
    return myadif;
}

void EQSL::next ()
{
  if (busy_ || queue_.isEmpty () || stopped_ || (enabled_ && !enabled_ ())) return;
  // (3.-294) the login as the Settings have it now; none: kept, nothing sent
  eqsl::Login const l {login_ ? login_ () : last_login_};
  if (l.user.isEmpty () || l.password.isEmpty ()) return;
  // (3.-295, the review 4) the oldest QSO logged under this user, each with its own QTH nickname; another user's wait
  int const i {first_sendable (l)};
  if (i < 0) return;
  in_flight_ = i;
  request_login_ = l;
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
  request_password_ = l.password;
  eqsl::Login const as {l.user, l.password, queue_[i].user.isEmpty () ? l.nickname : queue_[i].nickname};
  request_.configure (networkManager, eqsl::request_url (eqsl::upload_url (), eqsl::header (as) + QString::fromUtf8 (queue_[i].record)), QString {},
                      "JTDX_contest/" + QCoreApplication::applicationVersion () + " (https://github.com/ce3tsk/jtdx_contest)",
                      FileDownload::Validator {}, r);
  busy_ = true;
  request_.start_download ();       // may answer at once (no SSL): finish () runs before this returns
}

void EQSL::finish (eqsl::Answer const& answer)
{
  if (!busy_ || in_flight_ < 0 || in_flight_ >= queue_.size ()) return;
  busy_ = false;
  ++answers_;
  int const i {in_flight_};
  in_flight_ = -1;
  bool const after_ok {resumed_};
  resumed_ = false;
  eqsl::Answer a {answer};
  a.text = eqsl::scrubbed (a.text, request_password_);
  if (eqsl::Answer::Unclear == a.kind) a.text = a.text.left (120);   // (3.-293) after the scrub, never before it
  QString const call {queue_[i].call};
  auto const now = QDateTime::currentDateTimeUtc ();
  eqsl::Login const settings {login_ ? login_ () : last_login_};
  if (eqsl::Answer::LoginRefused == a.kind)
    {
      // (3.-295, the review 3) the refusal of a login changed since the request was made: not this one - asked again
      if (request_login_ != settings) {timer_.start (qso_upload::gap_ms); return;}
      // (3.-295, the review 2) refused again at once after OK, the login unchanged: this QSO's own (its date) - dropped
      if (after_ok && settings == stop_login_) a.kind = eqsl::Answer::Refused;
    }
  switch (a.kind)
    {
    case eqsl::Answer::Failed: case eqsl::Answer::Unclear:   // (3.-294) kept, tried again: 2, 5, 10, 30, then 60 minutes - or at the next QSO
      ++failures_;
      next_try_ = now.addSecs (qso_upload::retry_minutes (failures_) * 60);
      timer_.start (static_cast<int> (qMax<qint64> (0, now.msecsTo (next_try_))));
      Q_EMIT answered (call, a);
      return;
    case eqsl::Answer::LoginRefused:   // (3.-294) kept: nothing more until OK (resume ())
      stopped_ = true;
      stop_login_ = settings;
      Q_EMIT answered (call, a);
      return;
    default:   // added, in the log already, refused: eQSL has had the last word on it
      queue_.removeAt (i);
      save ();
      failures_ = 0;
      next_try_ = QDateTime {};
      Q_EMIT answered (call, a);
      if (!queue_.isEmpty ()) timer_.start (qso_upload::gap_ms);   // the rest of a run, 2 s apart - never a burst
    }
}
