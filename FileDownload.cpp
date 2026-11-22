#include "FileDownload.hpp"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QLocale>

#if defined (Q_OS_WIN)
# include "WinHttpFetch.hpp"
#else
# include <QSslSocket>
#endif

FileDownload::FileDownload (QObject * parent)
  : QObject {parent}
{
}

FileDownload::~FileDownload ()
{
#if defined (Q_OS_WIN)
  // cancels the fetch and waits for its thread; nothing of ours is touched afterwards
  delete fetch_;
  fetch_ = nullptr;
#endif
  if (reply_)
    {
      // no signals from a half-destroyed object
      disconnect (reply_, nullptr, this, nullptr);
      reply_->abort ();
      reply_->deleteLater ();
    }
}

void FileDownload::configure (QNetworkAccessManager * network_manager, QString const& source_url,
                              QString const& destination_filename, QString const& user_agent,
                              Validator validator)
{
  configure (network_manager, source_url, destination_filename, user_agent, validator, Request {});
}

void FileDownload::configure (QNetworkAccessManager * network_manager, QString const& source_url,
                              QString const& destination_filename, QString const& user_agent,
                              Validator validator, Request const& request, Transform transform)
{
  manager_ = network_manager;
  source_url_ = source_url;
  destination_filename_ = destination_filename;
  user_agent_ = user_agent;
  validator_ = validator;
  request_ = request;
  transform_ = transform;
}

/* CE3TSK 2026-11-21: Retry-After is either a number of seconds or an HTTP date (RFC 7231 7.1.3) */
QDateTime FileDownload::parse_retry_after (QByteArray const& value, QDateTime const& now)
{
  auto const text = QString::fromLatin1 (value).trimmed ();
  if (text.isEmpty ()) return {};
  bool number {false};
  auto const seconds = text.toLongLong (&number);
  if (number) return seconds >= 0 ? now.addSecs (seconds) : QDateTime {};
  // "Sun, 06 Nov 1994 08:49:37 GMT": English names whatever the program's language
  auto when = QLocale::c ().toDateTime (text, "ddd, dd MMM yyyy HH:mm:ss 'GMT'");
  if (!when.isValid ()) return {};
  when.setTimeSpec (Qt::UTC);
  return when;
}

bool FileDownload::running () const
{
#if defined (Q_OS_WIN)
  if (fetch_ && fetch_->running ()) return true;
#endif
  return reply_ && reply_->isRunning ();
}

void FileDownload::start_download ()
{
  if (running ()) return;
  body_.clear ();
  retry_after_ = QDateTime {};

#if defined (Q_OS_WIN)
  /* CE3TSK 2026-09-15: WinHTTP and Schannel, so there is no OpenSSL to be missing and no
     QSslSocket::supportsSsl () gate to fail. The Failure::NoSsl case below cannot arise here and
     the enumerator stays only so the caller's wording is the same on every platform. */
  if (!fetch_)
    {
      fetch_ = new WinHttpFetch {this};
      connect (fetch_, &WinHttpFetch::fetched, this, [this] (QByteArray const& body, int http_status, QByteArray const& retry_after) {
          deliver (body, http_status, Failure::Network, QString {}, retry_after);
        });
      connect (fetch_, &WinHttpFetch::failed, this, [this] (QString const& detail) {
          fail (Failure::Network, detail);
        });
    }
  {
    QByteArray headers;   // "Name: value\r\n" each, as WinHttpSendRequest takes them
    for (auto const& h : request_.headers) headers += h.first + ": " + h.second + "\r\n";
    fetch_->start (source_url_, user_agent_, request_.max_body_size, request_.method, headers, request_.body,
                   request_.timeout_ms > 0 ? request_.timeout_ms : 20000);
  }
#else
  if (!manager_) return;
  QUrl const url {source_url_};
  if ("https" == url.scheme () && !QSslSocket::supportsSsl ())
    {
      Q_EMIT error (Failure::NoSsl, url.toDisplayString ());
      return;
    }
  QNetworkRequest request {url};
  // (3.-280) a request carrying headers of its own - a token - is never redirected (Request in the header)
  request.setAttribute (QNetworkRequest::FollowRedirectsAttribute, request_.headers.isEmpty ());
  if (request_.headers.isEmpty ()) request.setMaximumRedirectsAllowed (10);
#if QT_VERSION >= QT_VERSION_CHECK (5, 15, 0)
  // Qt 5 has no transfer timeout by default: a server that stalls mid-transfer would never send
  // finished (), leaving the download - and its button - running until JTDX is restarted
  request.setTransferTimeout (request_.timeout_ms > 0 ? request_.timeout_ms : 30000);
#endif
  request.setRawHeader ("Accept", "*/*");
  request.setRawHeader ("User-Agent", user_agent_.toUtf8 ());   // country-files.com refuses requests without one
  for (auto const& h : request_.headers) request.setRawHeader (h.first, h.second);
  reply_ = "GET" == request_.method ? manager_->get (request)
                                    : manager_->sendCustomRequest (request, request_.method, request_.body);
  connect (reply_, &QNetworkReply::readyRead, this, [this] {
      if (!reply_) return;
      body_ += reply_->readAll ();
      if (body_.size () > request_.max_body_size) reply_->abort ();
    });
  connect (reply_, &QNetworkReply::finished, this, &FileDownload::finished);
#endif
}

void FileDownload::abort ()
{
#if defined (Q_OS_WIN)
  if (fetch_) fetch_->abort ();
#endif
  if (reply_ && reply_->isRunning ()) reply_->abort ();
}

void FileDownload::finished ()
{
  if (!reply_) return;
  QNetworkReply * const reply = reply_;
  reply_.clear ();                      // running() is false from here on
  reply->deleteLater ();
  body_ += reply->readAll ();

  Failure transport_failure {Failure::Network};
  QString transport_detail;             // stays null while the transport itself was happy
  if (QNetworkReply::NoError != reply->error ())
    {
      // an http URL redirected to https lands here when the SSL library is missing
#if !defined (Q_OS_WIN)
      if ("https" == reply->url ().scheme () && !QSslSocket::supportsSsl ())
        {
          transport_failure = Failure::NoSsl;
          transport_detail = reply->url ().toDisplayString ();
        }
      else
#endif
        {
          transport_failure = Failure::Network;
          transport_detail = reply->errorString ();
        }
      if (transport_detail.isNull ()) transport_detail = QString {""};   // never null once it has failed
    }

  deliver (body_, reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt (),
           transport_failure, transport_detail, reply->rawHeader ("Retry-After"));
}

void FileDownload::deliver (QByteArray const& body, int http_status,
                            Failure transport_failure, QString const& transport_detail,
                            QByteArray const& retry_after)
{
  if (body.size () > request_.max_body_size)
    {
      fail (Failure::TooLarge, QString::number (request_.max_body_size / (1024 * 1024)));
      return;
    }
  // a server that answered at all is reported by its status: Qt also flags a 404 as a network error
  if (http_status && 200 != http_status)
    {
      if (429 == http_status || 503 == http_status)
        retry_after_ = parse_retry_after (retry_after, QDateTime::currentDateTimeUtc ());
      fail (Failure::HttpStatus, QString::number (http_status));
      return;
    }
  if (!transport_detail.isNull ())
    {
      fail (transport_failure, transport_detail);
      return;
    }
  if (validator_)
    {
      auto const reason = validator_ (body);
      if (!reason.isEmpty ())
        {
          fail (Failure::Rejected, reason);
          return;
        }
    }
  QByteArray const content {transform_ ? transform_ (body) : body};
  QDir {}.mkpath (QFileInfo {destination_filename_}.absolutePath ());
  QSaveFile file {destination_filename_};
  if (!file.open (QIODevice::WriteOnly) || file.write (content) != content.size () || !file.commit ())
    {
      fail (Failure::Write, file.errorString ());
      return;
    }
  body_.clear ();
  Q_EMIT complete (destination_filename_);
}

void FileDownload::fail (Failure failure, QString const& detail)
{
  body_.clear ();
  Q_EMIT error (failure, detail);
}
