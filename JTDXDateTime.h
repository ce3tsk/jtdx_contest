#ifndef JTDXDATETIME_H__
#define JTDXDATETIME_H__

#include <QDateTime>

//
// JTDXDateTime to manipulate system clock time
//
//Created by Arvo, ES1JA 2020

class JTDXDateTime : public QDateTime
{
 public:
  explicit JTDXDateTime ();
  explicit JTDXDateTime (QDateTime &&other);
  explicit JTDXDateTime(const QDateTime &other);
  explicit JTDXDateTime(const QDate &date, const QTime &time, const QTimeZone &timeZone);
  explicit JTDXDateTime(const QDate &date, const QTime &time, Qt::TimeSpec spec, int offsetSeconds);
  explicit JTDXDateTime(const QDate &date, const QTime &time, Qt::TimeSpec spec = Qt::LocalTime);

  QDateTime currentDateTimeUtc2();
  QDateTime currentDateTime2();
  qint64 currentMSecsSinceEpoch2();
  /* CE3TSK 2026-10-23 (STATION_SYNC_PLAN.md, stationsync.h): the offset in two parts - the average Sync's base and a
     station sync's part - and every clock read uses their sum, to the millisecond. MainWindow::stationSyncDo () is the
     one place that sets them (test/station_sync_wiring.sh); GetOffset () is the sum, as the debug lines print it. */
  void SetOffsets (double base, double station) { base_ = base; station_ = station; offset_ = qRound64 ((base + station) * 1000.0); foffset_ = offset_ / 1000.0f; }
  float GetOffset () { return foffset_; }
  double GetBaseOffset () const { return base_; }
  double GetStationOffset () const { return station_; }
 private:
  float foffset_ =0;
  qint64 offset_ = 0;
  double base_ = 0.0;
  double station_ = 0.0;

  using QDateTime::currentDateTimeUtc;
  using QDateTime::currentDateTime;
  using QDateTime::currentMSecsSinceEpoch;
};

#endif
