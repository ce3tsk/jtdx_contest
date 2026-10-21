#ifndef DISTANCEFIELD_H
#define DISTANCEFIELD_H

// CE3TSK 2026-10-20: the distance and azimuth a decode's line shows after its country (IMPROVED_FEATURES_PLAN.md stage 3)
// - "[1234 km / 56°]" - in the Band Activity window, for a message that carries a grid. WS and JTDX_improved have the
// feature; this one is built from what theirs taught:
//   - nothing when My Grid is empty: JTDX_improved printed values azdist never set, WS a distance from "mm";
//   - one computation with the score: both ask Configuration's grid_distances () (azdist) - the score through
//     grid_distance_km (), the field through grid_distance_azimuth (); the DX panel calls azdist itself, with the same
//     grids (contest_grid ()), and prints its azimuth as azdist gives it ("Az: 360" for due north, the field 0°);
//   - the field is a part of the line of its own, after the country: the red wave of a likely phantom decode, laid
//     over the country, does not run on into it, and a grid in doubt shows no field at all;
//   - the column: always aligned (the operator, 2026-10-20) - the field starts where a fixed number of columns after
//     the start of the country column ends, measured in the window's own font's pixels, so a Japanese or Chinese name,
//     two columns a character, lines up too; a country too long for it is followed by one space. Theirs counted
//     characters and had a spacing setting (JTDX_improved one, WS two).

#include <cmath>
#include <QRegularExpression>
#include <QString>
#include <QtGlobal>

namespace distance_field
{
  // the columns from the start of the country column to the field: the names (14 - JTDX_improved's default, 13 plus
  // one: the country names fit it 85 in 100, nearly every busy one - USA with three states "USA, NY/NJ/CT" 13), or the
  // prefixes (5) - 12 while US states are shown, which follow the prefix too, "K, NY/NJ/CT" 11 (three states, as the
  // names' column; review 3.-161 1: at 5 every US line's field stood apart); a contest's points tag in front of them
  // (" 1p ") adds its own width
  int const nameColumns {14};
  int const prefixColumns {5};
  int const prefixStateColumns {12};

  // a square the distance can be measured to: 4 or 6 characters of a Maidenhead locator; RR73 is a roger, not a grid,
  // and AA00 the null some software sends with no locator set (as phantomdecodes.h judgeableGrid (); review 3.-161 2:
  // it was measured, to near the South Pole)
  inline bool usable (QString const& grid)
  {
    static QRegularExpression const square {QStringLiteral ("^[A-R]{2}[0-9]{2}([A-X]{2})?$"),
                                            QRegularExpression::CaseInsensitiveOption};
    return grid.compare (QLatin1String ("RR73"), Qt::CaseInsensitive) && !grid.startsWith (QLatin1String ("AA00"), Qt::CaseInsensitive)
           && square.match (grid).hasMatch ();
  }

  // the field: "[1234 km / 56°]", "[767 mi]", "[56°]" - or "" when neither is to be shown; the azimuth 0-359
  inline QString text (bool showDistance, bool showAzimuth, bool miles, int km, int mi, int azimuth)
  {
    QString f;
    if (showDistance) f = miles ? QString::number (mi) + QStringLiteral (" mi") : QString::number (km) + QStringLiteral (" km");
    if (showAzimuth)
      {
        if (!f.isEmpty ()) f += QStringLiteral (" / ");
        f += QString::number (((azimuth % 360) + 360) % 360) + QChar (0x00B0);   // azdist says 360 for due north
      }
    return f.isEmpty () ? f : QLatin1Char ('[') + f + QLatin1Char (']');
  }

  // the padding in front of the field: the whole spaces that fit between the end of the country text (textPx wide) and
  // the column (`columns` space widths after its start), and what they fall short of it spread over them as extra
  // letter spacing - so the field starts at the column to the pixel: a Japanese name's katakana, whose width is no
  // whole number of spaces, line up as a Latin name does (whole spaces alone left the fields up to a space apart,
  // seen 2026-10-20 in distance.sh). One plain space when the text reaches or passes the column, or a space's width
  // is not known.
  struct Padding
  {
    int spaces;
    qreal extraPx;   // added after each of them
  };
  inline Padding padding (qreal textPx, qreal spacePx, int columns)
  {
    if (spacePx <= 0) return {1, 0};
    qreal const gap {columns * spacePx - textPx};
    if (gap < spacePx - 1e-6) return {1, 0};
    int const n {int (std::floor (gap / spacePx + 1e-6))};   // the slack keeps an exact multiple from losing a space
    return {n, qMax (qreal (0), (gap - n * spacePx) / n)};
  }
}

#endif
