#ifndef STATUSLAMP_H
#define STATUSLAMP_H

/* CE3TSK 2026-11-25 (TODO.md 3.-300, the operator: "let's put status lamps and tooltips that show on the Settings > Logbooks
   tab for the individual services, e.g. to visualize if the send of logged QSOs is working (Success: green) or not (Error:
   red) or nothing happened (grey), and the same thing for the Download LoTW confirmations aspect"): a round lamp drawn into
   a QLabel - green, red or grey - with the tooltip of the line it heads. Its state is kept in the property "lamp" (0 grey,
   1 green, 2 red): what a test reads, the colours being the drawing's. */

#include <QColor>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QString>

namespace status_lamp
{
  enum State {Idle = 0, Good = 1, Bad = 2};

  inline QColor colour (State state)
  {
    return Good == state ? QColor {0x2e, 0xb8, 0x4b} : Bad == state ? QColor {0xe0, 0x3c, 0x31} : QColor {0x8c, 0x8c, 0x8c};
  }

  inline void show (QLabel * lamp, State state, QString const& tip)
  {
    if (!lamp) return;
    int const size {12};
    qreal const dpr {lamp->devicePixelRatioF ()};
    QPixmap pixmap {QSize {size, size} * dpr};
    pixmap.setDevicePixelRatio (dpr);
    pixmap.fill (Qt::transparent);
    {
      QPainter p {&pixmap};
      p.setRenderHint (QPainter::Antialiasing);
      p.setPen (QPen {colour (state).darker (140), 1});
      p.setBrush (colour (state));
      p.drawEllipse (QRectF {1, 1, size - 2.0, size - 2.0});
    }
    lamp->setPixmap (pixmap);
    lamp->setFixedSize (size, size);
    lamp->setProperty ("lamp", int (state));
    lamp->setToolTip (tip);
  }
}

#endif
