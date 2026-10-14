#ifndef DELETEDLOG_H__
#define DELETEDLOG_H__

#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSet>
#include <QString>
#include <QTextStream>
#include <QTime>

// CE3TSK 2026-10-12: the record of the decodes the prefix-to-grid table deleted.
//
// The decoder checks a decoded call against the squares its prefix can be heard from, and deletes what does
// not fit: a weak or a-priori decode in every mode, a rover's CQ at any strength - and, on a prefix the table
// lists as not allocated, a directed CQ and "CALL1 CALL2 R GRID" at any strength. Until now nothing told
// anybody. A station whose prefix or square the table has wrong lost every weak CQ unseen: on the day this
// was written the table accepted no square for the Azores and refused three squares sixty stations were on
// the air from. lib/deleted_decodes.f90 makes the decoder say what the table deleted - one line each, only
// when the variable below is set, which the program does for its own decoder:
//
//     <DecodeDeleted>135945 -21  0.3 1503 ~ CQ CU3EM HM68                        grid/1
//
// and this writes them to <month>_DELETED.TXT beside ALL.TXT, always, with no setting - a day's worth is a
// few dozen lines. A line is ALL.TXT's own line as far as the message, then the reason, then the dial:
//
//     20261012_135945 -21  0.3 1503 ~ CQ CU3EM HM68  grid/1  14.074 MHz
//
// The date is the day the period began: a period that began before midnight stands under the old day here,
// where ALL.TXT writes the day the line arrived.
// grid/n: the square is not in the table for that prefix. prefix/n: the prefix is not allocated. n is the
// check that deleted it (the list is in deleted_decodes.f90). A message that reached the screen from another
// attempt of the same decode is not in the file (one case apart: deleted while receiving and decoded later by
// the TX background, it is in both files). A line with no time or no reason writes nothing.
//
// The file is the record of what was heard ON THE AIR, each message once a period (class Record below):
//   - a file played back (File > Open) writes nothing. Its lines would stand under today's date and the dial
//     of the moment, among the day's own - and every replay of the same file would add them again;
//   - the Decode button and a double click on the waterfall decode the same audio again, and the decoder
//     reports that period's deletions again: a message already written for its period is not written twice;
//   - the dial is the one the decode began on. A decode cut short by a change of band reports its deletions
//     after the change.
namespace deleted_log
{
  // the variable that makes the decoder print the lines
  inline char const * environmentVariable () {return "JTDX_REPORT_DELETED";}

  inline bool isDeletedLine (QByteArray const& line) {return line.startsWith ("<DecodeDeleted>");}

  // The day the period began. The decoder gives the period's start as hhmmss, the clock gives now; a period
  // that began before midnight and is reported after it belongs to yesterday.
  inline QDate periodDate (QDateTime const& utc_now, QTime const& period)
  {
    return period.msecsSinceStartOfDay () - utc_now.time ().msecsSinceStartOfDay () > 12 * 3600 * 1000
      ? utc_now.date ().addDays (-1) : utc_now.date ();
  }

  // the period's start in the decoder's line, invalid when the line is not one of them
  inline QTime periodTime (QByteArray const& line)
  {
    if (!isDeletedLine (line) || line.size () < 21) return QTime {};
    auto const hhmmss = line.mid (15, 6);
    for (auto c : hhmmss) if (c < '0' || c > '9') return QTime {};
    return QTime {hhmmss.mid (0, 2).toInt (), hhmmss.mid (2, 2).toInt (), hhmmss.mid (4, 2).toInt ()};
  }

  // the month's file, named as ALL.TXT is
  inline QString fileName (QDate const& period_date) {return period_date.toString ("yyyyMM") + "_DELETED.TXT";}

  // what a new file begins with
  inline QString header ()
  {
    return QString {"# Decodes the prefix-to-grid table deleted: UTC, SNR, DT, audio frequency, mode, message, reason, dial.\n"
                    "# grid/n: the square is not in the table for that prefix. prefix/n: the prefix is not allocated.\n"
                    "# n is the check that deleted it. A real station in this file means the table is wrong for it.\n"};
  }

  // The file's line for one of the decoder's lines; empty when it is not a whole one. The decoder writes
  // '<DecodeDeleted>',i6.6,i4,f5.1,i5,1x,a1,1x,a37,1x,a - after the tag the message is the 37 characters
  // from the 24th, and the reason starts at the 62nd.
  inline QString fileLine (QDateTime const& utc_now, qint64 dial_hz, QByteArray const& line)
  {
    auto const period = periodTime (line);
    if (!period.isValid ()) return QString {};
    auto const columns = line.mid (15);
    auto const why = QString::fromLatin1 (columns.mid (61)).trimmed ();   // empty for a line cut short, and refused below
    if (!(why.startsWith ("grid/") || why.startsWith ("prefix/")) || why.contains (' ')) return QString {};
    auto decode = QString::fromLatin1 (columns.left (60));
    while (decode.endsWith (' ')) decode.chop (1);
    return QString {"%1_%2  %3  %4 MHz"}.arg (periodDate (utc_now, period).toString ("yyyyMMdd")).arg (decode).arg (why)
      .arg (QString::number (dial_hz / 1.e6, 'g', 12));
  }

  // one of the decoder's lines into the month's file in dir; false when nothing was written
  inline bool append (QDir const& dir, QDateTime const& utc_now, qint64 dial_hz, QByteArray const& line)
  {
    auto const text = fileLine (utc_now, dial_hz, line);
    if (text.isEmpty ()) return false;
    QFile f {dir.absoluteFilePath (fileName (periodDate (utc_now, periodTime (line))))};
    bool const fresh {!f.exists () || 0 == f.size ()};
    if (!f.open (QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) return false;
    QTextStream out {&f};
    if (fresh) out << header ();
    out << text << '\n';
    return true;
  }

  // the message in one of the decoder's lines, as the decoder printed it
  inline QByteArray message (QByteArray const& line) {return line.mid (15 + 23, 37).trimmed ();}

  // What the program keeps between the decoder's lines: see the three rules at the top.
  class Record
  {
  public:
    // A decode begins. fresh: of audio that was not decoded before - the period's own decode, not the Decode
    // button's. replay: the audio is a file's. dial_hz: where the receiver is now.
    // What is remembered is the period AND the message: a TX background that the next period's decode cuts
    // short reports its deletions after this was called for that next period, and the same station is
    // deleted again there - both lines belong in the file.
    void decodeBegins (bool fresh, bool replay, qint64 dial_hz)
    {
      if (fresh) written_.clear ();
      replay_ = replay;
      dial_hz_ = dial_hz;
    }

    // one of the decoder's lines into the month's file in dir; false when nothing was written
    bool take (QDir const& dir, QDateTime const& utc_now, QByteArray const& line)
    {
      if (replay_) return false;
      auto const what = line.mid (15, 6) + message (line);
      if (written_.contains (what)) return false;
      if (!append (dir, utc_now, dial_hz_, line)) return false;
      written_.insert (what);
      return true;
    }

  private:
    bool replay_ {false};
    qint64 dial_hz_ {0};
    QSet<QByteArray> written_;   // period and message of the lines in the file, since the last new audio
  };
}

#endif
