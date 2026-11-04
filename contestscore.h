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
    int total () const { return points * multipliers; }
  };

  // A station counts once per band, FT8 and FT4 alike: the first QSO in the log, a repeat scores nothing (the 2026 log held 20
  // of them). A QSO with no grid has no exchange: it scores nothing and takes no place from a later QSO with the grid.
  // qsos: oldest first for each call (LogBook::contestQsos ()).
  inline QList<Qso> counted (QList<Qso> const& qsos)
  {
    QList<Qso> c;
    QSet<QString> worked;
    for (auto const& q : qsos)
      {
        if (q.grid.length () < 4) continue;
        QString const key = q.call.toUpper () + ' ' + q.band;
        if (worked.contains (key)) continue;
        worked.insert (key);
        c << q;
      }
    return c;
  }

  // pointsOf: what a QSO with this grid scores. A multiplier: each 2 character field on each band, of the QSOs that count.
  template <class PointsOf>
  inline Score sum (QList<Qso> const& qsos, PointsOf pointsOf)
  {
    Score s;
    QSet<QString> fields;
    for (auto const& q : counted (qsos))
      {
        s.points += pointsOf (q.grid);
        fields.insert (q.grid.left (2).toUpper () + q.band);
      }
    s.multipliers = fields.size ();
    return s;
  }

  inline QString text (Score const& s)
  {
    return QString {"%1M x %2P = %3"}.arg (s.multipliers).arg (s.points).arg (s.total ());
  }
}

#endif
