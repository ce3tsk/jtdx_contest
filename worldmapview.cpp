// CE3TSK 2026-11-16 (TODO.md 3.-260, RECEPTION_MAP_PLAN.md stage 1): see worldmapview.h
#include "worldmapview.h"

#include <cmath>
#include <QAction>
#include <QByteArray>
#include <QColor>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QRectF>
#include <QScopedPointer>
#include <QTimer>
#include <QToolTip>
#include <QFontMetricsF>
#include <QTransform>
#include <QVector>
#include <QWheelEvent>
#include "JTDXDateTime.h"

#include "moc_worldmapview.cpp"

namespace
{
  int const nightDivisor {3};          // the shade is computed for every third pixel each way, and drawn smoothed
  double const wheelStep {1.25};       // one notch of the wheel
  double const keyStep {1.5};          // + and -
  double const doubleClickStep {2.0};

  // a polygon of the map on the unit square: its outline and holes as one path (odd-even, so a hole stays a hole)
  struct Shape
  {
    QPainterPath path;
    QRectF box;
  };

  // the map file inside the program: its header read at the first use, each level decoded when first drawn. One for
  // every map of the program - it never changes.
  struct Map
  {
    bool tried {false};
    bool ok {false};
    QString error;
    QByteArray bytes;
    world_map::File file;
    QVector<Shape> shapes[3];          // by world_map::Kind
    int state[3] {0, 0, 0};            // 0 not asked for yet, 1 there, -1 failed

    bool header ()
    {
      if (tried) return ok;
      tried = true;
      QFile f {":/worldmap.bin"};
      if (!f.open (QIODevice::ReadOnly))
        {
          error = QStringLiteral ("the map file is not in the program (:/worldmap.bin)");
          qWarning () << "Reception map:" << error;
          return false;
        }
      bytes = f.readAll ();
      std::string why;
      ok = world_map::readHeader (reinterpret_cast<unsigned char const *> (bytes.constData ()), static_cast<std::size_t> (bytes.size ()), file, why);
      if (!ok)
        {
          error = QString::fromStdString (why);
          qWarning () << "Reception map: the map file is refused:" << error;
        }
      return ok;
    }

    // a level's shapes; none when the file has no such level or the level is refused
    QVector<Shape> const * layer (int kind)
    {
      if (kind < 0 || kind > 2 || !header ()) return nullptr;
      if (state[kind]) return state[kind] > 0 ? &shapes[kind] : nullptr;
      state[kind] = -1;
      for (std::size_t i = 0; i < file.layers.size (); ++i)
        {
          if (file.layers[i].kind != kind) continue;
          world_map::Layer l;
          std::string why;
          if (!world_map::readLayer (reinterpret_cast<unsigned char const *> (bytes.constData ()), static_cast<std::size_t> (bytes.size ()), file, i, l, why))
            {
              error = QString::fromStdString (why);
              qWarning () << "Reception map: a level of the map file is refused:" << error;
              return nullptr;
            }
          QVector<Shape>& out = shapes[kind];
          out.reserve (static_cast<int> (l.polygons.size ()));
          for (auto const& p : l.polygons)
            {
              Shape s;
              for (std::uint32_t r = p.firstRing; r < p.firstRing + p.rings; ++r)
                {
                  auto const& ring = l.rings[r];
                  for (std::uint32_t k = 0; k < ring.count; ++k)
                    {
                      QPointF const at {world_map::xOfLongitude (l.lon[ring.first + k] / world_map::unitsPerDegree),
                                        world_map::yOfLatitude (l.lat[ring.first + k] / world_map::unitsPerDegree)};
                      if (k) s.path.lineTo (at); else s.path.moveTo (at);
                    }
                  s.path.closeSubpath ();
                }
              double const x1 {world_map::xOfLongitude (p.minLon / world_map::unitsPerDegree)}, x2 {world_map::xOfLongitude (p.maxLon / world_map::unitsPerDegree)};
              double const y1 {world_map::yOfLatitude (p.maxLat / world_map::unitsPerDegree)}, y2 {world_map::yOfLatitude (p.minLat / world_map::unitsPerDegree)};
              s.box = QRectF {x1, y1, x2 - x1, y2 - y1};
              out.push_back (s);
            }
          state[kind] = 1;
          return &out;
        }
      return nullptr;
    }
  };

  Map& theMap ()
  {
    static Map map;
    return map;
  }

  bool sameView (world_map::View const& a, world_map::View const& b)
  {
    return a.cx == b.cx && a.cy == b.cy && a.zoom == b.zoom && a.w == b.w && a.h == b.h;
  }

  // every shape of a level that a part of the window shows (in the window's pixels; all of it for the whole map), in
  // each copy of the world the view reaches. Two pixels more each way: a border's width and its smoothed edge reach
  // into the part from a shape that ends beside it.
  void drawShapes (QPainter& p, world_map::View const& v, QVector<Shape> const& shapes, QRectF const& part)
  {
    double const side {v.side ()};
    QRectF const seen {v.cx + (part.left () - 2 - v.w / 2.0) / side, v.cy + (part.top () - 2 - v.h / 2.0) / side,
                       (part.width () + 4) / side, (part.height () + 4) / side};
    for (int copy = -1; copy <= 1; ++copy)
      {
        QRectF const here {seen.translated (-copy, 0)};
        if (here.right () < 0.0 || here.left () > 1.0) continue;
        QTransform t;
        t.translate (v.w / 2.0 + (copy - v.cx) * side, v.h / 2.0 - v.cy * side);
        t.scale (side, side);
        p.setTransform (t);
        for (auto const& s : shapes)
          {
            if (s.box.right () < here.left () || s.box.left () > here.right () || s.box.bottom () < here.top () || s.box.top () > here.bottom ()) continue;
            p.drawPath (s.path);
          }
      }
    p.resetTransform ();
  }
}

WorldMapView::WorldMapView (QWidget * parent)
  : QWidget {parent}
  , m_timer {new QTimer {this}}
  , m_rest {new QTimer {this}}
{
  setAttribute (Qt::WA_OpaquePaintEvent);   // every pixel is painted here
  setFocusPolicy (Qt::StrongFocus);         // + - Home
  setCursor (Qt::OpenHandCursor);
  setMouseTracking (true);                  // the tooltip of a mark
  m_timer->setInterval (20 * 1000);
  connect (m_timer, &QTimer::timeout, this, &WorldMapView::minuteTick);
  m_rest->setSingleShot (true);
  m_rest->setInterval (700);
  connect (m_rest, &QTimer::timeout, this, &WorldMapView::rested);
  m_view = world_map::defaultView (width (), height (), false, 0.0);
}

WorldMapView::~WorldMapView ()
{
}

bool WorldMapView::mapLoaded ()
{
  return theMap ().header () && theMap ().layer (world_map::Land50);
}

QString WorldMapView::mapError ()
{
  return theMap ().error;
}

QSize WorldMapView::sizeHint () const
{
  return {733, 416};   // the main window's width; at it the map shows 78 N to 60 S whole
}

QSize WorldMapView::minimumSizeHint () const
{
  return {320, 200};
}

void WorldMapView::setDarkStyle (bool dark)
{
  if (dark == m_dark) return;
  m_dark = dark;
  update ();
}

void WorldMapView::setOwnLocator (QString const& locator)
{
  // (TODO.md 3.-261) FF46q, FF46qp1: the Settings take an odd character at the end - our station is then where the
  // locator before it has it, not nowhere
  std::string const whole {world_map::wholeLocator (locator.trimmed ().toStdString ())};
  m_own = world_map::place (whole);
  m_ownLocator = QString::fromStdString (whole);
  fitView ();   // at its own view the map follows our longitude
  update ();
}

QPointF WorldMapView::ownStationPixel () const
{
  double const side {m_view.side ()};
  double x {m_view.screenX (world_map::xOfLongitude (m_own.lon))};
  x -= side * std::round ((x - m_view.w / 2.0) / side);   // the copy nearest the middle
  return {x, m_view.screenY (world_map::yOfLatitude (m_own.lat))};
}

void WorldMapView::setClock (JTDXDateTime * clock)
{
  m_clock = clock;
  update ();
}

void WorldMapView::setTickInterval (int ms)
{
  m_timer->setInterval (ms);
}

bool WorldMapView::ticking () const
{
  return m_timer->isActive ();
}

void WorldMapView::setFixedTime (qint64 unixSeconds)
{
  m_fixedTime = unixSeconds;
  update ();
}

qint64 WorldMapView::nowSeconds () const
{
  if (m_fixedTime >= 0) return m_fixedTime;
  return (m_clock ? m_clock->currentMSecsSinceEpoch2 () : QDateTime::currentMSecsSinceEpoch ()) / 1000;
}

void WorldMapView::setView (double cx, double cy, double zoom)
{
  m_pinX = cx; m_pinY = cy; m_pinZoom = zoom;
  m_pinned = true;
  m_atHome = false;
  fitView ();
  update ();
}

void WorldMapView::home ()
{
  unrest ();
  m_atHome = true;
  m_pinned = false;
  fitView ();
  update ();
}

void WorldMapView::viewTouched ()
{
  unrest ();   // what a resting mouse was told was of the place then under it
  m_atHome = false;
  m_pinned = false;
  update ();
}

// The window's size decides how far north and south the middle may go, so a stored view is fitted from what was
// stored each time - fitted once to the size the widget has before it is shown, a view at the world's upper edge
// came back lower.
void WorldMapView::fitView ()
{
  if (m_atHome)
    {
      m_view = world_map::defaultView (width (), height (), m_own.ok, m_own.lon);
      return;
    }
  if (m_pinned)
    {
      m_view.cx = m_pinX; m_view.cy = m_pinY; m_view.zoom = m_pinZoom;
    }
  m_view.w = width (); m_view.h = height ();
  world_map::settle (m_view);
}

void WorldMapView::resizeEvent (QResizeEvent *)
{
  fitView ();
  update ();
}

void WorldMapView::wheelEvent (QWheelEvent * e)
{
  double const notches {e->angleDelta ().y () / 120.0};
  if (0.0 == notches)
    {
      e->ignore ();
      return;
    }
#if QT_VERSION >= QT_VERSION_CHECK (5, 14, 0)
  QPointF const at {e->position ()};
#else
  QPointF const at {e->posF ()};   // (TODO.md 3.-261) position () came with Qt 5.14
#endif
  world_map::zoomAbout (m_view, std::pow (wheelStep, notches), at.x (), at.y ());
  viewTouched ();
  e->accept ();
}

void WorldMapView::mousePressEvent (QMouseEvent * e)
{
  if (Qt::LeftButton != e->button ())
    {
      QWidget::mousePressEvent (e);
      return;
    }
  m_dragging = true;
  m_dragFrom = e->pos ();
  unrest ();
  unhover ();
  setCursor (Qt::ClosedHandCursor);
}

void WorldMapView::mouseMoveEvent (QMouseEvent * e)
{
  if (!m_dragging)
    {
      hover (e->pos (), true);
      return;
    }
  QPoint const by {e->pos () - m_dragFrom};
  if (by.isNull ()) return;
  m_dragFrom = e->pos ();
  world_map::drag (m_view, by.x (), by.y ());
  viewTouched ();
}

void WorldMapView::mouseReleaseEvent (QMouseEvent * e)
{
  if (Qt::LeftButton != e->button ()) return;
  m_dragging = false;
  setCursor (Qt::OpenHandCursor);
  hover (e->pos (), true);
}

void WorldMapView::mouseDoubleClickEvent (QMouseEvent * e)
{
  if (Qt::LeftButton != e->button ()) return;
  world_map::zoomAbout (m_view, doubleClickStep, e->pos ().x (), e->pos ().y ());
  viewTouched ();
}

void WorldMapView::keyPressEvent (QKeyEvent * e)
{
  switch (e->key ())
    {
    case Qt::Key_Plus:
    case Qt::Key_Equal:   // the + key without Shift
      world_map::zoomAbout (m_view, keyStep, width () / 2.0, height () / 2.0);
      viewTouched ();
      break;
    case Qt::Key_Minus:
      world_map::zoomAbout (m_view, 1.0 / keyStep, width () / 2.0, height () / 2.0);
      viewTouched ();
      break;
    case Qt::Key_Home:
      home ();
      break;
    default:
      QWidget::keyPressEvent (e);
    }
}

QMenu * WorldMapView::makeMenu ()
{
  auto * menu = new QMenu {this};
  connect (menu->addAction (tr ("Centre on my station")), &QAction::triggered, this, &WorldMapView::home);
  // three fits, the widest first; each greyed while the map has no such station
  auto fit = [this, menu] (QString const& text, Fit which) {
    QAction * const a {menu->addAction (text)};
    a->setEnabled (any (which));
    connect (a, &QAction::triggered, this, [this, which] {zoomTo (which);});
  };
  fit (tr ("Zoom to stations that hear me or I hear"), Either);
  fit (tr ("Zoom to stations that hear me"), HearMe);
  fit (tr ("Zoom to stations I hear"), IHear);
  if (!m_menuActions.isEmpty ())   // the window's: what is shown, how much is asked of the server
    {
      menu->addSeparator ();
      menu->addActions (m_menuActions);
    }
  return menu;
}

void WorldMapView::contextMenuEvent (QContextMenuEvent * e)
{
  QScopedPointer<QMenu> menu {makeMenu ()};
  menu->exec (e->globalPos ());
}

void WorldMapView::showEvent (QShowEvent *)
{
  m_timer->start ();
}

void WorldMapView::hideEvent (QHideEvent *)
{
  m_timer->stop ();
}

// the night moves a quarter of a degree a minute: the picture follows it by the minute
void WorldMapView::minuteTick ()
{
  if (nowSeconds () / 60 != m_nightMinute) update ();
}

// the sea, the land with its borders and the lakes of a part of the window (its pixels), at the level m_levelDrawn
void WorldMapView::paintBase (QPainter& p, QRect const& part)
{
  world_map::Colours const c {world_map::colours (m_dark)};
  p.save ();
  p.setClipRect (part);
  p.fillRect (part, QColor {c.sea});
  auto const * land = m_levelDrawn < 0 ? nullptr : theMap ().layer (m_levelDrawn ? world_map::Land10 : world_map::Land50);
  if (land)
    {
      p.setRenderHint (QPainter::Antialiasing);
      QPen border {QColor {c.border}};
      border.setWidthF (0.8);
      border.setCosmetic (true);
      p.setPen (border);
      p.setBrush (QColor {c.land});
      drawShapes (p, m_view, *land, part);
      if (auto const * lakes = theMap ().layer (world_map::Lakes))
        {
          p.setPen (Qt::NoPen);
          p.setBrush (QColor {c.sea});
          drawShapes (p, m_view, *lakes, part);
        }
    }
  p.restore ();
}

void WorldMapView::drawBase ()
{
  qreal const ratio {devicePixelRatioF ()};
  m_base = QPixmap {QSize {m_view.w, m_view.h} * ratio};
  m_base.setDevicePixelRatio (ratio);
  int level {world_map::level (m_view)};
  auto const * land = theMap ().layer (level ? world_map::Land10 : world_map::Land50);
  if (!land && level)   // the finer level refused: the coarser one
    {
      level = 0;
      land = theMap ().layer (world_map::Land50);
    }
  m_levelDrawn = land ? level : -1;
  m_baseView = m_view;
  m_baseDark = m_dark;
  m_baseRatio = ratio;
  {
    QPainter p {&m_base};
    paintBase (p, QRect {0, 0, m_view.w, m_view.h});
  }
  ++m_baseDraws;
}

/* (the review of 2026-11-16, TODO.md 3.-261: the whole base map was drawn again at every step of a drag - 8 to 34 ms a
   step, measured) The view moved by whole pixels (worldmap.h pan ()): the base map as it is drawn is moved by them, and
   only the strips that came into the window are drawn - the same pixels a drawing of the whole would give. */
void WorldMapView::moveBase (int dx, int dy)
{
  QPixmap moved {m_base.size ()};
  moved.setDevicePixelRatio (m_baseRatio);
  m_baseView = m_view;
  {
    QPainter p {&moved};
    p.setCompositionMode (QPainter::CompositionMode_Source);
    p.drawPixmap (dx, dy, m_base);
    p.setCompositionMode (QPainter::CompositionMode_SourceOver);
    QRect const strips[2] {
      dx > 0 ? QRect {0, 0, dx, m_view.h} : QRect {m_view.w + dx, 0, -dx, m_view.h},
      dy > 0 ? QRect {0, 0, m_view.w, dy} : QRect {0, m_view.h + dy, m_view.w, -dy}};
    for (auto const& strip : strips) if (!strip.isEmpty ()) paintBase (p, strip);
  }
  m_base = moved;
  ++m_baseMoves;
}

void WorldMapView::drawNight (qint64 minute)
{
  world_map::Sun const sun {world_map::subsolar (minute * 60.0)};
  double const full {world_map::colours (m_dark).night};
  int const w {m_view.w / nightDivisor + 1}, h {m_view.h / nightDivisor + 1};
  m_night = QImage {w, h, QImage::Format_ARGB32_Premultiplied};
  // (TODO.md 3.-261) a column's and a row's part of the sun's altitude worked out once each (worldmap.h Shade)
  std::vector<double> lons (static_cast<std::size_t> (w));
  for (int x = 0; x < w; ++x) lons[static_cast<std::size_t> (x)] = world_map::longitudeOfX (m_view.worldX (x * nightDivisor));
  world_map::Shade shade {sun, lons};
  for (int y = 0; y < h; ++y)
    {
      shade.row (world_map::latitudeOfY (m_view.worldY (y * nightDivisor)));
      auto * line = reinterpret_cast<QRgb *> (m_night.scanLine (y));
      for (int x = 0; x < w; ++x)
        {
          int const alpha {static_cast<int> (shade.at (static_cast<std::size_t> (x)) * full * 255.0 + 0.5)};
          line[x] = qRgba (0, 0, 0, alpha);   // black: premultiplied it is the same
        }
    }
  m_nightView = m_view;
  m_nightDark = m_dark;
  m_nightMinute = minute;
  ++m_nightDraws;
}

void WorldMapView::paintEvent (QPaintEvent *)
{
  if (m_view.w != width () || m_view.h != height ()) fitView ();   // painted before the first resize event
  if (m_base.isNull () || m_baseDark != m_dark || m_baseRatio != devicePixelRatioF ()) drawBase ();
  else if (!sameView (m_baseView, m_view))
    {
      // (TODO.md 3.-261) moved by whole pixels - a drag, an arrow key: the drawing is moved, not made again; whole pixels
      // of the screen too, where one of ours is more than one of its
      int dx {0}, dy {0};
      bool const moved {world_map::pan (m_baseView, m_view, dx, dy)
                        && std::floor (dx * m_baseRatio) == dx * m_baseRatio && std::floor (dy * m_baseRatio) == dy * m_baseRatio};
      if (moved) moveBase (dx, dy); else drawBase ();
    }
  qint64 const minute {nowSeconds () / 60};
  if (m_night.isNull () || !sameView (m_nightView, m_view) || m_nightDark != m_dark || m_nightMinute != minute) drawNight (minute);

  QPainter p {this};
  p.drawPixmap (0, 0, m_base);
  p.setRenderHint (QPainter::SmoothPixmapTransform);
  p.drawImage (QRectF {0, 0, qreal (m_night.width () * nightDivisor), qreal (m_night.height () * nightDivisor)}, m_night);
  paintEntities (p);
  paintStations (p);
  if (m_own.ok)   // our station: a ring with a dot, in the text's colour
    {
      world_map::Colours const c {world_map::colours (m_dark)};
      QPointF const at {ownStationPixel ()};
      p.setRenderHint (QPainter::Antialiasing);
      p.setBrush (Qt::NoBrush);
      p.setPen (QPen {QColor {c.ink}, 1.6});
      p.drawEllipse (at, 9.0, 9.0);
      p.setPen (QPen {QColor {c.surface}, 1.6});
      p.setBrush (QColor {c.ink});
      p.drawEllipse (at, 4.2, 4.2);
    }
  paintLine (p);
}

// ---------------------------------------------------------------------------------------------------- the stations
namespace
{
  double const ownRadius {11.0};       // our own mark's reach for the mouse: its ring, and two pixels
  double const dotRadius {2.5};        // a station that only reports
  double const markRadius {4.6};       // one that hears us; and half a diamond's width, one we hear
  int const labelPixels {10};

  // a frequency in MHz, its kilohertz and hertz apart: 14.076 882
  QString megahertz (long long hz)
  {
    QString const digits {QString::number (hz / 1000000.0, 'f', 6)};
    return digits.left (digits.size () - 3) + QLatin1Char (' ') + digits.right (3);
  }

  QString signedDb (int snr) {return (snr >= 0 ? QStringLiteral ("+") : QString {}) + QString::number (snr);}

  QString utc (long long seconds) {return QDateTime::fromSecsSinceEpoch (seconds, Qt::UTC).toString (QStringLiteral ("HH:mm:ss"));}

  // black or white, whichever reads better on a colour (WCAG 2's ratio)
  QColor inkOn (QColor const& c)
  {
    auto lin = [] (double u) {return u <= 0.04045 ? u / 12.92 : std::pow ((u + 0.055) / 1.055, 2.4);};
    double const l {0.2126 * lin (c.redF ()) + 0.7152 * lin (c.greenF ()) + 0.0722 * lin (c.blueF ())};
    return (l + 0.05) / 0.0533 > 1.05 / (l + 0.05) ? QColor {"#0b0b0b"} : QColor {"#ffffff"};
  }

  // A tooltip as it is shown: every line escaped - a receiver's antenna is typed by strangers, an entity's name may
  // be a translation, and a tooltip that looks like HTML is rendered as HTML.
  QString asHtml (QStringList const& lines)
  {
    QStringList escaped;
    for (auto const& l : lines) escaped << l.toHtmlEscaped ();
    return escaped.isEmpty () ? QString {} : QStringLiteral ("<p style=\"white-space:pre\">") + escaped.join (QStringLiteral ("<br>")) + QStringLiteral ("</p>");
  }

  QFont labelFont (QFont f)
  {
    f.setPixelSize (labelPixels);
    f.setBold (true);
    return f;
  }
}

void WorldMapView::setStations (std::vector<reception_spots::Station> const& stations, long long now)
{
  m_stations = stations;
  m_now = now;
  m_laidOut = false;
  if (m_mouseIn && !m_dragging) hover (m_mouse, false);   // the stations changed under a resting mouse
  update ();
}

void WorldMapView::setMode (QString const& mode) {m_mode = mode;}

void WorldMapView::setBottomInset (int pixels)
{
  m_bottomInset = pixels;
  m_laidOut = false;
  update ();
}

void WorldMapView::setLookups (std::function<QString (QString const&)> const& entity, std::function<QString (QString const&)> const& distance)
{
  m_entity = entity;
  m_distance = distance;
}

// an age as a label has it: seconds below a minute, minutes from then on (decision 9: translated - a translation as
// short as the language allows, an abbreviation, so that no plural is needed)
QString WorldMapView::ageText (long long seconds) const
{
  if (seconds < 0) seconds = 0;
  //: A station's age on the map, in seconds. As short as the language allows - an abbreviation.
  if (seconds < 60) return tr ("%1 s").arg (seconds);
  //: A station's age on the map, in minutes. As short as the language allows - an abbreviation.
  return tr ("%1 min").arg (seconds / 60);
}

QString WorldMapView::agoText (long long seconds) const
{
  if (seconds < 0) seconds = 0;
  //: How long ago, in a tooltip, in seconds. An abbreviation for the unit.
  if (seconds < 60) return tr ("%1 s ago").arg (seconds);
  //: How long ago, in a tooltip, in minutes. An abbreviation for the unit.
  return tr ("%1 min ago").arg (seconds / 60);
}

// where each mark stands - its copy of the world nearest the window's middle - and which labels are drawn
void WorldMapView::layOut ()
{
  if (m_laidOut && sameView (m_layoutView, m_view)) return;
  std::size_t const n {m_stations.size ()};
  m_xs.assign (n, 0.0); m_ys.assign (n, 0.0);
  m_boxes.assign (n, reception_spots::Box {0, 0, 0, 0});
  QFontMetricsF const fm {labelFont (font ())};
  double const side {m_view.side ()};
  for (std::size_t i = 0; i < n; ++i)
    {
      auto const& s = m_stations[i];
      double x {m_view.screenX (world_map::xOfLongitude (s.lon))};
      x -= side * std::round ((x - m_view.w / 2.0) / side);
      m_xs[i] = x;
      m_ys[i] = m_view.screenY (world_map::yOfLatitude (s.lat));
      if (reception_spots::HearsUs != s.kind || !m_showAges) continue;
      double const w {fm.horizontalAdvance (ageText (m_now - s.heardUs)) + (s.bothWays ? 12 : 8)}, h {s.bothWays ? 16.0 : 14.0};
      m_boxes[i] = reception_spots::Box {x - w / 2, m_ys[i] - (s.bothWays ? 17 : 14) - h / 2, w, h};
    }
  m_labels = reception_spots::placeLabels (m_boxes, m_view.w, m_view.h - m_bottomInset);
  m_layoutView = m_view;
  m_laidOut = true;
}

QPointF WorldMapView::stationPixel (std::size_t index)
{
  layOut ();
  return index < m_xs.size () ? QPointF {m_xs[index], m_ys[index]} : QPointF {};
}

bool WorldMapView::labelDrawn (std::size_t index)
{
  layOut ();
  return index < m_labels.size () && m_labels[index];
}

QRectF WorldMapView::labelBox (std::size_t index)
{
  layOut ();
  return index < m_boxes.size () ? QRectF {m_boxes[index].x, m_boxes[index].y, m_boxes[index].w, m_boxes[index].h} : QRectF {};
}

void WorldMapView::paintStations (QPainter& p)
{
  if (m_stations.empty ()) return;
  layOut ();
  world_map::Colours const c {world_map::colours (m_dark)};
  p.setRenderHint (QPainter::Antialiasing);
  QColor halo {c.land}; halo.setAlpha (200);
  QColor edge {c.ink2}; edge.setAlpha (220);
  QRectF const seen {-12, -12, m_view.w + 24.0, m_view.h + 24.0};
  for (std::size_t i = 0; i < m_stations.size (); ++i)   // all marks first, in their order
    {
      auto const& s = m_stations[i];
      QPointF const at {m_xs[i], m_ys[i]};
      if (!seen.contains (at)) continue;
      if (reception_spots::Reporting == s.kind)
        {
          p.setPen (QPen {halo, 0.7});
          p.setBrush (QColor {c.other});
          p.drawEllipse (at, dotRadius, dotRadius);
        }
      else if (reception_spots::WeHear == s.kind)
        {
          QPolygonF diamond;
          diamond << QPointF {at.x (), at.y () - markRadius} << QPointF {at.x () + markRadius, at.y ()} << QPointF {at.x (), at.y () + markRadius} << QPointF {at.x () - markRadius, at.y ()};
          p.setBrush (QColor {c.weHear});
          p.setPen (QPen {QColor {c.surface}, 3.0});
          p.drawPolygon (diamond);
          p.setPen (QPen {edge, 1.0});
          p.drawPolygon (diamond);
        }
      else
        {
          QColor const fill {s.hasSnr ? QColor {c.ramp[reception_spots::snrStep (true, s.snr)]} : QColor {c.surface}};   // a report without an SNR: hollow
          p.setBrush (fill);
          if (s.bothWays)   // the orange ring right on the dot, nothing between
            {
              p.setPen (QPen {QColor {c.weHear}, 2.4});
              p.drawEllipse (at, markRadius + 1.2, markRadius + 1.2);
            }
          else
            {
              p.setPen (QPen {QColor {c.surface}, 3.0});
              p.drawEllipse (at, markRadius, markRadius);
              p.setPen (QPen {edge, s.hasSnr ? 1.0 : 1.6});
              p.drawEllipse (at, markRadius, markRadius);
            }
        }
    }
  p.setFont (labelFont (font ()));
  for (std::size_t i = 0; i < m_stations.size (); ++i)   // then the labels, over all marks
    {
      if (!m_labels[i]) continue;
      auto const& s = m_stations[i];
      QRectF const box {m_boxes[i].x, m_boxes[i].y, m_boxes[i].w, m_boxes[i].h};
      QColor const fill {s.hasSnr ? QColor {c.ramp[reception_spots::snrStep (true, s.snr)]} : QColor {c.surface}};
      p.setBrush (fill);
      if (s.bothWays)   // framed in the orange alone
        {
          p.setPen (QPen {QColor {c.weHear}, 2.4});
          p.drawRoundedRect (box, 4, 4);
        }
      else
        {
          p.setPen (QPen {edge, 1.0});
          p.drawRoundedRect (box, 3, 3);
        }
      p.setPen (inkOn (fill));
      p.drawText (box, Qt::AlignCenter, ageText (m_now - s.heardUs));
    }
}

// the great circle from our station to the station under the mouse that hears us
void WorldMapView::paintLine (QPainter& p)
{
  if (m_under.empty () || !m_own.ok || m_under.front () >= m_stations.size ()) return;
  auto const& s = m_stations[m_under.front ()];
  if (reception_spots::HearsUs != s.kind) return;
  layOut ();
  world_map::Colours const c {world_map::colours (m_dark)};
  auto const path = world_map::greatCircle (m_own.lat, m_own.lon, s.lat, s.lon, 96);
  QPolygonF line;
  double x {world_map::xOfLongitude (m_own.lon)};
  for (auto const& q : path)
    {
      x = world_map::unwrappedX (world_map::xOfLongitude (q.lon), x);
      line << QPointF {m_view.screenX (x), m_view.screenY (world_map::yOfLatitude (q.lat))};
    }
  QPointF const end {m_xs[m_under.front ()], m_ys[m_under.front ()]};
  double const shift {end.x () - line.last ().x ()};   // the copy that ends at the mark as it is drawn
  for (auto& q : line) q.rx () += shift;
  p.setRenderHint (QPainter::Antialiasing);
  p.setBrush (Qt::NoBrush);
  p.setPen (QPen {QColor {c.surface}, 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin});
  p.drawPolyline (line);
  p.setPen (QPen {QColor {c.ink}, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin});
  p.drawPolyline (line);
  p.drawEllipse (end, s.bothWays ? 10.2 : 7.5, s.bothWays ? 10.2 : 7.5);
}

// what a tooltip says of the stations under the mouse, a line a list entry; an empty entry between two stations
QStringList WorldMapView::tooltipLines (std::vector<std::size_t> const& under) const
{
  QStringList lines;
  std::size_t shown {0};
  for (std::size_t i : under)
    {
      if (i >= m_stations.size ()) continue;
      if (6 == shown)
        {
          lines << QString {} << tr ("and %1 more").arg (under.size () - shown);
          break;
        }
      auto const& s = m_stations[i];
      if (shown++) lines << QString {};
      QString const call {QString::fromStdString (s.call)};
      QString const locator {QString::fromStdString (s.locator)};
      QString const entity {m_entity ? m_entity (call) : QString {}};
      lines << (QStringList {call, locator.left (4).toUpper () + locator.mid (4).toLower (), entity}.join (QStringLiteral ("   ")).trimmed ());
      if (s.hearsUs)
        {
          lines << tr ("heard us %1 UTC, %2").arg (utc (s.heardUs), agoText (m_now - s.heardUs));
          lines << tr ("our signal %1 MHz, %2, %3").arg (megahertz (s.ourFrequency), m_mode, s.hasSnr ? signedDb (s.snr) + QStringLiteral (" dB") : tr ("no report"));
        }
      else if (reception_spots::WeHear == s.kind)
        {
          lines << tr ("heard by us %1 UTC, %2").arg (utc (s.weHeard), agoText (m_now - s.weHeard));
          lines << QStringLiteral ("%1 MHz, %2, %3 dB").arg (megahertz (s.itsFrequency), m_mode, signedDb (s.ourSnr));
        }
      if (reception_spots::WeHear != s.kind)
        {
          if (s.hasTx) lines << tr ("Tx %1 MHz, %2").arg (megahertz (s.txFrequency), agoText (m_now - s.txTime));
          if (s.weHear) lines << tr ("heard by us %1 dB, %2").arg (signedDb (s.ourSnr), agoText (m_now - s.weHeard));
        }
      QString const distance {m_distance ? m_distance (locator) : QString {}};
      if (!distance.isEmpty ()) lines << distance;
      // (TODO.md 3.-261 and 3.-262, the operator: "remove 'does not report to PSK Reporter'", then "there is still a line
      // like: Reports to PSK Reporter, not us. Let's get rid of it") nothing is said of whether a station reports. What
      // the feed has of it is in the lines themselves: a finer locator, and - of any station that reports, asked for when
      // the mouse comes to it - its receiver's software and antenna
      if (s.hasInfo)
        {
          if (!s.info.decoder.empty ()) lines << QString::fromStdString (s.info.decoder);
          if (!s.info.antenna.empty ()) lines << QString::fromStdString (s.info.antenna);
        }
    }
  return lines;
}

// The stations under a point: those whose marks are (receptionspots.h pick ()); where no mark is, the station whose
// age is written there - with the others of its place, as its mark would have them.
std::vector<std::size_t> WorldMapView::underAt (double x, double y)
{
  layOut ();
  auto under = reception_spots::pick (m_stations, m_xs, m_ys, x, y);
  if (!under.empty ()) return under;
  for (std::size_t i = m_stations.size (); i-- > 0;)
    {
      if (i >= m_labels.size () || !m_labels[i]) continue;
      auto const& b = m_boxes[i];
      if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) return reception_spots::pick (m_stations, m_xs, m_ys, m_xs[i], m_ys[i]);
    }
  return under;
}

// Our own mark is under a point when the point is within its ring and no station's mark is under it.
bool WorldMapView::overOwn (double x, double y)
{
  if (!m_own.ok) return false;
  layOut ();
  QPointF const at {ownStationPixel ()};
  double const dx {x - at.x ()}, dy {y - at.y ()};
  return dx * dx + dy * dy <= ownRadius * ownRadius && reception_spots::pick (m_stations, m_xs, m_ys, x, y).empty ();
}

// "CE3TSK   FF46qp   Chile", then the window's line - the band, the mode and who hears whom
QStringList WorldMapView::ownLines () const
{
  QStringList lines;
  if (!m_own.ok) return lines;
  QString const entity {m_entity && !m_ownCall.isEmpty () ? m_entity (m_ownCall) : QString {}};
  lines << (QStringList {m_ownCall, m_ownLocator.left (4).toUpper () + m_ownLocator.mid (4).toLower (), entity}.join (QStringLiteral ("   ")).trimmed ());
  if (!m_ownSummary.isEmpty ()) lines << m_ownSummary;
  return lines;
}

void WorldMapView::setOwnInfo (QString const& call, QString const& summary)
{
  if (call == m_ownCall && summary == m_ownSummary) return;
  m_ownCall = call;
  m_ownSummary = summary;
  if (m_overOwn && m_mouseIn && !m_dragging) hover (m_mouse, false);   // the tooltip of a resting mouse follows
}

QString WorldMapView::tooltipAt (QPoint const& at)
{
  if (overOwn (at.x (), at.y ())) return ownLines ().join (QLatin1Char ('\n'));
  return tooltipLines (underAt (at.x (), at.y ())).join (QLatin1Char ('\n'));
}

// A tooltip as it is shown. Every line escaped - a receiver's antenna is typed by strangers, and a tooltip that looks
// like HTML is rendered as HTML.
QString WorldMapView::tooltipHtml (std::vector<std::size_t> const& under) const
{
  return asHtml (tooltipLines (under));
}

QString WorldMapView::tooltipHtmlAt (QPoint const& at)
{
  if (overOwn (at.x (), at.y ())) return asHtml (ownLines ());
  return tooltipHtml (underAt (at.x (), at.y ()));
}

QStringList WorldMapView::hovered () const
{
  QStringList calls;
  for (std::size_t i : m_under) if (i < m_stations.size ()) calls << QString::fromStdString (m_stations[i].call);
  return calls;
}

void WorldMapView::hover (QPoint const& at, bool moved)
{
  m_mouse = at;
  m_mouseIn = true;
  layOut ();
  bool const own {overOwn (at.x (), at.y ())};
  auto const under = own ? std::vector<std::size_t> {} : underAt (at.x (), at.y ());
  bool const changed {under != m_under || own != m_overOwn};
  bool const others {under != m_under};
  m_under = under;
  m_overOwn = own;
  if (others && !m_under.empty ()) Q_EMIT hoveredChanged ();   // (3.-262) their details are asked for now
  if (moved || !m_under.empty () || own) unrest ();   // what a resting mouse was told goes with its first move, and for a mark's tooltip
  if (own)   // our own station: its tooltip, and no line
    {
      QToolTip::showText (mapToGlobal (at) + QPoint {14, 12}, asHtml (ownLines ()), this);
      if (changed)
        {
          setCursor (Qt::ArrowCursor);
          update ();
        }
      return;
    }
  if (m_under.empty ())
    {
      if (changed)
        {
          QToolTip::hideText ();
          setCursor (Qt::OpenHandCursor);
          update ();
        }
      if (moved) m_rest->start ();   // (stage 4) resting here, it is told the entities of the square under it
      return;
    }
  QToolTip::showText (mapToGlobal (at) + QPoint {14, 12}, tooltipHtml (m_under), this);
  if (changed)
    {
      setCursor (Qt::ArrowCursor);
      update ();
    }
}

void WorldMapView::unhover ()
{
  if (m_under.empty () && !m_overOwn) return;
  m_under.clear ();
  m_overOwn = false;
  QToolTip::hideText ();
  update ();
}

void WorldMapView::leaveEvent (QEvent *)
{
  m_mouseIn = false;
  unrest ();
  unhover ();
}

// ------------------------------------------------------------------------------------------- stage 4: DXCC, the menu
namespace
{
  double const prefixPixelsPerDegree {4.0};   // the prefixes are written once a degree is so wide
  int const prefixPixels {10};
  int const entitiesTold {4};                 // of a square, to a resting mouse

  // letters this small in plain grey: with sub-pixel smoothing one letter came out blue and the next brown
  QFont prefixFont (QFont f)
  {
    f.setPixelSize (prefixPixels);
    f.setStyleStrategy (static_cast<QFont::StyleStrategy> (QFont::PreferAntialias | QFont::NoSubpixelAntialias));
    return f;
  }
}

void WorldMapView::setEntities (std::vector<dxcc_places::Entity> const& entities)
{
  m_entities = entities;
  m_entitiesLaidOut = false;
  update ();
}

void WorldMapView::setShowPrefixes (bool show)
{
  if (show == m_showPrefixes) return;
  m_showPrefixes = show;
  update ();
}

void WorldMapView::layOutEntities ()
{
  if (m_entitiesLaidOut && sameView (m_entityView, m_view) && m_entityInset == m_bottomInset) return;
  m_entityLabels.clear ();
  m_entityView = m_view;
  m_entityInset = m_bottomInset;
  m_entitiesLaidOut = true;
  double const side {m_view.side ()};
  if (side / 360.0 < prefixPixelsPerDegree) return;   // no room yet
  QFontMetricsF const fm {prefixFont (font ())};
  std::vector<dxcc_places::Box> boxes;
  QStringList texts;
  for (auto const& e : m_entities)   // the larger entity first: its prefix has the say
    {
      double x {m_view.screenX (world_map::xOfLongitude (e.lon))};
      x -= side * std::round ((x - m_view.w / 2.0) / side);
      double const y {m_view.screenY (world_map::yOfLatitude (e.lat))};
      QString const text {QString::fromStdString (e.prefix)};
      double const w {fm.horizontalAdvance (text) + 4.0}, h {prefixPixels + 4.0};
      if (x + w / 2 < 0 || x - w / 2 > m_view.w || y + h / 2 < 0 || y + h / 2 > m_view.h - m_bottomInset) continue;   // out of sight, or under the status line
      boxes.push_back (dxcc_places::Box {std::round (x - w / 2), std::round (y - h / 2), std::ceil (w), h});   // on whole pixels: the letters stay sharp
      texts << text;
    }
  auto const written = dxcc_places::firstCome (boxes);
  for (std::size_t i = 0; i < boxes.size (); ++i)
    if (written[i]) m_entityLabels.push_back (EntityLabel {texts[static_cast<int> (i)], QRectF {boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h}});
}

void WorldMapView::paintEntities (QPainter& p)
{
  if (!m_showPrefixes || m_entities.empty ()) return;
  layOutEntities ();
  if (m_entityLabels.empty ()) return;
  world_map::Colours const c {world_map::colours (m_dark)};
  p.setFont (prefixFont (font ()));
  QColor halo {c.land};
  halo.setAlpha (200);
  for (auto const& l : m_entityLabels)
    {
      p.setPen (halo);   // the land's colour round the letters: they stand on borders and on the night
      for (QPointF const& by : {QPointF {-1, 0}, QPointF {1, 0}, QPointF {0, -1}, QPointF {0, 1}}) p.drawText (l.box.translated (by), Qt::AlignCenter, l.text);
      p.setPen (QColor {c.ink2});
      p.drawText (l.box, Qt::AlignCenter, l.text);
    }
}

QStringList WorldMapView::prefixesDrawn ()
{
  QStringList out;
  if (!m_showPrefixes) return out;
  layOutEntities ();
  for (auto const& l : m_entityLabels) out << l.text;
  return out;
}

QRectF WorldMapView::prefixBox (QString const& prefix)
{
  if (!m_showPrefixes) return QRectF {};
  layOutEntities ();
  for (auto const& l : m_entityLabels) if (l.text == prefix) return l.box;
  return QRectF {};
}

// what a resting mouse is told: the locator under it, then the DXCC entities of that square - "CE   Chile" - the
// nearest first, four at most
QStringList WorldMapView::entityLines (QPoint const& at) const
{
  QStringList lines;
  if (m_entities.empty ()) return lines;
  double const lat {world_map::latitudeOfY (m_view.worldY (at.y ()))};
  double const lon {world_map::wrapX (m_view.worldX (at.x ())) * 360.0 - 180.0};
  auto const here = dxcc_places::at (m_entities, lat, lon);
  if (here.empty ()) return lines;
  lines << QString::fromStdString (world_map::locator (lat, lon, 6));
  int told {0};
  for (std::size_t i : here)
    {
      if (entitiesTold == told)
        {
          // (the second review, TODO.md 3.-264) a text of its own: "and %1 more" is the stations' under the mouse,
          // and a language that counts stations with their own word (Japanese, Korean) wrote that under the entities
          lines << tr ("and %1 more here").arg (here.size () - told);
          break;
        }
      lines << QString::fromStdString (m_entities[i].prefix) + QStringLiteral ("   ") + QString::fromStdString (m_entities[i].name);
      ++told;
    }
  return lines;
}

QString WorldMapView::entityTooltipAt (QPoint const& at) const
{
  return entityLines (at).join (QLatin1Char ('\n'));
}

void WorldMapView::setRestInterval (int ms)
{
  m_rest->setInterval (ms);
}

void WorldMapView::rested ()
{
  if (!m_mouseIn || m_dragging || !m_under.empty ()) return;
  QStringList const lines {entityLines (m_mouse)};
  if (lines.isEmpty ()) return;
  m_restText = lines.join (QLatin1Char ('\n'));
  QToolTip::showText (mapToGlobal (m_mouse) + QPoint {14, 12}, asHtml (lines), this);
}

void WorldMapView::unrest ()
{
  m_rest->stop ();
  if (m_restText.isEmpty ()) return;
  m_restText.clear ();
  QToolTip::hideText ();
}

void WorldMapView::setMenuActions (QList<QAction *> const& actions)
{
  m_menuActions = actions;
}

void WorldMapView::setShowAges (bool show)
{
  if (show == m_showAges) return;
  m_showAges = show;
  m_laidOut = false;
  if (m_mouseIn && !m_dragging) hover (m_mouse, false);   // an age under a resting mouse came or went
  update ();
}

namespace
{
  // whether a station is one a view is fitted to: it hears us, we hear it (whatever else it is), or either
  bool fits (reception_spots::Station const& s, WorldMapView::Fit which)
  {
    bool const hearsMe {reception_spots::HearsUs == s.kind};
    return WorldMapView::HearMe == which ? hearsMe : WorldMapView::IHear == which ? s.weHear : hearsMe || s.weHear;
  }
}

bool WorldMapView::any (Fit which) const
{
  for (auto const& s : m_stations) if (fits (s, which)) return true;
  return false;
}

// "Zoom to the stations that hear me" and its two neighbours: they and our own station as large as the window
// allows - room above for a mark's label, the status line's at the foot. The view is then the operator's, as after
// a turn of the wheel.
void WorldMapView::zoomTo (Fit which)
{
  std::vector<world_map::Place> places;
  for (auto const& s : m_stations) if (fits (s, which)) places.push_back (world_map::Place {true, s.lat, s.lon});
  if (places.empty ()) return;   // a greyed entry can still be triggered
  double const ref {m_own.ok ? m_own.lon : places.front ().lon};
  if (m_own.ok) places.push_back (m_own);
  m_view = world_map::fitTo (m_view, places, ref, world_map::Margins {34, 30, m_bottomInset + 16, 30});
  viewTouched ();
}
