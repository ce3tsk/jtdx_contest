#ifndef CONTESTSCORE_H
#define CONTESTSCORE_H

// CE3TSK 2026-11-03 (TODO.md 3.-221, 3.-222): the contest score: multipliers x points, over the QSOs that count. What a QSO is worth
// (contest_points ()) is asked of the caller, so that a contest with other points uses the same sum. Recomputed in full rather than
// accumulated: a scan of the contest log is about a thousand geodesics, a millisecond or two at QSO rate, and the display cannot
// drift from the file the way a running total would. test_wwdigiscore holds it, through MainWindow::updateContestScore ().

#include <QList>
#include <QSet>
#include <QString>

namespace contest_score
{
  struct Qso
  {
    QString call, band, grid;
  };

  struct Score
  {
    int multipliers = 0;
    int points = 0;
    bool multiplied = true;   // (2026-11-04) false: no multipliers, the score is the sum of the points (ARRL Digi)
    int total () const { return multiplied ? points * multipliers : points; }
  };

  // A station counts once per band, FT8 and FT4 alike: the first QSO in the log, a repeat scores nothing (the 2026 log held 20
  // of them). A QSO with no grid has no exchange: it scores nothing and takes no place from a later QSO with the grid.
  // (2026-11-07) missingGridScores: a contest where it still counts (the FT Challenge), so it is the first QSO like any other.
  // qsos: oldest first for each call (LogBook::contestQsos ()).
  inline QList<Qso> counted (QList<Qso> const& qsos, bool missingGridScores = false)
  {
    QList<Qso> c;
    QSet<QString> worked;
    for (auto const& q : qsos)
      {
        if (q.grid.length () < 4 && !missingGridScores) continue;
        QString const key = q.call.toUpper () + ' ' + q.band;
        if (worked.contains (key)) continue;
        worked.insert (key);
        c << q;
      }
    return c;
  }

  // pointsOf: what a QSO with this grid scores. A multiplier: each 2 character field on each band, of the QSOs that count; a QSO with
  // no grid brings none (missingGridScores: it counts, pointsOf with an empty grid says what it is worth).
  template <class PointsOf>
  inline Score sum (QList<Qso> const& qsos, PointsOf pointsOf, bool fieldMultipliers = true, bool missingGridScores = false)
  {
    Score s;
    s.multiplied = fieldMultipliers;
    QSet<QString> fields;
    for (auto const& q : counted (qsos, missingGridScores))
      {
        bool const hasGrid = q.grid.length () >= 4;   // (3.-231 review) a part grid typed in the dialog is none: the contest log says ZZ00 for it
        s.points += pointsOf (hasGrid ? q.grid : QString {});
        if (hasGrid) fields.insert (q.grid.left (2).toUpper () + q.band);
      }
    s.multipliers = fields.size ();
    return s;
  }

  // (2026-11-04) a contest whose points and multipliers come from the call: pointsOf (Qso), multipliersOf (Qso) their keys, each once on each band
  template <class PointsOf, class MultipliersOf>
  inline Score sum_by_call (QList<Qso> const& qsos, PointsOf pointsOf, MultipliersOf multipliersOf)
  {
    Score s;
    QSet<QString> multipliers;
    for (auto const& q : counted (qsos))
      {
        s.points += pointsOf (q);
        for (auto const& key : multipliersOf (q)) multipliers.insert (key + ' ' + q.band);
      }
    s.multipliers = multipliers.size ();
    return s;
  }

  inline QString text (Score const& s)
  {
    if (!s.multiplied) return QString {"%1P"}.arg (s.points);
    return QString {"%1M x %2P = %3"}.arg (s.multipliers).arg (s.points).arg (s.total ());
  }
}

#endif
