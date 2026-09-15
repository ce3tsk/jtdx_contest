#include "FileDownload.hpp"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QSslSocket>
#include <QFileInfo>
#include <QDir>
#include <QUrl>

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
  return reply_ && reply_->isRunning ();
}

void FileDownload::start_download ()
{
  if (running () || !manager_) return;
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
  body_.clear ();
  reply_ = manager_->get (request);
  connect (reply_, &QNetworkReply::readyRead, this, [this] {
      if (!reply_) return;
      body_ += reply_->readAll ();
      if (body_.size () > max_body_size) reply_->abort ();
    });
  connect (reply_, &QNetworkReply::finished, this, &FileDownload::finished);
}

void FileDownload::abort ()
{
  if (running ()) reply_->abort ();
}

void FileDownload::finished ()
{
  if (!reply_) return;
  QNetworkReply * const reply = reply_;
  reply_.clear ();                      // running() is false from here on
  reply->deleteLater ();
  body_ += reply->readAll ();

  auto fail = [this] (Failure failure, QString const& detail) {
      body_.clear ();
      Q_EMIT error (failure, detail);
    };

  if (body_.size () > max_body_size)
    {
      fail (Failure::TooLarge, QString::number (max_body_size / (1024 * 1024)));
      return;
    }
  // a server that answered at all is reported by its status: Qt also flags a 404 as a network error
  auto const status = reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt ();
  if (status && 200 != status)
    {
      fail (Failure::HttpStatus, QString::number (status));
      return;
    }
  if (QNetworkReply::NoError != reply->error ())
    {
      // an http URL redirected to https lands here when the SSL library is missing
      if ("https" == reply->url ().scheme () && !QSslSocket::supportsSsl ())
        fail (Failure::NoSsl, reply->url ().toDisplayString ());
      else
        fail (Failure::Network, reply->errorString ());
      return;
    }
  if (validator_)
    {
      auto const reason = validator_ (body_);
      if (!reason.isEmpty ())
        {
          fail (Failure::Rejected, reason);
          return;
        }
    }
  QDir {}.mkpath (QFileInfo {destination_filename_}.absolutePath ());
  QSaveFile file {destination_filename_};
  if (!file.open (QIODevice::WriteOnly) || file.write (body_) != body_.size () || !file.commit ())
    {
      fail (Failure::Write, file.errorString ());
      return;
    }
  body_.clear ();
  Q_EMIT complete (destination_filename_);
}
