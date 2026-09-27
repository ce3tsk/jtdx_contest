// -*- Mode: C++ -*-

#ifndef LogQSO_H
#define LogQSO_H

#ifdef QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include <QScopedPointer>

#include "Radio.hpp"
#include "JTDXDateTime.h"

namespace Ui {
  class LogQSO;
}

class QSettings;
class Configuration;

class LogQSO : public QDialog
{
  Q_OBJECT;

public:
  explicit LogQSO(QSettings *, Configuration const *, JTDXDateTime * jtdxtime, QWidget *parent = 0);
  ~LogQSO();
  void initLogQSO(QString const& hisCall, QString const& hisGrid, QString mode,
                  QString const& rptSent, QString const& rptRcvd, QString const& distance,
                  QString const& name, QDateTime const& dateTimeOn,
                  QDateTime const& dateTimeOff, Radio::Frequency dialFreq, bool autologging,
                  /* CE3TSK: contest points, 0 outside a contest. Required rather than
                     defaulted so the compiler names every call site. */
                  int points);

public slots:
  void accept();

signals:
  void acceptQSO (QDateTime const& QSO_date_off, QString const& call, QString const& grid
                  , Radio::Frequency dial_freq, QString const& mode
                  , QString const& rpt_sent, QString const& rpt_received
                  , QString const& tx_power, QString const& comments
                  , QString const& name, QDateTime const& QSO_date_on
                  , QString const& eqslcomments, QByteArray const& myadif2);

protected:
  void hideEvent (QHideEvent *) override;
  void showEvent (QShowEvent *) override;   // CE3TSK: see restoreSavedGeometry ()

private:
  void loadSettings ();
  void storeSettings () const;

  QScopedPointer<Ui::LogQSO> ui;
  QSettings * m_settings;
  Configuration const * m_config;
  QString m_txPower;
  QString m_comments;
  QString m_eqslcomments;
  Radio::Frequency m_dialFreq;
  QString m_myCall;
  QString m_myGrid;
  QDateTime m_dateTimeOn;
  QDateTime m_dateTimeOff;
  bool m_send_to_eqsl;
  QString m_eqsl_username;
  QString m_eqsl_passwd;
  QString m_eqsl_nickname;
  QString m_tcp_server_name;
  uint m_tcp_server_port;
  uint m_eqsltimer;
  bool m_enable_tcp_connection;
  bool m_debug;
  JTDXDateTime * m_jtdxtime;
  /* CE3TSK 2026-09-26: the size and place the operator left, restored after the first show as the
     Wide Graph's are - see restoreSavedGeometry () in logqso.cpp. */
  void restoreSavedGeometry ();
  QByteArray m_savedGeometry;
  QSize m_savedMinHint;
  bool m_hadSavedGeometry {false};   // no geometry ever saved: the layout's own hint decides
  bool m_geometryQueued {false};      // the deferred restore has been asked for
  bool m_geometryRestored {false};    // ... and has run: only then may the geometry be saved
};

#endif // LogQSO_H
