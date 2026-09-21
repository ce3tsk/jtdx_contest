#ifndef HTTP_FETCH_HPP
#define HTTP_FETCH_HPP

/* CE3TSK 2026-09-19: one small HTTP(S) GET whose body stays in memory.

   Written for the Fox verifier (FoxVerifier.hpp, SUPERFOX_PLAN.md milestone 2), which asks a
   server a one-line question every time a DXpedition's one-time code is decoded. It is the
   transport half of FileDownload, made reusable: the same choice of transport - the shared
   QNetworkAccessManager everywhere but on Windows, WinHttpFetch there, because the Windows build
   ships no OpenSSL (WinHttpFetch.hpp has the reasoning) - without the part that turns the body
   into a file. FileDownload itself is left as it is: its Windows half cannot be tested on the
   machine this was written on, and nothing is gained by moving working code; doing so is a TODO
   for when a Windows check is at hand.

   The contract, which is what the callers are built on:

   - exactly ONE of fetched() or failed() per start(), also after a timeout. A late answer from a
     transport that was already given up on is dropped. There is deliberately no abort(): nothing
     needs one, and on Windows it could not keep a "no signal follows" promise - WinHttpFetch
     refuses a new start() while a cancelled worker is still retiring, so the next request would
     not start and would be handed the old one's cancellation (code review 2026-09-19).
   - fetched() is emitted whenever the SERVER answered, whatever the status: a 404 is an answer
     (the verification server uses it to say "I do not know this callsign"), and Qt reports it as a
     network error. failed() is for the cases with no HTTP status at all.
   - the whole request is bounded by timeout_ms here, by a timer of our own: Qt 5 has no transfer
     timeout before 5.15, and WinHTTP's are per phase and far longer than a verification may take.
   - no tr(): failed() carries a kind and the transport's own words, the caller decides what the
     operator is told, so this class adds no translation context. */

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QPointer>
#include <QTimer>
#include <QNetworkReply>

class QNetworkAccessManager;
#if defined (Q_OS_WIN)
class WinHttpFetch;
#endif

class HttpFetch final
  : public QObject
{
  Q_OBJECT

public:
  enum class Failure {Network, NoSsl, Timeout, TooLarge};
  Q_ENUM (Failure)

  explicit HttpFetch (QNetworkAccessManager * network_manager, QObject * parent = nullptr);
  ~HttpFetch ();

  // does nothing while a request is running: one at a time, the caller queues
  void start (QString const& url, QString const& user_agent, qint64 max_body_size, int timeout_ms);
  bool running () const {return running_;}

  Q_SIGNAL void fetched (QByteArray const& body, int http_status);
  Q_SIGNAL void failed (HttpFetch::Failure failure, QString const& detail);

private:
  void finish_fetched (QByteArray const& body, int http_status);
  void finish_failed (Failure failure, QString const& detail);
  void release_transport ();
  void reply_finished ();

  QNetworkAccessManager * manager_;
  QPointer<QNetworkReply> reply_;
  QByteArray body_;
  qint64 max_body_size_ {0};
  bool running_ {false};
  QTimer timeout_;
#if defined (Q_OS_WIN)
  WinHttpFetch * fetch_ {nullptr};
  bool timed_out_ {false};             // our timer fired; the worker's own signal ends the request
#endif
};

#endif
