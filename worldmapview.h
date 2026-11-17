#ifndef WORLDMAPVIEW_H
#define WORLDMAPVIEW_H

// CE3TSK 2026-11-16 (TODO.md 3.-260, RECEPTION_MAP_PLAN.md stage 1): the world map as a widget - land, borders and
// lakes from the map file inside the program (":/worldmap.bin", Natural Earth, public domain), drawn by QPainter in
// Web Mercator, the night laid over them, our station marked. No map library and nothing fetched: worldmap.h has the
// rules, this draws them.
//
//   - wheel: zoom about the pointer; drag: move; double click: zoom in; + and -: zoom about the middle; Home, and
//     "Centre on my station" of the right-click menu: the view the map opens with;
//   - the map opens with our longitude in the middle and 78 N to 60 S in view, and keeps to that through every resize
//     until the operator zooms or drags (the view is then theirs, and kept); a stored view (setView ()) is held as it
//     was stored and fitted to the window's size whenever that changes, until the operator moves it - it is set
//     before the window has its size;
//   - two levels of detail, 1:50m for the world and 1:10m once zoomed in (worldmap.h level ()); the second is read
//     only when it is first asked for;
//   - the night follows the clock by the minute, while the window is shown;
//   - light and dark: setDarkStyle (), from View > Use dark style; every colour is of worldmap.h's one table.
//
// The base map is kept as a pixmap and drawn again only when the view, the size or the style change.
//
// Stage 3, the stations (receptionspots.h has who they are and their order; setStations () hands them over):
//   - one that only reports: a small dot; one we hear and that does not hear us: an orange diamond; one that hears us:
//     a dot in the colour of the SNR it gave us (hollow when its report had none) with its age above it - seconds
//     below a minute, minutes from then on, in the program's language - and, when we hear it too, an orange ring on
//     the dot and an orange frame round the age. Drawn in the order they come, all marks first, then the labels;
//     a label that would meet one already given is left out (receptionspots.h placeLabels ());
//   - the mouse on a mark: the station's tooltip - shown by us, so it comes whichever window is active - and, for one
//     that hears us, the great circle from our station to it. It follows the stations as they change under a resting
//     mouse. Every text of a tooltip is escaped: a receiver's antenna line is typed by strangers.
//
// Stage 4, DXCC and the menu (dxccplaces.h has the rules; setEntities () hands cty.dat's entities over):
//   - once a degree is 4 pixels wide, each entity's prefix is written at its cty.dat position, over the night and
//     under the stations - the larger entity's first, one that would meet another left out, none under the status
//     line; "Show DXCC prefixes" of the menu (the window's) switches them off;
//   - the mouse resting 0.7 s where no station is: a tooltip with the locator under it and the DXCC entities of that
//     square, the nearest first, four at most; nothing over a square no entity has;
//   - the menu: "Centre on my station", then three ways to fit the view to stations and our own - "Zoom to stations
//     that hear me or I hear", "Zoom to stations that hear me", "Zoom to stations I hear" (the operator, 2026-11-16,
//     who shortened all three the same day), each greyed while the map has no such station - and then the window's
//     own entries
//     (setMenuActions ());
//   - the ages above the stations that hear us can be switched off (setShowAges (), the window's "Show age labels");
//   - the mouse on a station's age, not only on its mark, has the station's tooltip and line (the operator,
//     2026-11-16): a mark under the mouse comes first, then the age written there;
//   - the mouse on our own station's mark has a tooltip too (the operator, 2026-11-16): our call, our locator, our
//     entity, and the window's line on how we are heard (setOwnInfo ()). A station's mark there comes first; no line
//     is drawn.

#include <functional>
#include <vector>
#include <QImage>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QWidget>
#include "worldmap.h"
#include "receptionspots.h"
#include "dxccplaces.h"

class JTDXDateTime;
class QAction;
class QMenu;
class QTimer;

class WorldMapView
  : public QWidget
{
  Q_OBJECT

public:
  explicit WorldMapView (QWidget * parent = nullptr);
  ~WorldMapView ();

  void setDarkStyle (bool dark);
  bool darkStyle () const {return m_dark;}

  void setOwnLocator (QString const& locator);   // our station; no locator, or none that is one: no station shown
  bool hasOwnStation () const {return m_own.ok;}
  QPointF ownStationPixel () const;              // where it is drawn: its copy nearest the window's middle

  void setClock (JTDXDateTime * clock);          // the program's UTC; none: the system's
  void setFixedTime (qint64 unixSeconds);        // the night of that moment, for the tests; negative: the clock again

  world_map::View view () const {return m_view;}
  bool atHome () const {return m_atHome;}        // still the view the map opens with
  void setView (double cx, double cy, double zoom);   // a view of the operator's (a stored one)
  void home ();

  int levelDrawn () const {return m_levelDrawn;} // the level of detail of the base map as last drawn
  int baseDraws () const {return m_baseDraws;}   // how often the base map was drawn
  int baseMoves () const {return m_baseMoves;}   // (3.-261) and how often moved instead, only the strips that came in drawn
  int nightDraws () const {return m_nightDraws;} // and the night
  void setTickInterval (int ms);                 // how often the clock is looked at for a new minute (20 s; the tests' is shorter)
  bool ticking () const;                         // whether it is looked at: only while the map is shown
  static bool mapLoaded ();                      // the map file inside the program read and sound
  static QString mapError ();                    // why not

  QMenu * makeMenu ();                           // the right-click menu, the caller's to show and delete

  // the stations, in the order they are drawn, and the moment their ages are counted from (UTC seconds)
  void setStations (std::vector<reception_spots::Station> const& stations, long long now);
  std::vector<reception_spots::Station> const& stations () const {return m_stations;}
  void setMode (QString const& mode);            // our mode, for the tooltips
  void setBottomInset (int pixels);              // so much of the map's foot is covered (the status line): no label there
  // a call's DXCC entity as the decode windows name it, and the distance and azimuth to a locator as they give them
  void setLookups (std::function<QString (QString const&)> const& entity, std::function<QString (QString const&)> const& distance);
  QString ageText (long long seconds) const;     // "45 s", "3 min"
  QString agoText (long long seconds) const;     // "45 s ago", "3 min ago"
  QPointF stationPixel (std::size_t index);      // where a station's mark stands
  bool labelDrawn (std::size_t index);           // whether its age is drawn
  QRectF labelBox (std::size_t index);           // and where
  QString tooltipAt (QPoint const& at);          // the tooltip the mouse gets there, as plain text; empty where no mark is
  QString tooltipHtmlAt (QPoint const& at);      // and as it is shown: every line escaped
  QStringList hovered () const;                  // the calls under the mouse, the first the one the line goes to

  // (stage 4)
  void setEntities (std::vector<dxcc_places::Entity> const& entities);   // cty.dat's DXCC entities, the larger first
  void setShowPrefixes (bool show);              // their prefixes written on the map, or not
  bool showPrefixes () const {return m_showPrefixes;}
  QStringList prefixesDrawn ();                  // the prefixes written in the view as it is, in the order they were given their place
  QRectF prefixBox (QString const& prefix);      // where one is written; a null box when it is not
  QString entityTooltipAt (QPoint const& at) const;   // what the resting mouse is told there: the locator and its square's entities; empty where none is
  QString restingTooltip () const {return m_restText;}   // what it is being told now, as plain text
  void setRestInterval (int ms);                 // how long the mouse rests before it is told (700 ms; the tests' is shorter)
  void setMenuActions (QList<QAction *> const& actions);   // the window's entries of the menu, after the map's own
  enum Fit {HearMe, IHear, Either};             // the stations a view is fitted to: that hear us, that we hear, or either
  bool any (Fit which) const;                    // such a station is on the map
  void zoomTo (Fit which);                       // the view fitted to them and our own station
  bool anyHearing () const {return any (HearMe);}
  void zoomToHearing () {zoomTo (HearMe);}
  void setOwnInfo (QString const& call, QString const& summary);   // our call and the window's line on how we are heard, for our own mark's tooltip
  bool ownHovered () const {return m_overOwn;}   // the mouse is on our own mark, and on no station's
  void setShowAges (bool show);                  // the ages above the stations that hear us written, or not
  bool showAges () const {return m_showAges;}

  QSize sizeHint () const override;
  QSize minimumSizeHint () const override;

  // (3.-262) the stations under the mouse are others than before - hovered () has them: the window asks the server for
  // the details of those that report
  Q_SIGNAL void hoveredChanged ();

protected:
  void paintEvent (QPaintEvent *) override;
  void resizeEvent (QResizeEvent *) override;
  void wheelEvent (QWheelEvent *) override;
  void mousePressEvent (QMouseEvent *) override;
  void mouseMoveEvent (QMouseEvent *) override;
  void mouseReleaseEvent (QMouseEvent *) override;
  void mouseDoubleClickEvent (QMouseEvent *) override;
  void keyPressEvent (QKeyEvent *) override;
  void contextMenuEvent (QContextMenuEvent *) override;
  void showEvent (QShowEvent *) override;
  void hideEvent (QHideEvent *) override;
  void leaveEvent (QEvent *) override;

private:
  qint64 nowSeconds () const;
  void viewTouched ();                           // the operator zoomed or dragged
  void fitView ();                               // the view for the window's size as it is now
  void drawBase ();
  void paintBase (QPainter& p, QRect const& part);   // (3.-261) the base map of a part of the window
  void moveBase (int dx, int dy);                    //   and the drawing moved by whole pixels, the strips that came in drawn
  void drawNight (qint64 minute);
  void minuteTick ();
  void layOut ();                                // where the marks and labels stand in the view as it is
  void paintStations (QPainter& p);
  void paintLine (QPainter& p);
  void unhover ();
  QStringList tooltipLines (std::vector<std::size_t> const& under) const;
  QString tooltipHtml (std::vector<std::size_t> const& under) const;
  void hover (QPoint const& at, bool moved);     // moved: the mouse itself; not: the stations changed under it
  std::vector<std::size_t> underAt (double x, double y);   // the stations under a point: by their marks, else by the age written there
  bool overOwn (double x, double y);             // the point is on our own mark and on no station's
  QStringList ownLines () const;                 // our own mark's tooltip
  void layOutEntities ();                        // which prefixes are written, and where
  void paintEntities (QPainter& p);
  QStringList entityLines (QPoint const& at) const;
  void rested ();                                // the mouse has rested where no station is
  void unrest ();                                // it moved, left or pressed: nothing is told any more

  bool m_dark {false};
  world_map::Place m_own {false, 0.0, 0.0};
  world_map::View m_view;
  bool m_atHome {true};
  bool m_pinned {false};                         // a stored view not moved since: its middle and zoom as stored
  double m_pinX {0.5};
  double m_pinY {0.5};
  double m_pinZoom {1.0};
  JTDXDateTime * m_clock {nullptr};
  qint64 m_fixedTime {-1};
  QTimer * m_timer;

  QPixmap m_base;                                // land, borders, lakes
  world_map::View m_baseView;                    // what m_base was drawn for
  bool m_baseDark {false};
  qreal m_baseRatio {0};
  int m_levelDrawn {-1};
  int m_baseDraws {0};
  int m_baseMoves {0};

  QImage m_night;                                // the shade, a third of the window's resolution
  world_map::View m_nightView;
  bool m_nightDark {false};
  qint64 m_nightMinute {-1};
  int m_nightDraws {0};

  bool m_dragging {false};
  QPoint m_dragFrom;

  std::vector<reception_spots::Station> m_stations;
  long long m_now {0};
  QString m_mode;
  int m_bottomInset {0};
  std::function<QString (QString const&)> m_entity, m_distance;
  std::vector<double> m_xs, m_ys;                // the marks' places, as laid out
  std::vector<reception_spots::Box> m_boxes;     // the labels wanted
  std::vector<bool> m_labels;                    // and which of them are drawn
  bool m_laidOut {false};
  world_map::View m_layoutView;
  std::vector<std::size_t> m_under;              // the stations under the mouse
  QPoint m_mouse;
  bool m_mouseIn {false};

  struct EntityLabel
  {
    QString text;
    QRectF box;
  };
  std::vector<dxcc_places::Entity> m_entities;
  bool m_showPrefixes {true};
  bool m_showAges {true};
  QString m_ownLocator, m_ownCall, m_ownSummary;
  bool m_overOwn {false};
  std::vector<EntityLabel> m_entityLabels;       // the prefixes written, as laid out
  bool m_entitiesLaidOut {false};
  world_map::View m_entityView;
  int m_entityInset {-1};
  QTimer * m_rest;                               // the mouse's rest
  QString m_restText;
  QList<QAction *> m_menuActions;
};

#endif
