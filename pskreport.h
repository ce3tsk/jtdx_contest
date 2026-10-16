#ifndef PSKREPORT_H
#define PSKREPORT_H
// CE3TSK 2026-10-15 (TODO 3.-91): the spots of one PSK Reporter datagram (psk_reporter.cpp, sent by UDP).
//
// Stock checked the size BEFORE adding a spot and stopped at 1400 bytes, so a datagram could end one spot past it -
// about 1435 bytes, an IP packet of about 1463 (IPv4) or 1483 (IPv6). A packet larger than its path's MTU is fragmented,
// and fragmented UDP is often dropped (NAT, firewalls, mobile links, VPNs, DS-Lite at 1460): the whole datagram - its
// spots - is lost without a word. Now a spot goes in only if the datagram with it, and with the set's closing padding,
// stays within MAX_PAYLOAD_LENGTH = 1200: an IP packet of at most 1228 (IPv4) or 1248 (IPv6) bytes, below the 1280 every
// IPv6 path must carry. Stock JTDX 2.2.160 lowered it to 1200 as well; WSJT-X keeps the whole packet within 1000.
// The record of a spot is stock's, byte for byte - but for the frequency (TODO 3.-108): stock declared it a 4-byte field
// (psk_reporter.cpp's template, "800500040000768F") and wrote it in hex padded to 8 digits, so a frequency of 2^32 Hz
// (4.29 GHz) or more - QO-100's 10.489 GHz downlink - came out as 9 digits: every field after it shifted, and the
// datagram, all its spots, garbled. Now it is a 5-byte field, as WSJT-X writes it (its PSKReporterIPFIX.cpp: 0x8005,
// length 5) - up to 1.1 THz.
#include <QHash>
#include <QQueue>
#include <QString>

namespace psk_report
{
  int constexpr MAX_PAYLOAD_LENGTH {1200};   // bytes of IPFIX payload in one UDP datagram
  int constexpr SET_PADDING {2};             // the "0000" that closes the sender-information set
  int constexpr FREQUENCY_BYTES {5};         // the frequency field (0x8005), as the sender template declares it
  qlonglong constexpr FREQUENCY_MASK {0xFFFFFFFFFFLL};

  // one spot as the sender-information set carries it, in hex: call, frequency, SNR, mode, grid, source, time
  inline QString spot_record (QHash<QString, QString> const& spot)
  {
    QString r;
    r += QString ("%1").arg (spot["call"].length (), 2, 16, QChar ('0')) + spot["call"].toUtf8 ().toHex ();
    r += QString ("%1").arg (spot["freq"].toLongLong () & FREQUENCY_MASK, 2 * FREQUENCY_BYTES, 16, QChar ('0'));
    r += QString ("%1").arg (spot["snr"].toInt (), 8, 16, QChar ('0')).right (2);
    r += QString ("%1").arg (spot["mode"].length (), 2, 16, QChar ('0')) + spot["mode"].toUtf8 ().toHex ();
    r += QString ("%1").arg (spot["grid"].length (), 2, 16, QChar ('0')) + spot["grid"].toUtf8 ().toHex ();
    r += QString ("%1").arg (1, 2, 16, QChar ('0'));   // REPORTER_SOURCE_AUTOMATIC
    r += QString ("%1").arg (spot["time"].toInt (), 8, 16, QChar ('0'));
    return r;
  }

  // the records of the spots taken from the front of the queue for one datagram; bytesSoFar: what the datagram holds
  // before them (its header, both templates, the receiver set and the sender set's own header). A spot is taken only if
  // the datagram with it and with the closing padding stays within MAX_PAYLOAD_LENGTH - but the first always is: a
  // datagram that could carry none would leave its spot queued for ever (the caller sends until the queue is empty).
  inline QString take_spots (QQueue<QHash<QString, QString>>& queue, int bytesSoFar)
  {
    QString records;
    while (!queue.isEmpty ())
      {
        QString const r {spot_record (queue.head ())};
        if (!records.isEmpty () && bytesSoFar + (records.size () + r.size ()) / 2 + SET_PADDING > MAX_PAYLOAD_LENGTH) break;
        records += r;
        queue.dequeue ();
      }
    return records;
  }
}
#endif
