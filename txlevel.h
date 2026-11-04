#ifndef TXLEVEL_H
#define TXLEVEL_H

// The Tx audio level (the digital gain, "Pwr" column of the main window).
//
// The program keeps the level as a whole number, 0 .. 450: tenths of a dB below full scale, counted upwards - 450 is
// 0.0 dB, 0 is -45.0 dB (MainWindow::on_outAttenuation_valueChanged, the per-band memories, the settings file). It
// used to be the value of a vertical slider; the control is now a spin box that shows and takes the level in dB, with
// one decimal. These two functions are the whole conversion between the two, so that nothing stored changes: a
// settings file written by the slider is read by the spin box, and the other way round.
//
// Plain C++ without Qt, so the conversion can be run without a window.

#include <cmath>

namespace tx_level
{
  int const units_max {450};             // the stored value of 0.0 dB
  double const db_min {-45.0};
  double const db_max {0.0};

  // stored value -> dB, one decimal exactly as the spin box holds it: 450 -> 0.0, 225 -> -22.5, 1 -> -44.9
  inline double db_from_units (int units)
  {
    if (units < 0) units = 0;
    if (units > units_max) units = units_max;
    return (units - units_max) / 10.0;
  }

  // dB -> stored value, rounded to the nearest tenth and held inside 0 .. 450
  inline int units_from_db (double db)
  {
    double const tenths {std::floor (db * 10.0 + 0.5)};
    double const units {units_max + tenths};
    if (units < 0.0) return 0;
    if (units > units_max) return units_max;
    return static_cast<int> (units);
  }
}

#endif // TXLEVEL_H
