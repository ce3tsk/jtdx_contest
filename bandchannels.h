#ifndef BANDCHANNELS_H
#define BANDCHANNELS_H

// CE3TSK 2026-09-26: which frequencies of the band selector's list get a band button, and which a
// button offers on a right click.
//
// View > Band buttons shows the rows marked default ("*"), each frequency once, sorted; a list with
// no default row at all for the mode shows every row, so the row never comes up empty. The band's
// other frequencies were reachable only through the band selector's drop-down - the operator's
// request 2026-09-26: "right click on 10m would present the 10m non default frequencies". So a
// right click offers the rows of the button's band that have NO BUTTON OF THEIR OWN: the
// non-default rows in the normal case, and nothing when every row already is a button. Both rules
// are here, one beside the other, so the menu can never offer a frequency that has a button or
// miss one that has not. Out of band (an empty band name) offers nothing: every out-of-band row
// shares that empty name, so they are no band's channels.
//
// The rows come in the band selector's own order (mode and IARU region filtered, or the contest's
// set while one runs); test_bandchannels has the rules.

#include <QList>
#include <QString>
#include <algorithm>
#include "Radio.hpp"

struct BandRow
{
  Radio::Frequency frequency;
  bool preferred;   // the row's default mark, the "*" in the band selector
};

// the frequencies that get a button, sorted, each once
inline QList<Radio::Frequency> band_button_frequencies (QList<BandRow> const& rows)
{
  QList<Radio::Frequency> wanted, every;
  for (auto const& row : rows)
    {
      if (!every.contains (row.frequency)) every << row.frequency;
      if (row.preferred && !wanted.contains (row.frequency)) wanted << row.frequency;
    }
  if (wanted.isEmpty ()) wanted = every;
  std::sort (wanted.begin (), wanted.end ());
  return wanted;
}

// the frequencies a button on `band` offers on a right click, sorted, each once; band_of names the
// band of a frequency (Bands::find)
template <typename BandOf>
QList<Radio::Frequency> band_channel_frequencies (QList<BandRow> const& rows, QString const& band, BandOf band_of)
{
  QList<Radio::Frequency> channels;
  if (band.isEmpty ()) return channels;
  auto const buttons = band_button_frequencies (rows);
  for (auto const& row : rows)
    if (!buttons.contains (row.frequency) && !channels.contains (row.frequency) && band_of (row.frequency) == band)
      channels << row.frequency;
  std::sort (channels.begin (), channels.end ());
  return channels;
}

#endif
