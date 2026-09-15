#ifndef FILE_DOWNLOAD_HPP
#define FILE_DOWNLOAD_HPP

/* CE3TSK: fetch one file over HTTP(S) and replace a local copy with it.

   Adapted from WSJT-X 3.0.2 Network/FileDownload.{hpp,cpp} (GPL-3.0). Kept: the small
   configure() / start_download() / abort() interface, redirects followed, the replacement
   written atomically through QSaveFile. Changed, because each was a real fault here:

   - only this download's own reply is connected. The original also connected the network
     manager's finished() and deleteLater()'d every reply that finished on it; JTDX hands the
     same manager to WSPRNet, EQSL and the manual viewer, whose replies it would have deleted
     under them.
   - exactly one of complete() or error() per start. The original split failures between
     error() and download_error(), and its caller listened to one, so a missing SSL library or
     a redirect loop left the download button disabled for good.
   - the body is held and validated before anything is written, so an error page served by a
     captive portal can never replace a good cty.dat.
   - no tr() here: error() carries a Failure and a detail, and the caller words it, so this
     class adds no translation context. */

#include <functional>

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QPointer>
#include <QNetworkReply>

class QNetworkAccessManager;
#if defined (Q_OS_WIN)
/* CE3TSK 2026-09-15: Windows fetches through WinHTTP and Schannel instead of QNetworkAccessManager
   and OpenSSL, which Qt 5.15.2 can only reach through libssl-1_1, a library neither Qt nor Windows
   provides. WinHttpFetch.hpp has the full reasoning. Only the transport differs: the size cap, the
   validator, the atomic replacement and the one-signal-per-start rule below are shared. */
class WinHttpFetch;
#endif

class FileDownload
  : public QObject
{
  Q_OBJECT

public:
  enum class Failure {Network, NoSsl, HttpStatus, TooLarge, Rejected, Write};
  Q_ENUM (Failure)

  // an empty string accepts the content, anything else is the reason it was refused
  using Validator = std::function<QString (QByteArray const&)>;

  explicit FileDownload (QObject * parent = nullptr);
  ~FileDownload ();

  void configure (QNetworkAccessManager * network_manager, QString const& source_url,
                  QString const& destination_filename, QString const& user_agent,
                  Validator validator = Validator {});
  bool running () const;

  Q_SLOT void start_download ();
  Q_SLOT void abort ();

  Q_SIGNAL void complete (QString const& filename) const;
  // detail: the network error text, the HTTP status, the URL without SSL, the validator's
  // reason or the file error - whichever the failure is about
  Q_SIGNAL void error (FileDownload::Failure failure, QString const& detail) const;

private:
  void finished ();
  /* CE3TSK 2026-09-15: the shared tail. Whichever transport fetched the body, this is where it is
     checked and becomes a file. transport_detail is null unless the transport itself failed, and
     that failure is applied only after the size and status checks - a server that answered at all
     is reported by its status, and Qt flags a 404 as a network error too. */
  void deliver (QByteArray const& body, int http_status,
                Failure transport_failure = Failure::Network,
                QString const& transport_detail = QString {});
  void fail (Failure failure, QString const& detail);

  QNetworkAccessManager * manager_ {nullptr};
  QString source_url_;
  QString destination_filename_;
  QString user_agent_;
  Validator validator_;
  QPointer<QNetworkReply> reply_;
  QByteArray body_;
#if defined (Q_OS_WIN)
  WinHttpFetch * fetch_ {nullptr};
#endif
};

#endif
