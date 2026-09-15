#include "FileDownload.hpp"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QFileInfo>
#include <QDir>
#include <QUrl>

#if defined (Q_OS_WIN)
# include "WinHttpFetch.hpp"
#else
# include <QSslSocket>
#endif

namespace
{
  // the LoTW activity file is about 6 MB; anything far past that is not what was asked for
  qint64 const max_body_size {64 * 1024 * 1024};
}

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
  manager_ = network_manager;
  source_url_ = source_url;
  destination_filename_ = destination_filename;
  user_agent_ = user_agent;
  validator_ = validator;
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

#if defined (Q_OS_WIN)
  /* CE3TSK 2026-09-15: WinHTTP and Schannel, so there is no OpenSSL to be missing and no
     QSslSocket::supportsSsl () gate to fail. The Failure::NoSsl case below cannot arise here and
     the enumerator stays only so the caller's wording is the same on every platform. */
  if (!fetch_)
    {
      fetch_ = new WinHttpFetch {this};
      connect (fetch_, &WinHttpFetch::fetched, this, [this] (QByteArray const& body, int http_status) {
          deliver (body, http_status);
        });
      connect (fetch_, &WinHttpFetch::failed, this, [this] (QString const& detail) {
          fail (Failure::Network, detail);
        });
    }
  fetch_->start (source_url_, user_agent_, max_body_size);
#else
  if (!manager_) return;
  QUrl const url {source_url_};
  if ("https" == url.scheme () && !QSslSocket::supportsSsl ())
    {
      Q_EMIT error (Failure::NoSsl, url.toDisplayString ());
      return;
    }
  QNetworkRequest request {url};
  request.setAttribute (QNetworkRequest::FollowRedirectsAttribute, true);
  request.setMaximumRedirectsAllowed (10);
#if QT_VERSION >= QT_VERSION_CHECK (5, 15, 0)
  // Qt 5 has no transfer timeout by default: a server that stalls mid-transfer would never send
  // finished (), leaving the download - and its button - running until JTDX is restarted
  request.setTransferTimeout (30000);
#endif
  request.setRawHeader ("Accept", "*/*");
  request.setRawHeader ("User-Agent", user_agent_.toUtf8 ());   // country-files.com refuses requests without one
  reply_ = manager_->get (request);
  connect (reply_, &QNetworkReply::readyRead, this, [this] {
      if (!reply_) return;
      body_ += reply_->readAll ();
      if (body_.size () > max_body_size) reply_->abort ();
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
           transport_failure, transport_detail);
}

void FileDownload::deliver (QByteArray const& body, int http_status,
                            Failure transport_failure, QString const& transport_detail)
{
  if (body.size () > max_body_size)
    {
      fail (Failure::TooLarge, QString::number (max_body_size / (1024 * 1024)));
      return;
    }
  // a server that answered at all is reported by its status: Qt also flags a 404 as a network error
  if (http_status && 200 != http_status)
    {
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
  QDir {}.mkpath (QFileInfo {destination_filename_}.absolutePath ());
  QSaveFile file {destination_filename_};
  if (!file.open (QIODevice::WriteOnly) || file.write (body) != body.size () || !file.commit ())
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
