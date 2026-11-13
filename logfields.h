#ifndef LOGFIELDS_H
#define LOGFIELDS_H

#include <QString>

/* CE3TSK: what goes into a log entry when the QSO did not run the textbook sequence.

   WW_DIGI_CONTEST_SUPPORT.md section 6b: four of 341 contest QSOs logged wrong because the
   logger read GUI state instead of the station's own record. All three rules below were
   inline in on_logQSOButton_clicked(), where nothing could test them.

   The QSOs that broke were completed from a late re-selection - the sequencer had let go, we
   had moved on, and a late decode of the partner's final message
   re-selected him, sent the 73 and logged inside the same pass. */

/* The QSO's start, as QsoHistory keeps it: seconds into the day, 0 when it was never recorded.
   The old code tested only `time < 86400`, so "never recorded" logged as midnight - JR5JEU and
   ZL3TE carry time_on 000000 because of this. When it is not known the caller keeps the current
   time: contest checking tolerates minutes of difference, so the moment of logging is always
   inside tolerance, and it cannot be stale the way a carried-over global can. */
inline bool log_time_known (unsigned t) { return t > 0 && t < 86400; }

/* The grid to log. `box` is the DX grid entry, which clearDX empties; `history` is what the
   station actually sent us, which QsoHistory still holds. N3RTW and E71AVW logged with an empty
   gridsquare although their grids had been decoded (CQ WW N3RTW FM18, CE3TSK E71AVW JN94). */
inline QString log_grid (QString const& box, QString const& history)
{
  return box.isEmpty () ? history : box;
}

/* CE3TSK 2026-10-14 (the operator): the DX station's grid as ADIF's GRIDSQUARE takes it - 8 characters at most.
   The DX grid box takes 10, and a 10 character grid went into the log file and Logged ADIF whole; GRIDSQUARE holds
   up to 8, and what lies beyond belongs in GRIDSQUARE_EXT (ADIF 3.1.4), which this program does not write. */
inline QString adif_gridsquare (QString const& grid)
{
  return grid.left (8);
}

/* CE3TSK 2026-11-07 (TODO.md 3.-231, the operator): the grid a contest's own log gets. A contest where a QSO with no grid still counts, the FT
   Challenge, writes ZZ00 for it, its FAQ's "enter ZZ00 as the Grid Square if you can". In the contest log only: the everyday log stays with
   none, so ZZ00 reaches no LoTW, QRZ or map upload. Inside the program the grid is simply missing (adif_read_gridsquare ()). */
inline QString contest_log_grid (QString const& grid, bool missingGridScores)
{
  /* (3.-231 review) fewer than 4 characters, as the score asks of the same QSO in memory (contest_score::counted (), sum ()): the two agree */
  return missingGridScores && grid.length () < 4 ? QString {"ZZ00"} : grid;
}

/* CE3TSK 2026-11-07: GRIDSQUARE as a log is read back: ZZ00, ZZ being no Maidenhead field, is no grid, in any log. So nothing sees it: no
   worked grid or field, no distance, not the DX Grid box. */
inline QString adif_read_gridsquare (QString const& field)
{
  return field.left (2).compare (QLatin1String {"ZZ"}, Qt::CaseInsensitive) == 0 ? QString {} : field;
}

/* CE3TSK 2026-11-05 (TODO.md 3.-235): a field of an ADIF record, by its name with the colon ("CALL:"); empty when it is not there. Moved
   here from ADIF::_extractField, unchanged, for the Cabrillo export to read a log with the same reader as the program. */
inline QString adif_field (QString const& line, QString const& fieldName)
{
    int fieldNameIndex = line.indexOf('<' + fieldName  ,0,Qt::CaseInsensitive);
    if (fieldNameIndex >=0)
    {
        int closingBracketIndex = line.indexOf('>',fieldNameIndex);
        int fieldLengthIndex = line.indexOf(':',fieldNameIndex);  // find the size delimiter
        int dataTypeIndex = -1;
        if (fieldLengthIndex >= 0)
        {
          dataTypeIndex = line.indexOf(':',fieldLengthIndex+1);  // check for a second : indicating there is a data type
          if (dataTypeIndex > closingBracketIndex)
            dataTypeIndex = -1; // second : was found but it was beyond the closing >
        }

        if ((closingBracketIndex > fieldNameIndex) && (fieldLengthIndex > fieldNameIndex) && (fieldLengthIndex< closingBracketIndex))
        {
            int fieldLengthCharCount = closingBracketIndex - fieldLengthIndex -1;
            if (dataTypeIndex >= 0)
              fieldLengthCharCount -= 2; // data type indicator is always a colon followed by a single character
            QString fieldLengthString = line.mid(fieldLengthIndex+1,fieldLengthCharCount);
            int fieldLength = fieldLengthString.toInt();
            if (fieldLength > 0)
            {
              QString field = line.mid(closingBracketIndex+1,fieldLength);
              return field;
            }
       }
    }
    return "";
}

/* The received exchange. `stored` is this station's own r_rep; `global` is m_rptRcvd, one
   variable holding the last report parsed from *any* message - and in the contest a "report"
   is a grid, which is how KD6WW's CM98 reached JR5JEU's entry. In a contest an empty exchange
   is logged empty: visible and fixable, where a plausible wrong one busts the QSO unnoticed.
   Outside a contest the old fallback stands - there the operator sees the report in the GUI. */
inline QString log_exchange (QString const& stored, QString const& global, bool contest)
{
  if (!stored.isEmpty ()) return stored;
  return contest ? QString {} : global;
}

/* CE3TSK 2026-11-13 (TODO.md 3.-248, GitHub issue 4, EA3FA): one transmission period of the logged mode, in ms - the step
   of the Log QSO's "+15 s" button beside End. The window opens as our first 73 or RR73 starts and End is the end of that
   transmission (MainWindow::on_logQSOButton_clicked), so his closing 73 is a period later, or more if he repeats it. FT8
   15 s, FT4 7.5 s, FT2 3.75 s, WSPR-2 120 s; every other mode JTDX logs (JT65, JT9, T10) 60 s. (review 2026-11-13) The
   Mode field as the operator may type it, so spaces and case do not matter; MainWindow's own period of each mode
   (m_TRperiod) must be the same - test/logperiod_wiring.sh compares the two, so a new mode cannot be left out here. */
inline qint64 log_period_ms (QString const& mode)
{
  QString const m {mode.trimmed ().toUpper ()};
  if (m == "FT8") return 15000;
  if (m == "FT4") return 7500;
  if (m == "FT2") return 3750;
  if (m == "WSPR-2") return 120000;
  return 60000;
}

// (review 2026-11-13) the period in seconds as the button and its tooltip write it: 15, 7.5, 3.75, 60
inline QString log_period_seconds (qint64 ms)
{
  return QString::number (ms / 1000., 'g', 6);
}

// the button's text: "+15 s", "+7.5 s", "+3.75 s", "+60 s" - the SI symbol, the same in every language
inline QString log_period_text (qint64 ms)
{
  return QStringLiteral ("+%1 s").arg (log_period_seconds (ms));
}

/* CE3TSK 2026-11-13 (TODO.md 3.-247, its review): the exchange sent and received of the QSO Logged UDP message, WSJT-X
   3.0's two fields - in a contest what the contest is scored on: the two 4 character grids where the grid is the
   exchange (WW Digi, ARRL Digi - the reports in the log's RST fields are only reports there), else the reports; outside a
   contest none, as WSJT-X sends them. */
struct UdpExchange
{
  QString sent;
  QString received;
};
inline UdpExchange udp_exchange (bool contest, bool gridExchange, QString const& myGrid, QString const& hisGrid,
                                 QString const& reportSent, QString const& reportReceived)
{
  if (!contest) return {};
  if (gridExchange) return {myGrid.left (4).toUpper (), hisGrid.left (4).toUpper ()};
  return {reportSent, reportReceived};
}

#endif
