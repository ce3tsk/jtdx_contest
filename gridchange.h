#ifndef GRIDCHANGE_H
#define GRIDCHANGE_H
// CE3TSK 2026-10-14 (TODO 3.-86, review 3.-82): after the station's own grid changed in the Settings, the messages are
// generated again (MainWindow::regenerateForGrid) and Tx2-Tx5 are put back as they were - unless a text is empty
// (nothing generated yet, or cleared by a CQ) or carries the old grid as a word of its own, in any case (WW Digi's
// exchange: "K1ABC CE3TSK FF46", "K1ABC CE3TSK R FF46"): those take the text generated now. A hand edit and a free Tx5
// stay. oldGrid: the grid before the change, of which the messages carry four characters.
#include <QString>
#include <QRegularExpression>

namespace grid_change
{
  inline bool keep_text (QString const& text, QString const& oldGrid)
  {
    if (text.isEmpty ()) return false;
    QString const grid4 {oldGrid.left (4)};
    if (grid4.isEmpty ()) return true;
    QRegularExpression const word {"\\b" + QRegularExpression::escape (grid4) + "\\b", QRegularExpression::CaseInsensitiveOption};
    return !word.match (text).hasMatch ();
  }
}
#endif
