#ifndef SWATCH_CONTRAST_H
#define SWATCH_CONTRAST_H

/* CE3TSK 2026-11-12 (TODO.md 3.-246): Settings > Notifications marks a colour pair that is hard to read.

   Every preview label (swatch) of the tab shows one text colour on one background, as a decoded line shows them: the
   message's colour (CQ, my call, standard call) and a notification's (new DXCC, worked, ...), swapped with "Inverse
   text/background color". The recommended palette (Configuration.cpp recommended_colors[], UI_DARK_STYLE.md) clears
   3:1 on every pair it can produce; a colour of the operator's own may not. Rather than change the operator's colours
   while a line is drawn (a patch of 2026-11-12 painted every highlighted line black or white, and the message's colour
   was lost with it), the swatch that would be hard to read says so where the colour is chosen: a dashed frame over it,
   and its tooltip gives the ratio and names the Recommended colors button.

   Nothing of the dialog's 600-odd style sheet calls changes: Marker is an event filter on the swatches, which reads the
   pair back from the style sheet the dialog has just given the label ("background: X;color: Y", the forms the dialog
   writes) - so a swatch repainted by any path is judged as it is shown. It draws over the label after the label has
   drawn itself, and never touches the style sheet, so it neither recurses nor moves the layout.

   The floor is 3:1, tools/ui-check/contrast.py's target for the decode notifications: 3:1 is the AA line for large
   text, and the decode windows use a bold monospace face; lifting "my call" to 4.5:1 on every background turns it pale
   pink and loses the signal its colour carries. The ratio is WCAG 2's, as contrast.py computes it. */

#include <algorithm>
#include <cmath>
#include <functional>
#include <QColor>
#include <QEvent>
#include <QHelpEvent>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QRegularExpression>
#include <QToolTip>

namespace swatch_contrast
{
  double constexpr floor_ratio {3.0};

  inline double luminance (QColor const& c)
  {
    auto const lin = [] (double v) {return v <= 0.04045 ? v / 12.92 : std::pow ((v + 0.055) / 1.055, 2.4);};
    return 0.2126 * lin (c.redF ()) + 0.7152 * lin (c.greenF ()) + 0.0722 * lin (c.blueF ());
  }

  // WCAG 2 contrast ratio, 1 to 21, the same whichever of the two is the text
  inline double ratio (QColor const& a, QColor const& b)
  {
    double const x = luminance (a), y = luminance (b);
    return (std::max (x, y) + 0.05) / (std::min (x, y) + 0.05);
  }

  // black or white, whichever stands out more against `background` (their contrasts are equal at sqrt (0.0525) - 0.05;
  // the rule of David A92GE's patch of 2026-11-12) - the colour of the frame, readable on any swatch
  inline QColor frame_colour (QColor const& background)
  {
    return luminance (background) >= std::sqrt (0.0525) - 0.05 ? QColor {Qt::black} : QColor {Qt::white};
  }

  struct Pair
  {
    QColor background, text;
    bool valid () const {return background.isValid () && text.isValid ();}
  };

  // the colours a swatch shows, read back from its style sheet: "background: X" and "color: Y" (never
  // "background-color" - the dialog's swatches do not use it); a sheet with a background and no text colour shows
  // `default_text` (Tx messages: the transmitted line's text is the window's own)
  inline Pair pair_of (QString const& sheet, QColor const& default_text)
  {
    static QRegularExpression const bg {R"((?:^|;)\s*background\s*:\s*([^;]+))"};
    static QRegularExpression const fg {R"((?:^|;)\s*color\s*:\s*([^;]+))"};
    Pair p;
    auto const b = bg.match (sheet);
    if (!b.hasMatch ()) return p;
    p.background = QColor {b.captured (1).trimmed ()};
    auto const f = fg.match (sheet);
    p.text = f.hasMatch () ? QColor {f.captured (1).trimmed ()} : default_text;
    return p;
  }

  inline bool hard_to_read (Pair const& p)
  {
    return p.valid () && ratio (p.background, p.text) < floor_ratio;
  }

  /* the event filter: installed on each swatch label, parented to the dialog. `default_text` gives the text colour of a
     swatch whose sheet names none, `tip` the translated tooltip for a ratio - both asked at the moment of use, so a
     style switched or a language changed while the dialog is open is followed */
  class Marker : public QObject
  {
  public:
    Marker (std::function<QColor ()> default_text, std::function<QString (double)> tip, QObject * parent)
      : QObject {parent}
      , default_text_ {std::move (default_text)}
      , tip_ {std::move (tip)}
    {
    }

    Pair pair (QLabel const * label) const {return pair_of (label->styleSheet (), default_text_ ());}

    bool eventFilter (QObject * o, QEvent * e) override
    {
      auto * label = qobject_cast<QLabel *> (o);
      if (!label || (e->type () != QEvent::Paint && e->type () != QEvent::ToolTip)) return false;
      Pair const p {pair (label)};
      if (!hard_to_read (p)) return false;
      if (e->type () == QEvent::Paint)
        {
          static_cast<QObject *> (label)->event (e);   // the swatch as its style sheet draws it - then the frame over it
          QPainter painter {label};
          painter.setPen (QPen {frame_colour (p.background), 2, Qt::DashLine});
          painter.drawRect (QRectF {label->rect ()}.adjusted (1, 1, -1, -1));
          return true;
        }
      /* (the operator 2026-11-13) shown for the swatch's window, not the swatch: a tooltip takes the style sheet of the
         widget it is shown for, and the swatch's own is the very pair it warns of - the tip came out in it, #ff6fff on
         #ff6fff, unreadable. For the window it is drawn as every other tooltip of the dialog. */
      QToolTip::showText (static_cast<QHelpEvent *> (e)->globalPos (), tip_ (ratio (p.background, p.text)), label->window ());
      return true;
    }

  private:
    std::function<QColor ()> default_text_;
    std::function<QString (double)> tip_;
  };
}

#endif
