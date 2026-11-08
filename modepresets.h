// -*- Mode: C++ -*-
/* CE3TSK 2026-11-07: a right click on a mode button opens that mode's decoding presets - the operator: "add the decode presets (FT8, FT4,
   FT2) accessible via a right click on their mode buttons ... and if there is no preset then a greyed out entry No presets. The idea is
   similar to the non-default frequencies for the Band buttons."

   The popup holds the Presets submenu's own entries (Decode > FT8, FT4 or FT2 decoding > Presets), not copies: a QAction can stand in
   more than one menu, so the tick, the recommended marks (bold, the coloured dot, the tier), a greyed state (S-Hound parks the presets)
   and what picking one does are the menu's, with nothing to keep in step. The submenu's look comes along: its green and its tooltips.
   A mode with no presets, or a submenu with nothing visible in it, gets one greyed entry, as a band with no other frequency does.

   Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software: you may
   redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 or (at your option) any later version. It is distributed WITHOUT ANY WARRANTY. */
#ifndef MODEPRESETS_H
#define MODEPRESETS_H

#include <QAction>
#include <QList>
#include <QMenu>
#include <QString>

namespace mode_presets
{
  // anything to pick: an entry that is neither a separator nor hidden
  inline bool offers (QMenu const * presets)
  {
    if (!presets) return false;
    for (auto const * a : presets->actions ()) if (!a->isSeparator () && a->isVisible ()) return true;
    return false;
  }

  // the popup of a mode button: the submenu's entries and its look, or `none` greyed
  inline void fill (QMenu * popup, QMenu const * presets, QString const& none)
  {
    if (!offers (presets))
      {
        popup->addAction (none)->setEnabled (false);
        return;
      }
    popup->addActions (presets->actions ());
    popup->setStyleSheet (presets->styleSheet ());
    popup->setToolTipsVisible (presets->toolTipsVisible ());
  }
}

#endif
