// CE3TSK 2026-10-20: View > Alternative layout - see altlayout.h and ALT_LAYOUT_PLAN.md.
#include "altlayout.h"

#include <QAbstractButton>
#include <QGridLayout>
#include <QHash>
#include <QLayout>
#include <QPointer>
#include <QRect>
#include <QSizePolicy>
#include <QSplitter>
#include <QTextDocument>
#include <QTextEdit>
#include <QVariant>
#include <QWidget>

#include "ui_altlayout.h"
#include "uilimits.h"

namespace
{
  QString const prefix {QStringLiteral ("alt_")};

  void collect (QLayout * layout, QList<QWidget *>& out)
  {
    if (!layout) return;
    for (int i = 0; i < layout->count (); ++i)
      {
        auto * const item = layout->itemAt (i);
        if (auto * const w = item->widget ()) out << w;
        else if (auto * const l = item->layout ()) collect (l, out);
      }
  }

  // a widget into another's place wherever that one is, in whichever of the four pane layouts holds it
  bool put (QList<QLayout *> const& roots, QWidget * from, QWidget * to)
  {
    for (auto * const root : roots)
      if (auto * const item = root->replaceWidget (from, to))
        {
          delete item;
          return true;
        }
    return false;
  }

  // a control and its placeholder trade places: three replaceWidget () calls through a scratch widget, because the
  // widget put into a layout must not be in one already. Symmetric, so the same call trades them back.
  bool trade_places (QList<QLayout *> const& roots, QWidget * a, QWidget * b)
  {
    QWidget scratch;
    return put (roots, b, &scratch) && put (roots, a, b) && put (roots, &scratch, a);
  }

  // the limits a widget is designed with in the arrangement it sits in: uilimits.h's record of them where it has
  // fitted the widget (jtdxLimits, and the horizontal policy it set aside while Narrow controls is on), else the
  // widget's own
  struct Limits
  {
    QSize minimum;
    QSize maximum;
    QSizePolicy policy;
  };

  Limits designed (QWidget const * w)
  {
    Limits l {w->minimumSize (), w->maximumSize (), w->sizePolicy ()};
    auto const kept = w->property ("jtdxLimits");
    if (kept.isValid ())
      {
        auto const r = kept.toRect ();
        l.minimum = QSize {r.x (), r.y ()};
        l.maximum = QSize {r.width (), r.height ()};
      }
    auto const policy = w->property ("jtdxPolicy");
    if (policy.isValid ()) l.policy.setHorizontalPolicy (QSizePolicy::Policy (policy.toInt ()));
    return l;
  }

  // a control takes the limits its placeholder carries, and the placeholder keeps the control's for the way back.
  // The control's record is rewritten to the new limits, so the next fit works from them; the policy set aside is
  // dropped with it, since the policy written now is the designed one (the next fit sets it aside again while Narrow
  // controls is on).
  void trade_limits (QWidget * control, QWidget * placeholder)
  {
    auto const mine = designed (control);
    auto const theirs = designed (placeholder);
    control->setMinimumSize (theirs.minimum);
    control->setMaximumSize (theirs.maximum);
    control->setSizePolicy (theirs.policy);
    control->setProperty ("jtdxLimits", QRect {theirs.minimum.width (), theirs.minimum.height (),
                                               theirs.maximum.width (), theirs.maximum.height ()});
    control->setProperty ("jtdxPolicy", QVariant {});
    placeholder->setMinimumSize (mine.minimum);
    placeholder->setMaximumSize (mine.maximum);
    placeholder->setSizePolicy (mine.policy);
    // and the placeholder's record with them: the parking widget is a window of its own, so every fit of all windows
    // (a font or style change, Narrow controls, the constructor's last fit) records the placeholders too, and a record
    // left behind here would hand the control the other arrangement's limits on the next trade (review 3.-166 1:
    // 18 buttons did)
    placeholder->setProperty ("jtdxLimits", QRect {mine.minimum.width (), mine.minimum.height (),
                                                   mine.maximum.width (), mine.maximum.height ()});
  }

  // the widgets the Tab key visits, in the window's focus chain: those taking focus by Tab and without a focus proxy -
  // Qt's own rule (QApplicationPrivate::focusNextPrevChild_helper), and the only ones QWidget::setTabOrder () orders
  QList<QPointer<QWidget>> tab_sequence (QWidget * window)
  {
    QList<QPointer<QWidget>> out;
    for (QWidget * w = window->nextInFocusChain (); w && w != window; w = w->nextInFocusChain ())
      if ((w->focusPolicy () & Qt::TabFocus) == Qt::TabFocus && !w->focusProxy ()) out << w;
    return out;
  }

  void set_tab_sequence (QWidget * window, QList<QPointer<QWidget>> const& sequence)
  {
    QWidget * previous {nullptr};
    for (auto const& w : sequence)
      {
        if (!w || w->window () != window) continue;
        if (previous) QWidget::setTabOrder (previous, w);
        previous = w;
      }
  }

  // a grid row at its tallest control's own height and a few pixels
  void fit_grid (QGridLayout * grid, int padding)
  {
    for (int row = 0; row < grid->rowCount (); ++row)
      {
        int height {0};
        for (int column = 0; column < grid->columnCount (); ++column)
          if (auto * const item = grid->itemAtPosition (row, column))
            if (auto * const w = item->widget ()) height = qMax (height, w->sizeHint ().height ());
        grid->setRowMinimumHeight (row, height > 0 ? height + padding : 0);
      }
  }
}

AltLayout::AltLayout (QSplitter * splitter)
  : splitter_ {splitter}
  , parking_ {new QWidget}
  , form_ {new Ui::AltLayout}
  , standard_ {splitter->widget (0), splitter->widget (1)}
  , alternative_ {nullptr, nullptr}
  , active_ {false}
{
  form_->setupUi (parking_.data ());
  alternative_[0] = form_->altLeftPane;
  alternative_[1] = form_->altRightPane;

  QHash<QString, QWidget *> controls;
  for (auto * const pane : standard_)
    for (auto * const w : AltLayout::controls (pane))
      {
        if (controls.contains (w->objectName ())) problems_ << QStringLiteral ("control %1 twice").arg (w->objectName ());
        controls.insert (w->objectName (), w);
      }
  QStringList placed;
  for (auto * const pane : alternative_)
    for (auto * const w : AltLayout::controls (pane))
      {
        auto const name = w->objectName ();
        if (!name.startsWith (prefix))
          {
            problems_ << QStringLiteral ("%1 in the alternative is no placeholder").arg (name);
            continue;
          }
        auto const control = name.mid (prefix.size ());
        if (placed.contains (control)) problems_ << QStringLiteral ("%1 has two placeholders").arg (control);
        placed << control;
        if (auto * const real = controls.value (control)) pairs_ << qMakePair (real, w);
        else problems_ << QStringLiteral ("%1 names no control of the main window").arg (name);
      }
  for (auto const& name : controls.keys ())
    if (!placed.contains (name) && !leftOut ().contains (name))
      problems_ << QStringLiteral ("%1 has no placeholder - it would vanish in the alternative").arg (name);
  problems_.sort ();

  /* the Tab order (review 3.-166 2). A control moved between the panes passes through the parking window, and Qt files
     it in that window's focus chain: one switch there and back put 105 of the standard's 106 Tab stops elsewhere. So
     the standard's sequence is kept from here, before anything has moved, and put back after the way back; the
     alternative gets its own - the same, with the controls' widgets in the order the alternative's form shows the
     controls (each pane top to bottom, each row left to right). */
  standardTabs_ = tab_sequence (splitter->window ());
  QList<QWidget *> shown;
  for (auto * const pane : alternative_)
    for (auto * const placeholder : AltLayout::controls (pane))
      for (auto const& pair : pairs_)
        if (pair.second == placeholder) shown << pair.first;
  auto const belongs = [&shown] (QWidget const * w) {
    for (auto * const control : shown)
      if (w == control || control->isAncestorOf (w)) return true;
    return false;
  };
  bool done {false};
  for (auto const& w : standardTabs_)
    {
      if (!w || !belongs (w)) { alternativeTabs_ << w; continue; }
      if (done) continue;
      done = true;   // the controls' widgets all where the first of them stood
      for (auto * const control : shown)
        for (auto const& v : standardTabs_)
          if (v && (v == control || control->isAncestorOf (v))) alternativeTabs_ << v;
    }
}

AltLayout::~AltLayout () = default;

QList<QWidget *> AltLayout::controls (QWidget * pane)
{
  QList<QWidget *> out;
  if (pane) collect (pane->layout (), out);
  return out;
}

QStringList AltLayout::leftOut ()
{
  return {QStringLiteral ("line"), QStringLiteral ("line_2")};
}

bool AltLayout::setActive (bool on)
{
  if (on == active_ || !problems_.isEmpty ()) return false;
  QList<QLayout *> const roots {standard_[0]->layout (), standard_[1]->layout (),
                                alternative_[0]->layout (), alternative_[1]->layout ()};
  // the keyboard focus where it was: a widget that leaves its parent loses it, and a switch is no reason to move it. The
  // window's own focus widget, not the application's: while another window has the focus (the Wide Graph, say) the
  // application's is that window's, and the main window's would be lost.
  QPointer<QWidget> const focused {splitter_->window ()->focusWidget ()};
  // the controls first, while the panes they go to are still the hidden ones: they appear with the pane, at once
  for (auto const& pair : pairs_)
    {
      trade_places (roots, pair.first, pair.second);
      if (pair.second->property ("altLimits").toBool ())
        {
          trade_limits (pair.first, pair.second);
          /* and fitted from them here, the traded controls alone: a refit of the whole window wrote uilimits.h's
             minimums over the ones the program's style sheets give AutoSeq, Split Tx/Rx and others (min-width: 5em),
             and the standard's right pane came back narrower in one column (review 3.-166, on screen) */
          JTDX::fit_one (pair.first);
        }
    }
  // then the panes; the splitter gives each its predecessor's geometry, and the one taken out (left parentless by
  // QSplitter) is parked - setParent () hides it. The arriving pane is shown explicitly: QSplitter gives it the hidden state of the
  // pane it replaces, and before the window is first shown that pane reports hidden (measured at start-up: both panes
  // stayed hidden and the window came up empty). On a window not yet shown, show () means "with the window".
  auto * const * const in = on ? alternative_ : standard_;
  for (int i = 0; i < 2; ++i)
    if (auto * const out = splitter_->replaceWidget (i, in[i]))
      {
        out->setParent (parking_.data ());
        in[i]->show ();
      }
  active_ = on;
  set_tab_sequence (splitter_->window (), on ? alternativeTabs_ : standardTabs_);
  if (focused && focused->isVisible ()) focused->setFocus (Qt::OtherFocusReason);
  fitRows ();
  return true;
}

void AltLayout::fitRows ()
{
  if (!active_) return;
  int const padding {6};
  fit_grid (form_->altReceiveGrid, padding);
  fit_grid (form_->altQsoGrid, padding);
  // the Rx Frequency window: four lines, its frame and the document's margin. The lines are measured in the window's
  // own font - the decoded text's is DisplayText's private format, and at the station's fonts (Ubuntu 11, Ubuntu Mono
  // 11) the two lines are as tall
  auto * const cell = form_->altRxGrid->itemAtPosition (2, 0);
  if (auto * const rx = qobject_cast<QTextEdit *> (cell ? cell->widget () : nullptr))
    form_->altRxGrid->setRowMinimumHeight (2, 4 * rx->fontMetrics ().lineSpacing () + 2 * rx->frameWidth ()
                                              + 2 * int (rx->document ()->documentMargin ()));
}
