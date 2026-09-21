#include "FoxVerifier.hpp"

#include <QNetworkAccessManager>
#include <QRegularExpression>
#include <QUrl>

#include "HttpFetch.hpp"
#include "qt_helpers.hpp"   // SkipEmptyParts, which is QString's before Qt 5.14 and Qt's after

namespace
{
  int const max_queue {8};              // MSHV's figure; a Fox sends one code every 30 s
  int const max_remembered {64};        // questions remembered, so a repeat is not asked again
  int const timeout_ms {5000};          // WSJT-X's figure
  qint64 const max_answer_size {1024};  // the answer is one short line
}

FoxVerifier::FoxVerifier (QNetworkAccessManager * network_manager, QString const& user_agent, QObject * parent)
  : QObject {parent}
  , fetch_ {new HttpFetch {network_manager, this}}
  , user_agent_ {user_agent}
  , base_urls_ {QString {default_base_url ()}}
{
  connect (fetch_, &HttpFetch::fetched, this, [this] (QByteArray const& body, int http_status) {
      Outcome const outcome = parse (http_status, body);
      if (Outcome::Failed != outcome) answered (outcome, Trouble::None, QString::fromUtf8 (body).simplified ());
      else if (http_status < 200 || http_status > 299) answered (outcome, Trouble::HttpStatus, QString {"HTTP %1"}.arg (http_status));
      else answered (outcome, Trouble::BadAnswer, "unexpected answer");
    });
  connect (fetch_, &HttpFetch::failed, this, [this] (HttpFetch::Failure failure, QString const& detail) {
      QString what; Trouble trouble {Trouble::Network};
      switch (failure)
        {
        case HttpFetch::Failure::NoSsl: what = "no SSL/TLS support"; trouble = Trouble::NoSsl; break;
        case HttpFetch::Failure::Timeout: what = "no answer in time"; trouble = Trouble::Timeout; break;
        case HttpFetch::Failure::TooLarge: what = "answer too large"; trouble = Trouble::TooLarge; break;
        case HttpFetch::Failure::Network: what = detail.isEmpty () ? QString {"network error"} : detail; break;
        }
      answered (Outcome::Failed, trouble, what);
    });
}

FoxVerifier::~FoxVerifier ()
{
}

QStringList FoxVerifier::clean_urls (QStringList const& base_urls)
{
  /* the ini file's "a, b" arrives as a list already (QSettings); a line typed with blanks or
     semicolons instead arrives as ONE string, which would be asked as one impossible URL */
  static QRegularExpression const separators {"[\\s,;]+"};
  QStringList urls;
  for (auto const& element : base_urls)
    for (auto const& given : element.split (separators, SkipEmptyParts))
      {
        QString url {given};
        while (url.endsWith ('/')) url.chop (1);
        if (!url.isEmpty () && !urls.contains (url, Qt::CaseInsensitive) && urls.size () < max_servers) urls << url;
      }
  if (urls.isEmpty ()) urls << QString {default_base_url ()};
  return urls;
}

void FoxVerifier::set_base_urls (QStringList const& base_urls)
{
  base_urls_ = clean_urls (base_urls);   // questions already queued keep the list they were asked under
}

QString FoxVerifier::bare_call (QString const& call)
{
  QString c {call.trimmed ().toUpper ()};
  if (c.startsWith ('<') && c.endsWith ('>')) c = c.mid (1, c.size () - 2);
  if (c.contains ("...")) return QString {};   // a hash nobody could resolve names nobody
  return c;
}

QString FoxVerifier::url_for (QString const& base_url, QString const& call, QDateTime const& slot_utc, QString const& code)
{
  /* The time is written from its digits, never through a format parse or a time spec conversion:
     the servers' clients send the UTC wall time WITHOUT a "Z" (both WSJT-X and MSHV do, by an
     accident of QDateTime's default spec), and that is the form known to work. The callsign is
     percent-encoded, "/" included - "VP2X%2FK1JT" is routed, a literal slash gives HTTP 500. */
  QDate const d {slot_utc.date ()};
  QTime const t {slot_utc.time ()};
  QString const stamp = QString {"%1-%2-%3T%4:%5:%6"}
    .arg (d.year (), 4, 10, QChar {'0'}).arg (d.month (), 2, 10, QChar {'0'}).arg (d.day (), 2, 10, QChar {'0'})
    .arg (t.hour (), 2, 10, QChar {'0'}).arg (t.minute (), 2, 10, QChar {'0'}).arg (t.second (), 2, 10, QChar {'0'});
  return base_url + "/check/" + QString::fromLatin1 (QUrl::toPercentEncoding (call)) + '/' + stamp + '/' + code + ".text";
}

FoxVerifier::Outcome FoxVerifier::parse (int http_status, QByteArray const& body)
{
  if (404 == http_status) return Outcome::Unknown;
  if (http_status < 200 || http_status > 299) return Outcome::Failed;
  QByteArray const line {body.trimmed ()};
  if (line.endsWith (" VERIFIED")) return Outcome::Verified;
  if (line.endsWith (" INVALID")) return Outcome::Invalid;
  return Outcome::Failed;               // an answer, but not one of the two this protocol has
}

QDateTime FoxVerifier::slot_from_time (QTime const& hhmmss, QDateTime const& now_utc)
{
  QDate date {now_utc.date ()};
  int const ahead = now_utc.time ().secsTo (hhmmss);   // > 0: that time of day is still to come today
  if (ahead > 12 * 3600) date = date.addDays (-1);
  return QDateTime {date, hhmmss, Qt::UTC};
}

FoxVerifier::Asked FoxVerifier::verify (QString const& call, QDateTime const& slot_utc, QString const& code, int hz)
{
  static QRegularExpression const six_digits {"^[0-9]{6}$"};
  if (!six_digits.match (code).hasMatch () || !slot_utc.isValid ()) return Asked::Malformed;
  if ("000000" == code) return Asked::Unsigned;
  QString const bare {bare_call (call)};
  if (bare.isEmpty ()) return call.contains ("...") ? Asked::HashedCall : Asked::Malformed;

  QString const key = bare + '|' + slot_utc.toString ("yyyyMMddHHmmss") + '|' + code;
  if (asked_.contains (key)) return Asked::Duplicate;
  if (queue_.size () >= max_queue) return Asked::QueueFull;
  asked_ << key;
  while (asked_.size () > max_remembered) asked_.removeFirst ();

  queue_ << Question {bare, slot_utc, code, hz, key, 0, base_urls_};
  if (!asking_) next ();
  return Asked::Yes;
}

void FoxVerifier::next ()
{
  if (queue_.isEmpty ()) { asking_ = false; return; }
  asking_ = true;
  Question const& q = queue_.first ();
  // may answer synchronously (no SSL, a bad URL): answered () copes with being called from here
  fetch_->start (url_for (q.servers.value (q.server), q.call, q.slot_utc, q.code), user_agent_, max_answer_size, timeout_ms);
}

void FoxVerifier::answered (Outcome outcome, Trouble trouble, QString const& detail)
{
  if (queue_.isEmpty ()) return;
  /* "not known" is the ONE answer that passes the question on to the next server (FoxVerifier.hpp):
     it stays at the head of the queue, so the order of results is the order of questions */
  if (Outcome::Unknown == outcome && queue_.first ().server + 1 < queue_.first ().servers.size ())
    {
      ++queue_.first ().server;
      next ();
      return;
    }
  Question const q = queue_.takeFirst ();
  if (Outcome::Failed == outcome) asked_.removeAll (q.key);   // not answered: may be asked again
  Q_EMIT result (q.call, q.slot_utc, q.hz, outcome, trouble, detail, q.servers.value (q.server));
  next ();
}
