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

#endif
