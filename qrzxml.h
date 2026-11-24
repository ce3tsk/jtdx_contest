#ifndef QRZXML_H
#define QRZXML_H

/* CE3TSK 2026-11-24 (TODO.md 3.-283, GRID_FILL_PLAN.md stage 3): QRZ.com's XML data - a call's grid, for a QSO that gave
   none (gridfill.h, the last source). The operator: "last and lowest priority access via paid subscription APIs like
   QRZ.com"; his subscription includes XML data (2026-11-23).

   - The XML service (https://xmldata.qrz.com/xml/current/), not the logbook API: it logs in with the QRZ.com username
     and password (loginfile.h: JTDX_qrz_xml.login, the owner's alone) and answers with a session key; a call is asked
     with that key. Both are POSTs - neither the password nor the key in a URL - through FileDownload's reply mode (its
     transports, WinHTTP on Windows; no redirect followed). JTDX_QRZ_XML for a test.
   - A session that ended (QRZ.com's "Session Timeout", "Invalid session key") is logged into again, once.
   - An account with no XML subscription ("non-subscriber"), or a login QRZ.com refused, asks no more until the login
     changes: it would only be refused again.
   - When it asks is MainWindow's (G6): as a QSO with no grid starts, once a call a run, never waited for.
   - What a grid must be (G3, as adif_grid.py): the record's call the one asked - a slashed call answered by another
     record is another station's place - and a grid QRZ.com did not only estimate from the country or a state (geoloc
     dxcc, state, none); the rest is gridfill.h's usable ().

   test_gridfill has the rules and a stand-in of the service on 127.0.0.1. */

#include <QDateTime>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include "FileDownload.hpp"

class QNetworkAccessManager;

namespace qrz_xml
{
  constexpr char const * url {"https://xmldata.qrz.com/xml/current/"};
  constexpr char const * api_env {"JTDX_QRZ_XML"};
  QString base ();   // the service, or JTDX_QRZ_XML's

  struct Login
  {
    QString user;
    QString password;
    bool empty () const {return user.trimmed ().isEmpty () || password.isEmpty ();}
    bool operator == (Login const& o) const {return user == o.user && password == o.password;}
    bool operator != (Login const& o) const {return !(*this == o);}
  };

  QByteArray login_body (Login const& login, QString const& agent);
  QByteArray lookup_body (QString const& key, QString const& call);

  struct Session
  {
    QString key;
    QString error;
    QString subscription;   // QRZ.com's SubExp: the date it ends, or "non-subscriber"
    bool subscriber () const {return !subscription.contains (QLatin1String {"non-subscriber"}, Qt::CaseInsensitive);}
    bool ended () const;     // the key no longer good: log in again
  };
  struct Record
  {
    QString call;
    QString grid;
    QString geoloc;
  };
  Session session (QByteArray const& xml);
  Record record (QByteArray const& xml);
  QString grid_of (QString const& asked, Record const& r);   // the grid a QSO with `asked` may take, or ""

  class Lookup
    : public QObject
  {
    Q_OBJECT

  public:
    enum class State {NoLogin, Idle, LoggedIn, NoSubscription, Refused, Failed};
    explicit Lookup (QObject * parent = nullptr);
    void configure (QNetworkAccessManager * manager, QString const& user_agent, QString const& agent);
    void set_login (Login const& login);   // another login: asked anew, every call again
    void ask (QString const& call);        // once a call while the login stays; one request at a time
    State state () const {return state_;}
    QString detail () const {return detail_;}             // QRZ.com's words, or the transport's
    QString subscription () const {return subscription_;}
    QDateTime logged_in () const {return logged_in_;}
    Q_SIGNAL void answered (QString const& call, QString const& grid) const;   // "" when none
    Q_SIGNAL void state_changed () const;

  private:
    void next ();
    void request (QByteArray const& body);
    void replied (int http_status, QByteArray const& body);
    void failed (FileDownload::Failure failure, QString const& detail);
    void set (State s, QString const& detail = QString {});
    void give_up (QString const& detail);   // (review) a failure: the call it was for dropped, the rest go on
    enum class Step {None, LoggingIn, LookingUp};
    QNetworkAccessManager * manager_ {nullptr};
    QString user_agent_;
    QString agent_;
    Login login_;
    QString key_;
    QStringList queue_;
    QSet<QString> asked_;
    FileDownload request_;
    Step step_ {Step::None};
    bool again_ {false};      // a session ended under this lookup: logged into again once
    bool stale_ {false};      // the login changed while a request was under way: its answer let go
    State state_ {State::NoLogin};
    QString detail_;
    QString subscription_;
    QDateTime logged_in_;
  };
}

#endif
