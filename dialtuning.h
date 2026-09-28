#ifndef DIALTUNING_H
#define DIALTUNING_H

// CE3TSK: tuning the dial from its frequency label - the mouse wheel (2026-09-15) and clicks
// (2026-09-28).
//
// The label reads like "7.074 000". Over one of the three kHz digits after the decimal point - in
// that text the 0, the 7 and the 4 - a wheel notch or a click moves the dial by that digit's step,
// 100, 10 or 1 kHz. Anywhere else on the label it moves by 1 kHz, as the 1 kHz digit does (the
// operator's request 2026-09-28: the MHz digits, the Hz digits and the space around the text all
// step 1 kHz). A left click steps down and a right click up. The steps carry the way a sum does
// (7.079 + 1 kHz = 7.080) and never leave the band the dial is in.
//
// MainWindow::dialFrequencyWheel () and dialFrequencyClick () feed these; test_dialtuning has the
// rules.

#include <QFontMetrics>
#include <QString>
#include <QtGlobal>
#include "Radio.hpp"

// The step in Hz at x, where text is the label's text drawn from text_left with metrics. The kHz
// digits are 7, 6 and 5 characters from the end in any locale (the decimal separator and the space
// before the Hz digits are one character each). A digit reaches from where the text before it ends
// to where the text through it ends: a prefix's advance is not the sum of its characters' (fractional
// advances, kerning), and "the digit's own advance" left a pixel between two digits that stepped
// neither of them.
inline qint64 dial_step_at (QString const& text, QFontMetrics const& metrics, int text_left, int x)
{
  if (text.size () >= 8)
    for (int i = 0; i < 3; ++i)
      {
        int const at {text.size () - 7 + i};
        int const from {text_left + metrics.horizontalAdvance (text.left (at))};
        int const to {text_left + metrics.horizontalAdvance (text.left (at + 1))};
        if (text.at (at).isDigit () && x >= from && x < to)
          return i == 0 ? 100000 : i == 1 ? 10000 : 1000;
      }
  return 1000;
}

// A click's direction: the left button steps down, the right one up, any other not at all.
inline int dial_click_direction (Qt::MouseButton button)
{
  return button == Qt::LeftButton ? -1 : button == Qt::RightButton ? 1 : 0;
}

// Where a step of hz (a signed number of Hz) takes the dial from `from`, or 0 when it may not go:
// no frequency yet, not above zero, or out of the band the dial is in. band_of names a frequency's
// band, empty outside every band - so a dial out of band may move while it stays out of band.
template <typename BandOf>
inline Radio::Frequency dial_step_target (Radio::Frequency from, qint64 hz, BandOf band_of)
{
  if (!from) return 0;
  qint64 const to {static_cast<qint64> (from) + hz};
  if (to <= 0 || band_of (static_cast<Radio::Frequency> (to)) != band_of (from)) return 0;
  return static_cast<Radio::Frequency> (to);
}

#endif
