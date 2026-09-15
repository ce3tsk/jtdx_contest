#ifndef WIN_HTTP_FETCH_HPP
#define WIN_HTTP_FETCH_HPP

/* CE3TSK 2026-09-15: one HTTP(S) GET through WinHTTP, so Windows needs no OpenSSL.

   Qt 5.15.2 reaches TLS only through OpenSSL 1.1, which it loads by name at run time
   (libssl-1_1-x64.dll, libcrypto-1_1-x64.dll). Those are not part of Qt, not part of JTSDK and
   not part of Windows, so unless the installer carries them QSslSocket::supportsSsl() is false
   and every https request fails - which is what shipped in 3.0.0-rc06. OpenSSL 3 is not a way
   out: Qt 5.15.2 looks for the 1.1 file names, and two of the functions it resolves
   (SSL_get_peer_certificate, EVP_PKEY_base_id) became header macros in OpenSSL 3, so a renamed
   OpenSSL 3 loads, reports itself, and then fails every connection with "the peer did not
   present any certificate" - measured, not assumed.

   WinHTTP is part of Windows, validates through Schannel and the Windows certificate store, and
   picks up the system proxy. So on Windows the transport is this class and nothing is shipped;
   everywhere else FileDownload keeps using QNetworkAccessManager, where the platform's own
   OpenSSL is present and maintained.

   WinHTTP's synchronous calls are the simple ones, and they would block the GUI for the length
   of the transfer, so the work runs on a private thread and the result arrives as a queued
   signal. abort() sets a flag the read loop checks between chunks rather than closing the
   handle under a blocking call; with the session timeouts set in the .cpp that bounds how long
   a cancel or an application exit can wait, without racing WinHTTP's own handle teardown.

   Exactly one of fetched() or failed() is emitted per start(), which is the guarantee
   FileDownload is built on. */

#include <QObject>
#include <QByteArray>
#include <QString>

class WinHttpFetch final
  : public QObject
{
  Q_OBJECT

public:
  explicit WinHttpFetch (QObject * parent = nullptr);
  ~WinHttpFetch ();

  // max_body_size is not enforced as a failure here: the read loop simply stops once the body is
  // past it and the caller reports it, so the size rule stays in one place
  void start (QString const& url, QString const& user_agent, qint64 max_body_size);
  void abort ();
  bool running () const;

  Q_SIGNAL void fetched (QByteArray const& body, int http_status);
  // the transport-level reason, already worded by Windows; the caller decides how to present it
  Q_SIGNAL void failed (QString const& detail);

private:
  class Worker;

  Q_SLOT void worker_finished ();

  Worker * worker_ {nullptr};
};

#endif
