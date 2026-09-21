#ifndef FOX_VERIFIER_HPP
#define FOX_VERIFIER_HPP

/* CE3TSK 2026-09-19: is this Fox the DXpedition it says it is?

   A DXpedition running WSJT-X 2.7 or later, or MSHV, can sign its transmissions with a one-time
   code (RFC 6238 TOTP, six digits, from a key issued by the NCDXF). A SuperFox carries the code in
   every transmission - the receiver prints it as "$VERIFY$ <call> <code>" - and an old-style
   multi-stream Fox sends it now and then as the free text "<call>.<code>". A Hound holds no key:
   it asks a server that knows the keys,

       GET <base>/check/<call>/<yyyy-MM-ddThh:mm:ss>/<code>.text        base = https://www.9dx.cc

   with the UTC start of the slot the code was received in, and the answer is one line ending in
   " VERIFIED" or " INVALID", or HTTP 404 when the server does not know the callsign at all.
   (Probed 2026-09-19: K1JT / 2025-01-08T10:12:00 / 958844 still answers VERIFIED - the test
   vector. SUPERFOX_PLAN.md A7 has the contract, SUPERFOX_MSHV.md 8-9 MSHV's client.)

   This is neither WSJT-X's Network/FoxVerifier nor MSHV's client, though it speaks their protocol:
   both need OpenSSL, which this program's Windows build does not ship (HttpFetch.hpp), and each has
   faults not worth inheriting - WSJT-X's leaks an object per request, never reports a failure and
   asks again for every repeat; MSHV's blocks the GUI thread to connect, parses HTTP by hand and
   drops its queue silently; both take the DATE from the wall clock, so a code received at 23:59:45
   and checked after midnight is sent with tomorrow's date. What is kept from them is behaviour:
   no question for an unsigned code (000000) or a hashed call (<...>), one question per code, a
   short queue, and old-style and SuperFox codes through the same door. A question that FAILED (no
   network, no answer in time) is forgotten again: the same code may be asked once more - a
   failure is not an answer, and "one question per code" must not turn a blip into "never".

   SEVERAL SERVERS, ASKED IN ORDER (2026-09-19). www.9dx.cc holds the keys the NCDXF issues; a
   station may be registered with another provider instead (hamdx.org calls itself an alternate
   one and speaks the same protocol; MSHV ships both addresses). The list is a ranking by trust,
   not a pool: a server is asked ONLY when every server before it answered "callsign not known"
   (HTTP 404). An earlier server's verdict is final either way, and an earlier server's FAILURE
   ends the question too - were a later server consulted after "invalid" or after a timeout,
   registering somebody else's callsign at the weakest provider on the list would buy a
   "verified" whenever the better one objected or was unreachable. The result names the server
   that gave it, so the caller can say who vouched.

   One object, owned by the main window. verify () either queues a question or says why not;
   exactly one result () follows each queued question. No tr() here: the caller words everything. */

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QByteArray>
#include <QList>
#include <QStringList>

class QNetworkAccessManager;
class HttpFetch;

class FoxVerifier final
  : public QObject
{
  Q_OBJECT

public:
  enum class Outcome {Verified, Invalid, Unknown, Failed};   // Unknown: the server has no such callsign
  Q_ENUM (Outcome)
  enum class Asked {Yes, Unsigned, HashedCall, Malformed, Duplicate, QueueFull};
  Q_ENUM (Asked)
  /* why an Outcome::Failed failed. The caller tells the operator ONCE per kind, and must be able
     to tell the kinds apart without reading `detail`, which can differ from one request to the
     next (it can hold the request's own URL). None for the three real outcomes. */
  enum class Trouble {None, NoSsl, Timeout, Network, TooLarge, HttpStatus, BadAnswer};
  Q_ENUM (Trouble)

  static constexpr char const * default_base_url () {return "https://www.9dx.cc";}

  FoxVerifier (QNetworkAccessManager * network_manager, QString const& user_agent, QObject * parent = nullptr);
  ~FoxVerifier ();

  static constexpr int max_servers = 4;
  /* the servers, best first: each trimmed and without its trailing slashes, empties and repeats
     dropped, at most max_servers kept; an empty list means the default alone */
  void set_base_urls (QStringList const& base_urls);
  QStringList base_urls () const {return base_urls_;}
  void set_base_url (QString const& base_url) {set_base_urls (QStringList {base_url});}
  QString base_url () const {return base_urls_.value (0);}
  static QStringList clean_urls (QStringList const& base_urls);

  /* call: as decoded, with or without the <> a resolved hashed call is printed in.
     slot_utc: the UTC start of the period the code was received in - date AND time; its time spec
     is ignored, the digits are taken as UTC. code: six digits. hz: where the Fox was, handed back. */
  Asked verify (QString const& call, QDateTime const& slot_utc, QString const& code, int hz);

  /* server: the base URL of the server that gave this answer - for Unknown the LAST one asked,
     every one before it having said the same */
  Q_SIGNAL void result (QString const& call, QDateTime const& slot_utc, int hz,
                        FoxVerifier::Outcome outcome, FoxVerifier::Trouble trouble, QString const& detail,
                        QString const& server) const;

  // the pieces, static so they can be tested without a network
  static QString bare_call (QString const& call);                       // "<VP2X/K1JT>" -> "VP2X/K1JT", "<...>" -> ""
  static QString url_for (QString const& base_url, QString const& call, QDateTime const& slot_utc, QString const& code);
  static Outcome parse (int http_status, QByteArray const& body);
  /* the date a decode's hhmmss belongs to, given the clock: today's, unless that time of day lies
     more than 12 hours in the future - then it was received before midnight and is yesterday's */
  static QDateTime slot_from_time (QTime const& hhmmss, QDateTime const& now_utc);

private:
  struct Question {QString call; QDateTime slot_utc; QString code; int hz; QString key; int server; QStringList servers;};
  void next ();
  void answered (Outcome outcome, Trouble trouble, QString const& detail);

  HttpFetch * fetch_;
  QString user_agent_;
  QStringList base_urls_;
  QList<Question> queue_;
  bool asking_ {false};
  QStringList asked_;                   // "call|slot|code" of the last questions that got (or await) an ANSWER, oldest first
};

#endif
