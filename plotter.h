// -*- Mode: C++ -*-
///////////////////////////////////////////////////////////////////////////
// Some code in this file and accompanying files is based on work by
// Moe Wheatley, AE4Y, released under the "Simplified BSD License".
// For more details see the accompanying file LICENSE_WHEATLEY.TXT
///////////////////////////////////////////////////////////////////////////

#ifndef PLOTTER_H
#define PLOTTER_H

#ifdef QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QFrame>
#include <QImage>
#include <QVector>
#include <QElapsedTimer>
#include <QTimer>
#include <cstring>
#include <vector>
#include "JTDXDateTime.h"
#include "cqspots.h"
#include "tracespots.h"
#define VERT_DIVS 7	//specify grid screen divisions
#define HORZ_DIVS 20

class CPlotter : public QFrame
{
  Q_OBJECT;

public:
  explicit CPlotter(QWidget *parent = 0);
  ~CPlotter();

  JTDXDateTime * m_jtdxtime;
  QSize minimumSizeHint() const;
  QSize sizeHint() const;
  bool    m_bScaleOK;

  void draw(float swide[], bool bScroll);		//Update the waterfall
  void SetRunningState(bool running);
  void setPlotZero(int plotZero);
  int  plotZero();
  void setPlotGain(int plotGain);
  int  plotGain();
  int  plot2dGain();
  void setPlot2dGain(int n);
  int  plot2dZero();
  void setPlot2dZero(int plot2dZero);
  void setStartFreq(int f);
  int startFreq();
  int  plotWidth();
  void UpdateOverlay();
  void setDataFromDisk(bool b);
  void setRxRange(int fMin);
  void setBinsPerPixel(int n);
  int  binsPerPixel();
  void setWaterfallAvg(int n);
  void setRxFreq(int n);
  void DrawOverlay();
  int  rxFreq();
  void setFsample(int n);
  void setNsps(double trperiod, int nsps);
  void setTxFreq(int n);
  void setMode(QString mode);
  void setTopJT65(int n);
  void setModeTx(QString modeTx);
  void SetPercent2DScreen(int percent);
  int  Fmax();
  void setDialFreq(double d);
  void setCurrent(bool b) {m_bCurrent = b;}
  bool current() const {return m_bCurrent;}
  void setCumulative(bool b) {m_bCumulative = b;}
  bool cumulative() const {return m_bCumulative;}
  void setBreadth(qint32 w) {m_w = w;}
  qint32 breadth() const {return m_w;}
  float fSpan() const {return m_fSpan;}
  void setLockTxFreq(bool b) {m_lockTxFreq = b;}
  void setFilter(bool b);
  void setHoundFilter(bool b);
  void setDarkStyle(bool b);
  void setColours(QVector<QColor> const& cl);
  void setTimestamp(int n);
  void setScale(bool b);
  void setBars(bool b);
  void showFreq(bool b);
  void setFlatten(bool b);
  void setTol(int n);
  void setRxBand(QString band);
  // CQ calls at the bottom of the waterfall (cqspots.h): the callsign of a station calling CQ, written over
  // its audio frequency for the given number of seconds; 0 switches it off
  void setCqDuration(int seconds);
  int  cqDuration() const {return m_cqBoard.duration_s();}
  void addCqCall(QString const& call, int freq, qint64 dial, QString const& mode, bool replay);   // the air it was heard on
  void setCqOrigin(qint64 dial, QString const& mode, bool replay);   // the air the waterfall shows now
  void clearCqCalls();
  // (2026-11-15) each line the Band Activity window shows, with the period it was decoded of - its time, the second of
  // the UTC day, and its air: the mouse over a signal's trace in the waterfall names it, "K1ABC -12" (tracespots.h)
  void addDecode(QString const& message, int snr, int freq, int time_s, qint64 dial, QString const& mode, bool replay);
  void beginReplay();                // a recording is about to be drawn: its first row begins its own period
  int  tracePeriods() const {return static_cast<int>(m_traces.periods());}   // the periods kept (for test_cqcalls)

signals:
  void freezeDecode1(int n);
  void setFreq1(int rxFreq, int txFreq);
  void setRxFreq1(int rxFreq);
  void filter_on1();
  void toggle_filter1();

protected:
  //re-implemented widget event handlers
  void paintEvent(QPaintEvent *event);
  void resizeEvent(QResizeEvent* event);
  virtual void keyPressEvent( QKeyEvent *e );
  bool event(QEvent* e) override;          // a CQ call's tooltip, on its label or bracket

private:

  void MakeFrequencyStrs();
  int XfromFreq(float f);
  float FreqfromX(int x);
  float signalBandwidth() const;           // width in Hz of one signal of the mode, 0 if there is none
  void drawCqCalls(QPainter& painter);     // the CQ calls over the waterfall's bottom edge
  QString cqCallsAt(QPoint const& p, QRect* area = nullptr) const;   // the call(s) drawn at p, one a line, or ""
  QString tracesAt(QPoint const& p, QRect* area = nullptr) const;    // the signals whose trace is at p, one a line, or ""
  QString callsAt(QPoint const& p, QRect* area = nullptr) const;     // a CQ call's label or bracket first, else the traces
  void noteRow();                          // a row just drawn (tracespots.h)
  void followTip();                        // the resting mouse's tooltip, after the waterfall moved under it

  bool    m_bCurrent;
  bool    m_bCumulative;
  bool    m_lockTxFreq;
  bool    m_filter;
  bool    m_houndFilter;
  bool	  m_useDarkStyle;
  bool	  m_bars;
  bool    m_freq;
  float   m_fSpan;

  qint32  m_plotZero;
  qint32  m_plotGain;
  qint32  m_plot2dGain;
  qint32  m_plot2dZero;
  qint32  m_binsPerPixel;
  qint32  m_waterfallAvg;
  qint32  m_w;
  qint32  m_Flatten;
  qint32  m_nSubMode;
  qint32  m_topJT65;
  qint32  m_timestamp;

  QPixmap m_DialOverlayPixmap;
  QPixmap m_HoverOverlayPixmap;
  QPixmap m_WaterfallPixmap;
  QPixmap m_2DPixmap;
  QPixmap m_ScalePixmap;
  QPixmap m_OverlayPixmap;
  QPoint m_pos;
  QSize   m_Size;
  QString m_Str;
  QString m_HDivText[483];
  QString m_mode;
  QString m_modeTx;
  QString m_rxBand;

  bool    m_Scale;
  bool    m_Running;
  bool    m_paintEventBusy;
  bool    m_dataFromDisk;

  double  m_fftBinWidth;
  double  m_dialFreq;
  double  m_xOffset;
  double  m_TRperiod;

  float   m_sum[2048];

  cq_spots::Board m_cqBoard;
  cq_spots::Origin m_cqNow;                // the air shown now: only its calls are drawn
  QElapsedTimer   m_cqClock;               // monotonic: the calls' age does not follow a clock that is set
  QTimer          m_cqTimer;               // runs while there are calls on the board, to let them expire
  // (2026-11-15) where the last paint put each call: its label, and its bracket - for a call with no row the only place
  // it shows. A label names its own call; a bracket every call heard on that signal
  struct CqHit {QRect area; QString call; bool label;};
  std::vector<CqHit> m_cqHits;
  trace_spots::Board m_traces;             // (2026-11-15) the waterfall's rows, its periods and their decoded lines
  bool    m_replayPending;                 // a recording's first row is still to come
  QPoint  m_traceTipAt;                    // where the resting mouse's tooltip was asked for, while it shows: the
  QString m_traceTip;                      // waterfall moves on under it, and the tooltip follows what is there

  qint32  m_dBStepSize;
  qint32  m_FreqUnits;
  qint32  m_hdivs;
  qint32  m_line;
  qint32  m_fSample;
  qint32  m_xClick;
  qint32  m_freqPerDiv;
  qint32  m_nsps;
  qint32  m_Percent2DScreen;
  qint32  m_Percent2DScreen0;
  qint32  m_h;
  qint32  m_h1;
  qint32  m_h2;
  qint32  m_rxFreq;
  qint32  m_txFreq;
  qint32  m_fMin;
  qint32  m_fMax;
  qint32  m_startFreq;
  qint32  m_tol;
  qint32  m_lastMouseX;
  qint32  m_lastPaintedX;
  qint32  m_lastMouseY;                    // (2026-11-15) a call's label lies above or below another's
  QString m_lastTip;                       // the frequency tooltip last shown, with the call(s) under the mouse
  char    m_sutc[6];

private slots:
  void leaveEvent(QEvent *event);
  void mouseMoveEvent(QMouseEvent * event);
  void mousePressEvent(QMouseEvent *event);
  void mouseDoubleClickEvent(QMouseEvent *event);
};

extern QVector<QColor> g_ColorTbl;

#endif // PLOTTER_H
