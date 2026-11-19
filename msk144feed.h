// -*- Mode: C++ -*-
#ifndef MSK144FEED_H
#define MSK144FEED_H
/* CE3TSK 2026-11-17 (MSK144_PLAN.md step 1a): how MSK144's audio is handed to its decoder - the arithmetic, with a
 * test (test/harness/test_msk144feed.cpp).
 *
 * MSK144 is decoded while the period is being received, not after it: every time 3584 new samples (0.3 s) have
 * arrived, the decoder is given the newest 7168 (0.6 s) - a window, overlapping the one before it by half. That is
 * WSJT-X's rule (its lib/hspec.f90), and these are its numbers. In WSJT-X the rule stands in a Fortran routine that
 * also draws the spectrogram and calls the decoder in the thread that paints the screen; here the window is cut out
 * by the main window (MainWindow::mskSink) and decoded in a thread of its own (msk144decoder.h), in order.
 *
 * k is the number of samples of the period received so far - what the Detector announces at the end of each block.
 * The window that ends at k is the samples [k - 7168, k): WSJT-X 3.0.2 took (k - 7168, k], one sample late and one
 * not yet written; 3.3.0-beta6 corrected it (d4328170d), and this is the corrected rule.
 */
#include <QString>
#include <QStringList>
#include <QVector>
#include <QDateTime>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace msk144
{
  constexpr int window = 7168;                 // mskrtd's NZ
  constexpr int block = window / 2;            // the Detector's block in this mode: 3584 = 7 * 512, as WSJT-X's
  constexpr int last_sample = 30 * 12000;      // the longest T/R period

  // a window ends at k: at every block's end from the second on, to the end of the longest period
  inline bool window_ends_at (int k)
  {
    return k >= window && k <= last_sample && 0 == k % block;
  }

  // the moment of the window that ends at k, in seconds from the period's start: WSJT-X's (k - 7168) / 12000
  inline float window_time (int k)
  {
    return float (k - window) / 12000.0f;
  }

  // WSJT-X passes over a window either of whose halves is all zeros: nothing was received there (the start of
  // monitoring, a recording shorter than its period)
  inline bool silent (qint16 const * w)
  {
    bool first {false}, second {false};
    for (int i = 0; i < block && !first; ++i) first = 0 != w[i];
    for (int i = block; i < window && !second; ++i) second = 0 != w[i];
    return !first || !second;
  }

  // the windows a recording of n samples holds when it is played back: one at each block's end that the recording
  // reaches - k = 7168, 10752, ... up to the largest multiple of 3584 that is not beyond n nor beyond the period
  inline int last_window_end (int n, double period_s)
  {
    int const limit = qMin (qMin (n, int (period_s * 12000.0 + 0.5)), last_sample);
    return limit < window ? 0 : limit - limit % block;
  }

  // the start of the period that the millisecond ms of the day lies in, as hhmmss - the time a decode of that
  // period is shown with. Every T/R period of MSK144 (5, 10, 15, 30 s) divides a minute.
  inline int period_start_utc (qint64 ms_of_day, double period_s)
  {
    if (period_s <= 0.0) return 0;
    qint64 const p = qint64 (period_s * 1000.0 + 0.5);
    qint64 const start = ms_of_day - ms_of_day % p;
    int const s = int (start / 1000);
    return 10000 * (s / 3600) + 100 * ((s / 60) % 60) + s % 60;
  }

  /* The period a window received just now belongs to, as hhmmss. The window ended k samples into its period, so its
     period began k / 12000 s before now - about: the Detector begins a period at the first block of audio that
     arrives after the clock's boundary, and that block was recorded before it, so k runs ahead of the clock by up to
     the length of such a block; and the main window comes to the window a little after it was written. The first
     pulls the estimate before the boundary, the second pushes it past - so a second is added before the period is
     taken: right while the audio's blocks are shorter than a second and the main window less than the period minus a
     second late. (WSJT-X takes the period of the clock when it comes to the window: the period's last window, read a
     moment late, is then timed with the NEXT period.)
     (2026-11-19, TODO.md 3.-272; found by tools/ui-check/msk144qso.sh's case 2 on a program that came up 0.8 s before
     a boundary) That second was added whatever had begun the count - and the Detector counts from 0 not only at its
     own period's start but whenever the audio comes on (Monitor, the program's start, the end of a transmission).
     Audio that came on in a period's last second was so given to the NEXT period: his CQ was shown with the time of
     our own period, and a double click on it answered him in his. fromBoundary says what began the count: the
     Detector's own period (mskSink () saw it count from 0 again) - the second, as before; or the audio coming on -
     half a second. That is right for every window read before the next boundary (a window is read at least its own
     length, 0.597 s, after its count began), and it still covers a block of audio recorded before a boundary that
     the main window did not see the Detector pass (the audio came on less than a block before it). */
  inline qint64 count_margin_ms (bool fromBoundary) { return fromBoundary ? 1000 : 500; }
  inline int live_period_utc (qint64 ms_of_day_now, int k, double period_s, bool fromBoundary)
  {
    qint64 const day = 86400000;
    qint64 const ms = ((ms_of_day_now - qint64 (k) * 1000 / 12000 + count_margin_ms (fromBoundary)) % day + day) % day;
    return period_start_utc (ms, period_s);
  }

  /* (2026-11-18, the review 3.-270 P1) What closing the period being received asks for. A period is closed by the
     Detector's next one (its first block), and by the start of our own transmission, which ends the receiving
     wherever in the period it comes - at the clock's boundary a moment before the Detector would, or inside the
     period (a late start, Tune). lastK: the samples of the period at the block before, 0 when nothing has been
     received since a period was closed; open: windows of it are with the decoder and its end is not; endPosted: its
     end is (handed over with its last window).
       End      its end follows its windows to the decoder; the sequencer's pass comes back with it, however late
       Pass     it gave the decoder nothing (no audio): the sequencer's pass now
       Nothing  its end is on its way or back already, or there was no period
     Until this review a transmission's start began a new run instead, and what the decoder had not yet returned of
     the period - its last lines, its END - was passed over: no pass, and the sequencer shut to the next message. */
  enum class PeriodClose {Nothing, End, Pass};
  inline PeriodClose period_close (int lastK, bool open, bool endPosted)
  {
    if (lastK <= 0) return PeriodClose::Nothing;
    if (open) return PeriodClose::End;
    return endPosted ? PeriodClose::Nothing : PeriodClose::Pass;
  }

  /* (the review 3.-270 P6) The moment a period's decode is counted from, in milliseconds of the day - what the
     sequencer asks "which period of the pair was decoded" of (autoseqstep.h's decoded_first_period (), which steps
     back from it as from a decode's trigger: 5 s, a third of a period shorter than 7.5 s). The other modes decode a
     period at its end and note the clock then. MSK144's lines come all through the period and its end when the
     decoder is through with it, so the moment is told from the period itself: 0.9 s before its end, where FT8's
     trigger sits. */
  inline qint64 decode_moment_ms (int period_hhmmss, double period_s)
  {
    qint64 const start = (qint64 (period_hhmmss / 10000) * 3600 + (period_hhmmss / 100 % 100) * 60 + period_hhmmss % 100) * 1000;
    return start + qint64 (period_s * 1000.0 + 0.5) - 900;
  }

  /* How late in its period the audio began, in seconds - 0 for a period received from its start. The Detector counts
     the period's samples from 0 again whenever the audio is switched on (Monitor, the end of our own transmission),
     wherever in the period that is, so k / 12000 is then the time since the audio came on and a ping's moment would
     be told from there (WSJT-X tells it so). From the clock: the audio began at about now - k / 12000; when that is
     more than half a second into its period - the Detector's own start is within a block of audio of the boundary -
     the moment is moved by it. The same margin as live_period_utc (), by what began the count (2026-11-19: audio
     that came on in a period's last second is that late in ITS period, not on time in the next). */
  inline float late_start_s (qint64 ms_of_day_now, int k, double period_s, bool fromBoundary)
  {
    if (period_s <= 0.0) return 0.0f;
    qint64 const day = 86400000;
    qint64 const p = qint64 (period_s * 1000.0 + 0.5);
    qint64 const margin = count_margin_ms (fromBoundary);
    qint64 const began = ((ms_of_day_now - qint64 (k) * 1000 / 12000) % day + day) % day;
    qint64 const into = ((began + margin) % day) % p - margin;  // -margin .. p - margin, from the period's start
    return into > 500 ? float (into) / 1000.0f : 0.0f;
  }

  /* (step 1b) The recording of a period as it is saved: the period's length, the audio where it belongs in it.
     `got` is the audio received, from the first sample the Detector counted. When the audio began late in its
     period (Monitor came on inside it) it is put that much later, with nothing before it: the file is named by the
     period's start, and a ping is then where the decoder said it was. What was not received stays at nothing
     (the last block of a period that the Detector ended early). */
  inline QVector<qint16> period_recording (QVector<qint16> const& got, float late_start, double period_s)
  {
    int const n = qMax (0, int (period_s * 12000.0 + 0.5));
    // nothing, as long as the audio was late; the audio; and the whole brought to the period's length - cut there,
    // or filled with nothing. No index is worked out, so none can be wrong (a mutant of the first version copied
    // past the recording's end and no row could see it).
    QVector<qint16> out (qBound (0, int (late_start * 12000.0f + 0.5f), n), 0);
    out += got;
    out.resize (n);
    return out;
  }

  /* the name of a period's recording without its ".wav", yyMMdd_hhmmss as every mode's with seconds, from the
     period's start (hhmmss) and the UTC date and time of now: the period began today, unless its time of day is
     later than now's - then it began before midnight */
  inline QString recording_name (QDateTime const& now_utc, int period_hhmmss)
  {
    QTime const t {period_hhmmss / 10000, period_hhmmss / 100 % 100, period_hhmmss % 100};
    QDate d {now_utc.date ()};
    if (t > now_utc.time ()) d = d.addDays (-1);
    return QString {"%1%2%3_%4%5%6"}.arg (d.year () % 100, 2, 10, QChar {'0'}).arg (d.month (), 2, 10, QChar {'0'})
           .arg (d.day (), 2, 10, QChar {'0'}).arg (t.hour (), 2, 10, QChar {'0'}).arg (t.minute (), 2, 10, QChar {'0'})
           .arg (t.second (), 2, 10, QChar {'0'});
  }

  /* (step 1b) WSJT-X remembers MSK144's T/R period in three places - for 2 m, for 6 m and 4 m together, for every
     other band - and starts them at 30, 15 and 30 s. `band` is the band's name as the program writes it. */
  enum TrSlot {tr_other = 0, tr_2m = 1, tr_6m = 2};
  inline TrSlot tr_slot (QString const& band)
  {
    return band == "2m" ? tr_2m : (band == "6m" || band == "4m") ? tr_6m : tr_other;
  }
  inline int tr_default (TrSlot slot) { return slot == tr_6m ? 15 : 30; }
  inline bool tr_valid (int s) { return s == 5 || s == 10 || s == 15 || s == 30; }

  /* ---- short messages: WSJT-X's "Sh" (2026-11-18, MSK144_PLAN.md section 17) ----
     "<W9XYZ K1ABC> R+03": a frame of 40 symbols instead of 144, 20 ms instead of 72 - sixteen bits, twelve of them a
     hash of the two calls as they stand between the brackets (the addressed station first), four the report. It can
     say sixteen things (genmsk40.f90); a station sends it with R and a report, RRR or 73, and decodes one only when
     the hash is that of its own call and the DX call. */

  // the report a short message can carry, one of seven - WSJT-X's rule (its genStdMsgs and processMessage)
  inline int sh_report (int n)
  {
    if (n <= -2) return -3;
    if (n <= 1) return 0;
    if (n <= 4) return 3;
    if (n <= 7) return 6;
    if (n <= 11) return 10;
    if (n <= 14) return 13;
    return 16;
  }

  /* may the two stations use short messages here? The sequencer reads a decode's message in its first 24 columns
     (DecodedText), and "<" + call + " " + call + "> R+03" is eight more than the two calls: sixteen characters of
     calls at most. A longer pair's short message lost its end there - "R+03" read as R+0, and with one character
     more RRR as RR - so for such a pair the messages stay the long ones and the decoder is not asked for short ones
     (WSJT-X has no such limit; two standard calls have 12 at most, 16 with /P or /R on both). */
  inline bool sh_fits (QString const& call1, QString const& call2)
  {
    return !call1.isEmpty () && !call2.isEmpty () && call1.size () + call2.size () <= 16;
  }

  // what a short message begins with: "<" + the station addressed + " " + the sender + "> "
  inline QString sh_head (QString const& to, QString const& from) { return '<' + to + ' ' + from + "> "; }

  /* a Tx message in the short form - what genmsk_128_90.f90 asks before it hands the text to genmsk40: "<" in the
     first place and a blank before the first ">" */
  inline bool sh_form (QString const& text)
  {
    QString const t {text.trimmed ()};
    int const close {t.indexOf ('>')};
    return t.startsWith ('<') && close > 0 && t.left (close).contains (' ');
  }

  /* ... and one that goes out as it is written: the ">" in the ninth place or later, ONE blank, and one of the sixteen
     things with nothing after it. genmsk40 is less strict, and sends something else than the text reads: it finds
     the ">" before it takes doubled blanks out, so with one inside the brackets the hash is of other characters and
     the roger is lost - a message nobody can read, given back as the text written plainly (test_msk144feed); and it
     reads four characters after the ">" and lets anything follow them. Such a text is not sent (guiUpdate ()). */
  inline bool sh_sendable (QString const& text)
  {
    static QStringList const says {"-03", "+00", "+03", "+06", "+10", "+13", "+16",
                                   "R-03", "R+00", "R+03", "R+06", "R+10", "R+13", "R+16", "RRR", "73"};
    QString const t {text.trimmed ().toUpper ()};
    int const close {t.indexOf ('>')};
    QString const pair {t.mid (1, close - 1)};   // two calls, one blank between them
    return sh_form (t) && close >= 8 && 1 == pair.count (' ') && !pair.startsWith (' ') && !pair.endsWith (' ')
           && t.mid (close + 1, 1) == " " && says.contains (t.mid (close + 2));
  }

  /* a decode line of this program's - the message in its 26 columns from the 23rd - whose message is a short one,
     "... & <CE3TSK K1ABC> R+03": the line as the long message it stands for would stand there, "... & CE3TSK K1ABC
     R+03", every column after the message where it was. For the reader that takes a line's words as they stand -
     a double click (MainWindow::processMessage ()): "<CE3TSK" was nobody's call to it, and his roger was answered as
     a CQ. Any other line comes back as it is - a hashed call of a long message, "<PJ4/K1ABC> CE3TSK RRR", keeps its
     brackets, as it always did there. */
  inline QString sh_line_plain (QString const& line)
  {
    QString const message {line.mid (23, 26)};
    if (!sh_form (message)) return line;
    QString plain {message.trimmed ()};
    plain.remove (plain.indexOf ('>'), 1).remove (0, 1);
    return line.left (23) + plain.leftJustified (26, ' ') + line.mid (49);
  }

  /* mskrtd's line, "hhmmss dB T Freq &  message" (i6.6,i4,f5.1,i5,a4,a37), as this program's decoders write theirs:
     ONE blank between the mode character and the message, so that the character is in column 21 and the message
     begins in column 23, where DecodedText and every reader after it look for them. The column that holds FT8's DT
     holds T here: the moment in the period, 0 to 30 s. A line that is not mskrtd's comes back as it is.
     (2026-11-17, step 2) And the message in its 26 columns, 23 to 48, with a blank in column 49 - the width of an
     FT8 or FT4 line (decoder.f90: a26, then the marks). Step 1 cut the blanks at the end, and the windows wrote the
     country straight after the message: read back from the window - a double click - one of its letters stood in
     column 49, where the readers look for a decode's mark ('d', ',', '.', '?', '1' are marks), and the line's copy in
     the Rx Frequency window carried a piece of the country as message text. A message longer than 26 characters is
     cut there, as FT8's and FT4's are. */
  inline QString display_line (QString const& line)
  {
    if (line.size () > 24 && line.at (20) == ' ' && line.at (22) == ' ' && line.at (23) == ' ')
      return (line.left (23) + line.mid (24, 26)).leftJustified (50, ' ');
    return line;
  }
}
#endif
