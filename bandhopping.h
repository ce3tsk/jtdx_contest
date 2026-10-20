#ifndef BANDHOPPING_H
#define BANDHOPPING_H

// CE3TSK 2026-10-20: band hopping - its rules, out of MainWindow so test_bandhopping can walk them
// (IMPROVED_FEATURES_PLAN.md, stage 1).
//
// Tab 3 of the Tx tabs lists the FT8, FT4 and FT2 rows of the frequency table (Settings > Frequencies, the station's
// IARU region); the operator ticks some and presses Band Hopping, and at the top of every minute - or of every even
// minute - the program moves to the next ticked row, mode and dial, and listens there. WS (formerly WSJT-X Improved)
// and JTDX_improved have the feature; this one is built from what theirs taught:
//   - the rows come from the frequency table, not from frequencies written into the code (JTDX_improved), so a
//     regional or a DXpedition frequency is one entry in Settings away (WS does the same);
//   - an empty selection is "no hop" - both of theirs then called themselves until the stack ran out;
//   - where to hop next follows from where the station is now, not from a position kept in a function static that
//     survived a stop and a start, and the two-minute phase is the even UTC minute rather than a count of earlier
//     timer firings;
//   - every reason not to hop has a name, so the program says why it will not start or why it stopped.

#include <QList>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <algorithm>
#include "Radio.hpp"

namespace band_hopping
{
  // a row of the frequency table that hopping can go to
  struct Entry
  {
    Radio::Frequency frequency;
    QString mode;   // "FT8", "FT4" or "FT2"
  };
  inline bool operator== (Entry const& a, Entry const& b) { return a.frequency == b.frequency && a.mode == b.mode; }

  // the modes hopping offers: Tab 3's columns, and the order of the hops
  inline QStringList modes () { return {"FT8", "FT4", "FT2"}; }

  // an entry as the settings keep it, "FT8@14074000"; from_key gives frequency 0 for anything that is not one
  inline QString key (Entry const& e) { return e.mode + '@' + QString::number (e.frequency); }
  inline Entry from_key (QString const& k)
  {
    auto const at = k.indexOf ('@');
    if (at <= 0) return {0, {}};
    bool ok {false};
    auto const f = k.mid (at + 1).toULongLong (&ok);
    auto const mode = k.left (at);
    if (!ok || !f || !modes ().contains (mode)) return {0, {}};
    return {f, mode};
  }

  // the hop order: the mode's column first (FT8, FT4, FT2), then the frequency; a mode hopping does not offer
  // sorts after them all
  inline int column (QString const& mode)
  {
    auto const i = modes ().indexOf (mode);
    return i < 0 ? modes ().size () : i;
  }
  inline bool before (Entry const& a, Entry const& b)
  {
    auto const ca = column (a.mode), cb = column (b.mode);
    return ca != cb ? ca < cb : a.frequency < b.frequency;
  }

  // the ticked entries in hop order, each once
  inline QList<Entry> in_order (QList<Entry> entries)
  {
    std::stable_sort (entries.begin (), entries.end (), before);
    entries.erase (std::unique (entries.begin (), entries.end ()), entries.end ());
    return entries;
  }

  // the index in `ticked` (hop order) of the entry to hop to from `here` - the station's dial and mode now: the first
  // ticked entry after it, wrapping round to the first. -1 when nothing is ticked (WS and JTDX_improved recursed
  // there). Taking `here` rather than a remembered position also makes a hop that ended elsewhere - its mode refused,
  // the rig on another dial - carry on from where the station actually is.
  inline int next (QList<Entry> const& ticked, Entry const& here)
  {
    if (ticked.isEmpty ()) return -1;
    for (int i = 0; i < ticked.size (); ++i)
      if (before (here, ticked[i])) return i;
    return 0;
  }

  // whether a hop is due: at the top of a minute - second 0, 1 or 2, since guiUpdate's ticks can miss a second when
  // the program is busy - every minute or every even UTC minute, and once in a minute. A period of every mode hopping
  // offers ends at the top of every minute (FT8 4, FT4 8, FT2 16 a minute), so a hop never cuts a period in two.
  inline bool due (int minute_of_day, int second, int interval, int last_minute_of_day)
  {
    if (second < 0 || second > 2) return false;
    if (minute_of_day == last_minute_of_day) return false;
    return interval == 1 || 0 == minute_of_day % 2;
  }

  // the minute a hop made at once (the button's right click) stands for, so the timer's hop does not follow it a
  // moment later: one in the first seconds of a minute (0-2, due ()'s) is that minute's, one in its last three seconds
  // the next minute's; -1 at any other second (the next due minute hops as it would have)
  inline int minute_of_manual_hop (int minute_of_day, int second)
  {
    if (second >= 0 && second <= 2) return minute_of_day;
    if (second >= 57 && second <= 59) return (minute_of_day + 1) % 1440;
    return -1;
  }

  // a change the program makes itself - a hop, Settings OK or a rig reconnecting putting the mode back, a halt of its
  // own - is not the operator taking over: for the guard's span the flag is set, and put back as it was after (review
  // 3.-155 2 and 10: Settings OK stopped hopping as "the band or mode was changed")
  class OwnChange
  {
  public:
    explicit OwnChange (bool& flag) : flag_ (flag), was_ (flag) { flag_ = true; }
    ~OwnChange () { flag_ = was_; }
  private:
    OwnChange (OwnChange const&);
    OwnChange& operator= (OwnChange const&);
    bool& flag_;
    bool const was_;
  };

  // the station's state, as hopping needs to know it
  struct Gates
  {
    bool ticked = false;         // at least one entry ticked
    bool rig = false;            // rig control (CAT) works: hopping without it moves the display, not the radio
    bool enableTx = false;
    bool transmitting = false;
    bool tuning = false;
    bool monitoring = false;     // hopping listens; Monitor off is the operator's no
    bool filePlayback = false;   // a .wav file being decoded
    bool hound = false;          // Hound, SuperFox Hound included: the Fox's frequency is the station's
    bool contest = false;        // a contest running (WW Digi, ARRL Digi): no unattended QSYs
    bool scheduler = false;      // the band scheduler: two automatic QSYs would fight
  };

  enum class Refusal {None, NothingTicked, NoRig, EnableTx, Transmitting, Tuning, MonitorOff, FilePlayback, Hound,
                      Contest, Scheduler};

  // why hopping may not start, or must stop - or None. One reason, the first in this order.
  inline Refusal refusal (Gates const& g)
  {
    if (!g.ticked) return Refusal::NothingTicked;
    if (!g.rig) return Refusal::NoRig;
    if (g.transmitting) return Refusal::Transmitting;
    if (g.tuning) return Refusal::Tuning;
    if (g.enableTx) return Refusal::EnableTx;
    if (g.filePlayback) return Refusal::FilePlayback;
    if (!g.monitoring) return Refusal::MonitorOff;
    if (g.hound) return Refusal::Hound;
    if (g.contest) return Refusal::Contest;
    if (g.scheduler) return Refusal::Scheduler;
    return Refusal::None;
  }

  // a row of the frequency table, as Tab 3 is built from it
  struct Row
  {
    Radio::Frequency frequency;
    QString mode;
    bool preferred;   // the row's default mark, the "*" of the band selector
    QString band;     // its band's name, "20m"; empty out of band
  };

  // one check box of Tab 3
  struct Offered
  {
    Entry entry;
    bool preferred;
    bool byFrequency;   // labelled by its frequency rather than by its band
  };

  // what Tab 3 offers for one mode: its default rows first, in frequency order, each labelled by its band - or by its
  // frequency where the band has more than one default row of the mode, or none (out of band); then the mode's other
  // rows (DXpedition and local frequencies, kept in Settings > Frequencies), labelled by their frequency. Each
  // frequency once; a frequency with a default row and another row is the default one.
  inline QList<Offered> offered (QList<Row> const& rows, QString const& mode)
  {
    QList<Row> defaults, others;
    for (auto const& r : rows)
      {
        if (r.mode != mode) continue;
        auto const same = [&r] (Row const& x) { return x.frequency == r.frequency; };
        if (r.preferred)
          {
            if (std::none_of (defaults.begin (), defaults.end (), same)) defaults << r;
          }
        else if (std::none_of (others.begin (), others.end (), same)) others << r;
      }
    others.erase (std::remove_if (others.begin (), others.end (), [&defaults] (Row const& r) {
        return std::any_of (defaults.begin (), defaults.end (), [&r] (Row const& d) { return d.frequency == r.frequency; });
      }), others.end ());
    auto const by_frequency = [] (Row const& a, Row const& b) { return a.frequency < b.frequency; };
    std::sort (defaults.begin (), defaults.end (), by_frequency);
    std::sort (others.begin (), others.end (), by_frequency);
    QList<Offered> out;
    for (auto const& d : defaults)
      {
        auto const shared = std::count_if (defaults.begin (), defaults.end (), [&d] (Row const& x) { return x.band == d.band; });
        out << Offered {{d.frequency, mode}, true, d.band.isEmpty () || shared > 1};
      }
    for (auto const& o : others) out << Offered {{o.frequency, mode}, false, true};
    return out;
  }
}

#endif
