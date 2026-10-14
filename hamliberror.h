#ifndef HAMLIBERROR_H__
#define HAMLIBERROR_H__

#include <QString>
#include <QStringList>

// CE3TSK 2026-10-12: the text shown for a Hamlib error code.
//
// HamlibTransceiver keeps a table of texts, one for each of Hamlib's error codes, and error_check () read it
// with errortable.at (abs (ret_code)) - nothing checked the code against the table, and QStringList::at ()
// checks nothing in a release build. Two faults came of it:
//
//   - the table had 20 entries and Hamlib 4.7 has codes up to 22: the rig is not powered on (20), a limit
//     exceeded (21), access denied (22) - on Windows that is the serial port another program holds;
//   - the PTT-only object - what the program uses for RTS or DTR keying when the rig itself is driven through
//     OmniRig, Ham Radio Deluxe or DX Lab Suite Commander (TransceiverFactory.cpp) - never filled the table,
//     so EVERY error read an empty list. A PTT port that could not be opened ended the program with a
//     segmentation fault where "Hamlib error: IO error while opening connection to rig" belonged.
//     (The object then composed that text and nobody showed it: the three wrappers did not listen to it,
//     and the rig came online without its PTT port. That is put right in pttonly.h, 2026-10-13.)
//
// The table is now built in one place for both objects (HamlibTransceiver::error_texts ()) and covers the
// codes of the Hamlib of today; a code beyond it - a Hamlib newer than the table - gets its number.
// test/harness/test_hamliberror.cpp holds the table against the header the program is built with.
//
// Hamlib returns the NEGATIVE of the code; either sign is taken. The widening keeps the negation of the
// most negative int defined.
inline QString hamlibErrorText (QStringList const& table, int ret_code)
{
  qint64 const code {ret_code < 0 ? -static_cast<qint64> (ret_code) : static_cast<qint64> (ret_code)};
  if (code < table.size ()) return table.at (static_cast<int> (code));
  return QString {"error code %1"}.arg (code);
}

#endif
