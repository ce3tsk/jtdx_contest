#ifndef ACTIONGATE_H
#define ACTIONGATE_H

// CE3TSK 2026-09-26: QAction::trigger () fires a DISABLED action as readily as an enabled one.
//
// Qt refuses a disabled action only in its own UI - the menu entry and a button cannot be clicked.
// Code calling trigger () gets the action's slot whatever the action's state: measured, Alt+C
// switched a tuning station from FT8 to FT4 with the whole Mode menu greyed for the transmission.
// A caller that must obey the gates on an action (the contest lock, the transmit gate) goes through
// here. test_actiongate pins both halves, so a Qt that changes trigger () is noticed.

#include <QAction>

inline bool trigger_if_enabled (QAction * action)
{
  if (!action || !action->isEnabled ()) return false;
  action->trigger ();
  return true;
}

#endif
