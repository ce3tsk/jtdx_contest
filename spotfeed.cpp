// CE3TSK 2026-11-16 (TODO.md 3.-260, RECEPTION_MAP_PLAN.md stage 2): see spotfeed.h
#include "spotfeed.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRandomGenerator>
#include <QTcpSocket>

#include "moc_spotfeed.cpp"

namespace
{
  unsigned const keepAliveSeconds {180};   // what the server is told: we ping every minute, so three are missed before it lets go
  int const infoTopicsAPacket {50};        // (3.-264) receivers' details asked for in one packet, at most

  std::vector<std::string> list (QStringList const& topics)
  {
    std::vector<std::string> out;
    for (auto const& t : topics) out.push_back (t.toStdString ());
    return out;
  }
}

QString SpotFeed::defaultHost () {return QStringLiteral ("mqtt.pskreporter.info");}
quint16 SpotFeed::defaultPort () {return 1883;}

SpotFeed::SpotFeed (QObject * parent)
  : QObject {parent}
  , m_host {defaultHost ()}
  , m_port {defaultPort ()}
  , m_socket {new QTcpSocket {this}}
  , m_retryMs {m_t.firstRetryMs}
{
  m_retryTimer.setSingleShot (true);
  m_answerTimer.setSingleShot (true);
  connect (&m_retryTimer, &QTimer::timeout, this, &SpotFeed::open);
  connect (&m_answerTimer, &QTimer::timeout, this, [this] {drop (QStringLiteral ("no answer from the server"));});
  connect (&m_keepAliveTimer, &QTimer::timeout, this, &SpotFeed::onKeepAlive);
  connect (m_socket, &QTcpSocket::connected, this, &SpotFeed::onConnected);
  connect (m_socket, &QTcpSocket::readyRead, this, &SpotFeed::onReadyRead);
  connect (m_socket, &QTcpSocket::disconnected, this, [this] {if (Off != m_state && Waiting != m_state) drop (QStringLiteral ("the server closed the connection"));});
  // (the review of 2026-11-16, TODO.md 3.-261) errorOccurred came with Qt 5.15; before it the signal was error, a
  // name the socket has a function of as well
#if QT_VERSION >= QT_VERSION_CHECK (5, 15, 0)
  auto const failed = &QAbstractSocket::errorOccurred;
#else
  auto const failed = static_cast<void (QAbstractSocket::*) (QAbstractSocket::SocketError)> (&QAbstractSocket::error);
#endif
  connect (m_socket, failed, this, [this] (QAbstractSocket::SocketError) {
      if (Off != m_state && Waiting != m_state) drop (m_socket->errorString ());
    });
}

SpotFeed::~SpotFeed ()
{
  m_state = Off;   // the socket's last signals find nothing to do
}

void SpotFeed::setServer (QString const& host, quint16 port)
{
  m_host = host;
  m_port = port;
}

void SpotFeed::setTimings (Timings const& t)
{
  m_t = t;
  m_retryMs = m_t.firstRetryMs;
}

// the spot topics for what is to be listened to now; none when our band or mode is nothing a topic may hold
QStringList SpotFeed::wanted () const
{
  QStringList topics;
  std::string const band {m_band.toStdString ()}, mode {m_mode.toStdString ()}, call {m_ownCall.toStdString ()};
  if (!m_reduced)
    {
      QString const all {QString::fromStdString (reception_spots::bandTopic (band, mode))};
      if (!all.isEmpty ()) topics << all;
    }
  else
    {
      QString const own {QString::fromStdString (reception_spots::ownTopic (band, mode, call))};
      QString const sample {QString::fromStdString (reception_spots::sampleTopic (band, mode))};
      if (!own.isEmpty ()) topics << own;
      if (!sample.isEmpty ()) topics << sample;
    }
  return topics;
}

QStringList SpotFeed::topics () const
{
  QStringList all {wanted ()};
  for (auto const& call : m_infoCalls) all << QString::fromStdString (reception_spots::infoTopic (call.toStdString ()));
  return all;
}

void SpotFeed::listen (QString const& band, QString const& mode, QString const& ownCall, bool reduced)
{
  // (the review of 2026-11-16, TODO.md 3.-261) "Reduced data" alone leaves the receivers asked about as they are: the
  // band's, the mode's and the call's are the same, and the board, which asks about each once, would never ask again
  bool const other {band != m_band || mode != m_mode || ownCall != m_ownCall};
  bool const changed {other || reduced != m_reduced};
  QStringList infoBefore;
  if (other) for (auto const& call : m_infoCalls) infoBefore << QString::fromStdString (reception_spots::infoTopic (call.toStdString ()));
  m_band = band; m_mode = mode; m_ownCall = ownCall; m_reduced = reduced;
  if (other)
    {
      m_infoCalls.clear ();   // the receivers asked about were another band's, mode's or call's
      m_infoRefused.clear ();
      for (auto& calls : m_infoPackets) calls.clear ();   // (3.-264) an acknowledgement still to come is no word on the new band's receivers
    }
  if (wanted ().isEmpty ())   // nothing a topic may hold - no band, no mode: there is nothing to ask the server for
    {
      stop ();
      return;
    }
  if (Off == m_state)
    {
      m_retryMs = m_t.firstRetryMs;
      open ();
      return;
    }
  // a connection being made subscribes to what is wanted once the server has accepted it; one that has done so already
  // is told of the change as a live one is
  if (!changed || !asked ()) return;
  QStringList const now {wanted ()};
  QStringList gone {infoBefore};
  for (auto const& t : m_subscribed) if (!now.contains (t)) gone << t;
  // (3.-264) what is left - the receivers asked about among it - in packets of 50 topics, as it was asked for
  for (int at = 0; at < gone.size (); at += infoTopicsAPacket) send (mqtt_codec::unsubscribe (nextId (), list (gone.mid (at, infoTopicsAPacket))));
  QStringList more;
  for (auto const& t : now) if (!m_subscribed.contains (t)) more << t;
  m_subscribed = now;
  if (!more.isEmpty ()) send (mqtt_codec::subscribe (nextId (), list (more)));
}

void SpotFeed::stop ()
{
  m_retryTimer.stop (); m_answerTimer.stop (); m_keepAliveTimer.stop ();
  State const was {m_state};
  m_state = Off;   // before the socket is touched: its signals are then no failure
  if (QAbstractSocket::ConnectedState == m_socket->state ())   // a goodbye, and the connection closed once it is on its way
    {
      m_socket->write (mqtt_codec::disconnect ().data (), 2);
      m_socket->flush ();
      m_socket->disconnectFromHost ();
    }
  else m_socket->abort ();
  m_subscribed.clear ();
  m_infoCalls.clear ();
  m_infoRefused.clear ();
  m_infoPackets.clear ();
  m_error.clear ();
  if (Off != was) Q_EMIT stateChanged ();
}

// (the second review, TODO.md 3.-264) The server has been asked for the spots on this connection: from then on what
// is wanted is asked for at once - a spot may come before the acknowledgement (3.-261), and with it the window's
// question about its receiver. Before that there is nothing to ask on: what is wanted goes out once the server has
// accepted the connection (subscribeWanted ()).
bool SpotFeed::asked () const
{
  return Live == m_state || (Connecting == m_state && m_firstSubscribe);
}

// Receivers' details asked of the server: 50 topics a packet at most, each packet's calls kept until it is
// acknowledged - the acknowledgement says which of them the server gives.
void SpotFeed::subscribeInfo (QStringList const& calls)
{
  for (int at = 0; at < calls.size (); at += infoTopicsAPacket)
    {
      QStringList const some {calls.mid (at, infoTopicsAPacket)};
      QStringList topics;
      for (auto const& call : some) topics << QString::fromStdString (reception_spots::infoTopic (call.toStdString ()));
      unsigned const id {nextId ()};
      m_infoPackets.insert (id, some);
      send (mqtt_codec::subscribe (id, list (topics)));
    }
}

void SpotFeed::askInfo (QString const& call)
{
  QString const c {call.toUpper ()};
  QString const topic {QString::fromStdString (reception_spots::infoTopic (c.toStdString ()))};
  if (topic.isEmpty () || m_infoCalls.contains (c) || m_infoRefused.contains (c)) return;
  m_infoCalls.insert (c);
  // (3.-264) at once from the moment the spots are asked for - it was: only once they were acknowledged, and what
  // the window asked in between was noted and never asked of the server
  if (asked ()) subscribeInfo (QStringList {c});
}

// (the second review, TODO.md 3.-264) A receiver's details wanted no longer - its station reports no longer
// (receptionspots.h Board::prune ()): its subscription is ended. They only ever grew: one for every station that had
// heard us since the map was opened, all asked for again at every reconnect.
void SpotFeed::dropInfo (QString const& call)
{
  QString const c {call.toUpper ()};
  m_infoRefused.remove (c);   // a station that comes back may be asked about once more
  if (!m_infoCalls.remove (c)) return;
  if (asked ()) send (mqtt_codec::unsubscribe (nextId (), {reception_spots::infoTopic (c.toStdString ())}));
}

int SpotFeed::retryIn () const
{
  return Waiting == m_state && m_retryTimer.isActive () ? (m_retryTimer.remainingTime () + 999) / 1000 : 0;
}

// a packet's number: 1 to 65535, round again
unsigned SpotFeed::nextId ()
{
  m_packetId = m_packetId % 65535 + 1;
  return m_packetId;
}

void SpotFeed::setState (State s)
{
  if (s == m_state) return;
  m_state = s;
  Q_EMIT stateChanged ();
}

void SpotFeed::open ()
{
  m_retryTimer.stop ();
  m_reader.reset ();
  m_subscribed.clear ();
  m_infoPackets.clear ();
  m_firstSubscribe = 0;
  m_socket->abort ();
  setState (Connecting);
  m_answerTimer.start (m_t.answerMs);
  m_socket->connectToHost (m_host, m_port);
}

void SpotFeed::drop (QString const& why)
{
  if (Off == m_state) return;
  m_answerTimer.stop (); m_keepAliveTimer.stop ();
  bool const settled {Live == m_state && m_live.isValid () && m_live.elapsed () >= m_t.settledMs};
  m_state = Waiting;   // before the socket is touched: its signals are then no second failure
  m_socket->abort ();
  m_error = why;
  if (settled) m_retryMs = m_t.firstRetryMs;
  int const wait {m_retryMs + (m_t.chancePercent > 0 ? static_cast<int> (QRandomGenerator::global ()->bounded (static_cast<quint32> (m_retryMs / 100 * m_t.chancePercent + 1))) : 0)};
  m_retryMs = qMin (m_retryMs * 2, m_t.lastRetryMs);
  m_retryTimer.start (wait);
  Q_EMIT stateChanged ();
}

void SpotFeed::send (std::string const& bytes)
{
  m_socket->write (bytes.data (), static_cast<qint64> (bytes.size ()));
}

void SpotFeed::onConnected ()
{
  if (Connecting != m_state) return;
  m_heard.start ();
  // a name of chance: the server needs one, and it is not to be the callsign
  QString const name {QStringLiteral ("jtdxc-%1").arg (QRandomGenerator::global ()->generate (), 8, 16, QLatin1Char ('0'))};
  send (mqtt_codec::connect (name.toStdString (), keepAliveSeconds));
}

void SpotFeed::subscribeWanted ()
{
  QStringList const all {wanted ()};
  if (all.isEmpty ())   // cannot be: listen () connects only with something to ask for
    {
      stop ();
      return;
    }
  m_subscribed = all;
  m_firstSubscribe = nextId ();
  send (mqtt_codec::subscribe (m_firstSubscribe, list (all)));
  // (the second review, TODO.md 3.-264) The receivers asked about before this connection follow in packets of their
  // own: they were in the one packet with the spots - hundreds of topics after an evening on a band - and a single one
  // of them refused took the connection with it, at this try and at every one after it.
  QStringList calls {m_infoCalls.values ()};
  calls.sort ();
  subscribeInfo (calls);
}

void SpotFeed::onReadyRead ()
{
  if (Off == m_state || Waiting == m_state) return;
  QByteArray const bytes {m_socket->readAll ()};
  m_heard.start ();
  m_reader.feed (bytes.constData (), static_cast<std::size_t> (bytes.size ()));
  mqtt_codec::Packet p;
  while (m_reader.next (p))
    {
      switch (p.type)
        {
        case mqtt_codec::Connack:
          if (Connecting != m_state || m_firstSubscribe) {drop (QStringLiteral ("the server answered out of turn")); return;}
          if (p.returnCode) {drop (QStringLiteral ("the server refused the connection (%1)").arg (p.returnCode)); return;}
          subscribeWanted ();
          break;
        case mqtt_codec::Suback:
          {
            // (the second review, TODO.md 3.-264) the answer to a packet that asked for receivers' details: one the
            // server will not give is not asked about again - and that is all. The spots are another matter: without
            // them there is no feed, and their refusal ends the connection as it always did.
            auto const info = m_infoPackets.find (p.packetId);
            if (info != m_infoPackets.end ())
              {
                QStringList const calls {info.value ()};
                m_infoPackets.erase (info);
                for (int i = 0; i < calls.size () && i < static_cast<int> (p.granted.size ()); ++i)
                  if (p.granted[static_cast<std::size_t> (i)])
                    {
                      m_infoCalls.remove (calls[i]);
                      m_infoRefused.insert (calls[i]);
                    }
                break;
              }
            for (int g : p.granted) if (g) {drop (QStringLiteral ("the server refused a subscription")); return;}
            if (Connecting == m_state && p.packetId == m_firstSubscribe)
              {
                m_answerTimer.stop ();
                m_live.start ();
                m_keepAliveTimer.start (m_t.keepAliveMs);
                m_error.clear ();
                setState (Live);
              }
          }
          break;
        case mqtt_codec::Publish:
          {
            // (the review of 2026-11-16, TODO.md 3.-261) A server may send what a subscription matches before it
            // acknowledges the subscription (MQTT 3.1.1, 3.8.4) - the receivers' retained details at a reconnect: taken
            // from the moment the subscription is asked. Before that nothing was asked for.
            if (!asked ()) {drop (QStringLiteral ("the server answered out of turn")); return;}
            QString const topic {QString::fromStdString (p.topic)};
            QByteArray const payload {p.payload.data (), static_cast<int> (p.payload.size ())};
            if (topic.startsWith (QLatin1String ("pskr/filter/")))
              {
                reception_spots::Spot s;
                if (parse (payload, s)) {++m_spots; Q_EMIT spot (s);} else ++m_refused;
              }
            else if (topic.startsWith (QLatin1String ("pskr/rxinfo/")))
              {
                if (payload.isEmpty ()) Q_EMIT infoGone (topic.mid (12).replace (QLatin1Char ('.'), QLatin1Char ('/')));
                else
                  {
                    QString call;
                    reception_spots::Info i;
                    if (parseInfo (payload, call, i)) Q_EMIT info (call, i); else ++m_refused;
                  }
              }
            else ++m_refused;
          }
          break;
        default:   // UNSUBACK, PINGRESP: taken
          break;
        }
      if (Off == m_state || Waiting == m_state) return;   // a slot of ours stopped the feed
    }
  if (m_reader.broken ()) drop (QString::fromStdString (m_reader.error ()));
}

void SpotFeed::onKeepAlive ()
{
  if (Live != m_state) return;
  if (m_heard.isValid () && m_heard.elapsed () > m_t.silenceMs)
    {
      drop (QStringLiteral ("the server has gone silent"));
      return;
    }
  send (mqtt_codec::pingreq ());
}

bool SpotFeed::parse (QByteArray const& json, reception_spots::Spot& spot)
{
  QJsonParseError error;
  QJsonDocument const doc {QJsonDocument::fromJson (json, &error)};
  if (QJsonParseError::NoError != error.error || !doc.isObject ()) return false;
  QJsonObject const o {doc.object ()};
  auto text = [&o] (char const * key, std::string& out) {
    QJsonValue const v {o.value (QLatin1String (key))};
    if (!v.isString ()) return false;
    out = v.toString ().toStdString ();
    return true;
  };
  // (3.-264) the sender's locator ("sl") is not read: nothing has used it since 3.-261
  reception_spots::Spot s;
  if (!text ("b", s.band) || !text ("md", s.mode) || !text ("sc", s.sender) || !text ("rc", s.receiver) || !text ("rl", s.receiverLocator)) return false;
  QJsonValue const f {o.value (QLatin1String ("f"))}, rp {o.value (QLatin1String ("rp"))}, t {o.value (QLatin1String ("t"))}, tx {o.value (QLatin1String ("t_tx"))};
  if (!f.isDouble () || !(t.isDouble () || tx.isDouble ())) return false;
  double const frequency {f.toDouble ()}, time {tx.isDouble () ? tx.toDouble () : t.toDouble ()};
  if (!(frequency >= 0 && frequency < 1e15) || !(time >= 0 && time < 1e12)) return false;
  s.frequency = static_cast<long long> (frequency);
  s.time = static_cast<long long> (time);
  if (rp.isDouble ())
    {
      double const snr {rp.toDouble ()};
      if (!(snr >= -1000 && snr <= 1000)) return false;
      s.hasSnr = true;
      s.snr = static_cast<int> (snr);
    }
  else if (!rp.isNull () && !rp.isUndefined ()) return false;
  spot = s;
  return true;
}

bool SpotFeed::parseInfo (QByteArray const& json, QString& call, reception_spots::Info& info)
{
  QJsonParseError error;
  QJsonDocument const doc {QJsonDocument::fromJson (json, &error)};
  if (QJsonParseError::NoError != error.error || !doc.isObject ()) return false;
  QJsonObject const o {doc.object ()};
  QJsonValue const c {o.value (QLatin1String ("call"))};
  if (!c.isString () || !reception_spots::goodCall (c.toString ().toStdString ())) return false;
  auto text = [&o] (char const * key) {
    QJsonValue const v {o.value (QLatin1String (key))};
    return v.isString () ? v.toString ().left (200).toStdString () : std::string {};   // what a stranger typed, cut to a length a tooltip can hold
  };
  call = c.toString ().toUpper ();
  info.decoder = text ("decoder");
  info.antenna = text ("antenna");
  return true;
}
