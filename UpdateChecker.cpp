#include "UpdateChecker.hpp"

#include <QRegularExpression>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include "HttpFetch.hpp"
#include "revision_utils.hpp"

// CE3TSK 2026-09-22: see UpdateChecker.hpp

char const * UpdateChecker::url () {return "https://ce3tsk.com/download/latest.json";}
char const * UpdateChecker::download_page () {return "https://ce3tsk.com/download/";}

char const * UpdateChecker::platform ()
{
#if defined (Q_OS_WIN)
  return "windows";
#elif defined (Q_OS_MACOS)
  return "macos";
#else
  return "linux";
#endif
}

UpdateChecker::UpdateChecker (QNetworkAccessManager * network_manager, QString const& user_agent, QObject * parent)
  : QObject {parent}
  , fetch_ {new HttpFetch {network_manager, this}}
  , user_agent_ {user_agent}
{
  connect (fetch_, &HttpFetch::fetched, this, [this] (QByteArray const& body, int http_status) {
      if (200 != http_status)
        {
          Q_EMIT failed (Trouble::HttpStatus, QString::number (http_status), silent_);
          return;
        }
      Latest latest;
      int order {0};
      if (!parse (body, platform (), &latest) || !compare (latest.version, version (true), &order))
        {
          Q_EMIT failed (Trouble::Unreadable, latest.version.left (40), silent_);
          return;
        }
      if (order > 0) Q_EMIT newer (latest, silent_);
      else Q_EMIT current (latest, silent_);
    });
  connect (fetch_, &HttpFetch::failed, this, [this] (HttpFetch::Failure failure, QString const& detail) {
      Trouble trouble {Trouble::Network};
      switch (failure)
        {
        case HttpFetch::Failure::Network: trouble = Trouble::Network; break;
        case HttpFetch::Failure::NoSsl: trouble = Trouble::NoSsl; break;
        case HttpFetch::Failure::Timeout: trouble = Trouble::Timeout; break;
        case HttpFetch::Failure::TooLarge: trouble = Trouble::TooLarge; break;
        }
      Q_EMIT failed (trouble, detail, silent_);
    });
}

bool UpdateChecker::running () const {return fetch_->running ();}

void UpdateChecker::check (bool silent)
{
  /* one at a time (HttpFetch's rule): a check asked for while one runs rides on it - a menu check made loud,
     since the operator is now waiting for the answer */
  if (fetch_->running ())
    {
      if (!silent) silent_ = false;
      return;
    }
  silent_ = silent;
  /* JTDX_UPDATE_URL: a TEST HOOK - another address for the file (a local server with a newer version, to see the
     icon and the dialog before anything is published); nothing else reads it */
  auto const hook = qEnvironmentVariable ("JTDX_UPDATE_URL");
  fetch_->start (hook.isEmpty () ? QString {url ()} : hook, user_agent_, 64 * 1024, 10000);
}

bool UpdateChecker::parse (QByteArray const& body, QString const& platform, Latest * latest)
{
  QJsonParseError error;
  auto const doc = QJsonDocument::fromJson (body, &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject ()) return false;
  auto const top = doc.object ();
  auto const own = top.value ("platforms").toObject ().value (platform).toObject ();
  // this platform's entry, key by key, falling back on the top level
  auto const pick = [&top, &own] (char const * key) { return own.contains (key) ? own.value (key) : top.value (key); };
  latest->version = pick ("version").toString ().trimmed ();
  auto const changes = pick ("changes");
  QStringList lines;
  if (changes.isArray ())
    {
      for (auto const& v : changes.toArray ())
        {
          auto const t = v.toString ().trimmed ();
          if (!t.isEmpty ()) lines << QString {"- "} + t;
        }
    }
  else if (changes.isString ()) lines << changes.toString ().trimmed ();
  latest->changelog = lines.join ('\n');
  latest->url = pick ("url").toString ().trimmed ();
  if (!latest->url.startsWith ("https://")) latest->url = download_page ();   // never an http:// or a file: page
  int dummy;
  return compare (latest->version, latest->version, &dummy);
}

bool UpdateChecker::compare (QString const& a, QString const& b, int * result)
{
  static QRegularExpression const re {R"(^v?(\d+)\.(\d+)\.(\d+)(?:-rc(\d+))?$)", QRegularExpression::CaseInsensitiveOption};
  auto const ma = re.match (a.trimmed ());
  auto const mb = re.match (b.trimmed ());
  if (!ma.hasMatch () || !mb.hasMatch ()) return false;
  for (int i = 1; i <= 3; ++i)
    {
      auto const x = ma.captured (i).toInt ();
      auto const y = mb.captured (i).toInt ();
      if (x != y) {*result = x < y ? -1 : 1; return true;}
    }
  // the same number: a release (no -rc) is newer than any of its release candidates
  auto const ra = ma.captured (4).isEmpty () ? 1000000 : ma.captured (4).toInt ();
  auto const rb = mb.captured (4).isEmpty () ? 1000000 : mb.captured (4).toInt ();
  *result = ra == rb ? 0 : (ra < rb ? -1 : 1);
  return true;
}
