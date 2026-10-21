#ifndef ALTLAYOUT_H
#define ALTLAYOUT_H

// CE3TSK 2026-10-20: View > Alternative layout (ALT_LAYOUT_PLAN.md) - the main window's controls in a second
// arrangement, after JTDX_improved's AL layout: Band Activity with the receive buttons and the dial under it, the Rx
// Frequency window at the top right, a row of Hound, the four lamps, AutoSeq and TX minute, the QSO buttons in two
// rows, the meter beside the Tx tabs.
//
// The arrangement is a Designer form of its own, altlayout.ui: one placeholder per control of the splitter's two
// panes, named alt_<the control's name>. Switching trades every control with its placeholder through
// QLayout::replaceWidget () - the control into the placeholder's cell, the placeholder into the control's - and the
// splitter's two panes with the form's two. The same trade switches back, and Qt keeps every cell, span, stretch and
// alignment, so the standard arrangement returns exactly as mainwindow.ui built it (measured: swaptest in
// test/experiments/altlayout_proto/, and test_altlayout). Nothing is copied: the controls are the same objects, so
// every signal, shortcut, tooltip, style and ui-> pointer stays as it was. The Tab order is each arrangement's own: the
// standard's as it was, the alternative's following its form.
//
// A placeholder marked altLimits carries the control's size limits in the alternative; they trade too, and the
// control's "jtdxLimits" follows, so uilimits.h's fit_size_limits () fits from whichever arrangement is shown. The
// traded controls are refitted here; nothing else needs it - no other control's limits change - and a refit of the
// whole window would write over the minimums the program's style sheets set (review 3.-166).
//
// Built only when the alternative is first switched on (the operator, 2026-10-20: the standard view behaves and looks
// as it did before): until then the window holds nothing of it.

#include <QList>
#include <QPair>
#include <QPointer>
#include <QScopedPointer>
#include <QStringList>

class QGridLayout;
class QLayout;
class QSplitter;
class QWidget;
namespace Ui { class AltLayout; }

class AltLayout
{
public:
  // the splitter's two panes as they are now are the standard arrangement
  explicit AltLayout (QSplitter * splitter);
  ~AltLayout ();
  AltLayout (AltLayout const&) = delete;
  AltLayout& operator = (AltLayout const&) = delete;

  bool active () const { return active_; }

  // what keeps the switch from being safe: a placeholder that names no control of the standard panes, a control of
  // theirs with no placeholder (a control left out would vanish while the alternative is shown), a name twice. Empty
  // is the only state setActive () switches in.
  QStringList problems () const { return problems_; }

  // the switch, both ways; false (and nothing done) with problems () or when already so
  bool setActive (bool on);

  // the alternative's row heights from the current font: its two button grids at the buttons' own height and a few
  // pixels (ALT_LAYOUT_PLAN.md D22), the Rx Frequency window at least four lines (find 5). Nothing while the standard
  // is shown. Run after a switch and after a font or style change.
  void fitRows ();

  // the controls of a pane: every widget that is an item of the pane's layout or of a layout inside it - a container
  // (DX_controls_widget, the Tx tab stack) counts as one
  static QList<QWidget *> controls (QWidget * pane);

  // the controls no placeholder stands for: the standard dial row's two separator lines, which the alternative has no
  // place for. They stay in the parked standard pane.
  static QStringList leftOut ();

private:
  QSplitter * splitter_;
  QScopedPointer<QWidget> parking_;          // the form's top widget; whatever pane is not shown is kept in it, hidden
  QScopedPointer<Ui::AltLayout> form_;
  QWidget * standard_[2];
  QWidget * alternative_[2];
  QList<QPair<QWidget *, QWidget *>> pairs_; // (control, placeholder)
  QList<QPointer<QWidget>> standardTabs_;    // the Tab order of each arrangement (review 3.-166 2)
  QList<QPointer<QWidget>> alternativeTabs_;
  QStringList problems_;
  bool active_;
};

#endif
