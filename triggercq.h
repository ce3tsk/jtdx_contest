#ifndef TRIGGERCQ_H
#define TRIGGERCQ_H
// CE3TSK 2026-10-15 (TODO 3.-94): the fields of a UDP TriggerCQ message (NetworkMessage.hpp, type 51), as MessageClient
// reads them after the header: the direction (utf8), the TX period (bool) and Send (bool).
//
// Stock read the direction and then Send, so a sender writing the documented three fields had its TX period taken for
// Send and its Send never read, and the TX period stayed false - every TriggerCQ moved transmission to the second period
// (15/45 in FT8). A sender writing the direction and Send alone - the only form the old reader understood - is still read
// so, and its TX period left as it is; one writing the direction alone, which stock read with Send off, is read so again
// (TODO 3.-105, the review). A message without even the direction is not read.
#include <QByteArray>
#include <QDataStream>
#include <QString>

namespace trigger_cq
{
  struct Request
  {
    QString direction;
    bool periodGiven {false};   // false: a two-field sender - the TX period is not touched
    bool txPeriod {false};      // true: the first period (TX 00/30 in FT8, the even minute in JT65/JT9/T10)
    bool send {false};
  };

  inline bool read (QDataStream& in, Request& r)
  {
    QByteArray direction;
    in >> direction;
    if (QDataStream::Ok != in.status ()) return false;
    r.direction = QString::fromUtf8 (direction);
    r.periodGiven = false; r.send = false;
    if (in.atEnd ()) return true;   // the direction alone: Send off, the TX period left as it is
    bool first {false};
    in >> first;
    if (QDataStream::Ok != in.status ()) return false;
    if (in.atEnd ())   // direction and Send alone
      {
        r.send = first;
        return true;
      }
    bool second {false};
    in >> second;
    if (QDataStream::Ok != in.status ()) return false;
    r.periodGiven = true; r.txPeriod = first; r.send = second;
    return true;
  }
}
#endif
