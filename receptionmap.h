#ifndef RECEPTIONMAP_H
#define RECEPTIONMAP_H

// CE3TSK 2026-11-16 (TODO.md 3.-260, RECEPTION_MAP_PLAN.md): View > Reception map - a window of its own with the world
// map (worldmapview.h), the stations on it and a status line.
//
// Stage 1, the window:
//   - its size and place are kept and put back by the one rule of geometryrestore.h, as the Wide Graph's are;
//   - the view (the zoom and the middle) is kept with them, once the operator has zoomed or dragged; until then the
//     map opens at its own view (worldmap.h defaultView ());
//   - it reopens at the program's start when it was open at the last exit (decision 3): "open" is written true by
//     closeForExit () - the program closing with the window shown - and false when the operator closes the window;
//   - its own group of the settings, "ReceptionMap", which a stock JTDX sharing the ini never reads.
//
// Stage 3, the stations:
//   - the feed (spotfeed.h) listens only while the window is open: started when it is shown, stopped - and what it
//     gave forgotten - when it is closed. Its server is the settings' feedHost and feedPort (mqtt.pskreporter.info,
//     1883; a sandbox's are its own);
//   - what WE hear is not the feed's: it is the program's log of its own decodes (setHeardLog ()), there from the
//     program's start, so the window opens with the last 15 minutes in it, with no internet as well; a station we
//     hear whose messages gave no grid is placed by the program's own lookup (setLookups ()), asked at every look
//     (the second review, TODO.md 3.-264: its answers were kept for ten minutes, which kept a station just logged
//     off the map that long);
//   - once a second the stations are brought up to now (receptionspots.h Board::stations ()) and handed to the map,
//     and the status line says for which band, mode and call they are, how many hear us, how many we hear, how many
//     both ways, how many report, and how the feed is when it is not simply there. A spot of the feed is only kept
//     when it arrives - the map is drawn by the clock, not by the spot.
//
// Stage 4, the menu's entries of the window (the map's own come first: worldmapview.h) - each checkable, each kept
// in the settings the moment it is changed:
//   - "Show stations we hear" (on): off, the stations we hear and that do not hear us are not drawn - the one heard
//     both ways stays, ring and frame with it; the status line counts on;
//   - "Show other stations" (on): off, the stations that only report are not drawn;
//   - "Show age labels" (on; the operator, 2026-11-16): off, no age is written above the stations that hear us;
//   - "Show DXCC prefixes" (on): the entities' prefixes on the map;
//   - "Show 30 minutes" (off; the operator, 2026-11-16): on, the map shows the last 30 minutes instead of 15 - what
//     we hear and the feed's stations are kept for 30 minutes whatever is shown, so the longer window is there the
//     moment it is chosen; the status line says which;
//   - "Reduced data" (off): on, the server is asked for the reports of our own call and for its 1 % sample of the
//     band instead of every report of band and mode (decision 2) - at once, on the connection there is.

#include <functional>
#include <QByteArray>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include "receptionspots.h"
#include "feedcalls.h"

class JTDXDateTime;
class QAction;
class QSettings;
class ReceptionStatus;
class SpotFeed;
class WorldMapView;

class ReceptionMap
  : public QWidget
{
  Q_OBJECT

public:
  typedef std::function<QString (QString const&)> Lookup;

  ReceptionMap (QSettings * settings, JTDXDateTime * clock, QWidget * parent = nullptr);
  ~ReceptionMap ();

  static bool wasOpen (QSettings * settings);   // open when the program was last closed

  void setDarkStyle (bool dark);
  void setOwnLocator (QString const& locator);
  WorldMapView * map () const {return m_map;}

  // our call, band and mode (our names): whose reports, and of which air. A change empties the map of the feed's
  // stations and has the feed listen anew.
  void setStation (QString const& ownCall, QString const& band, QString const& mode);
  void setHeardLog (reception_spots::HeardLog const * log);   // what we hear; the program's, not ours to keep
  // a call's grid as the program itself knows it (its log, CALL3.TXT) or "", a call's DXCC entity, and the distance
  // and azimuth to a locator - all as the decode windows have them
  void setLookups (Lookup const& localGrid, Lookup const& entity, Lookup const& distance);

  SpotFeed * feed () const {return m_feed;}
  reception_spots::Counts counts () const {return m_counts;}
  QString statusText () const;                  // the status line's longest text
  QString statusShown () const;                 // the one it has room for, as last drawn
  bool legendShown () const;                    // and whether the legend had room beside it
  void refresh ();                              // the stations and the status line brought up to now
  void setNow (std::function<long long ()> const& now);   // the tests' clock (UTC seconds)
  void setRefreshInterval (int ms) {m_refresh.setInterval (ms);}   // how often the window looks by itself (a second; a test's may be longer)
  // (2026-11-16, TODO.md 3.-263) the feed's compound calls for the decoder (feedcalls.h): the file they are kept in -
  // compound_calls_feed.txt in the decoder's data directory; none given, none collected. (3.-264) The file is the
  // operator's to delete: once it is gone the window's list is gone with it, and collecting begins anew.
  void setFeedCallsFile (QString const& path);
  feed_calls::Collector const& feedCalls () const {return m_feedCalls;}

  Q_SLOT void closeForExit ();                  // the program is closing

protected:
  void showEvent (QShowEvent *) override;
  void closeEvent (QCloseEvent *) override;
  void resizeEvent (QResizeEvent *) override;

private:
  void restoreSavedGeometry ();
  void saveSettings (bool open);
  void keep (char const * key, bool value);     // a menu entry's state into the settings, at once
  void listen ();                               // the feed told what to listen to, when the window is open
  void forget ();                               // the feed stopped and what it gave let go
  void writeFeedCalls (bool closing);           // (3.-263) the feed's compound calls to their file, when they are others than it has
  long long now () const;

  QSettings * m_settings;
  WorldMapView * m_map;
  ReceptionStatus * m_status;
  SpotFeed * m_feed;
  reception_spots::Board m_board;
  reception_spots::HeardLog const * m_heard {nullptr};
  reception_spots::HeardLog m_noneHeard;
  reception_spots::Counts m_counts;
  Lookup m_localGrid;
  QString m_ownCall, m_band, m_mode;
  bool m_reduced {false};
  QAction * m_showWeHear;                       // the menu's entries of the window
  QAction * m_showOthers;
  QAction * m_showAges;
  QAction * m_showPrefixes;
  QAction * m_show30;
  QAction * m_reducedData;
  bool m_feedOff {false};                       // JTDX_RECEPTION_FEED says so: no feed at all
  feed_calls::Collector m_feedCalls;            // (3.-263) nonstandard calls two receivers reported, for the decoder
  QString m_feedCallsFile;
  bool m_feedCallsDirty {false};                //   the list is another than its file
  bool m_feedCallsOnDisk {false};               //   (3.-264) the file was there when last read or written: gone since, it was deleted
  long long m_feedCallsTried {-1};              //   when its writing was last tried in this run (the window's clock); -1 never
  QTimer m_refresh;
  long long m_pruned {0};
  std::function<long long ()> m_now;
  QStringList m_statusTexts;

  QByteArray m_savedGeometry;
  QSize m_savedMinHint;
  bool m_hadSavedGeometry {false};
  bool m_geometryRestored {false};   // the window has been through a show: its geometry may be written
  bool m_exiting {false};
};

#endif
