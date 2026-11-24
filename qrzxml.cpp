// CE3TSK 2026-11-24 (TODO.md 3.-283, GRID_FILL_PLAN.md stage 3): QRZ.com's XML data - see qrzxml.h
#include "qrzxml.h"

#include <QHash>
#include <QUrl>
#include <QXmlStreamReader>

namespace qrz_xml
{
  QString base ()
  {
    QByteArray const set {qgetenv (api_env)};
    return set.isEmpty () ? QString {url} : QString::fromLatin1 (set);
  }

  QByteArray login_body (Login const& login, QString const& agent)
  {
    return "username=" + QUrl::toPercentEncoding (login.user.trimmed ()) + "&password=" + QUrl::toPercentEncoding (login.password)
      + "&agent=" + QUrl::toPercentEncoding (agent);
  }

  QByteArray lookup_body (QString const& key, QString const& call)
  {
    return "s=" + QUrl::toPercentEncoding (key) + "&callsign=" + QUrl::toPercentEncoding (call.trimmed ().toUpper ());
  }

  bool Session::ended () const
  {
    return key.isEmpty () || error.contains (QLatin1String {"Session Timeout"}, Qt::CaseInsensitive)
      || error.contains (QLatin1String {"Invalid session key"}, Qt::CaseInsensitive);
  }

  namespace
  {
    // the texts of a parent's children, by their local names (the service's namespace aside)
    QHash<QString, QString> children (QByteArray const& xml, QString const& parent)
    {
      QHash<QString, QString> got;
      QXmlStreamReader r {xml};
      int depth {0}, inside {-1};
      while (!r.atEnd ())
        {
          auto const t = r.readNext ();
          if (QXmlStreamReader::StartElement == t)
            {
              ++depth;
              if (-1 == inside && r.name () == parent) inside = depth;   // the first such parent only
              else if (inside > 0 && depth == inside + 1)
                {
                  QString const name {r.name ().toString ()};
                  QString const text {r.readElementText (QXmlStreamReader::SkipChildElements)};
                  --depth;   // readElementText consumed the end
                  if (!got.contains (name)) got.insert (name, text.trimmed ());
                }
            }
          else if (QXmlStreamReader::EndElement == t)
            {
              if (depth == inside) inside = -2;
              --depth;
            }
        }
      return got;
    }
  }

  Session session (QByteArray const& xml)
  {
    auto const c = children (xml, QStringLiteral ("Session"));
    return Session {c.value ("Key"), c.value ("Error"), c.value ("SubExp")};
  }

  Record record (QByteArray const& xml)
  {
    auto const c = children (xml, QStringLiteral ("Callsign"));
    return Record {c.value ("call"), c.value ("grid"), c.value ("geoloc")};
  }

  QString grid_of (QString const& asked, Record const& r)
  {
    QString const a {asked.trimmed ().toUpper ()};
    if (a.isEmpty () || r.call.trimmed ().toUpper () != a) return {};   // another record's - not this station's place
    QString const geoloc {r.geoloc.trimmed ().toLower ()};
    if (QLatin1String {"dxcc"} == geoloc || QLatin1String {"state"} == geoloc || QLatin1String {"none"} == geoloc) return {};   // only estimated
    return r.grid.trimmed ();
  }

  Lookup::Lookup (QObject * parent)
    : QObject {parent}
  {
    connect (&request_, &FileDownload::replied, this, &Lookup::replied);
    connect (&request_, &FileDownload::error, this, &Lookup::failed);
  }

  void Lookup::configure (QNetworkAccessManager * manager, QString const& user_agent, QString const& agent)
  {
    manager_ = manager;
    user_agent_ = user_agent;
    agent_ = agent;
  }

  void Lookup::set_login (Login const& login)
  {
    if (login == login_ && State::NoLogin != state_) return;
    login_ = login;
    key_.clear ();
    asked_.clear ();
    queue_.clear ();
    subscription_.clear ();
    logged_in_ = QDateTime {};
    stale_ = Step::None != step_;   // a request under way finishes; its answer is the old login's, let go
    set (login_.empty () ? State::NoLogin : State::Idle);
  }

  void Lookup::ask (QString const& call)
  {
    QString const c {call.trimmed ().toUpper ()};
    if (c.isEmpty () || !manager_ || State::NoLogin == state_ || State::Refused == state_ || State::NoSubscription == state_
        || asked_.contains (c)) return;
    asked_.insert (c);
    queue_ << c;
    next ();
  }

  void Lookup::next ()
  {
    if (Step::None != step_ || queue_.isEmpty ()) return;
    if (key_.isEmpty ())
      {
        step_ = Step::LoggingIn;
        request (login_body (login_, agent_));
      }
    else
      {
        step_ = Step::LookingUp;
        request (lookup_body (key_, queue_.first ()));
      }
  }

  void Lookup::request (QByteArray const& body)
  {
    FileDownload::Request r;
    r.method = "POST";
    r.headers = {{"Content-Type", "application/x-www-form-urlencoded"}};
    r.body = body;
    r.max_body_size = 256 * 1024;
    r.timeout_ms = 20000;
    r.reply_only = true;
    request_.configure (manager_, base (), QString {}, user_agent_, FileDownload::Validator {}, r);
    request_.start_download ();
  }

  void Lookup::replied (int http_status, QByteArray const& body)
  {
    auto const step = step_;
    step_ = Step::None;
    if (stale_ || Step::None == step)
      {
        stale_ = false;
        next ();
        return;
      }
    if (200 != http_status)
      {
        give_up (QStringLiteral ("HTTP %1").arg (http_status));
        return;
      }
    Session const s {session (body)};
    if (Step::LoggingIn == step)
      {
        if (s.key.isEmpty ())
          {
            // none at all (a page not the service's): a failure; QRZ.com's words: refused, asked no more until the login changes
            if (s.error.isEmpty ()) give_up (QStringLiteral ("an answer not understood"));
            else
              {
                queue_.clear ();
                set (State::Refused, s.error);
              }
            return;
          }
        subscription_ = s.subscription;
        if (!s.subscriber ())
          {
            queue_.clear ();
            set (State::NoSubscription);
            return;
          }
        key_ = s.key;
        logged_in_ = QDateTime::currentDateTimeUtc ();
        set (State::LoggedIn);
        next ();
        return;
      }
    // a lookup
    QString const call {queue_.isEmpty () ? QString {} : queue_.first ()};
    Record const rec {record (body)};
    if (rec.call.isEmpty () && s.ended () && !s.error.startsWith (QLatin1String {"Not found"}, Qt::CaseInsensitive))
      {
        key_.clear ();
        if (!again_)
          {
            again_ = true;   // logged into again once; the call stays first
            next ();
            return;
          }
        give_up (s.error.isEmpty () ? QStringLiteral ("the session ended") : s.error);
        return;
      }
    again_ = false;
    if (!queue_.isEmpty ()) queue_.removeFirst ();
    Q_EMIT answered (call, grid_of (call, rec));   // not found, or no grid it may take: ""
    if (State::LoggedIn != state_) set (State::LoggedIn);
    next ();
  }

  void Lookup::failed (FileDownload::Failure, QString const& detail)
  {
    step_ = Step::None;
    if (stale_)
      {
        stale_ = false;
        next ();
        return;
      }
    give_up (detail);
  }

  /* (the review 2026-11-24, 3) a failure drops the call it was for, answered by nobody - not the calls queued behind it:
     each goes on and gets its own request (K2AAA's request failing left K2BBB, a second QSO's call, never asked). Each call
     has one try a run (asked_), so a service that is down costs one request a call queued. */
  void Lookup::give_up (QString const& detail)
  {
    if (!queue_.isEmpty ()) queue_.removeFirst ();
    again_ = false;
    set (State::Failed, detail);
    next ();
  }

  void Lookup::set (State s, QString const& detail)
  {
    bool const changed {s != state_ || detail != detail_};
    state_ = s;
    detail_ = detail;
    if (changed) Q_EMIT state_changed ();
  }
}
