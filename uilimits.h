#ifndef UILIMITS_H
#define UILIMITS_H

/* CE3TSK 2026-09-17: the one place that reconciles the .ui's hard pixel size limits with the
   font actually in use.
 
   mainwindow.ui pins ~30 widgets with maximumSize caps chosen for the font it was drawn at, so a
   larger application font cannot grow past them and the text is clipped ("Rx 305 Hz" loses the
   Hz, "GenMsgs" the s). Raising each cap to the widget's own sizeHint fixes that, but for a
   BUTTON sizeHint is the wrong measure: QPushButton::sizeHint carries the style's minimum button
   width (80 px here) whatever the label is, so the two arrow buttons pbR2T / pbT2R - a single
   glyph each, capped at 40 px in the .ui - were widened to 80 px apiece. Measured cost: the right
   column's minimum width rose from 573 px to 624 px against stock JTDX at 9 pt, and the operator
   cannot drag the splitter any further left than that minimum. JM1SZY reported exactly this,
   comparing the contest edition with stock JTDX 2.2.160-rc7 on Windows.
 
   So a button is measured by what its LABEL needs - text, icon, the style's own margins and the
   check/radio indicator - and never widened past its natural sizeHint. A label that genuinely
   needs the room still gets it: in Dutch, enableTxButton wants 139 px against a 100 px cap and is
   raised; the arrows are left alone. Everything that is not a button keeps the sizeHint rule,
   which is right for labels, spin boxes and combo boxes, where sizeHint IS the content.
 
   Measured with test probes over the 19 translations at 9 and 14 pt (see the session notes):
   English floor 624 -> 581 px against stock's 573; German, Dutch, Japanese and Russian unchanged,
   because there the width is what the translated labels actually need; every font above the
   design size unchanged, so the anti-clipping fix keeps working. */

#include <QAbstractButton>
#include <QAbstractScrollArea>
#include <QCoreApplication>
#include <QEvent>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVariant>
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QSizePolicy>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionSpinBox>
#include <QRegularExpression>
#include <QSplitter>
#include <QWidget>

namespace JTDX
{
  /* CE3TSK 2026-09-17: View -> Narrow controls.  The right-hand column's floor is the sum of its
     contents' minimum widths, so the only way to drag the splitter further left is to let those
     contents go below what they need.  This is the allowance, in percent, that the operator has
     agreed to: 0 is off and the window behaves exactly as before, 25 is what the menu sets.

     Measured on mainwindow.ui at 9 pt, right column floor 563 px:
         allowance   buttons only   every leaf but the entry widgets
             20 %        -25 px                 -40 px
             25 %        -33 px                 -52 px
             30 %        -41 px                 -63 px
     Labels contribute nothing (buttons-only and buttons+labels measure the same), while the
     sliders and the signal meter do - and those have no text to lose, which is why the rule is
     "every leaf except the entry widgets" rather than "every button".

     NOT applied below about 10 %: at 5 % the column measured 12 px WIDER, because pinning an
     explicit minimum just under a widget's hint changes how the grid distributes its columns.
     The menu offers nothing in that range.

     What it costs: Qt does not elide a push button's label, it clips it, and the label is
     centred, so a squeezed button loses characters from BOTH ends ("Spotted" -> "potted", not
     "Spo...").  At 25 % that is one character an end on the longest few labels; measured, not
     assumed.  Entry widgets are excluded outright so no frequency can ever lose a digit. */
  /* what the menu switches on.  One number, bracketed by the operator at 20-30 %; 25 buys 52 px
     of the 563 px column and costs about a character an end on the longest few labels. */
  int const NARROW_ALLOWANCE = 25;

  inline int& narrow_allowance ()
  {
    static int percent = 0;
    return percent;
  }

  /* The window's own floor under the allowance.  NOT zero: dropping MainWindow's minimum
     altogether let the sandbox drag the window to 300 px, which is not "narrower", it is broken.
     The operator's allowance is a tolerance, so it bounds the window exactly as it bounds the
     widgets inside it, and the two agree: at 25 % the contents fit the width the window is now
     allowed to reach. */
  /* The window's floor is NOT reduced by the same percentage as the widgets, and the difference
     was measured in the running application, not guessed:

         733 px   the .ui's own MainWindow minimum, and what the window manager is told
         675 px   still clean - every control present and readable, slight truncation at the edge
         640 px   degrading - "TX" and "AutoSeq" lose characters, the meter scale crowds
         549 px   BROKEN - Tune, Monitor, Stop and the meter are cut off by the window edge

     549 is 733 less the widget allowance of 25 %, and it does not work: forcing the window under
     what its contents can absorb does not reflow them, it lays them out at their minimums and
     lets the window edge clip whole controls away.  The 25 % on the widgets is what makes the
     contents FIT the width the window is allowed; the two numbers do different jobs.

     Re-measured again once the unit suffixes were dropped as well (below): 640 px is clean, with
     both frequency boxes showing "Tx  435" / "Rx  435" and their spin arrows intact; at 630 the
     arrows start to be cut and at 615 the Rx box has lost them altogether, which is a loss of
     function, not of looks.  So the window is allowed 12 % and lands on 645, inside the width
     that was checked.  Whenever the widget rules change, re-measure this in the sandbox; it is a
     screenshot judgement about the real layout, not arithmetic. */
  int const NARROW_WINDOW_ALLOWANCE = 12;

  inline int narrowed_window_minimum (int from_ui)
  {
    if (narrow_allowance () <= 0) return from_ui;
    return from_ui * (100 - NARROW_WINDOW_ALLOWANCE) / 100;
  }

  /* CE3TSK 2026-09-18: a splitter pane's floor, chosen instead of inherited.

     The two ways of squeezing the right-hand column did not agree, and the operator noticed:
     narrowing the WINDOW got further than dragging the SPLITTER. Measured at 11 pt with Narrow
     controls on - the splitter honours each pane's minimumSizeHint (left 200, right 475) and
     refuses to pass it, while the window's own floor is arithmetic, 733 * 88 % = 645, which never
     consults the contents. At 645 the right pane is forced to about 440, below its own minimum,
     and Qt then clips whole controls rather than reflowing them. So the window path was already
     allowing what the drag refused.

     This makes the drag reach the same width. Qt's qSmartMinSize returns 0 for a widget whose
     policy in that direction is Ignored, so the pane's contents stop setting the floor and an
     explicit minimumWidth becomes it. Verified on a throwaway splitter: a pane whose contents
     demand 475 stops at 440 with the policy Ignored and a 440 minimum, at 300 with a 300
     minimum, and back at 475 the moment both are restored.

     The original policy and minimum are remembered on the pane itself, so floor <= 0 puts it
     back exactly - Narrow controls off has to leave no trace, as it does for the widgets. */
  inline void set_pane_floor (QSplitter * splitter, int index, int floor)
  {
    if (!splitter) return;
    auto * const pane = splitter->widget (index);
    if (!pane) return;
    static char const * const kept_policy = "jtdx_pane_policy";
    static char const * const kept_minimum = "jtdx_pane_minimum";
    if (floor > 0)
      {
        if (!pane->property (kept_policy).isValid ())
          {
            pane->setProperty (kept_policy, int (pane->sizePolicy ().horizontalPolicy ()));
            pane->setProperty (kept_minimum, pane->minimumWidth ());
          }
        /* and let the fitting pass remember the pane's limits as they are NOW, before the floor
           goes on: if fit_one sees the pane for the first time with the floor already applied it
           records the FLOOR as "what the .ui asked for" and restores that later instead of the
           .ui's own value. Caught by test_uilimits, which failed on exactly that. */
        if (!pane->property ("jtdxLimits").isValid ())
          pane->setProperty ("jtdxLimits", QRect {pane->minimumWidth (), pane->minimumHeight (),
                                                  pane->maximumWidth (), pane->maximumHeight ()});
        auto policy = pane->sizePolicy ();
        policy.setHorizontalPolicy (QSizePolicy::Ignored);
        pane->setSizePolicy (policy);
        pane->setMinimumWidth (floor);
      }
    else if (pane->property (kept_policy).isValid ())
      {
        auto policy = pane->sizePolicy ();
        policy.setHorizontalPolicy (QSizePolicy::Policy (pane->property (kept_policy).toInt ()));
        pane->setSizePolicy (policy);
        pane->setMinimumWidth (pane->property (kept_minimum).toInt ());
        pane->setProperty (kept_policy, QVariant {});
        pane->setProperty (kept_minimum, QVariant {});
      }
    splitter->refresh ();
  }

  /* CE3TSK 2026-09-17: entry widgets get an allowance too, but never a flat one.  Measured on
     mainwindow.ui at 9 pt, minimum against the widest text the widget can ever show:

         rptSpinBox       120   "Report 49"      83    37 px spare
         DTCenterSpinBox   92   "DT 2.5 s"       70    22
         candListSpinBox   95   "CL  100 %"      86     9
         TxFreqSpinBox    105   "Tx  5000  Hz"  107    -2   ALREADY two pixels short
         RxFreqSpinBox    106   "Rx  5000  Hz"  108    -2   ALREADY two pixels short
         sbTxPercent      120   "Tx Pct 100  %" 117     3

     A percentage applied to all of them would take digits off the frequency boxes, which are the
     fields you can least afford to misread - and they have no room at all before it starts.  So
     each entry widget may shrink only into ITS OWN spare room: the unused width inside its edit
     field, and not a pixel further.  rptSpinBox gives 37, the frequency boxes give nothing, and
     nothing has to be decided by guesswork. */
  inline QString worst_entry_text (QWidget const * w)
  {
    /* CE3TSK 2026-09-18, review: the widest value is not always the maximum - a negative
       minimum carries a sign. rptSpinBox runs -50..49, so "R -50" is 6 px wider than "R 49" at
       12 pt, and with the prefix shortened to "R " that is most of what was left of the 8 px
       anti-clip pad. DTCenterSpinBox (-2.0..2.5) has the same shape. */
    auto const wider = [w] (QString const& a, QString const& b) {
        return w->fontMetrics ().horizontalAdvance (a) >= w->fontMetrics ().horizontalAdvance (b)
               ? a : b;
      };
    if (auto const * s = qobject_cast<QSpinBox const *> (w))
      return wider (s->prefix () + QString::number (s->maximum ()) + s->suffix (),
                    s->prefix () + QString::number (s->minimum ()) + s->suffix ());
    if (auto const * d = qobject_cast<QDoubleSpinBox const *> (w))
      return wider (d->prefix () + QString::number (d->maximum (), 'f', d->decimals ()) + d->suffix (),
                    d->prefix () + QString::number (d->minimum (), 'f', d->decimals ()) + d->suffix ());
    if (auto const * c = qobject_cast<QComboBox const *> (w))
      {
        QString widest;
        for (int i = 0; i < c->count (); ++i)
          if (c->fontMetrics ().horizontalAdvance (c->itemText (i))
              > c->fontMetrics ().horizontalAdvance (widest)) widest = c->itemText (i);
        return widest;
      }
    if (auto const * e = qobject_cast<QLineEdit const *> (w)) return e->text ();
    return QString ();
  }

  /* CE3TSK 2026-09-17, operator's request: the unit suffix is dropped while Narrow controls is
     on.  "Tx  5000  Hz" needs 107 px of text against a minimum of 105, so the box is ALREADY
     clipping part of its own " Hz" at its minimum today; without the suffix the text needs 74 and
     the box can hold a clean "Tx  435".  Measured saving: 33 px of text, about 11 px of minimum
     width per box once the arrows and frame are counted.

     The .ui suffix is remembered here, which is the pattern that went wrong for Hound's
     stylesheet - and the difference is checked, not assumed: nothing in the program calls
     setSuffix() on these boxes (grep: the only setSuffix outside the .ui is WSPRBandHopping's own
     spin box), so the value captured on first sight is the .ui's and stays authoritative.  If
     that ever stops being true this has to move to deriving the suffix rather than restoring it. */
  inline void narrow_entry_suffix (QWidget * child)
  {
    QString current;
    auto * const spin = qobject_cast<QSpinBox *> (child);
    auto * const dspin = qobject_cast<QDoubleSpinBox *> (child);
    if (spin) current = spin->suffix ();
    else if (dspin) current = dspin->suffix ();
    else return;
    if (!child->property ("jtdxSuffix").isValid ())
      {
        if (current.isEmpty ()) return;                 // nothing to drop, nothing to remember
        child->setProperty ("jtdxSuffix", current);
      }
    QString const want = narrow_allowance () > 0 ? QString {}
                                                 : child->property ("jtdxSuffix").toString ();
    if (want == current) return;
    if (spin) spin->setSuffix (want); else dspin->setSuffix (want);
  }

  /* how far this entry widget may be taken down: its spare room, never more.  Returns the floor,
     which is the current minimum when there is nothing to spare. */
  /* What the widget spends on things that are not text: the spin box arrows, the combo box
     arrow, the frame.  Asked of the style at the width we are considering rather than at the
     widget's current size, which during start-up may still be zero.  Measured 12 px for the
     frequency spin boxes - and NOT counting it was a real bug: entry_floor treated the whole
     difference between the minimum and the text as slack and returned floors 12 px too narrow,
     which would have clipped the very digits this is meant to protect. */
  inline int entry_chrome (QWidget * w, int width)
  {
    if (auto * const sb = qobject_cast<QAbstractSpinBox *> (w))
      {
        QStyleOptionSpinBox o;
        o.initFrom (sb);
        o.subControls = QStyle::SC_All;
        o.rect = QRect {0, 0, width, sb->sizeHint ().height ()};
        QRect const ed = sb->style ()->subControlRect (QStyle::CC_SpinBox, &o,
                                                       QStyle::SC_SpinBoxEditField, sb);
        if (ed.width () > 0 && ed.width () < width) return width - ed.width ();
      }
    else if (auto * const cb = qobject_cast<QComboBox *> (w))
      {
        QStyleOptionComboBox o;
        o.initFrom (cb);
        o.rect = QRect {0, 0, width, cb->sizeHint ().height ()};
        QRect const ed = cb->style ()->subControlRect (QStyle::CC_ComboBox, &o,
                                                      QStyle::SC_ComboBoxEditField, cb);
        if (ed.width () > 0 && ed.width () < width) return width - ed.width ();
      }
    else if (qobject_cast<QLineEdit *> (w))
      {
        return 2 * w->style ()->pixelMetric (QStyle::PM_DefaultFrameWidth, nullptr, w) + 4;
      }
    return width;            // unknown: claim it all, so nothing is shrunk by guesswork
  }

  inline int entry_floor (QWidget * w, int settled)
  {
    QString const text = worst_entry_text (w);
    if (text.isEmpty ()) return settled;              // nothing to measure - leave it alone
    /* text + what the style spends on chrome + 8 px so a digit never sits against the frame */
    int const floor = w->fontMetrics ().horizontalAdvance (text) + entry_chrome (w, settled) + 8;
    return floor < settled ? floor : settled;
  }

  /* a leaf whose minimum may be taken below what it needs.  Containers are excluded because
     their minimum is the sum of their contents and squeezing them would double-count; entry
     widgets because a clipped number is a wrong number; and anything the layout is already free
     to shrink (an Ignored horizontal policy), because pinning a minimum there RAISES the floor. */
  inline bool is_entry (QWidget const * w)
  {
    return qobject_cast<QAbstractSpinBox const *> (w) || qobject_cast<QComboBox const *> (w)
        || qobject_cast<QLineEdit const *> (w);
  }

  inline bool narrowable (QWidget const* w)
  {
    /* an entry widget is not a container even though Qt gives it internal children: a QSpinBox
       owns a qt_spinbox_lineedit and a QComboBox its view, and testing for children alone
       excluded every one of them - which is why they all "held (no spare)" the first time. */
    if (!is_entry (w) && !w->findChildren<QWidget *> ().isEmpty ()) return false;
    /* ...but their internal machinery is the spin box's business, not ours */
    if (w->parentWidget () && is_entry (w->parentWidget ())) return false;
    if (qobject_cast<QAbstractScrollArea const *> (w)) return false;
    if (w->sizePolicy ().horizontalPolicy () == QSizePolicy::Ignored) return false;
    return true;
  }

  // what a button's label needs: the text, an icon if it has one, the style's margins and frame,
  // and the indicator of a check box or radio button.  Deliberately NOT sizeHint().
  // what the button actually draws: the label with its mnemonic marker removed, plus any icon
  inline int drawn_width (QAbstractButton const * button)
  {
    QString text = button->text ();
    text.replace ("&&", "\x01");               // a literal ampersand survives as itself
    text.remove ('&');                        // the mnemonic marker is not drawn
    text.replace ("\x01", "&");
    int width = button->fontMetrics ().horizontalAdvance (text);
    if (!button->icon ().isNull ()) width += button->iconSize ().width () + 4;
    return width;
  }

  /* Is the label actually being clipped?  Asked of Qt's own content rectangle rather than of
     label_width () below: label_width is a recommended MINIMUM and over-measures against the
     style's own sizeHint (98 against 92 for "Enable Tx" here), so using it as the test flagged a
     button sitting at its natural width as too narrow.  Measured, and caught by test_uilimits. */
  inline bool label_clipped_at (QPushButton const * button, int width)
  {
    QStyleOptionButton option;
    option.initFrom (button);
    option.text = button->text ();
    option.icon = button->icon ();
    /* CE3TSK 2026-09-19, review: asked of a GIVEN width, not of whatever the layout last handed
       the button. The caller below decides a maximum and must judge that number; measuring the
       live rect instead made the answer depend on the current width, so the same call could
       decide differently on successive passes and the cap oscillated (89, 120, 89, 120 over four
       identical calls). */
    option.rect.setWidth (width > 0 ? width : option.rect.width ());
    QRect const content = button->style ()->subElementRect (QStyle::SE_PushButtonContents,
                                                            &option, button);
    return drawn_width (button) > content.width ();
  }

  inline bool label_clipped (QPushButton const * button)
  {
    return label_clipped_at (button, button->width ());
  }

  inline int label_width (QAbstractButton const * button, int slack = 2)
  {
    QStyleOptionButton option;
    option.initFrom (button);
    int width = drawn_width (button);
    width += 2 * button->style ()->pixelMetric (QStyle::PM_ButtonMargin, &option, button)
           + 2 * button->style ()->pixelMetric (QStyle::PM_DefaultFrameWidth, &option, button);
    if (qobject_cast<QCheckBox const *> (button) || qobject_cast<QRadioButton const *> (button))
      {
        width += button->style ()->pixelMetric (QStyle::PM_IndicatorWidth, &option, button) + 6;
      }
    /* two pixels of slack: under a style sheet the padding comes from the sheet rather than from
       PM_ButtonMargin, and this measure then lands a pixel short - "SWL" wanted 30 and measured 29
       against the dark sheet.  Two pixels cannot revive the 80 px cushion this exists to avoid.
       The slack is a parameter because the "is this button too narrow for its label" test below
       must NOT have it: with it, a button sitting at exactly its natural width measured as too
       narrow and was left-aligned when nothing was clipped (caught by test_uilimits). */
    return width + slack;
  }

  /* CE3TSK 2026-09-18: one widget's limits, declared here because the filter below re-applies
     them - see fit_one for why a stylesheet makes that necessary. */
  inline void fit_one (QWidget * child);

  /* CE3TSK 2026-09-17: left-align a push button's label, but only while the button is actually
     too narrow for it.

     Qt does not elide a push button's label - it clips it - and the label is centred, so a
     squeezed button loses characters from BOTH ends: "Spotted" renders as "potted", not
     "Spo...".  Left-aligned it loses only the tail, which is what the operator asked for.

     Measured before it was written: `text-align: left` on its own moves the ink from x=12 to
     x=2 and removing the sheet again restores the button pixel for pixel (0 differing pixels).
     The rule carries NO padding - `padding-left` changed the button's SIZE, which would feed
     straight back into the minimum-width arithmetic above.  It costs 18 pixels of corner
     antialiasing while applied, because any per-widget sheet hands the widget to
     QStyleSheetStyle; that was measured too and is the whole of the visual cost.

     KNOWN LIMIT, stated rather than hidden: the alignment is re-evaluated on resize and on
     show.  A button whose TEXT changes without its size changing ("DX Call" -> "Spotted")
     keeps the previous alignment until the next resize.  Dragging the splitter - which is when
     any of this is visible at all - generates resizes, so the case that matters is covered. */
  class NarrowLabels : public QObject       // deliberately no Q_OBJECT: nothing here needs moc
  {
  public:
    using QObject::QObject;

    /* OUR rule, marked so it can be found and removed again without knowing what else is in the
       sheet.  The whole point: this function must NEVER cache and restore somebody else's
       stylesheet.  It used to remember a "base" the first time it saw the button and write that
       back when the button widened - but mainwindow's setHoundButtonStyle() sets Hound's sheet
       (colour, min-width: 5em, padding: 3px) LATER than the first sight, so the remembered base
       was empty and restoring it wiped Hound's real style. One transient narrow layout pass was
       enough to trigger it, which is why the operator saw Hound change without ever seeing it
       clipped. Now every call starts from the CURRENT sheet, strips our marked rule and re-adds
       it if needed, so anything the application set in the meantime survives untouched. */
    static char const * marker () { return "\n/*jtdx-narrow*/ QPushButton { text-align: left; }"; }

    static void apply (QPushButton * b)
    {
      QString sheet = b->styleSheet ();
      bool const has = sheet.contains (marker ());
      bool const tight = label_clipped (b);
      /* the state is read back from the sheet itself rather than from a remembered flag: if the
         application replaces the sheet while our rule is on it, the rule goes with it and the
         flag would have been left lying about what the button is wearing. */
      if (has == tight) return;
      sheet.remove (marker ());
      if (tight) sheet += marker ();
      b->setStyleSheet (sheet);
    }

  protected:
    bool eventFilter (QObject * o, QEvent * e) override
    {
      if (QEvent::Resize == e->type () || QEvent::Show == e->type ())
        {
          if (auto * b = qobject_cast<QPushButton *> (o)) apply (b);
        }
      /* CE3TSK 2026-09-18: and a stylesheet undoes the narrow allowance, so it has to be put
         back. Qt's QStyleSheetStyle calls setMinimumWidth () from a sheet's min-width, and the
         application re-styles these buttons on every state change - AutoSeq restyles on the
         QSO state, Hound on the mode, AutoTx and Enable Tx when they go green - so each restyle
         reset the minimum to the sheet's 5em and the button stopped squeezing. Measured: with
         Narrow controls on and the splitter dragged in, AutoSeq3 and Hound sat at 93 px while
         their neighbours went to 50; re-applying the allowance after the sheet takes them to
         69. The guard is for our own apply () above, which sets a sheet of its own. */
      if (QEvent::StyleChange == e->type () && narrow_allowance () > 0)
        {
          static bool busy {false};
          if (!busy)
            {
              busy = true;
              if (auto * w = qobject_cast<QWidget *> (o)) fit_one (w);
              busy = false;
            }
        }
      return QObject::eventFilter (o, e);
    }
  };

  /* Re-evaluate every push button now, whatever events have or have not arrived.  The filter
     alone is not enough: a button whose width does not change gets NO resize event, so once it
     had been left-aligned nothing ever took the alignment off again.  Called after each layout
     change rather than relying on the event. */
  inline void refresh_narrow_labels (QWidget * top)
  {
    for (auto * b : top->findChildren<QPushButton *> ()) NarrowLabels::apply (b);
  }

  /* Watch every push button in this window.  Safe to call repeatedly - a button is only
     installed on once.  Check boxes and radio buttons are left out on purpose: their text is
     already left of the indicator, so there is nothing to re-align. */
  inline void watch_narrow_labels (QWidget * top)
  {
    static NarrowLabels * filter = nullptr;
    if (!filter) filter = new NarrowLabels {qApp};
    for (auto * b : top->findChildren<QPushButton *> ())
      {
        if (b->property ("jtdxWatched").toBool ()) continue;
        b->installEventFilter (filter);
        b->setProperty ("jtdxWatched", true);
      }
  }

  /* CE3TSK 2026-09-18: a lower minimum is not enough to make a button give way.

     QSizePolicy::Minimum carries a grow flag and no SHRINK flag, so the layout treats the
     button's sizeHint as a width it should keep and takes any squeeze out of its neighbours
     instead. Measured with Narrow controls on, dragging the splitter in: AutoSeq3 and Hound held
     93 px all the way down to the 645 px floor even after their minimums had come down to 52 and
     36, while the entry boxes beside them lost characters. TxMinuteButton, Fixed in the .ui, read
     90 / 87 / 65 / 77 across window widths - not even monotonic, because the row rebalanced
     around whatever was still shrinkable.

     So while the allowance is on, a button that cannot shrink is made Preferred - grow AND
     shrink - and the original policy is remembered on the button so that turning Narrow controls
     off restores it exactly. Buttons only: an entry widget must not be squeezed at all (a clipped
     number is a wrong number) and a container's minimum is the sum of its contents. */
  inline void set_button_shrinkable (QWidget * child, bool shrinkable)
  {
    auto * const button = qobject_cast<QAbstractButton *> (child);
    if (!button) return;
    static char const * const kept = "jtdxPolicy";
    auto policy = button->sizePolicy ();
    if (shrinkable)
      {
        if (policy.horizontalPolicy () & QSizePolicy::ShrinkFlag) return;   // already gives way
        if (!button->property (kept).isValid ())
          button->setProperty (kept, int (policy.horizontalPolicy ()));
        policy.setHorizontalPolicy (QSizePolicy::Preferred);
        button->setSizePolicy (policy);
      }
    else if (button->property (kept).isValid ())
      {
        policy.setHorizontalPolicy (QSizePolicy::Policy (button->property (kept).toInt ()));
        button->setSizePolicy (policy);
        button->setProperty (kept, QVariant {});
      }
  }

  /* CE3TSK 2026-09-18: and the sheet's own min-width has to go while the allowance is on.

     Lowering a button's minimumWidth was not enough and neither was making it shrinkable: the
     application's sheets carry `min-width: 5em` on the coloured buttons (19 call sites) and
     `min-width: 63px` on Enable Tx, and that inflates minimumSizeHint - which is what a grid
     COLUMN's minimum is made of. AutoSeq3, Hound and Split Tx/Rx share column 2 of gridLayout,
     so the column could not shrink however low each widget's own minimum went. Measured at the
     645 px floor, with everything else already in place: they held 93 px; with the sheet's
     min-width replaced by 0 they went to 52 and 46.

     The declaration is rewritten in place rather than the sheet rebuilt, the pre-change sheet is
     remembered so that Narrow controls off puts it back exactly, and because the application
     re-styles these buttons on every state change the StyleChange filter runs this again for the
     new sheet. Buttons only, as with the policy above. */
  inline void neutralise_sheet_minimum (QWidget * child, bool narrow)
  {
    auto * const button = qobject_cast<QAbstractButton *> (child);
    if (!button) return;
    static char const * const kept = "jtdxSheet";
    static char const * const kept_stripped = "jtdxSheetStripped";
    /* The declaration is REMOVED, not set to zero. `min-width: 0px` looks harmless and is not:
       QStyleSheetStyle adds the box model to whatever the sheet declares, so a zero min-width
       still produced setMinimumWidth (0 + 6 px padding + 2 px border) = 8 - an 8 px floor on
       AutoSeq3 against a 75 px hint, which is what the row dump showed and a probe then
       reproduced step by step. With the declaration gone Qt sets no minimum from the sheet at
       all and the minimum this pass computed is the one that stands. */
    auto const strip = [] (QString sheet) {
        static QRegularExpression const re {QStringLiteral (R"(\s*min-width\s*:\s*[^;}]+;?)")};
        return sheet.remove (re);
      };
    /* The left-align marker rides on the SAME stylesheet string (NarrowLabels::apply appends
       it), and appending it sends a StyleChange straight back into the filter and into here. If
       the tests below see the marker they no longer recognise our own rewrite: the sheet then
       looks like "the application replaced it with one that pins nothing", both remembered
       values are dropped, and Narrow controls off leaves the button wearing the stripped sheet
       with its min-width gone for good. Reproduced with a probe - "min-width restored: NO -
       LOST" - after the review pointed at it. So every comparison here is made on the sheet
       WITHOUT the marker, and the marker is put back on whatever is written. */
    auto const bare = [] (QString sheet) { return sheet.remove (NarrowLabels::marker ()); };
    bool const marked = button->styleSheet ().contains (NarrowLabels::marker ());
    auto const with_marker = [marked] (QString sheet) {
        return marked ? sheet + NarrowLabels::marker () : sheet;
      };
    if (narrow)
      {
        auto const sheet = bare (button->styleSheet ());
        /* ours already - compared against the stripped form we stored, because after stripping
           there is no min-width left in the text to recognise */
        if (button->property (kept_stripped).isValid ()
            && sheet == button->property (kept_stripped).toString ()) return;
        /* the application has replaced the sheet with one that pins nothing: what we remembered
           is now stale and must go, or turning Narrow controls off would put an old sheet back
           over the current one - the wrong COLOUR, not just the wrong width. Found in review:
           styleChanged () gives AutoTxButton a 5em sheet in one branch and a
           QPushButton:checked sheet with no min-width in the other. */
        if (!sheet.contains (QLatin1String {"min-width"}))
          {
            button->setProperty (kept, QVariant {});
            button->setProperty (kept_stripped, QVariant {});
            return;
          }
        auto const stripped = strip (sheet);
        button->setProperty (kept, sheet);
        button->setProperty (kept_stripped, stripped);
        button->setStyleSheet (with_marker (stripped));
      }
    else if (button->property (kept).isValid ())
      {
        auto const original = button->property (kept).toString ();
        auto const stripped = button->property (kept_stripped).toString ();
        button->setProperty (kept, QVariant {});
        button->setProperty (kept_stripped, QVariant {});
        /* and only put it back if what the button is wearing IS our stripped version of it - if
           the application has set something else since, its sheet is the current truth and ours
           is history. Same rule NarrowLabels::apply follows for its own marker. */
        if (bare (button->styleSheet ()) == stripped) button->setStyleSheet (with_marker (original));
      }
  }

  /* CE3TSK 2026-09-18: the per-widget half of fit_size_limits, on its own so that the
     StyleChange filter can re-run it for a single widget. It always recomputes from the limits
     the .ui asked for (jtdxLimits), so calling it again can never ratchet a minimum down - which
     a "take 75 % of whatever the minimum is now" helper would have done. */
  inline void fit_one (QWidget * child)
  {
      /* Before anything is measured OR remembered: the suffix changes the widget's own hints,
         and the sheet's min-width changes what Qt imposes as a minimum. The sheet has to be
         dealt with FIRST, in this order, for two measured reasons:

           - QStyleSheetStyle re-polishes on every setStyleSheet and writes the sheet's own
             minimum onto the widget, so a sheet changed AFTER this pass had set a minimum simply
             clobbered it (traced: our 84 px became 0 the moment the declaration was removed);
           - and the minimum the sheet imposed would otherwise be captured below as "what the .ui
             asked for" - 113 px of `min-width: 5em` recorded as a design value, and restored as
             one later. */
      narrow_entry_suffix (child);
      neutralise_sheet_minimum (child, narrow_allowance () > 0 && narrowable (child));
      /* remember what the .ui asked for the first time we see the widget, and always work from
         that - otherwise a font increase ratchets the limits up and a later decrease cannot
         bring them back down, leaving the layout inflated until the next restart. */
      if (!child->property ("jtdxLimits").isValid ())
        {
          child->setProperty ("jtdxLimits", QRect {child->minimumWidth (), child->minimumHeight (),
                                                   child->maximumWidth (), child->maximumHeight ()});
        }
      auto const from_ui = child->property ("jtdxLimits").toRect ();
      auto const hint = child->sizeHint ();
      auto const * const button = qobject_cast<QAbstractButton *> (child);
      int const needed = button ? qMin (hint.width (), qMax (from_ui.width (), label_width (button)))
                                : hint.width ();
      child->setMaximumWidth (from_ui.width () < QWIDGETSIZE_MAX
                              ? qMax (from_ui.width (), needed) : from_ui.width ());
      child->setMaximumHeight (from_ui.height () < QWIDGETSIZE_MAX
                               ? qMax (from_ui.height (), hint.height ()) : from_ui.height ());
      /* a button's label is the whole point of the button, so it must not be squeezed below it:
         GenMsgs is pinned at a 60px minimum and S meter is sized oddly by its own stylesheet,
         and both lost characters at a larger font.

         CE3TSK 2026-09-19: computed in FULL before it is written. Writing a minimum resizes the
         widget on the spot, so writing the label's need first and the allowance second let an
         intermediate value escape: Log QSO jumped from its 60 px cell to 76, the layout put it
         back, our StyleChange filter re-ran this pass, and it jumped again - an oscillation whose
         last state is what the operator saw as "at the end it unsqueezes", with the button drawn
         over its neighbour. Traced with a resize watcher that logged which pass was running.
         One write, final value, no transient. */
      int floor_w {button ? qMax (from_ui.x (), qMin (hint.width (), label_width (button)))
                          : from_ui.x ()};
      if (narrow_allowance () > 0 && narrowable (child))
        {
          /* the floor Qt will actually use: an explicit minimumWidth overrides minimumSizeHint,
             and the button rule above has just computed one BELOW the hint (a button's hint
             carries the style's 80 px whatever the label is).  Taking qMax of the two here
             instead measured the allowance against the hint and could RAISE a button's minimum -
             caught by test_uilimits, which failed on exactly that. */
          int const settled = floor_w > 0 ? floor_w : child->minimumSizeHint ().width ();
          int want = settled * (100 - narrow_allowance ()) / 100;
          /* an entry widget never goes below what its own content needs, whatever the
             allowance says - a clipped number is a wrong number */
          if (is_entry (child)) want = qMax (want, entry_floor (child, settled));
          if (want < settled) floor_w = want;
        }
      /* ...but a splitter pane whose floor we are holding is not ours to write: set_pane_floor
         put an explicit minimum there on purpose, and writing the .ui's 0 over it wipes the
         floor. The two rules were fighting and whoever ran last won - measured at start-up, where
         the font applied after the window is built re-ran this pass and left the pane at 0 until
         the next toggle. The mark is set_pane_floor's own remembered policy. */
      if (!child->property ("jtdx_pane_policy").isValid ()) child->setMinimumWidth (floor_w);
      if (button) child->setMinimumHeight (qMax (from_ui.y (), hint.height ()));
      /* the policy and the marked cap, which depend on the minimum just written */
      if (narrow_allowance () > 0 && narrowable (child))
        {
          set_button_shrinkable (child, true);
          /* CE3TSK 2026-09-18: and a button MARKED for it is capped at what its label needs
             while the allowance is on, so it sizes to its content instead of holding the
             designer's number. The operator asked for this on the TX minute button, which is
             capped at 90 in the .ui and kept 90 at every width because its row's labels gave way
             instead.

             Marked one widget at a time, with a dynamic property in the .ui, because the general
             rule - every button the .ui capped - broke the layout: those buttons sit in a
             QGridLayout and a grid shares its column width between rows, so lowering one item's
             maximum shrank the column for all of them and Qt then drew each at its own minimum,
             overlapping its neighbour. Enable Tx over Halt Tx, Log QSO over Erase, reported from
             the running application and confirmed against the build before the rule. Anything
             added here needs the same look at the layout it lives in.

             Never above the cap this pass settled (that would undo the anti-clipping raise for a
             long translation), never below the minimum just computed, and a candidate that clips
             per the style's own content rectangle is dropped - label_width () spends the base
             style's margins, not a stylesheet's padding and border. */
          if (button && child->property ("jtdxNarrowCap").toBool ())
            {
              int const settled_cap = child->maximumWidth ();
              int const candidate = qMax (child->minimumWidth (),
                                          qMin (settled_cap, label_width (button)));
              /* decided BEFORE anything is written, like the minimum above: the candidate is
                 judged at ITS own width. Writing it first and asking afterwards made the answer
                 depend on the width the layout happened to have given the button, so the cap
                 alternated between the candidate and the designer's number on successive passes
                 - the same escaping-intermediate-value failure, one branch over. Found in
                 review, which measured 89 / 120 / 89 / 120 over four identical calls. */
              auto const * const push = qobject_cast<QPushButton *> (child);
              bool const fits = !push || !label_clipped_at (push, candidate);
              if (candidate < settled_cap && fits) child->setMaximumWidth (candidate);
            }
        }
      else
        {
          set_button_shrinkable (child, false);   /* off leaves no trace, as everything here does */
        }
  }

  /* Raise every hard limit in this window to what the current font needs, and no further.  Run at
     start-up and again whenever the application font changes; safe to run repeatedly. */
  inline void fit_size_limits (QWidget * top)
  {
    for (auto * child : top->findChildren<QWidget *> ()) fit_one (child);
  }
}

#endif
