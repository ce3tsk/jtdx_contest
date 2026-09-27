#ifndef GEOMETRYRESTORE_H
#define GEOMETRYRESTORE_H

// CE3TSK 2026-09-26: a window's saved size and place, put back - ONE rule for the main window, the
// Wide Graph and the Log QSO dialog. A review found it in three copies that had already drifted
// apart (only the Log QSO dialog honoured Qt's refusal), and the operator chose one shared function.
//
// restore_grown_geometry () restores `saved`, then lets the window grow by as much as the layout's
// minimum has RISEN since it was saved - which is what a larger application font does - and never
// past that minimum, so a size the operator chose is otherwise kept exactly (restoreMainGeometry,
// 2026-09-15). `saved_min_hint` is the minimumSizeHint () stored beside the geometry; an invalid one
// (a geometry saved before it was recorded) restores the geometry as saved.
//
// It returns false when there is nothing to restore OR Qt refuses it, and then changes nothing. Qt's
// restoreGeometry () bails out when either screen - the one the geometry was saved on, the one now -
// is more than 25 % wider than the other: Qt 5.15 refuses a saved/current width ratio outside
// 0.8..1.25, a guard for high-DPI changes (review 2026-09-26: "more than 25 % wider or narrower" was
// wrong on the narrow side - a screen 22 % narrower is already refused). Measured, a 2240 px screen's
// geometry on a 1600 px one. A refusal is logged, so that "my window came back at another size" can
// be traced. What a window does then is its own choice, so it is left to the caller: the Log QSO
// dialog opened at its .ui width of 374 px with its fields crushed until it checked.
//
// test_geometry has the rules.

#include <QByteArray>
#include <QDebug>
#include <QSize>
#include <QWidget>

namespace JTDX
{
  inline bool restore_grown_geometry (QWidget * window, QByteArray const& saved, QSize const& saved_min_hint)
  {
    if (saved.isEmpty ()) return false;
    if (!window->restoreGeometry (saved))
      {
        qDebug () << "saved geometry refused by Qt (saved on a screen of another width):"
                  << window->metaObject ()->className ();
        return false;
      }
    if (!saved_min_hint.isValid ()) return true;
    auto const needed = window->minimumSizeHint ();
    auto const growth = (needed - saved_min_hint).expandedTo (QSize {0, 0});
    window->resize (window->size ().expandedTo ((window->size () + growth).boundedTo (needed)));
    return true;
  }
}

#endif
