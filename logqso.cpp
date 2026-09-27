#include "logqso.h"

#include <QTcpSocket>
#include <QString>
#include <QSettings>
#include <QTimer>   /* CE3TSK: the queued geometry restore */
#include "geometryrestore.h"   /* CE3TSK 2026-09-26: the restore rule, shared */
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

#include "JTDXMessageBox.hpp"

#include "logbook/adif.h"
#include "Configuration.hpp"
#include "Bands.hpp"

#include "ui_logqso.h"
#include "moc_logqso.cpp"

LogQSO::LogQSO(QSettings * settings, Configuration const * config, JTDXDateTime * jtdxtime, QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::LogQSO)
  , m_settings (settings)
  , m_config {config}
  , m_jtdxtime {jtdxtime}
{
  ui->setupUi(this);
  ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("&OK"));
  ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("&Cancel"));
//  setWindowTitle("JTDX " + versnumber.simplified () + " - Log QSO");
  setWindowTitle(QCoreApplication::applicationName () + " v" + QCoreApplication::applicationVersion () + " - Log QSO");

  loadSettings ();
}

LogQSO::~LogQSO ()
{
}

void LogQSO::loadSettings ()
{
  m_settings->beginGroup ("LogQSO");
  /* CE3TSK 2026-09-26: the size and place the operator left are applied here AND, properly, after
     the first show - see restoreSavedGeometry (). */
  m_savedGeometry = m_settings->value ("geometry").toByteArray ();
  m_savedMinHint = m_settings->value ("geometryMinHint").toSize ();
  /* A geometry Qt refuses counts as none: restoreGeometry () bails out when either screen - the one
     it was saved on, the one now - is more than 25 % wider than the other (geometryrestore.h).
     Measured: a 2240 px screen's geometry on a 1600 px one was refused, and without the line below
     the dialog opened at the .ui's 374 px with its fields crushed. As for a fresh profile, the size
     and place the operator then uses on this screen are what the next hide saves - the refused
     geometry is replaced, as the main window's and the Wide Graph's are. */
  m_hadSavedGeometry = JTDX::restore_grown_geometry (this, m_savedGeometry, QSize {});   // as saved: the growth rule waits for the settled layout
  /* CE3TSK: the .ui rect (374px) is narrower than the layout wants, so the date/time and
     band fields were crushed and lost characters. sizeHint() is the width the layout
     actually needs and it tracks the application font, so grow to it - when no saved geometry
     was restored (a fresh profile, or one Qt refused).
     2026-09-26: ONLY there. It used to apply to a saved size too, and sizeHint () is the
     PREFERRED size: a dialog the operator had made smaller came back at 525x263 every time
     (measured: left at 480x240). A size the operator chose now prevails, as the Wide Graph's
     does since the same day. */
  if (!m_hadSavedGeometry) resize (size ().expandedTo (sizeHint ()));
  ui->cbTxPower->setChecked (m_settings->value ("SaveTxPower", false).toBool ());
  ui->cbComments->setChecked (m_settings->value ("SaveComments", false).toBool ());
  ui->cbEqslComments->setChecked (m_settings->value ("SaveEQSLComments", false).toBool ());
  m_txPower = m_settings->value ("TxPower", "").toString ();
  m_comments = m_settings->value ("LogComments", "").toString();
  m_eqslcomments = m_settings->value ("LogEQSLComments", "").toString();
  m_settings->endGroup ();
}

void LogQSO::storeSettings () const
{
  m_settings->beginGroup ("LogQSO");
  /* CE3TSK 2026-09-26: the geometry only once the saved one has been put back after a show - a
     hide before that would write the constructor's size over the one the operator left. */
  if (m_geometryRestored)
    {
      m_settings->setValue ("geometry", saveGeometry ());
      m_settings->setValue ("geometryMinHint", minimumSizeHint ());
    }
  m_settings->setValue ("SaveTxPower", ui->cbTxPower->isChecked ());
  m_settings->setValue ("SaveComments", ui->cbComments->isChecked ());
  m_settings->setValue ("SaveEQSLComments", ui->cbEqslComments->isChecked ());
  m_settings->setValue ("TxPower", m_txPower);
  m_settings->setValue ("LogComments", m_comments);
  m_settings->setValue ("LogEQSLComments", m_eqslcomments);
  m_settings->endGroup ();
}

void LogQSO::initLogQSO(QString const& hisCall, QString const& hisGrid, QString mode,
                        QString const& rptSent, QString const& rptRcvd, QString const& distance,
                        QString const& name, QDateTime const& dateTimeOn, QDateTime const& dateTimeOff,
                        Radio::Frequency dialFreq, bool autologging, int points)
{
  m_send_to_eqsl=m_config->send_to_eqsl();
  ui->call->setText(hisCall);
  ui->grid->setText(hisGrid);
  ui->name->setText(name);
  ui->txPower->setText("");
  ui->comments->setText("");
  ui->eqslcomments->setText("");
  ui->lab11->setVisible(m_send_to_eqsl);
  ui->eqslcomments->setVisible(m_send_to_eqsl);
  ui->cbEqslComments->setVisible(m_send_to_eqsl);
  if (ui->cbTxPower->isChecked ()) ui->txPower->setText(m_txPower);
  if (ui->cbComments->isChecked ()) ui->comments->setText(m_comments);
  if (ui->cbEqslComments->isChecked ()) ui->eqslcomments->setText(m_eqslcomments);
  QString t="";
  if(m_config->report_in_comments()) {
    t=mode;
    if(rptSent!="") t+="  Sent: " + rptSent;
    if(rptRcvd!="") t+="  Rcvd: " + rptRcvd;
    ui->comments->setText(t);
  }
  if(m_config->distance_in_comments()) {
    /* CE3TSK: in a contest the comment is headed by the contest and the year taken from the
       QSO itself, not from the clock, so that a log edited or replayed later still reads
       correctly. The distance then follows without its own label. */
    QString const contest = m_config->special_op_name();
    QString dist;
    if(contest.isEmpty()) {
      dist = "Distance: " + distance;
    } else {
      dist = contest + " " + QString::number(dateTimeOn.date().year());
      /* a QSO without a grid has no distance, so do not leave the separator dangling */
      if(!distance.isEmpty()) dist += " - " + distance;
      /* CE3TSK: points are 0 when the distance is not known, and are not printed then */
      if(points > 0) dist += " - " + QString::number(points) + (points == 1 ? " point" : " points");
    }
    if(t.isEmpty()) t=dist;
    else t+="  " + dist;
    ui->comments->setText(t);
  }
  if(m_config->log_as_RTTY() and mode.left(3)=="JT9") mode="RTTY";
  ui->mode->setText(mode);
  ui->sent->setText(rptSent);
  ui->rcvd->setText(rptRcvd);
  ui->start_date_time->setDateTime (dateTimeOn);
  ui->end_date_time->setDateTime (dateTimeOff);
  m_dialFreq=dialFreq;
  m_myCall=m_config->my_callsign();
  m_myGrid=m_config->my_grid();
  m_tcp_server_name=m_config->tcp_server_name();
  m_tcp_server_port=m_config->tcp_server_port();
  m_enable_tcp_connection=m_config->enable_tcp_connection();
  m_debug=m_config->write_decoded_debug();
  ui->band->setText(m_config->bands ()->find (dialFreq));

  if(!autologging) {
	 /* CE3TSK 2026-09-26, the operator: the dialog must open in front, so that it stays
	    visible. show () alone neither raises a dialog that is already open nor brings one over
	    another window of the program - measured: with the Wide Graph clicked over an open Log
	    QSO, asking for the dialog again left it under the Wide Graph. The dialog only stays
	    above the main window, its parent. So it is raised, and a minimized one is restored
	    (show () leaves it minimized) - only the minimized flag is cleared, so a maximized one
	    comes back maximized. A dialog ALREADY OPEN is not activated: the automatic prompt to log
	    (MainWindow's logQSOTimer) arrives while the operator may be typing, and with the
	    keyboard taken Enter would log the QSO and Esc cancel it (review 2026-09-26). A dialog
	    that was closed is still given the keyboard by the window manager as it is mapped -
	    measured, as the build before this change did too; left so. */
	 setWindowState (windowState () & ~Qt::WindowMinimized);
	 show ();
	 raise ();
  }
  else {
	 accept();
  }
}

void LogQSO::accept()
{
  QString hisCall,hisGrid,mode,rptSent,rptRcvd,time,band;
  QString comments,eqslcomments,name;
  hisCall=ui->call->text();
  hisGrid=ui->grid->text();
  mode=ui->mode->text();
  rptSent=ui->sent->text();
  rptRcvd=ui->rcvd->text();
  m_dateTimeOn = ui->start_date_time->dateTime ();
  m_dateTimeOff = ui->end_date_time->dateTime ();
  band=ui->band->text();
  name=ui->name->text();
  m_txPower=ui->txPower->text();
  comments=ui->comments->text();
  m_comments=comments;
  eqslcomments=ui->eqslcomments->text();
  m_eqslcomments=eqslcomments;
  QString strDialFreq(QString::number(m_dialFreq / 1.e6,'f',6));

  //Log this QSO to ADIF file "wsjtx_log.adi"
  QString filename = "wsjtx_log.adi";  // TODO allow user to set
  ADIF adifile;
  auto adifilePath = QDir {QStandardPaths::writableLocation (QStandardPaths::DataLocation)}.absoluteFilePath ("wsjtx_log.adi");
  adifile.init(adifilePath);
  if (!adifile.addQSOToFile(hisCall,hisGrid,mode,rptSent,rptRcvd,m_dateTimeOn,m_dateTimeOff,band,comments,name,strDialFreq,m_myCall,m_myGrid,m_txPower,m_send_to_eqsl))
  {
      JTDXMessageBox::information_message(0,"","Cannot open file \"" + adifilePath + "\".");
   }

  /* CE3TSK: a contest keeps its own log beside the normal one, named for the contest and its
     period. Everything a contest infers - worked before, multipliers, highlighting, the
     ranking and the score - is read back from this file, so that a grid field or a callsign
     worked in an earlier running does not count against this one.

     Written in addition to the normal log, never instead of it. A failure here is reported as
     loudly as one on the main log: silent divergence would leave the score quietly wrong with
     nothing to say so. */
  QString const contestFile = contest_log_filename (m_config->special_op_id (), m_dateTimeOn);
  if (!contestFile.isEmpty ())
    {
      ADIF contestAdi;
      auto contestPath = QDir {QStandardPaths::writableLocation (QStandardPaths::DataLocation)}.absoluteFilePath (contestFile);
      contestAdi.init (contestPath);
      if (!contestAdi.addQSOToFile (hisCall,hisGrid,mode,rptSent,rptRcvd,m_dateTimeOn,m_dateTimeOff,band,comments,name,strDialFreq,m_myCall,m_myGrid,m_txPower,m_send_to_eqsl))
        {
          JTDXMessageBox::information_message(0,"","Cannot open contest log \"" + contestPath + "\".");
        }
    }

//Log this QSO to file "wsjtx.log"
  static QFile f {QDir {QStandardPaths::writableLocation (QStandardPaths::DataLocation)}.absoluteFilePath ("wsjtx.log")};
  if(!f.open(QIODevice::Text | QIODevice::Append)) {
    JTDXMessageBox::information_message(0,"","Cannot open file \"" + f.fileName () + "\" for append:" + f.errorString ());
  } else {
    QString logEntry=m_dateTimeOn.date().toString("yyyy-MM-dd,") +
      m_dateTimeOn.time().toString("hh:mm:ss,") + 
      m_dateTimeOff.date().toString("yyyy-MM-dd,") +
      m_dateTimeOff.time().toString("hh:mm:ss,") + hisCall + "," +
      hisGrid + "," + strDialFreq + "," + mode +
      "," + rptSent + "," + rptRcvd + "," + m_txPower +
      "," + comments + "," + name;
    QTextStream out(&f);
    out << logEntry <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif

    f.close();
  }

//Clean up and finish logging
    QString myadif;
    QByteArray myadif2;
    myadif="<BAND:" + QString::number(band.length()) + ">" + band;
    myadif+=" <STATION_CALLSIGN:" + QString::number(m_myCall.length()) + ">" + m_myCall;
    if (m_myGrid.length() > 7) {
      myadif+=" <MY_GRIDSQUARE:8>" + m_myGrid.left(8);
    }
    else if (m_myGrid.length() > 3) {
      myadif+=" <MY_GRIDSQUARE:" + QString::number(m_myGrid.length()) + ">" + m_myGrid;
    }
    myadif+=" <CALL:" + QString::number(hisCall.length()) + ">" + hisCall;
    myadif+=" <FREQ:" + QString::number(strDialFreq.length()) + ">" + strDialFreq;
    if (mode == "FT4" || mode == "FT2") myadif+=" <MODE:4>MFSK <SUBMODE:"  + QString::number(mode.length()) + ">" + mode;   // CE3TSK: FT2 too
    else myadif+=" <MODE:"  + QString::number(mode.length()) + ">" + mode;
    myadif+=" <QSO_DATE:8>" + m_dateTimeOn.date().toString("yyyyMMdd");
    myadif+=" <TIME_ON:6>" + m_dateTimeOn.time().toString("hhmmss");
    myadif+=" <QSO_DATE_OFF:8>" + m_dateTimeOff.date().toString("yyyyMMdd");
    myadif+=" <TIME_OFF:6>" + m_dateTimeOff.time().toString("hhmmss");
    myadif+=" <RST_SENT:" + QString::number(rptSent.length()) + ">" + rptSent;
    myadif+=" <RST_RCVD:" + QString::number(rptRcvd.length()) + ">" + rptRcvd;
    if (m_txPower.length() >0) {
       myadif+=" <TX_PWR:" + QString::number(m_txPower.length()) + ">" + m_txPower;
    }
    if (hisGrid.length() > 3) {
      myadif+=" <GRIDSQUARE:" + QString::number(hisGrid.length()) + ">" + hisGrid;
    }
    if (name.length() > 0) {
      myadif+=" <NAME:" + QString::number(name.length()) + ">" + name;
    }
    if (comments.length() > 0) {
      myadif+=" <COMMENT:" + QString::number(comments.length()) + ">" + comments;
    }
    if (m_send_to_eqsl) {
      myadif+=" <EQSL_QSL_SENT:1>Y <EQSL_QSLSDATE:8>" + m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd");
    }
    myadif+=" <EOR> ";
    myadif2 = myadif.trimmed().toUtf8();

  Q_EMIT acceptQSO (m_dateTimeOff, hisCall, hisGrid, m_dialFreq, mode, rptSent, rptRcvd, m_txPower, comments, name, m_dateTimeOn, eqslcomments, myadif2);
  
  myadif="<command:3>Log <parameters:" + QString::number(myadif.length()) + "> " + myadif;
  myadif2 = myadif.toUtf8();
  if (m_enable_tcp_connection){
    QTcpSocket socket;
    static QFile f2 {QDir {QStandardPaths::writableLocation (QStandardPaths::DataLocation)}.absoluteFilePath ("tcptrace.txt")};
    bool fopen = false;
    if(m_debug) { 
      if(f2.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) fopen = true;
      else {
      JTDXMessageBox::warning_message (0, "", " File Open Error "
                                  , tr ("Cannot open \"%1\" for append: %2")
                                  .arg (f2.fileName ()).arg (f2.errorString ()));
      }
    }
    if(fopen)  { QTextStream out2(&f2); out2 << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss.zzz") << "(" << m_jtdxtime->GetOffset() << ")" << " Connecting to " << m_tcp_server_name << ":" << m_tcp_server_port <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif
 }
    socket.connectToHost(m_tcp_server_name, m_tcp_server_port);
    if (socket.waitForConnected(1000)) {
      socket.write(myadif2);
      if(fopen) { QTextStream out2(&f2); out2 << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss.zzz") << "(" << m_jtdxtime->GetOffset() << ")" << " Host connected, sent message: " << myadif2 <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif
 }
      if (socket.waitForReadyRead(1000)){
        myadif2 = socket.readAll();
        if(fopen) { QTextStream out2(&f2); out2 << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss.zzz") << "(" << m_jtdxtime->GetOffset() << ")" << " Received response from host: " << myadif2 <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif
 }
        if (myadif2.left(3) == "NAK") {
          JTDXMessageBox::critical_message(0, "",myadif2 + " QSO data rejected by external software");
        }
      } else {
      if(fopen) { QTextStream out2(&f2); out2 << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss.zzz") << "(" << m_jtdxtime->GetOffset() << ")" << " Getting response from host is timed out" <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif
 }
      }  
      socket.close();
    } else {
      if(fopen) { QTextStream out2(&f2); out2 << m_jtdxtime->currentDateTimeUtc2().toString("yyyyMMdd_hhmmss.zzz") << "(" << m_jtdxtime->GetOffset() << ")" << " Host connection timed out" <<
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                 endl;
#else
                 Qt::endl;
#endif
 }
      JTDXMessageBox::critical_message(0, "", "TCP QSO data transfer: " + socket.errorString());
    }
    if(fopen) f2.close();
  }
  QDialog::accept();
}

/* CE3TSK 2026-09-26: the operator's own size and place, restored where they stick - the Wide
   Graph's reasoning (widegraph.cpp, restoreSavedGeometry), which this follows. In
   the constructor the layout has not settled: the eQSL row is still there (initLogQSO hides it
   when eQSL is off), and a window manager may place a dialog over its parent when it is first
   mapped. So the saved geometry is applied AGAIN, queued after the first show. The growth rule is
   the main window's (restoreMainGeometry, 2026-09-15): grow only by as much as the layout's
   minimum has RISEN since the geometry was saved - which is what a larger application font does -
   and never past that minimum - one function for the three windows, geometryrestore.h. */
void LogQSO::showEvent (QShowEvent * e)
{
  QDialog::showEvent (e);
  if (!m_geometryQueued)
    {
      m_geometryQueued = true;   // once per run: a size set during the session is not undone
      QTimer::singleShot (0, this, [this] { restoreSavedGeometry (); });   // by the next show
    }
}

void LogQSO::restoreSavedGeometry ()
{
  m_geometryRestored = true;   // from here the window is what the operator sees - saved on hide
  if (!m_hadSavedGeometry)
    {
      /* None saved, or refused: the height the layout needs NOW. The constructor measured it with
         the eQSL row still in the layout, and a window does not shrink when a row goes, so with
         eQSL off the dialog kept an empty band the height of that row (review 2026-09-26; 264 px
         against 232, measured). Only the height: the constructor's width holds every field (525 px
         here), while sizeHint () at this point asks for 952 px - measured, far too wide. */
      resize (width (), qMin (height (), sizeHint ().height ()));
      return;
    }
  JTDX::restore_grown_geometry (this, m_savedGeometry, m_savedMinHint);   // geometryrestore.h
}

// closeEvent is only called from the system menu close widget for a
// modeless dialog so we use the hideEvent override to store the
// window settings
void LogQSO::hideEvent (QHideEvent * e)
{
  storeSettings ();
  QDialog::hideEvent (e);
}
