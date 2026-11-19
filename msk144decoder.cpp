// CE3TSK 2026-11-17 (MSK144_PLAN.md step 1a): see msk144decoder.h
#include "msk144decoder.h"

#include <QElapsedTimer>
#include <QThread>

#include "msk144feed.h"
#include "wsjtx_config.h"

#include "moc_msk144decoder.cpp"

extern "C" {
  // lib/msk144/msk144_window.f90
  void msk144_window_ (short id2[], int * nutc, float * tsec, int * ntol, int * nrxfreq, int * ndepth,
                       char const * mycall, char const * hiscall, bool * bshmsg, bool * btrain, double pcoeffs[],
                       bool * bswl, char const * datadir, char * line,
                       fortran_charlen_t, fortran_charlen_t, fortran_charlen_t, fortran_charlen_t);
  void msk144_in_program_ ();
}

Msk144Decoder::Msk144Decoder (QString const& data_dir, QObject * parent)
  : QObject {parent}
  , m_run {0}
  , m_dataDir {data_dir.toLocal8Bit ()}
  , m_lines {0}
  , m_linesRun {0}
  , m_started {false}
  /* (the review 3.-270, P1 and P12) for the tests alone: every window takes this many milliseconds more, as on a
     machine too slow for the mode - the period's last lines and its end then come back after the next period has
     begun, after a transmission's start (tools/ui-check/msk144qso.sh). Not set: nothing. */
  , m_testLagMs {qBound (0, qEnvironmentVariableIntValue ("JTDX_TEST_MSK144_LAG_MS"), 2000)}
{
}

void Msk144Decoder::decode (Msk144Window const& w)
{
  if (w.run != m_run.load ()) return;   // of a run that has ended: the mode, the band or the source has changed
  /* (the review 3.-270 P7) a run that ended before its period did left its lines counted: they are not the next
     period's (Save decoded kept a period that gave no line) */
  if (w.run != m_linesRun) { m_lines = 0; m_linesRun = w.run; }

  Msk144Window of {w};
  of.samples.clear ();

  if (w.samples.isEmpty ())             // the period's end: every window before it has been decoded
    {
      Q_EMIT periodEnd (of, m_lines);
      m_lines = 0;
      return;
    }
  if (w.samples.size () != msk144::window) return;

  if (!m_started)
    {
      msk144_in_program_ ();            // this process keeps none of jtdxjt9's files
      m_started = true;
    }

  QElapsedTimer timer;
  timer.start ();

  // mskrtd is given its own copy to scale; the calls blank padded to twelve characters, as it reads them
  QVector<qint16> id2 {w.samples};
  int nutc {w.utc};
  float tsec {msk144::window_time (w.k) + w.lateStart};
  int ntol {w.ftol};
  int nrxfreq {w.rxFreq};
  int ndepth {w.depth};
  QByteArray const mycall {w.myCall.toLatin1 ().leftJustified (12, ' ', true)};
  QByteArray const hiscall {w.hisCall.toLatin1 ().leftJustified (12, ' ', true)};
  bool bshmsg {w.shMsgs};
  bool btrain {false};                  // no phase measurement in this program (MSK144_PLAN.md decision 10)
  double pcoeffs[5] = {0., 0., 0., 0., 0.};
  bool bswl {w.swl};
  char line[80];
  line[0] = 0;

  msk144_window_ (id2.data (), &nutc, &tsec, &ntol, &nrxfreq, &ndepth, mycall.constData (), hiscall.constData (),
                  &bshmsg, &btrain, pcoeffs, &bswl, m_dataDir.constData (), line,
                  static_cast<fortran_charlen_t> (12), static_cast<fortran_charlen_t> (12),
                  static_cast<fortran_charlen_t> (m_dataDir.size ()), static_cast<fortran_charlen_t> (sizeof line));
  if (m_testLagMs > 0) QThread::msleep (m_testLagMs);

  if (line[0])
    {
      // the line ends at its NUL (mskrtd writes one after the message); never past the buffer
      int n {0};
      while (n < int (sizeof line) && line[n]) ++n;
      ++m_lines;
      Q_EMIT decoded (QString::fromLatin1 (line, n), of);
    }
  Q_EMIT windowDone (w.run, int (timer.elapsed ()));
}
