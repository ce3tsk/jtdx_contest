#ifndef SPOTFEED_H
#define SPOTFEED_H

// CE3TSK 2026-11-16 (TODO.md 3.-260, RECEPTION_MAP_PLAN.md stage 2): the feed of mqtt.pskreporter.info, listened to -
// plain MQTT over TCP, port 1883 (decision 11), with our own small codec (mqttcodec.h): no MQTT library.
//
//   - ONE connection, narrow subscriptions (receptionspots.h has the topics): every spot of our band and mode - or,
//     reduced, our own spots and one in a hundred of the rest - and the details of the receivers asked for, each by its
//     own topic. Never everything.
//   - (the second review, TODO.md 3.-264) The receivers' details are asked for apart from the spots: in packets of
//     their own, 50 topics at most, at once from the moment the server has been asked for the spots - and let go again
//     (dropInfo ()) when their station reports no longer. A receiver's details the server refuses are that
//     receiver's alone: it is not asked about again, the connection stays.
//   - It only listens: nothing but CONNECT, SUBSCRIBE, UNSUBSCRIBE, PINGREQ and DISCONNECT is ever sent, and the
//     client's name is a random one - never the callsign.
//   - Polite when things go wrong: one try at a time; after a failure the next try 5 s later, then 10, 20 ... up to
//     5 minutes, each a little later by chance so that a server coming back is not met by everyone at once; 5 s again
//     once a connection has held for half a minute.
//   - A stream that is not MQTT, an answer that does not come, a subscription to the spots refused, silence: the
//     connection is dropped and tried again as above. Nothing of a broken stream is used.
//   - Everything read is untrusted: a spot is handed on only as far as its JSON gives each part in its kind
//     (parse ()); whether it may be kept is receptionspots.h's sound ().
//
// It runs in the thread it lives in (the window's), on the socket's own signals. test_spotfeed has the rules, against
// a broker of its own - no test ever reaches the real server.

#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include "mqttcodec.h"
#include "receptionspots.h"

class QTcpSocket;

class SpotFeed
  : public QObject
{
  Q_OBJECT

public:
  enum State {Off, Connecting, Live, Waiting};

  struct Timings
  {
    int keepAliveMs {60 * 1000};      // a PINGREQ this often
    int silenceMs {150 * 1000};       // nothing at all from the server for so long: gone
    int answerMs {15 * 1000};         // from the connection's start to the subscriptions' acknowledgement
    int firstRetryMs {5 * 1000};
    int lastRetryMs {300 * 1000};
    int settledMs {30 * 1000};        // a connection that held so long begins again at the first retry
    int chancePercent {20};           // a retry is up to so much later, by chance
  };

  explicit SpotFeed (QObject * parent = nullptr);
  ~SpotFeed ();

  static QString defaultHost ();      // mqtt.pskreporter.info
  static quint16 defaultPort ();      // 1883
  void setServer (QString const& host, quint16 port);
  QString host () const {return m_host;}
  quint16 port () const {return m_port;}
  void setTimings (Timings const& t);

  // what to listen to, and start listening if not yet: our band and mode (our names; receptionspots.h turns them into
  // the feed's), our call, and whether all spots or the reduced set. A change while live changes the subscriptions.
  // With nothing to listen to - no band, no mode, or one that no topic may hold - there is no connection at all.
  void listen (QString const& band, QString const& mode, QString const& ownCall, bool reduced);
  void stop ();                       // DISCONNECT and nothing more until the next listen ()
  void askInfo (QString const& call); // a receiver's details, once a call
  void dropInfo (QString const& call);   // (3.-264) ... wanted no longer: its subscription ended

  State state () const {return m_state;}
  int retryIn () const;               // seconds to the next try, while Waiting
  QString lastError () const {return m_error;}
  QStringList topics () const;        // what is subscribed to now, or would be
  quint64 spots () const {return m_spots;}        // spots handed on since the start
  quint64 refused () const {return m_refused;}    // messages whose JSON was no spot and no receiver's details

  // a spot of the feed's JSON: false unless every part it needs is there in its kind (a call a string, a frequency a
  // number ...). The time is t_tx where the feed gives it, else t.
  static bool parse (QByteArray const& json, reception_spots::Spot& spot);
  static bool parseInfo (QByteArray const& json, QString& call, reception_spots::Info& info);

Q_SIGNALS:
  void spot (reception_spots::Spot const& spot);
  void info (QString const& call, reception_spots::Info const& info);
  void infoGone (QString const& call);
  void stateChanged ();

private:
  void open ();
  void drop (QString const& why);     // the connection given up, the next try set
  void setState (State s);
  unsigned nextId ();
  void onConnected ();
  void onReadyRead ();
  void onKeepAlive ();
  void send (std::string const& bytes);
  void subscribeWanted ();
  void subscribeInfo (QStringList const& calls);
  bool asked () const;                // the server has been asked for the spots on this connection
  QStringList wanted () const;

  QString m_host;
  quint16 m_port;
  Timings m_t;
  QTcpSocket * m_socket;
  mqtt_codec::Reader m_reader;
  State m_state {Off};
  QString m_error;
  QString m_band, m_mode, m_ownCall;
  bool m_reduced {false};
  QStringList m_subscribed;           // the spot topics the server has been asked for on this connection
  QSet<QString> m_infoCalls;          // receivers whose details are asked for
  QSet<QString> m_infoRefused;        // (3.-264) ... and those the server would not give: not asked for again
  QHash<unsigned, QStringList> m_infoPackets;   // the packets that asked for details and are not acknowledged yet: their calls
  unsigned m_packetId {0};
  unsigned m_firstSubscribe {0};      // the packet whose acknowledgement makes the connection live
  int m_retryMs;
  QTimer m_retryTimer, m_answerTimer, m_keepAliveTimer;
  QElapsedTimer m_heard, m_live;
  quint64 m_spots {0};
  quint64 m_refused {0};
};

#endif
