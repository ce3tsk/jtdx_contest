#include "qsoupload.h"
// CE3TSK 2026-11-23 (TODO.md 3.-282, QSO_UPLOAD_PLAN.md): the logged QSOs to Club Log and QRZ.com - qsoupload.h has the rules

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSaveFile>
#include <QUrl>
#include "clublog.h"
#include "qrzlogbook.h"

#include "moc_qsoupload.cpp"

namespace qso_upload
{
  QString site_name (Site site)
  {
    return Site::ClubLog == site ? QStringLiteral ("Club Log") : QStringLiteral ("QRZ.com");
  }

  QString field (QByteArray const& record, QString const& name)
  {
    QString const text {QString::fromUtf8 (record)};
    int at {0};
    while ((at = text.indexOf ('<', at)) >= 0)
      {
        int const close {text.indexOf ('>', at)};
        if (close < 0) break;
        auto const parts = text.mid (at + 1, close - at - 1).split (':');
        at = close + 1;
        if (parts.size () < 2) continue;   // <EOR>, <EOH>
        bool ok {false};
        int const length {parts[1].trimmed ().toInt (&ok)};
        if (!ok || length < 0) continue;
        if (0 == parts[0].trimmed ().compare (name, Qt::CaseInsensitive)) return text.mid (at, length).trimmed ();
        at += length;   // past the value: it may hold a '<'
      }
    return {};
  }

  QByteArray one_line (QByteArray const& record)
  {
    QByteArray line {record};
    line.replace ('\r', ' ').replace ('\n', ' ');   // one for one: the ADIF lengths stay right
    return line;
  }

  QString clublog_url (QString const& base, int records)
  {
    if (base.isEmpty () || records < 1) return {};
    return base + (records > 1 ? QStringLiteral ("/qso/batch") : QStringLiteral ("/qso"));
  }

  QByteArray clublog_body (QList<QByteArray> const& records, QString const& own_call)
  {
    auto item = [&own_call] (QByteArray const& record) {
      auto call = field (record, QStringLiteral ("STATION_CALLSIGN"));
      if (call.isEmpty ()) call = own_call;
      return QJsonObject {{"call", call.trimmed ().toUpper ()}, {"adif", QString::fromUtf8 (record)}};
    };
    if (1 == records.size ()) return QJsonDocument {item (records.first ())}.toJson (QJsonDocument::Compact);
    QJsonArray qsos;
    for (auto const& record : records) qsos.append (item (record));
    return QJsonDocument {QJsonObject {{"qsos", qsos}}}.toJson (QJsonDocument::Compact);
  }

  QByteArray qrz_body (QString const& key, QByteArray const& record)
  {
    return "KEY=" + QUrl::toPercentEncoding (key) + "&ACTION=INSERT&ADIF=" + QUrl::toPercentEncoding (QString::fromUtf8 (record));
  }

  namespace
  {
    Result made (Result::Kind kind, int http_status, QString const& reason, Result::Scope scope = Result::Request)
    {
      Result r;
      r.kind = kind;
      r.scope = scope;
      r.http_status = http_status;
      r.reason = reason;
      return r;
    }

    QString status_text (int http_status)
    {
      return http_status ? QString {"HTTP %1"}.arg (http_status) : QStringLiteral ("no answer");
    }

    // one QSO's part of a Club Log answer: the object of POST /qso, or one entry of a batch's results
    Result clublog_item (QJsonObject const& o, int http_status)
    {
      auto const state = o.value ("status").toString ().trimmed ().toLower ();
      auto const message = o.value ("message").toString ().trimmed ();
      Result r {made (Result::Sent, http_status, o.value ("warning").toString ().trimmed ())};
      auto const id = o.value ("qsoid");
      if (id.isDouble ()) r.id = QString::number (static_cast<qint64> (id.toDouble ()));
      else if (id.isString ()) r.id = id.toString ();
      r.test = o.value ("test_mode").toBool ();
      if ("duplicate" == state) r.kind = Result::Duplicate;
      else if ("rejected" == state || (message.contains ("dropped", Qt::CaseInsensitive) && "created" != state))
        {
          // "If the message says the QSO was dropped, it was not kept"
          r.kind = Result::Rejected;
          r.reason = message.isEmpty () ? state : message;
        }
      else if ("updated" == state && r.reason.isEmpty ()) r.reason = message;
      /* (review, 2026-11-23) created and updated alone are the site's "I have it"; no status (an answer that is not Club
         Log's JSON) or a word not known here is not understood: kept and asked again - a second copy comes back a
         duplicate, while a QSO taken for sent and not kept would be lost */
      else if ("created" != state && "updated" != state)
        {
          r = made (Result::Retry, http_status, state.isEmpty () ? QStringLiteral ("no status in the answer")
                                                                 : QStringLiteral ("the status \"%1\"").arg (state));
          r.test = o.value ("test_mode").toBool ();
        }
      return r;
    }
  }

  QList<Result> clublog_results (int http_status, QByteArray const& body, int records)
  {
    QList<Result> out;
    auto all = [&out, records] (Result const& r) {
      for (int i = 0; i < records; ++i) out << r;
      return out;
    };
    auto const o = QJsonDocument::fromJson (body).object ();
    auto message = o.value ("error").toObject ().value ("message").toString ().trimmed ();
    if (message.isEmpty ()) message = status_text (http_status);
    if (200 == http_status || 201 == http_status)
      {
        if (records < 2) return {clublog_item (o, http_status)};
        auto const results = o.value ("results").toArray ();
        for (int i = 0; i < records; ++i)
          out << (i < results.size () ? clublog_item (results[i].toObject (), http_status)
                                      : made (Result::Retry, http_status, QStringLiteral ("no result for this QSO in the answer")));
        return out;
      }
    switch (http_status)
      {
      case 401: return all (made (Result::Refused, http_status, message, Result::Token));
      case 403:                                                              // not on the token's account
      case 404: return all (made (Result::Refused, http_status, message, Result::Call));   // never registered
      case 422:   // this QSO refused - "Do not send the same body again"; a batch refused as a whole goes one by one
        return records < 2 ? all (made (Result::Rejected, http_status, message)) : all (made (Result::OneByOne, http_status, message));
      case 429: return all (made (Result::Wait, http_status, message));
      default: break;
      }
    if (http_status >= 400 && http_status < 500) return all (made (Result::Refused, http_status, message, Result::Request));
    return all (made (Result::Retry, http_status, message));   // 5xx: "Do not immediately repeat a write"; none; a redirect
  }

  Result qrz_result (int http_status, QByteArray const& body)
  {
    if (401 == http_status || 403 == http_status) return made (Result::Refused, http_status, status_text (http_status), Result::Token);
    if (429 == http_status || 503 == http_status) return made (Result::Wait, http_status, status_text (http_status));
    if (http_status >= 400 && http_status < 500) return made (Result::Refused, http_status, status_text (http_status), Result::Request);
    if (200 != http_status) return made (Result::Retry, http_status, status_text (http_status));
    // NAME=value pairs joined by '&'; a part that does not begin with a NAME= belongs to the value before it
    QList<QPair<QString, QString>> pairs;
    for (auto const& part : QString::fromUtf8 (body).trimmed ().split ('&'))
      {
        int const eq {part.indexOf ('=')};
        QString const name {eq > 0 ? part.left (eq) : QString {}};
        bool const named {!name.isEmpty () && name == name.toUpper () && !name.contains (' ')};
        if (named) pairs << qMakePair (name, QUrl::fromPercentEncoding (part.mid (eq + 1).toUtf8 ()));
        else if (!pairs.isEmpty ()) pairs.last ().second += '&' + QUrl::fromPercentEncoding (part.toUtf8 ());
      }
    auto value = [&pairs] (QString const& name) {
      for (auto const& p : pairs) if (p.first == name) return p.second.trimmed ();
      return QString {};
    };
    auto const result = value ("RESULT").toUpper ();
    auto const reason = value ("REASON");
    if ("OK" == result || "REPLACE" == result)
      {
        Result r {made (Result::Sent, http_status, QString {})};
        r.id = value ("LOGID");
        if (r.id.isEmpty ()) r.id = value ("LOGIDS");
        return r;
      }
    if ("FAIL" == result)
      return reason.contains ("duplicate", Qt::CaseInsensitive) ? made (Result::Duplicate, http_status, reason)
                                                                : made (Result::Rejected, http_status, reason.isEmpty () ? QStringLiteral ("no reason given") : reason);
    if ("AUTH" == result) return made (Result::Refused, http_status, reason.isEmpty () ? QStringLiteral ("RESULT=AUTH") : reason, Result::Token);
    return made (Result::Retry, http_status, QStringLiteral ("the answer was not QRZ.com's"));
  }

  QList<QByteArray> read_queue (QString const& path)
  {
    QFile f {path};
    if (!f.open (QIODevice::ReadOnly)) return {};
    auto const lines = f.readAll ().split ('\n');
    int first {0};
    for (int i = 0; i < lines.size (); ++i)
      if (0 == lines[i].trimmed ().compare ("<EOH>", Qt::CaseInsensitive)) {first = i + 1; break;}
    QList<QByteArray> records;
    for (int i = first; i < lines.size (); ++i)
      {
        auto const line = lines[i].trimmed ();
        if (line.toUpper ().contains ("<EOR>")) records << line;
      }
    return records;
  }

  bool write_queue (QString const& path, QList<QByteArray> const& records, Site site)
  {
    if (records.isEmpty ()) return !QFile::exists (path) || QFile::remove (path);
    QByteArray text {"JTDX: QSOs logged and not sent to " + site_name (site).toUtf8 ()
                     + " yet - one record a line; each is taken out once the site has it (QSO_UPLOAD_PLAN.md)\n<EOH>\n"};
    for (auto const& record : records) text += one_line (record) + '\n';
    QSaveFile f {path};
    return f.open (QIODevice::WriteOnly) && f.write (text) == text.size () && f.commit ();
  }

  int retry_minutes (int failures)
  {
    static int const steps[] {2, 5, 10, 30, 60};
    return steps[qBound (0, failures - 1, 4)];
  }

  QDateTime wait_until (QDateTime const& retry_after, QDateTime const& now)
  {
    if (!retry_after.isValid ()) return now.addSecs (5 * 60);
    auto const soonest = now.addSecs (60), latest = now.addSecs (max_wait_hours * 3600);
    return retry_after < soonest ? soonest : retry_after > latest ? latest : retry_after;
  }

  Uploader::Uploader (Site site, QObject * parent)
    : QObject {parent}
    , site_ {site}
  {
    timer_.setSingleShot (true);
    connect (&timer_, &QTimer::timeout, this, &Uploader::send_now);
    connect (&request_, &FileDownload::replied, this, &Uploader::answered);
    connect (&request_, &FileDownload::error, this, &Uploader::failed);
  }

  void Uploader::configure (QNetworkAccessManager * manager, QString const& queue_file, QString const& user_agent)
  {
    manager_ = manager;
    queue_file_ = queue_file;
    user_agent_ = user_agent;
    queue_ = read_queue (queue_file_);   // what an earlier run could not send
  }

  void Uploader::set_credential (QString const& credential)
  {
    credential_ = credential.trimmed ();
  }

  void Uploader::set_enabled (bool on)
  {
    enabled_ = on;
  }

  void Uploader::set_refused_calls (QStringList const& calls)
  {
    refused_calls_.clear ();
    for (auto const& call : calls) refused_calls_ << call.trimmed ().toUpper ();
  }

  void Uploader::token_good ()
  {
    if (stopped_ && Result::Token == stop_scope_) stopped_ = false;
  }

  QString Uploader::base () const
  {
    return Site::ClubLog == site_ ? club_log::api_base () : qrz_logbook::api_base ();
  }

  QString Uploader::station_of (QByteArray const& record) const
  {
    auto call = field (record, QStringLiteral ("STATION_CALLSIGN"));
    if (call.isEmpty ()) call = own_call_;
    return call.trimmed ().toUpper ();
  }

  QStringList Uploader::station_calls () const
  {
    QStringList calls;
    for (auto const& record : queue_)
      {
        auto const call = station_of (record);
        if (!calls.contains (call)) calls << call;
      }
    return calls;
  }

  int Uploader::first_sendable () const
  {
    for (int i = 0; i < queue_.size (); ++i)
      if (!refused_calls_.contains (station_of (queue_[i]))) return i;
    return -1;
  }

  bool Uploader::may_send () const
  {
    return enabled_ && !blocked_ && !stopped_ && !credential_.isEmpty () && manager_ && !base ().isEmpty () && 0 == in_flight_;
  }

  void Uploader::add (QByteArray const& record)
  {
    if (!enabled_) return;
    auto const line = one_line (record).trimmed ();
    if (line.isEmpty ()) return;
    queue_ << line;
    save ();
    bool const busy {in_flight_ > 0};
    int const answers {answers_};
    send_now ();   // at once - also while a failure's back-off runs (U1); never before a Retry-After
    /* (review) one not sent now told, for the main window's note - unless an answer came at once (it was told then) or
       a request is under way (it goes with the next) */
    if (answers != answers_) return;
    auto const call = field (line, QStringLiteral ("CALL"));
    if (blocked_ || stopped_ || refused_calls_.contains (station_of (line))) Q_EMIT kept (true, call, queue_.size ());
    else if (!busy && 0 == in_flight_) Q_EMIT kept (false, call, queue_.size ());
  }

  void Uploader::send_now ()
  {
    if (!may_send ()) return;
    int const first {first_sendable ()};
    if (first < 0) return;   // nothing waits, or only QSOs set aside
    auto const now = QDateTime::currentDateTimeUtc ();
    if (not_before_.isValid () && now < not_before_)
      {
        next_try_ = not_before_;
        timer_.start (static_cast<int> (qMax<qint64> (0, now.msecsTo (not_before_))));
        return;
      }
    timer_.stop ();
    /* (review) the oldest QSO that may go and, for a Club Log batch, the next ones of the same station call - a request
       is one call's, so that a refusal of a call is that call's (Club Log keeps a log a call) */
    int const most {Site::ClubLog == site_ && !one_by_one_ ? clublog_batch_max : 1};
    auto const station = station_of (queue_[first]);
    QList<int> at;
    QList<QByteArray> records;
    for (int i = first; i < queue_.size () && at.size () < most; ++i)
      if (station_of (queue_[i]) == station) {at << i; records << queue_[i];}
    int const n {records.size ()};
    FileDownload::Request request;
    request.method = "POST";
    request.reply_only = true;
    request.max_body_size = max_reply_size;
    request.timeout_ms = timeout_ms;
    QString url;
    if (Site::ClubLog == site_)
      {
        url = clublog_url (base (), n);
        request.headers = {{"Authorization", "Bearer " + credential_.toLatin1 ()}, {"Content-Type", "application/json"}};
        request.body = clublog_body (records, own_call_);
      }
    else
      {
        url = base ();
        // a header of its own: FileDownload then follows no redirect, and the key reaches only the host it is for
        request.headers = {{"Content-Type", "application/x-www-form-urlencoded"}};
        request.body = qrz_body (credential_, records.first ());
      }
    in_flight_ = n;
    in_flight_at_ = at;
    request_credential_ = credential_;
    next_try_ = QDateTime {};
    request_.configure (manager_, url, QString {}, user_agent_, FileDownload::Validator {}, request);
    request_.start_download ();
  }

  void Uploader::answered (int http_status, QByteArray const& body)
  {
    int const n {in_flight_};
    in_flight_ = 0;
    if (n < 1) return;
    ++answers_;
    apply (Site::ClubLog == site_ ? clublog_results (http_status, body, n) : QList<Result> {qrz_result (http_status, body)});
  }

  void Uploader::failed (FileDownload::Failure failure, QString const& detail)
  {
    int const n {in_flight_};
    in_flight_ = 0;
    if (n < 1) return;
    ++answers_;
    QString why;
    switch (failure)
      {
      case FileDownload::Failure::NoSsl: why = QStringLiteral ("SSL/TLS support is not installed"); break;
      case FileDownload::Failure::TooLarge: why = QStringLiteral ("the answer was too large"); break;
      default: why = detail; break;   // the network's own words (never the token: it is in a header)
      }
    Result r {made (Result::Retry, 0, why)};
    QList<Result> results;
    for (int i = 0; i < n; ++i) results << r;
    apply (results);
  }

  void Uploader::apply (QList<Result> const& results)
  {
    /* the records ended (Sent, Duplicate, Rejected) leave the queue; the first other answer says what comes next. (review)
       The request's records found where they were in the queue - not always its head now that a request is one call's;
       the queue only grows while a request is under way */
    QList<int> const at {in_flight_at_};
    in_flight_at_.clear ();
    QList<int> gone;
    QList<QPair<Result, QString>> ended;
    Result next;
    QByteArray next_record;
    bool held {false};
    int const n {qMin (results.size (), at.size ())};
    for (int k = 0; k < n; ++k)
      {
        int const i {at[k]};
        if (i < 0 || i >= queue_.size ()) continue;
        auto const& r = results[k];
        if (Result::Sent == r.kind || Result::Duplicate == r.kind || Result::Rejected == r.kind)
          {
            ended << qMakePair (r, field (queue_[i], QStringLiteral ("CALL")));
            gone << i;
          }
        else if (!held) {next = r; next_record = queue_[i]; held = true;}
      }
    QList<QByteArray> kept;
    for (int i = 0; i < queue_.size (); ++i) if (!gone.contains (i)) kept << queue_[i];
    queue_ = kept;
    save ();
    for (auto const& e : ended) Q_EMIT done (e.first, e.second, queue_.size ());
    auto const now = QDateTime::currentDateTimeUtc ();
    if (!held)
      {
        failures_ = 0;
        if (first_sendable () >= 0) {next_try_ = now.addMSecs (gap_ms); timer_.start (gap_ms);}   // the rest of a run
        return;
      }
    auto const call = field (next_record, QStringLiteral ("CALL"));
    switch (next.kind)
      {
      case Result::Refused:
        {
          /* (review, 2026-11-23) asked with a token or key since replaced (OK pressed while the request was under way): the
             refusal is the old one's, not the new one's - the QSOs kept and asked again with the new one */
          if (request_credential_ != credential_)
            {
              next_try_ = now.addMSecs (gap_ms);
              timer_.start (gap_ms);
              return;
            }
          auto const station = station_of (next_record);
          if (Result::Call == next.scope)
            {
              // (review) that call's QSOs set aside, never asked again until its refusal is lifted; the others go on
              if (!refused_calls_.contains (station)) refused_calls_ << station;
              next_try_ = QDateTime {};
              Q_EMIT refused (next, station, call, queue_.size ());
              if (first_sendable () >= 0) {next_try_ = now.addMSecs (gap_ms); timer_.start (gap_ms);}
              return;
            }
          stopped_ = true;   // the user must act: OK (with another token or key for the token's), or the next start -
          stop_scope_ = next.scope;   // the token's also lifted by a good Club Log download (token_good ())
          next_try_ = QDateTime {};
          Q_EMIT refused (next, station, call, queue_.size ());
          return;
        }
      case Result::OneByOne:   // Club Log refused the batch as a whole: the same records, one a request
        one_by_one_ = true;
        next_try_ = now.addMSecs (gap_ms);
        timer_.start (gap_ms);
        return;
      case Result::Wait:
        not_before_ = wait_until (request_.retry_after (), now);
        next_try_ = not_before_;
        break;
      default:   // Retry
        ++failures_;
        next_try_ = now.addSecs (retry_minutes (failures_) * 60);
        break;
      }
    timer_.start (static_cast<int> (qMax<qint64> (0, now.msecsTo (next_try_))));
    Q_EMIT delayed (next, call, next_try_, queue_.size ());
  }

  void Uploader::save ()
  {
    if (!queue_file_.isEmpty ()) write_queue (queue_file_, queue_, site_);
  }
}
