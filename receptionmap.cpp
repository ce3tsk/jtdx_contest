// CE3TSK 2026-11-16 (TODO.md 3.-260, RECEPTION_MAP_PLAN.md): see receptionmap.h
#include "receptionmap.h"

#include <algorithm>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QFile>
#include <QFontMetricsF>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QSaveFile>
#include <QSettings>
#include <QShowEvent>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include "geometryrestore.h"
#include "SettingsGroup.hpp"
#include "spotfeed.h"
#include "worldmapview.h"

#include "moc_receptionmap.cpp"

namespace
{
  char const * const group {"ReceptionMap"};
  int const statusHeight {20};

  // the UTC day of a moment as yyyymmdd (3.-263: the day a compound call of the feed's was last reported)
  int utcDay (long long seconds)
  {
    QDate const d {QDateTime::fromSecsSinceEpoch (seconds, Qt::UTC).date ()};
    return d.year () * 10000 + d.month () * 100 + d.day ();
  }
}

// The status line: laid over the map's lowest 20 pixels, so the window is the map's size. Its text is the longest of
// those handed over that fits beside the legend; when none does the legend goes, and then the longest that fits alone.
class ReceptionStatus
  : public QWidget
{
public:
  explicit ReceptionStatus (QWidget * parent)
    : QWidget {parent}
  {
    setAttribute (Qt::WA_TransparentForMouseEvents);   // the map under it keeps the mouse
  }

  void set (QStringList const& texts, QString const& bothWays, QString const& weHear)
  {
    if (texts == m_texts && bothWays == m_bothWays && weHear == m_weHear) return;
    m_texts = texts; m_bothWays = bothWays; m_weHear = weHear;
    update ();
  }

  void setDark (bool dark)
  {
    m_dark = dark;
    update ();
  }

  QString shown () const {return m_shown;}       // the text as last drawn
  bool legendShown () const {return m_legend;}

protected:
  void paintEvent (QPaintEvent *) override
  {
    world_map::Colours const c {world_map::colours (m_dark)};
    QPainter p {this};
    p.setRenderHint (QPainter::Antialiasing);
    QColor back {c.surface};
    back.setAlpha (225);
    p.fillRect (rect (), back);
    QFont f {font ()};
    f.setPixelSize (11);
    p.setFont (f);
    QFontMetricsF const fm {f};
    // the legend's entries, from the right: the four steps of the SNR, a station we hear, one heard both ways
    QString const minus {QChar {0x2212}};
    QStringList const steps {QString {QChar {0x2264}} + QLatin1Char (' ') + minus + QStringLiteral ("18"), minus + QStringLiteral ("17") + QChar {0x2026} + minus + QStringLiteral ("10"),
                             minus + QStringLiteral ("9") + QChar {0x2026} + minus + QStringLiteral ("1"), QString {QChar {0x2265}} + QStringLiteral (" 0 dB")};
    double legend {8 + fm.horizontalAdvance (m_weHear) + 14 + 10 + fm.horizontalAdvance (m_bothWays) + 20 + 10};
    for (auto const& s : steps) legend += fm.horizontalAdvance (s) + 14 + 10;
    m_shown.clear ();
    m_legend = false;
    for (auto const& t : m_texts)
      if (8 + fm.horizontalAdvance (t) + 16 + legend <= width ())
        {
          m_shown = t; m_legend = true;
          break;
        }
    if (!m_legend)
      {
        if (!m_texts.isEmpty ()) m_shown = m_texts.last ();
        for (auto const& t : m_texts)
          if (8 + fm.horizontalAdvance (t) + 8 <= width ())
            {
              m_shown = t;
              break;
            }
      }
    p.setPen (QColor {c.ink2});
    p.drawText (QRectF {8, 0, width () - 8.0, qreal (height ())}, Qt::AlignVCenter | Qt::AlignLeft, m_shown);
    if (!m_legend) return;
    double x {width () - 8.0};
    double const y {height () / 2.0};
    for (int i = 3; i >= 0; --i)
      {
        double const w {fm.horizontalAdvance (steps[i])};
        x -= w;
        p.setPen (QColor {c.ink2});
        p.drawText (QRectF {x, 0, w, qreal (height ())}, Qt::AlignVCenter, steps[i]);
        x -= 14;
        p.setPen (QPen {QColor {c.surface}, 1});
        p.setBrush (QColor {c.ramp[i]});
        p.drawEllipse (QPointF {x + 5, y}, 4.6, 4.6);
        x -= 10;
      }
    double w {fm.horizontalAdvance (m_weHear)};
    x -= w;
    p.setPen (QColor {c.ink2});
    p.drawText (QRectF {x, 0, w, qreal (height ())}, Qt::AlignVCenter, m_weHear);
    x -= 14;
    QPolygonF diamond;
    diamond << QPointF {x + 5, y - 4.6} << QPointF {x + 9.6, y} << QPointF {x + 5, y + 4.6} << QPointF {x + 0.4, y};
    p.setPen (QPen {QColor {c.surface}, 1});
    p.setBrush (QColor {c.weHear});
    p.drawPolygon (diamond);
    x -= 10;
    w = fm.horizontalAdvance (m_bothWays);
    x -= w;
    p.setPen (QColor {c.ink2});
    p.drawText (QRectF {x, 0, w, qreal (height ())}, Qt::AlignVCenter, m_bothWays);
    x -= 20;
    p.setBrush (QColor {c.ramp[2]});
    p.setPen (QPen {QColor {c.weHear}, 2.0});
    p.drawEllipse (QPointF {x + 9, y}, 5.0, 5.0);
  }

private:
  QStringList m_texts;
  QString m_bothWays, m_weHear;
  QString m_shown;
  bool m_legend {false};
  bool m_dark {false};
};

bool ReceptionMap::wasOpen (QSettings * settings)
{
  SettingsGroup g {settings, group};
  return settings->value ("open", false).toBool ();
}

ReceptionMap::ReceptionMap (QSettings * settings, JTDXDateTime * clock, QWidget * parent)
  : QWidget {parent, Qt::Window}
  , m_settings {settings}
  , m_map {new WorldMapView {this}}
  , m_status {new ReceptionStatus {this}}
  , m_feed {new SpotFeed {this}}
{
  // "Reception Map", both words with a capital as the Wide Graph's title has them (the operator, 2026-11-16); the View
  // menu's entry stays "Reception map", as its neighbours are written
  setWindowTitle (QApplication::applicationName () + " - " + tr ("Reception Map"));
  auto * layout = new QVBoxLayout {this};
  layout->setContentsMargins (0, 0, 0, 0);
  layout->addWidget (m_map);
  m_map->setClock (clock);
  m_map->setBottomInset (statusHeight);   // no label under the status line
  m_status->raise ();

  SettingsGroup g {m_settings, group};
  // the size and place the operator left: as saved here, and by the rule of geometryrestore.h after the first show
  // (restoreSavedGeometry ()), as the Wide Graph does it
  m_savedGeometry = m_settings->value ("geometry").toByteArray ();
  m_savedMinHint = m_settings->value ("geometryMinHint").toSize ();
  m_hadSavedGeometry = JTDX::restore_grown_geometry (this, m_savedGeometry, QSize {});
  if (!m_hadSavedGeometry) resize (sizeHint ());
  // the view, when the operator had made it their own; a value that is no number is none
  if (!m_settings->value ("viewHome", true).toBool ())
    {
      bool okX, okY, okZoom;
      double const x {m_settings->value ("viewX").toDouble (&okX)}, y {m_settings->value ("viewY").toDouble (&okY)};
      double const zoom {m_settings->value ("viewZoom").toDouble (&okZoom)};
      if (okX && okY && okZoom) m_map->setView (x, y, zoom);
    }
  // the feed's server: its own unless the settings name another; a port that is none is its own. And over both, for
  // the tests and the sandboxes, which must never reach the real server: JTDX_RECEPTION_FEED=host:port, or "off" for
  // no feed at all - a value that is neither is "off" too, never the real server.
  bool okPort;
  int port {m_settings->value ("feedPort", SpotFeed::defaultPort ()).toInt (&okPort)};
  QString host {m_settings->value ("feedHost", SpotFeed::defaultHost ()).toString ().trimmed ()};
  if (qEnvironmentVariableIsSet ("JTDX_RECEPTION_FEED"))
    {
      QString const sealed {qEnvironmentVariable ("JTDX_RECEPTION_FEED").trimmed ()};
      int const colon {sealed.lastIndexOf (QLatin1Char (':'))};
      bool okSealed {false};
      int const sealedPort {colon > 0 ? sealed.mid (colon + 1).toInt (&okSealed) : 0};
      if (okSealed && sealedPort > 0 && sealedPort < 65536)
        {
          host = sealed.left (colon); port = sealedPort; okPort = true;
        }
      else m_feedOff = true;
    }
  m_feed->setServer (host.isEmpty () ? SpotFeed::defaultHost () : host, okPort && port > 0 && port < 65536 ? static_cast<quint16> (port) : SpotFeed::defaultPort ());
  // (stage 4) the menu's entries of the window, as they were left
  auto entry = [this] (QString const& text, char const * key, bool byDefault) {
    auto * a = new QAction {text, this};
    a->setCheckable (true);
    a->setChecked (m_settings->value (key, byDefault).toBool ());
    return a;
  };
  m_showWeHear = entry (tr ("Show stations we hear"), "showWeHear", true);
  m_showOthers = entry (tr ("Show other stations"), "showOthers", true);
  m_showAges = entry (tr ("Show age labels"), "showAges", true);
  m_showPrefixes = entry (tr ("Show DXCC prefixes"), "showPrefixes", true);
  m_show30 = entry (tr ("Show 30 minutes"), "show30Minutes", false);
  m_reducedData = entry (tr ("Reduced data"), "reducedData", false);
  auto * line = new QAction {this};
  line->setSeparator (true);
  m_map->setMenuActions ({m_showWeHear, m_showOthers, m_showAges, m_showPrefixes, m_show30, line, m_reducedData});
  m_board.setWindow (m_show30->isChecked () ? reception_spots::longWindowSeconds : reception_spots::windowSeconds);
  m_map->setShowAges (m_showAges->isChecked ());
  m_map->setShowPrefixes (m_showPrefixes->isChecked ());
  m_reduced = m_reducedData->isChecked ();
  connect (m_showWeHear, &QAction::toggled, this, [this] (bool on) {keep ("showWeHear", on); refresh ();});
  connect (m_showOthers, &QAction::toggled, this, [this] (bool on) {keep ("showOthers", on); refresh ();});
  connect (m_showAges, &QAction::toggled, this, [this] (bool on) {keep ("showAges", on); m_map->setShowAges (on);});
  connect (m_showPrefixes, &QAction::toggled, this, [this] (bool on) {keep ("showPrefixes", on); m_map->setShowPrefixes (on);});
  connect (m_show30, &QAction::toggled, this, [this] (bool on) {
      keep ("show30Minutes", on);
      m_board.setWindow (on ? reception_spots::longWindowSeconds : reception_spots::windowSeconds);
      refresh ();
    });
  connect (m_reducedData, &QAction::toggled, this, [this] (bool on) {keep ("reducedData", on); m_reduced = on; listen ();});   // the feed asks anew on the connection it has

  connect (m_feed, &SpotFeed::spot, this, [this] (reception_spots::Spot const& s) {
      long long const t {now ()};
      if (!m_board.addSpot (s, t)) return;   // kept; drawn by the clock
      // (2026-11-16, TODO.md 3.-263) and a sound report's sender, where its call is one that travels as a hash and
      // a second receiver has it too, is a compound call for the decoder (feedcalls.h) - not our own
      if (!m_feedCallsFile.isEmpty () && QString::fromStdString (s.sender).compare (m_ownCall, Qt::CaseInsensitive)
          && m_feedCalls.note (s.sender, s.receiver, utcDay (t))) m_feedCallsDirty = true;
    });
  // (2026-11-16, TODO.md 3.-262) a receiver's details: asked for at once of the stations that hear us (refresh ()), and of
  // any other station that reports when the mouse comes to it - there are over a thousand of them on a band, and
  // each is a subscription of its own. What comes while the mouse is still there is shown at once.
  connect (m_map, &WorldMapView::hoveredChanged, this, [this] {
      for (auto const& call : m_map->hovered ()) if (m_board.infoWanted (call.toStdString ())) m_feed->askInfo (call);
    });
  connect (m_feed, &SpotFeed::info, this, [this] (QString const& call, reception_spots::Info const& i) {
      m_board.setInfo (call.toStdString (), i);
      if (m_map->hovered ().contains (call, Qt::CaseInsensitive)) refresh ();
    });
  connect (m_feed, &SpotFeed::infoGone, this, [this] (QString const& call) {
      m_board.dropInfo (call.toStdString ());
      if (m_map->hovered ().contains (call, Qt::CaseInsensitive)) refresh ();
    });
  connect (m_feed, &SpotFeed::stateChanged, this, &ReceptionMap::refresh);
  m_refresh.setInterval (1000);
  connect (&m_refresh, &QTimer::timeout, this, &ReceptionMap::refresh);
}

ReceptionMap::~ReceptionMap ()
{
}

void ReceptionMap::setDarkStyle (bool dark)
{
  m_map->setDarkStyle (dark);
  m_status->setDark (dark);
}

void ReceptionMap::setOwnLocator (QString const& locator)
{
  m_map->setOwnLocator (locator);
}

void ReceptionMap::keep (char const * key, bool value)
{
  SettingsGroup g {m_settings, group};
  m_settings->setValue (key, value);
}

void ReceptionMap::setHeardLog (reception_spots::HeardLog const * log)
{
  m_heard = log;
}

void ReceptionMap::setLookups (Lookup const& localGrid, Lookup const& entity, Lookup const& distance)
{
  m_localGrid = localGrid;
  m_map->setLookups (entity, distance);
}

void ReceptionMap::setNow (std::function<long long ()> const& now)
{
  m_now = now;
}

long long ReceptionMap::now () const
{
  return m_now ? m_now () : QDateTime::currentSecsSinceEpoch ();
}

void ReceptionMap::setStation (QString const& ownCall, QString const& band, QString const& mode)
{
  if (ownCall == m_ownCall && band == m_band && mode == m_mode) return;
  m_ownCall = ownCall; m_band = band; m_mode = mode;
  m_board.setStation (ownCall.toStdString (), band.toStdString (), mode.toStdString ());   // empties it
  m_map->setMode (mode);
  listen ();
  refresh ();
}

// The feed listens only while the window is open - shown, or minimised.
void ReceptionMap::listen ()
{
  if (!isVisible () || m_exiting || m_feedOff) return;
  m_feed->listen (m_band, m_mode, m_ownCall, m_reduced);
}

// (2026-11-16, TODO.md 3.-263) Where the feed's compound calls are kept: what the file has from earlier days is in
// the list from the start - a call is kept for 30 days, not for one opening of the map.
void ReceptionMap::setFeedCallsFile (QString const& path)
{
  m_feedCallsFile = path;
  m_feedCalls = feed_calls::Collector {};
  m_feedCallsDirty = false;
  m_feedCallsOnDisk = false;
  if (path.isEmpty ()) return;
  QFile f {path};
  if (!f.open (QIODevice::ReadOnly)) return;
  m_feedCalls.load (f.readAll ().toStdString (), utcDay (now ()));
  m_feedCallsOnDisk = true;
}

// The list to its file when it is another than the file has - through a new file put in the old one's place, so the
// decoder never reads half a list. At most once a minute (at the window's end: two seconds after the last): the
// decoder tells a new file by its size and its second (packjt77.f90 feed_refresh).
//
// (the second review, TODO.md 3.-264) Two things are other than they were:
//   - The file is the operator's to delete - its own first lines say so, the README and the plan: "deleting the file
//     undoes all of it". It did not while the program ran: the list is here as well - in a window that stays when
//     the map is closed - and the next writing put every call back. A file that was there and is gone now takes the
//     window's list with it, the calls and those waiting for a second receiver; what is reported from then on is
//     collected anew.
//   - A writing that fails - a full disk, a directory that may not be written - was tried again at every look, a
//     second apart. It waits its minute as one that worked.
void ReceptionMap::writeFeedCalls (bool closing)
{
  if (m_feedCallsFile.isEmpty ()) return;
  if (m_feedCallsOnDisk && !QFile::exists (m_feedCallsFile))
    {
      m_feedCalls = feed_calls::Collector {};
      m_feedCallsOnDisk = false;
      m_feedCallsDirty = false;
    }
  if (!m_feedCallsDirty) return;
  long long const t {now ()};
  if (m_feedCallsTried >= 0 && t - m_feedCallsTried < (closing ? 2 : 60)) return;
  m_feedCallsTried = t;
  m_feedCalls.expire (utcDay (t));
  QByteArray const text {QByteArray::fromStdString (m_feedCalls.text ())};
  QSaveFile f {m_feedCallsFile};
  if (!f.open (QIODevice::WriteOnly) || f.write (text) != text.size () || !f.commit ()) return;
  m_feedCallsOnDisk = true;
  m_feedCallsDirty = false;
}

void ReceptionMap::forget ()
{
  writeFeedCalls (true);   // (3.-263) what the feed gave since the last minute
  m_refresh.stop ();
  m_feed->stop ();
  m_board = reception_spots::Board {};
  m_board.setStation (m_ownCall.toStdString (), m_band.toStdString (), m_mode.toStdString ());
  m_board.setWindow (m_show30->isChecked () ? reception_spots::longWindowSeconds : reception_spots::windowSeconds);
}

QString ReceptionMap::statusText () const
{
  return m_statusTexts.value (0);
}

QString ReceptionMap::statusShown () const {return m_status->shown ();}
bool ReceptionMap::legendShown () const {return m_status->legendShown ();}

void ReceptionMap::refresh ()
{
  long long const t {now ()};
  if (t / 30 != m_pruned)
    {
      // (the second review, TODO.md 3.-264) a station that reports no longer is asked about no longer: its details'
      // subscription is ended (they only ever grew)
      for (auto const& call : m_board.prune (t)) m_feed->dropInfo (QString::fromStdString (call));
      m_pruned = t / 30;
    }
  // (3.-264) The program's own lookup, asked at every look: the log's is a hash, CALL3.TXT's an index (call3file.h). Its
  // answers were kept for ten minutes - from when CALL3.TXT was read through for every call (3.-261) - and all that
  // did by now was keep a station just logged with its grid off the map for up to ten minutes.
  auto const lookup = [this] (std::string const& call) {
    return m_localGrid ? m_localGrid (QString::fromStdString (call)).toStdString () : std::string {};
  };
  auto stations = m_board.stations (m_heard ? *m_heard : m_noneHeard, lookup, t, &m_counts);
  // (stage 4) what the menu has switched off is not drawn; it is counted all the same
  bool const weHear {m_showWeHear->isChecked ()}, others {m_showOthers->isChecked ()};
  if (!weHear || !others)
    stations.erase (std::remove_if (stations.begin (), stations.end (), [weHear, others] (reception_spots::Station const& s) {
          return (reception_spots::WeHear == s.kind && !weHear) || (reception_spots::Reporting == s.kind && !others);
        }), stations.end ());
  for (auto const& call : m_board.infoToAsk (t)) m_feed->askInfo (QString::fromStdString (call));
  // (3.-262) and of a station under a resting mouse that began to report since the mouse came to it
  for (auto const& call : m_map->hovered ()) if (m_board.infoWanted (call.toStdString ())) m_feed->askInfo (call);
  m_map->setStations (stations, t);
  writeFeedCalls (false);   // (3.-263) the feed's compound calls, when there are new ones - once a minute at most

  // the status line, the longest first: what is left out as the window gets narrower - our call and the window's
  // length, then the reporting stations, then the clock
  QString const air {(m_band + QStringLiteral ("  ") + m_mode).trimmed ()};
  QString state;
  // (the review of 2026-11-16, TODO.md 3.-261; the plan's 5.9) a mode or band no topic of the feed can hold - JT9+JT65,
  // WSPR-2, no band: nothing is asked of the server, and the line says so
  if (reception_spots::bandTopic (m_band.toStdString (), m_mode.toStdString ()).empty ()) state = tr ("no feed for this band and mode");
  else if (SpotFeed::Connecting == m_feed->state ()) state = tr ("connecting");
  else if (SpotFeed::Waiting == m_feed->state ()) state = tr ("no connection - next try in %1 s").arg (m_feed->retryIn ());
  QString const hears {m_counts.hears == m_counts.hearsOnMap ? tr ("hears %1").arg (m_counts.hears) : tr ("hears %1 (%2 on the map)").arg (m_counts.hears).arg (m_counts.hearsOnMap)};
  QString const numbers {tr ("heard by %1").arg (m_counts.heardBy) + QStringLiteral (", ") + hears + QStringLiteral (", ") + tr ("both ways %1").arg (m_counts.bothWays)};
  QString const reporting {tr ("%1 reporting").arg (m_counts.reporting)};
  QString const clock {QDateTime::fromSecsSinceEpoch (t, Qt::UTC).toString (QStringLiteral ("HH:mm")) + QStringLiteral (" UTC")};
  QString const gap {QStringLiteral ("   ")};
  QString const tail {state.isEmpty () ? QString {} : gap + state};
  m_statusTexts = QStringList {
    air + gap + (m_ownCall.isEmpty () ? QString {} : m_ownCall + QLatin1Char (' ')) + numbers + gap + reporting + gap + m_map->ageText (m_board.window ()) + gap + clock + tail,
    air + gap + numbers + gap + reporting + gap + clock + tail,
    air + gap + numbers + gap + clock + tail,
    air + gap + numbers + tail};
  m_map->setOwnInfo (m_ownCall, air + gap + numbers);   // our own mark's tooltip: the band, the mode, who hears whom
  m_status->set (m_statusTexts, tr ("both ways"), tr ("we hear"));
}

void ReceptionMap::resizeEvent (QResizeEvent * e)
{
  QWidget::resizeEvent (e);
  m_status->setGeometry (0, height () - statusHeight, width (), statusHeight);
}

void ReceptionMap::showEvent (QShowEvent * e)
{
  QWidget::showEvent (e);
  if (!m_geometryRestored)
    {
      m_geometryRestored = true;   // only once: a size set during the session is not undone by a second show
      QTimer::singleShot (0, this, [this] {restoreSavedGeometry ();});
    }
  m_status->setGeometry (0, height () - statusHeight, width (), statusHeight);
  m_status->raise ();
  listen ();
  refresh ();   // what we hear is there at once: the last 15 minutes of the program's own decodes
  m_refresh.start ();
}

void ReceptionMap::restoreSavedGeometry ()
{
  if (!m_hadSavedGeometry) return;   // none saved, or refused: the size hint stands
  JTDX::restore_grown_geometry (this, m_savedGeometry, m_savedMinHint);
}

void ReceptionMap::saveSettings (bool open)
{
  SettingsGroup g {m_settings, group};
  if (m_geometryRestored)   // not the size of a window that was never shown
    {
      m_settings->setValue ("geometry", saveGeometry ());
      m_settings->setValue ("geometryMinHint", minimumSizeHint ());
    }
  auto const v = m_map->view ();
  m_settings->setValue ("viewHome", m_map->atHome ());
  m_settings->setValue ("viewX", v.cx);
  m_settings->setValue ("viewY", v.cy);
  m_settings->setValue ("viewZoom", v.zoom);
  m_settings->setValue ("open", open);
}

// the operator closes the window: it stays closed at the next start - and the feed is listened to no longer
void ReceptionMap::closeEvent (QCloseEvent * e)
{
  if (!m_exiting) saveSettings (false);
  forget ();
  QWidget::closeEvent (e);
}

// the program closes: the window comes back at the next start if it is shown now
void ReceptionMap::closeForExit ()
{
  m_exiting = true;
  saveSettings (isVisible ());
  close ();
}
