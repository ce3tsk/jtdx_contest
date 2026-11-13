#ifndef SATLOG_H
#define SATLOG_H

/* CE3TSK 2026-11-13 (TODO.md 3.-247): satellite QSOs in the log - ADIF PROP_MODE, SAT_NAME, SAT_MODE, FREQ_RX and BAND_RX.

   The Log QSO dialog has WSJT-X's four rows - Prop Mode, Satellite, Sat Mode, RX Frequency, each with Retain (DF2ET's
   patch, in WSJT-X Improved since 2022 and in WSJT-X since 3.0.0; GPL-3.0, as this program) - and its satellite list,
   sat.dat. This header holds what the dialog adds to WSJT-X's way:

   - prefill (): on a satellite's uplink the four rows are filled in from the frequency, autolog too. One window today:
     QO-100's narrowband transponder - uplink 2400.000 to 2400.500 MHz, received 8089.500 MHz higher - written as Razvan
     YO2MAX's ADIF line of 2026-11-01 has it (BAND 13cm with the uplink, BAND_RX 3cm with the downlink, SAT, QO-100, SX).
     Only the uplink: the logged band stays the dial's band, as in WSJT-X, so every worked-before per band stays right. A
     dial showing the downlink (3cm) is not recognised - WSJT-X logs 3cm there too.
     2 m and 70 cm have no window: their satellite segments are shared with terrestrial work and move with Doppler.
   - freq_rx (): the RX Frequency as typed - a comma read as the decimal point (the field showed "10489,540000" in a
     European locale), written with six decimals as FREQ is; anything that is not a frequency writes no field.

   A prefilled value is never a retained one (logqso.cpp): only the operator's own choice is kept for the next QSO, so
   the next HF QSO after a QO-100 one is not logged as a satellite QSO - WSJT-X's Retain keeps whatever was shown. */

#include <cmath>
#include <QString>
#include "Radio.hpp"

namespace sat_log
{
  // (review 2026-11-13) ADIF PROP_MODE of a satellite QSO, in one place
  char constexpr sat_prop_mode[] {"SAT"};

  // the highest RX Frequency taken, MHz: ADIF's top band, 1mm, ends at 250 GHz
  double constexpr max_rx_mhz {300000.};

  /* (review 2026-11-13) what the RX Frequency field lets the operator type: MHz with at most one decimal point or comma -
     no unit, no thousands separator, no "inf" - so nothing typed is dropped from the log without a word. Up to six digits
     each side; QRegularExpressionValidator matches the whole text. */
  char constexpr rx_typed[] {R"(\d{0,6}([.,]\d{0,6})?)"};

  struct Window
  {
    Radio::Frequency uplink_low;       // the logged frequency (dial + Tx audio), Hz, inclusive
    Radio::Frequency uplink_high;
    Radio::Frequency downlink_shift;   // added to the receive frequency (dial + Rx audio)
    char const * satellite;            // SAT_NAME as LoTW spells it, the key of sat.dat
    char const * sat_mode;             // SAT_MODE
  };

  Window constexpr windows[] = {
    {2400000000ull, 2400500000ull, 8089500000ull, "QO-100", "SX"},
  };

  struct Prefill
  {
    bool on {false};
    QString prop_mode;
    QString satellite;
    QString sat_mode;
    QString freq_rx;   // MHz, six decimals
  };

  inline QString mhz (Radio::Frequency f) {return QString::number (f / 1.e6, 'f', 6);}

  // tx: the frequency logged as FREQ (dial + Tx audio); rx: dial + Rx audio
  inline Prefill prefill (Radio::Frequency tx, Radio::Frequency rx)
  {
    for (auto const& w : windows)
      if (tx >= w.uplink_low && tx <= w.uplink_high)
        {
          Prefill p;
          p.on = true;
          p.prop_mode = QStringLiteral ("SAT");
          p.satellite = QString::fromLatin1 (w.satellite);
          p.sat_mode = QString::fromLatin1 (w.sat_mode);
          p.freq_rx = mhz (rx + w.downlink_shift);
          return p;
        }
    return {};
  }

  // the RX Frequency field, MHz as typed: "10489.541104" or "10489,541104" -> "10489.541104"; empty when it is none
  inline QString freq_rx (QString const& typed)
  {
    QString t {typed.trimmed ()};
    t.replace (QLatin1Char (','), QLatin1Char ('.'));
    bool ok {false};
    double const v {t.toDouble (&ok)};
    // (review 2026-11-13) a finite frequency up to 300 GHz: QString::toDouble takes "inf" and "1e30"
    return ok && std::isfinite (v) && v > 0. && v <= max_rx_mhz ? QString::number (v, 'f', 6) : QString {};
  }

  // its frequency in Hz, for the band; 0 for none
  inline Radio::Frequency hz (QString const& freq_rx_mhz)
  {
    bool ok {false};
    double const v {freq_rx_mhz.toDouble (&ok)};
    // in range before the cast: a double out of quint64's range cast to it is undefined
    return ok && std::isfinite (v) && v > 0. && v <= max_rx_mhz ? static_cast<Radio::Frequency> (v * 1.e6 + .5) : 0;
  }

  /* (review 2026-11-13) the satellite's ADIF fields, written by one function for every record - the log file (lower
     case), the loggers' UDP and TCP record (upper case) - so their order and names cannot drift apart */
  struct Fields
  {
    QString prop_mode;
    QString sat_name;
    QString sat_mode;
    QString band_rx;
    QString freq_rx;
  };

  // "<NAME:length>value", nothing for an empty value
  inline QString tag (QString const& name, QString const& value)
  {
    return value.isEmpty () ? QString {} : QLatin1Char ('<') + name + QLatin1Char (':') + QString::number (value.size ())
        + QLatin1Char ('>') + value;
  }

  // " <prop_mode:..>.. <sat_name:..>.. <sat_mode:..>.. <band_rx:..>.. <freq_rx:..>..", each one only when not empty
  inline QString adif (Fields const& f, bool upper)
  {
    QString t;
    auto const add = [&] (char const* name, QString const& value) {
      QString const n {QString::fromLatin1 (name)};
      if (!value.isEmpty ()) t += QLatin1Char (' ') + tag (upper ? n.toUpper () : n, value);
    };
    add ("prop_mode", f.prop_mode);
    add ("sat_name", f.sat_name);
    add ("sat_mode", f.sat_mode);
    add ("band_rx", f.band_rx);
    add ("freq_rx", f.freq_rx);
    return t;
  }
}

#endif
