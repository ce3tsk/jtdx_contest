// -*- Mode: C++ -*-
#ifndef MSK144DECODER_H
#define MSK144DECODER_H
/* CE3TSK 2026-11-17 (MSK144_PLAN.md step 1a): the MSK144 decoder of the running program - WSJT-X's decoder
 * (lib/msk144), in a thread of its own.
 *
 * WSJT-X decodes each 0.6 s window in the thread that paints the screen. Here the main window cuts the window out
 * of the received audio (MainWindow::mskSink, msk144feed.h) and hands a COPY of it to this object, which lives in
 * its own thread: the windows wait in that thread's queue and are decoded one after the other, in the order they
 * were handed over. A slow moment delays a line by a fraction of a second; no window is passed over, the screen
 * does not wait, and the same audio gives the same lines whatever the machine's speed.
 *
 * The decoder is not this process's alone to call twice: mskrtd keeps the noise level, the last message and the
 * recent calls from one window to the next. So there is ONE of these, and nothing else calls the routine.
 *
 * A window is of a "run": the main window raises the run's number when what is being received changes - the mode,
 * the band, monitoring stopped, a recording opened - and windows of an older run still waiting are passed over, as
 * are their lines when they arrive.
 */
#include <QObject>
#include <QVector>
#include <QString>
#include <QByteArray>
#include <atomic>
#include <QMetaType>

struct Msk144Window
{
  QVector<qint16> samples;   // msk144::window of them; none: the period has ended (see periodEnd)
  int utc = 0;               // the period's start, hhmmss
  int k = 0;                 // the samples of the period at the window's end
  float lateStart = 0.0f;    // how late in its period the audio began, s (msk144::late_start_s): added to the moment
  int ftol = 100;            // F Tol, Hz
  int rxFreq = 1500;         // the receive frequency, Hz
  int depth = 1;             // 1 Fast, 2 Normal, 3 Deep
  QString myCall;
  QString hisCall;
  bool shMsgs = false;       // short messages
  bool swl = false;
  int run = 0;
  qint64 dial = 0;           // the dial the window was received on, Hz
  bool replay = false;       // a recording played back
};
Q_DECLARE_METATYPE (Msk144Window)

class Msk144Decoder final
  : public QObject
{
  Q_OBJECT

public:
  explicit Msk144Decoder (QString const& data_dir, QObject * parent = nullptr);

  // from the main window's thread: from now on windows of another run are passed over
  void setRun (int run) {m_run.store (run);}
  int run () const {return m_run.load ();}

  // a window, or (no samples) the end of its period - in this object's thread, one at a time
  Q_SLOT void decode (Msk144Window const&);

  // mskrtd's line as it wrote it, and what it is of (the window without its samples)
  Q_SIGNAL void decoded (QString const& line, Msk144Window const& of) const;
  // every window of the period before it has been decoded; lines is how many it gave
  Q_SIGNAL void periodEnd (Msk144Window const& of, int lines) const;
  // what a window took, in milliseconds
  Q_SIGNAL void windowDone (int run, int msecs) const;

private:
  std::atomic<int> m_run;
  QByteArray m_dataDir;
  int m_lines;
  int m_linesRun;            // the run m_lines counts in
  bool m_started;
  int m_testLagMs;            // JTDX_TEST_MSK144_LAG_MS: see the constructor
};
#endif
