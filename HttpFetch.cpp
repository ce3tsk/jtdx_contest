#include "HttpFetch.hpp"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>

#if defined (Q_OS_WIN)
# include "WinHttpFetch.hpp"
#else
# include <QSslSocket>
#endif

HttpFetch::HttpFetch (QNetworkAccessManager * network_manager, QObject * parent)
  : QObject {parent}
  , manager_ {network_manager}
{
  timeout_.setSingleShot (true);
  connect (&timeout_, &QTimer::timeout, this, [this] {
      if (!running_) return;
#if defined (Q_OS_WIN)
      /* CE3TSK: WinHttpFetch is used exactly as FileDownload uses it - one object, reused - and it
         refuses a new start () while its worker runs. So a timeout here only ASKS the worker to
         stop; the request ends when the worker's own failed () arrives (at the next chunk, or when
         the WinHTTP phase it is blocked in times out), and is reported as a timeout then. That
         keeps "one signal per start" in the transport's hands instead of racing it. */
      timed_out_ = true;
      if (fetch_) fetch_->abort ();
#else
      release_transport ();
      finish_failed (Failure::Timeout, QString {});
#endif
    });
}

HttpFetch::~HttpFetch ()
{
  running_ = false;
  release_transport ();
#if defined (Q_OS_WIN)
  delete fetch_;                        // cancels the fetch and waits for its thread, as FileDownload does
  fetch_ = nullptr;
#endif
}

void HttpFetch::release_transport ()
{
  if (reply_)
    {
      QNetworkReply * const reply = reply_;
      reply_.clear ();
      disconnect (reply, nullptr, this, nullptr);   // no signal from a reply we have given up on
      reply->abort ();
      reply->deleteLater ();
    }
}

void HttpFetch::start (QString const& url, QString const& user_agent, qint64 max_body_size, int timeout_ms)
{
  if (running_) return;
  running_ = true;
  body_.clear ();
  max_body_size_ = max_body_size;
#if defined (Q_OS_WIN)
  timed_out_ = false;
  if (!fetch_)
    {
      fetch_ = new WinHttpFetch {this};
      connect (fetch_, &WinHttpFetch::fetched, this, [this] (QByteArray const& body, int http_status) {
          /* a COMPLETE answer is delivered whether or not our timer fired meanwhile: the worker may
             have finished just before it, with its signal still queued behind the timer's - and a
             verdict thrown away as "no answer in time" is never asked for again (each code is new) */
          finish_fetched (body, http_status);
        });
      connect (fetch_, &WinHttpFetch::failed, this, [this] (QString const& detail) {
          finish_failed (timed_out_ ? Failure::Timeout : Failure::Network, timed_out_ ? QString {} : detail);
        });
    }
  timeout_.start (timeout_ms);
  fetch_->start (url, user_agent, max_body_size);
#else
  QUrl const qurl {url};
  if (!manager_ || !qurl.isValid ())
    {
      finish_failed (Failure::Network, url);
      return;
    }
  if ("https" == qurl.scheme () && !QSslSocket::supportsSsl ())
    {
      finish_failed (Failure::NoSsl, qurl.toDisplayString ());
      return;
    }
#if QT_VERSION < QT_VERSION_CHECK (5, 15, 0)
  /* as eqsl.cpp and wsprnet.cpp do on the same shared manager: in Qt 5 it can stay "not accessible"
     after the connection dropped and came back, and then every request fails at once, until the
     program is restarted, with "Network access is disabled" */
  if (QNetworkAccessManager::Accessible != manager_->networkAccessible ()) manager_->setNetworkAccessible (QNetworkAccessManager::Accessible);
#endif
  QNetworkRequest request {qurl};
  request.setAttribute (QNetworkRequest::FollowRedirectsAttribute, true);
  request.setMaximumRedirectsAllowed (10);   // as FileDownload: one rule for both users of this transport
  request.setRawHeader ("Accept", "*/*");
  request.setRawHeader ("User-Agent", user_agent.toUtf8 ());
  reply_ = manager_->get (request);
  connect (reply_, &QNetworkReply::readyRead, this, [this] {
      if (!reply_) return;
      body_ += reply_->readAll ();
      if (body_.size () > max_body_size_)
        {
          /* too large for an ANSWER. But a status that is not 2xx is the answer by itself - the
             verification server's 404 means "callsign not known" whatever page comes with it, and
             reported as TooLarge (a failure) it would end a question that the next server on the
             list should have been asked. The headers are in by the time a body arrives. */
          int const http_status = reply_->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt ();
          body_.truncate (int (max_body_size_));
          QByteArray const head {body_};
          release_transport ();
          if (http_status && (http_status < 200 || http_status > 299)) finish_fetched (head, http_status);
          else finish_failed (Failure::TooLarge, QString::number (max_body_size_));
        }
    });
  connect (reply_, &QNetworkReply::finished, this, &HttpFetch::reply_finished);
  timeout_.start (timeout_ms);
#endif
}

void HttpFetch::reply_finished ()
{
  if (!reply_) return;
  QNetworkReply * const reply = reply_;
  reply_.clear ();
  reply->deleteLater ();
  body_ += reply->readAll ();
  int const http_status = reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt ();
  // a server that answered at all is reported by its status: Qt flags a 404 as a network error
  // too, and for the verification server a 404 IS the answer ("callsign not known")
  if (http_status)
    {
      finish_fetched (body_, http_status);
      return;
    }
#if !defined (Q_OS_WIN)
  if ("https" == reply->url ().scheme () && !QSslSocket::supportsSsl ())
    {
      finish_failed (Failure::NoSsl, reply->url ().toDisplayString ());
      return;
    }
#endif
  finish_failed (Failure::Network, reply->errorString ());
}

void HttpFetch::finish_fetched (QByteArray const& body, int http_status)
{
  if (!running_) return;                // given up on already: exactly one signal per start
  timeout_.stop ();
  running_ = false;
  if (body.size () > max_body_size_ && http_status >= 200 && http_status <= 299)
    {
      Q_EMIT failed (Failure::TooLarge, QString::number (max_body_size_));
      return;
    }
  QByteArray const answer {body.left (int (max_body_size_))};   // body may be body_, which the next start () clears; an error page is cut, its status is the answer
  body_.clear ();
  Q_EMIT fetched (answer, http_status);
}

void HttpFetch::finish_failed (Failure failure, QString const& detail)
{
  if (!running_) return;
  timeout_.stop ();
  running_ = false;
  body_.clear ();
  Q_EMIT failed (failure, detail);
}
