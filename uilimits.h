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
#include <QCheckBox>
#include <QRadioButton>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStyle>
#include <QStyleOptionButton>
#include <QWidget>

namespace JTDX
{
  // what a button's label needs: the text, an icon if it has one, the style's margins and frame,
  // and the indicator of a check box or radio button.  Deliberately NOT sizeHint().
  inline int label_width (QAbstractButton const * button)
  {
    QStyleOptionButton option;
    option.initFrom (button);
    QString text = button->text ();
    text.replace ("&&", "\x01");               // a literal ampersand survives as itself
    text.remove ('&');                        // the mnemonic marker is not drawn
    text.replace ("\x01", "&");
    int width = button->fontMetrics ().horizontalAdvance (text);
    if (!button->icon ().isNull ()) width += button->iconSize ().width () + 4;
    width += 2 * button->style ()->pixelMetric (QStyle::PM_ButtonMargin, &option, button)
           + 2 * button->style ()->pixelMetric (QStyle::PM_DefaultFrameWidth, &option, button);
    if (qobject_cast<QCheckBox const *> (button) || qobject_cast<QRadioButton const *> (button))
      {
        width += button->style ()->pixelMetric (QStyle::PM_IndicatorWidth, &option, button) + 6;
      }
    /* two pixels of slack: under a style sheet the padding comes from the sheet rather than from
       PM_ButtonMargin, and this measure then lands a pixel short - "SWL" wanted 30 and measured 29
       against the dark sheet.  Two pixels cannot revive the 80 px cushion this exists to avoid. */
    return width + 2;
  }

  /* Raise every hard limit in this window to what the current font needs, and no further.  Run at
     start-up and again whenever the application font changes; safe to run repeatedly. */
  inline void fit_size_limits (QWidget * top)
  {
    for (auto * child : top->findChildren<QWidget *> ())
      {
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
           and both lost characters at a larger font. */
        if (button)
          {
            child->setMinimumWidth (qMax (from_ui.x (), qMin (hint.width (), label_width (button))));
            child->setMinimumHeight (qMax (from_ui.y (), hint.height ()));
          }
      }
  }
}

#endif
