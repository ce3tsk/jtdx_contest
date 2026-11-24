#ifndef RECEPTIONSPOTS_H
#define RECEPTIONSPOTS_H

// CE3TSK 2026-11-16 (TODO.md 3.-260, RECEPTION_MAP_PLAN.md stages 2 and 3): what the Reception map knows of the
// stations it shows - its rules, free of Qt.
//
// TWO sources, kept apart:
//
//   - the feed of mqtt.pskreporter.info (spotfeed.cpp), for our band and mode: every station that REPORTS there, and
//     among them those that heard US. The Board holds them for 15 minutes.
//   - what WE hear: the program's own decodes, and nothing else (the operator, 2026-11-16: "For the stations received by
//     me please use the local information directly, i.e. do not use remote information so that it works even when there
//     is no internet or the pskreporter mqtt server is down, and also it can directly fill the last 15 minutes from the
//     local data when the window is opened"). The HeardLog holds them from the program's start, window or no window,
//     for every band and mode. A station we hear is placed by the grid of its own message - the newest in the 15
//     minutes, else the last it sent while the program ran - and else by what the program itself knows of the call
//     (its log, CALL3.TXT; the caller's lookup). Never by the feed's locator. With no place it is not on the map, and
//     counted.
//
// The three kinds a station of the map is, in the order they are drawn - the later over the earlier:
//   Reporting   it reports on our band and mode, and neither hears us nor is heard by us (a small dot);
//   WeHear      we hear it and it does not hear us (a diamond, no label), the oldest first;
//   HearsUs     it hears us (a dot coloured by the SNR it gave us, its age above it), the oldest first - and those we
//               hear as well, "both ways", after all the others.
// A station that reports and that we hear, but that the program cannot place by itself, stays Reporting - it is on
// the map by the feed alone - and its tooltip says that we hear it.
//
// Also here: the feed's topic names, what a spot must be to be kept, which labels of a crowd are drawn, and which
// station the mouse is on.
//
// test_spotfeed has the rules.

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "worldmap.h"
#include "tracespots.h"

namespace reception_spots
{
  long long const windowSeconds {900};          // what the map shows: the last 15 minutes
  long long const longWindowSeconds {1800};     // or the last 30, with the menu's "Show 30 minutes" (the operator, 2026-11-16)
  long long const keptSeconds {1800};           // what is kept: the longer of the two, so that the switch has its half hour at once
  long long const futureGraceSeconds {120};     // a clock two minutes fast is still taken
  // (2026-11-24, TODO.md 3.-289) the square others heard a call send: so many receivers must agree, and so many are kept
  // a call - the newest - for it (a strong station is heard by hundreds; two that agree are what is asked)
  int const spottedReceivers {2};
  std::size_t const spottedKept {16};
  long long const gridMemorySeconds {86400};    // how long the grid a station sent is remembered for placing it
  std::size_t const maxCallLength {20};

  inline std::string upper (std::string s)
  {
    for (auto& c : s) if (c >= 'a' && c <= 'z') c = static_cast<char> (c - 'a' + 'A');
    return s;
  }

  // ------------------------------------------------------------------------------------------ the feed's names
  // a call or a receiver's name as a topic's level may hold it: 1 to 20 of letters, digits, / - and . (receivers' names
  // need not be callsigns: "ZL2005SWL", "14AC76", "K9TRV-5")
  inline bool goodCall (std::string const& call)
  {
    if (call.empty () || call.size () > maxCallLength) return false;
    for (char c : call)
      if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || '/' == c || '-' == c || '.' == c)) return false;
    return true;
  }

  // a band or a mode as a topic's level may hold it: letters, digits and a point ("1.25m"), 1 to 12
  inline bool goodWord (std::string const& word)
  {
    if (word.empty () || word.size () > 12) return false;
    for (char c : word) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || '.' == c)) return false;
    return true;
  }

  // our band's name as the feed has it: ours are ADIF's, and the feed says 2200m where ADIF says 2190m
  inline std::string feedBand (std::string const& ours) {return "2190m" == ours ? std::string {"2200m"} : ours;}

  // a slash of a callsign is a point in a topic (a slash would be a level more)
  inline std::string topicCall (std::string call)
  {
    for (auto& c : call) if ('/' == c) c = '.';
    return upper (call);
  }

  // every spot of a band and mode; our own spots alone; one spot in a hundred of them; one receiver's details. Empty
  // when a part is not what a topic may hold - nothing is subscribed then.
  inline std::string bandTopic (std::string const& band, std::string const& mode)
  {
    return goodWord (band) && goodWord (mode) ? "pskr/filter/v2/" + feedBand (band) + "/" + mode + "/#" : std::string {};
  }
  inline std::string ownTopic (std::string const& band, std::string const& mode, std::string const& call)
  {
    return goodWord (band) && goodWord (mode) && goodCall (call) ? "pskr/filter/v2/" + feedBand (band) + "/" + mode + "/" + topicCall (call) + "/#" : std::string {};
  }
  inline std::string sampleTopic (std::string const& band, std::string const& mode)
  {
    return goodWord (band) && goodWord (mode) ? "pskr/filter/v2raw_1pc/" + feedBand (band) + "/" + mode + "/#" : std::string {};
  }
  inline std::string infoTopic (std::string const& call)
  {
    return goodCall (call) ? "pskr/rxinfo/" + topicCall (call) : std::string {};
  }

  // a locator that places a station: a locator (worldmap.h), and not AA00 - what software sends that was told no
  // locator, as the distance after a decode has it (distancefield.h)
  inline bool placed (std::string const& locator)
  {
    return world_map::place (locator).ok && upper (locator).compare (0, 4, "AA00") != 0;
  }

  // the grid of a decoded message: as above, and not RR73 - a roger that reads as a locator
  inline bool messageGrid (std::string const& grid) {return placed (grid) && upper (grid) != "RR73";}

  // (2026-11-16, TODO.md 3.-262, the operator: "if we get information from mqtt pskreporter, then let's add that to the
  // tooltip, e.g. if the locator has more precision than the 4 characters obtained via the protocols") Is `finer` the
  // same place as `coarse`, said more exactly: a locator that begins with all of the other and goes on. PM95fr refines
  // PM95, PM95fr12 refines PM95fr; PM96fr does not refine PM95, nor PM95 itself.
  inline bool refines (std::string const& finer, std::string const& coarse)
  {
    return !coarse.empty () && finer.size () > coarse.size () && placed (finer) && upper (finer).compare (0, coarse.size (), upper (coarse)) == 0;
  }

  // ------------------------------------------------------------------------------------------ a decoded line of ours
  // (the review of 2026-11-16, TODO.md 3.-261: the second word of a message was taken for its sender - "TNX BOB 73 GL"
  // made BOB a station heard, "TU; W9XYZ K1ABC R 579 MA" the one it was sent TO) Whose a decoded message is, and the
  // grid it gives of itself. The sender is the waterfall's (tracespots.h sender (): a CQ's call, the second call of an
  // exchange, a Fox's second half, a call out of its brackets; "" for free text, an unknown hash, a roger where a call
  // would be). The grid is the word after the sender's call - after an "R" too - where that is a locator and not
  // RR73. The message WHOLE (DecodedText::wholeMessage ()), whatever follows it on its line left alone.
  struct Line
  {
    std::string sender;            // "" - nobody's: nothing is heard
    std::string grid;              // "" - it gave none
  };

  inline Line line (std::string const& message)
  {
    Line l;
    l.sender = trace_spots::sender (message);
    if (l.sender.empty ()) return l;
    std::vector<std::string> const w {trace_spots::detail::words (message)};
    for (std::size_t i = w.size (); i-- > 0;)
      {
        if (w[i] != l.sender && w[i] != "<" + l.sender + ">") continue;
        std::size_t g {i + 1};
        if (g < w.size () && w[g] == "R") ++g;
        if (g < w.size () && messageGrid (w[g])) l.grid = w[g];
        break;
      }
    return l;
  }

  // ---------------------------------------------------------------------------------------------------- a spot
  struct Spot
  {
    std::string band;              // the feed's name
    std::string mode;
    std::string sender;
    std::string receiver;
    std::string receiverLocator;
    long long frequency {0};       // Hz
    bool hasSnr {false};
    int snr {0};
    long long time {0};            // when the transmission began (t_tx), else what the receiver reported (t); UTC seconds
    std::string senderLocator;     // (2026-11-24, TODO.md 3.-289) the sender's, as the receiver's software gave it ("sl"): what
                                   // it decoded from the sender's messages, as a rule; may be empty
  };

  // whether a spot may be kept: of our band and mode, its calls what a call may be, its receiver somewhere, a
  // frequency and a report that can be, its time within what is kept - the longer window - and not from the future
  inline bool sound (Spot const& s, std::string const& band, std::string const& mode, long long now)
  {
    return upper (s.band) == upper (feedBand (band)) && upper (s.mode) == upper (mode)
           && goodCall (s.sender) && goodCall (s.receiver) && placed (s.receiverLocator)   // the sender's locator refuses no spot: (3.-289) only the fill's square reads it
           && s.frequency > 0 && s.frequency < 1000000000000LL
           && (!s.hasSnr || (s.snr >= -60 && s.snr <= 99))
           && s.time <= now + futureGraceSeconds && now - s.time <= keptSeconds;
  }

  struct Info                      // a receiver's details, of pskr/rxinfo
  {
    std::string decoder;
    std::string antenna;
  };

  // The moment a decoded line's period began, UTC seconds: the line carries the second of the day only, so it is put to
  // the day it must be of - the day of `now`, or the one before when that second is still to come (a line of 23:59:45
  // shown just after midnight).
  inline long long periodTime (int secondOfDay, long long now)
  {
    long long const day {now - (now % 86400 + 86400) % 86400};
    long long t {day + secondOfDay};
    if (t > now + futureGraceSeconds) t -= 86400;
    return t;
  }

  // ------------------------------------------------------------------------------------------------ what we hear
  struct Heard
  {
    std::string call;
    std::string grid;              // of its own messages: the newest within the window, else the last it sent; may be empty
    long long time {0};            // our newest decode of it, UTC seconds
    int snr {0};
    long long frequency {0};       // Hz: the dial and the audio
  };

  // The program's own decodes, from its start: the newest of each call on each band and mode, kept for 30 minutes -
  // the longer window; and the grid each call last sent, for a day.
  class HeardLog
  {
  public:
    void hear (std::string const& call, std::string const& band, std::string const& mode, std::string const& grid, long long time, int snr, long long frequency)
    {
      if (!goodCall (call) || band.empty () || mode.empty ()) return;
      std::string const c {upper (call)};
      if (messageGrid (grid))
        {
          Grid& g = m_grids[c];
          if (time >= g.time) {g.grid = grid; g.time = time;}
        }
      Entry& e = m_heard[key (band, mode)][c];
      if (time < e.time) return;   // an older line than the one kept
      e.time = time; e.snr = snr; e.frequency = frequency;
    }

    // those heard on a band and mode in the window before `now` - 15 minutes, or the 30 asked for - the oldest first
    // (and by call where the time is one)
    std::vector<Heard> heard (std::string const& band, std::string const& mode, long long now, long long window = windowSeconds) const
    {
      std::vector<Heard> out;
      auto const air = m_heard.find (key (band, mode));
      if (air == m_heard.end ()) return out;
      for (auto const& kv : air->second)
        {
          if (now - kv.second.time > window || kv.second.time > now + futureGraceSeconds) continue;
          Heard h;
          h.call = kv.first; h.time = kv.second.time; h.snr = kv.second.snr; h.frequency = kv.second.frequency;
          auto const g = m_grids.find (kv.first);
          if (g != m_grids.end () && now - g->second.time <= gridMemorySeconds) h.grid = g->second.grid;
          out.push_back (h);
        }
      std::sort (out.begin (), out.end (), [] (Heard const& a, Heard const& b) {return a.time != b.time ? a.time < b.time : a.call < b.call;});
      return out;
    }

    // (2026-11-23, TODO.md 3.-283, GRID_FILL_PLAN.md) the grid a call last sent, heard by this program within the day
    // before `now` - or within less, `within` seconds (the review: a mobile's half hour) - for a QSO logged with none
    // (gridfill.h); or ""
    std::string gridOf (std::string const& call, long long now, long long within = gridMemorySeconds) const
    {
      auto const g = m_grids.find (upper (call));
      return g != m_grids.end () && now - g->second.time <= std::min (within, gridMemorySeconds) && g->second.time <= now + futureGraceSeconds
        ? g->second.grid : std::string {};
    }

    // what is too old to be shown again is let go
    void prune (long long now)
    {
      for (auto air = m_heard.begin (); air != m_heard.end ();)
        {
          for (auto it = air->second.begin (); it != air->second.end ();)
            if (now - it->second.time > keptSeconds) it = air->second.erase (it); else ++it;
          if (air->second.empty ()) air = m_heard.erase (air); else ++air;
        }
      for (auto it = m_grids.begin (); it != m_grids.end ();)
        if (now - it->second.time > gridMemorySeconds) it = m_grids.erase (it); else ++it;
    }

    std::size_t calls () const
    {
      std::size_t n {0};
      for (auto const& air : m_heard) n += air.second.size ();
      return n;
    }
    std::size_t grids () const {return m_grids.size ();}

  private:
    struct Entry
    {
      long long time {0};
      int snr {0};
      long long frequency {0};
    };
    struct Grid
    {
      std::string grid;
      long long time {0};
    };
    static std::string key (std::string const& band, std::string const& mode) {return upper (band) + "/" + upper (mode);}

    std::map<std::string, std::map<std::string, Entry>> m_heard;   // band/mode -> call -> its newest line
    std::map<std::string, Grid> m_grids;                           // call -> the grid it last sent
  };

  // ------------------------------------------------------------------------------------------- a station of the map
  enum Kind {Reporting = 0, WeHear = 1, HearsUs = 2};

  struct Station
  {
    std::string call;
    Kind kind {Reporting};
    bool bothWays {false};         // HearsUs, and we hear it
    std::string locator;           // where it is drawn: the feed's for a station that reports; for one we hear ours - or
                                   // (3.-262) the feed's of it where that is our own, said more exactly
    double lat {0};
    double lon {0};

    bool hearsUs {false};
    long long heardUs {0};         // when it last received us
    bool hasSnr {false};
    int snr {0};                   // the report it gave us
    long long ourFrequency {0};    // where it received us

    bool hasTx {false};            // someone reported IT in the window
    long long txFrequency {0};
    long long txTime {0};

    bool weHear {false};           // from our own decodes
    long long weHeard {0};
    int ourSnr {0};                // the report we would give it
    long long itsFrequency {0};    // where we received it

    bool hasInfo {false};
    Info info;
  };

  struct Counts
  {
    int heardBy {0};               // stations that hear us
    int hears {0};                 // stations we hear
    int hearsOnMap {0};            // of those, the ones the map shows as heard: placed by ourselves, or hearing us
    int bothWays {0};
    int reporting {0};             // stations reporting on our band and mode, those that hear us among them
  };

  // The feed's side: who reports on our band and mode, who of them heard us, and where stations transmit - kept for
  // 30 minutes and shown for the window's 15, or for 30 (setWindow ()). A change of our call, band or mode drops it
  // all.
  class Board
  {
  public:
    typedef std::function<std::string (std::string const&)> GridLookup;   // a call's grid as the program itself knows it, or ""

    // true when something changed - the board is empty then
    bool setStation (std::string const& ownCall, std::string const& band, std::string const& mode)
    {
      std::string const c {upper (ownCall)};
      if (c == m_ownCall && band == m_band && mode == m_mode) return false;
      m_ownCall = c; m_band = band; m_mode = mode;
      m_reporting.clear (); m_hearsUs.clear (); m_transmits.clear (); m_info.clear (); m_asked.clear ();
      m_sightings.clear ();   // (3.-289)
      return true;
    }

    // the window shown: 15 minutes, or the 30 of "Show 30 minutes" - nothing else. What is kept does not change with
    // it, so the longer window is full the moment it is chosen.
    void setWindow (long long seconds) {m_window = longWindowSeconds == seconds ? longWindowSeconds : windowSeconds;}
    long long window () const {return m_window;}

    std::string const& ownCall () const {return m_ownCall;}
    std::string const& band () const {return m_band;}
    std::string const& mode () const {return m_mode;}

    // (2026-11-23, TODO.md 3.-283, GRID_FILL_PLAN.md) the locator a call reports to PSK Reporter as a receiver - its own
    // software's - from its newest report within what is kept before `now`; "" when it reports none here
    std::string reportedLocator (std::string const& call, long long now) const
    {
      auto const r = m_reporting.find (upper (call));
      return r != m_reporting.end () && now - r->second.lastReport <= keptSeconds && r->second.lastReport <= now + futureGraceSeconds
        ? r->second.locator : std::string {};
    }

    /* (2026-11-24, TODO.md 3.-289, the operator: "build the sl source with the two-receiver rule") the square a call was
       heard sending by other stations: the first four characters of the sender's locator ("sl") of their spots of it -
       what their software decoded from its messages - when at least two receivers, neither we nor the call itself, gave
       the same square in their newest spot of it within what is kept before `now`, and more of them than gave any other;
       "" else. One receiver's phantom decode is no place, nor are two that two others contradict. */
    std::string spottedSquare (std::string const& call, long long now) const
    {
      auto const seen = m_sightings.find (upper (call));
      if (seen == m_sightings.end ()) return {};
      std::map<std::string, int> receivers;
      for (auto const& kv : seen->second)
        if (now - kv.second.time <= keptSeconds && kv.second.time <= now + futureGraceSeconds) ++receivers[kv.second.square];
      std::string best;
      int most {0}, next {0};
      for (auto const& kv : receivers)
        if (kv.second > most) {next = most; most = kv.second; best = kv.first;}
        else if (kv.second > next) next = kv.second;
      return most >= spottedReceivers && most > next ? best : std::string {};
    }
    // (3.-289) how many receivers' squares are kept, of every call - for the tests: the bound, the pruning
    std::size_t sightingsKept () const
    {
      std::size_t n {0};
      for (auto const& kv : m_sightings) n += kv.second.size ();
      return n;
    }

    // a spot of the feed; false when it is not sound (and nothing is kept of it)
    bool addSpot (Spot const& s, long long now)
    {
      if (!sound (s, m_band, m_mode, now)) return false;
      std::string const rx {upper (s.receiver)}, tx {upper (s.sender)};
      Transmit& t = m_transmits[tx];
      if (s.time >= t.time) {t.time = s.time; t.frequency = s.frequency;}
      // (the review of 2026-11-16, TODO.md 3.-261) A report of OUR OWN - a station that reports to PSK Reporter gets its
      // own spots back - makes us no station of the map: we are its middle. It still says where the other transmits.
      if (!m_ownCall.empty () && rx == m_ownCall) return true;
      Reporter& r = m_reporting[rx];
      if (s.time >= r.lastReport) {r.lastReport = s.time; r.locator = s.receiverLocator;}
      if (!m_ownCall.empty () && tx == m_ownCall)
        {
          Hearing& h = m_hearsUs[rx];
          if (s.time >= h.time) {h.time = s.time; h.hasSnr = s.hasSnr; h.snr = s.snr; h.frequency = s.frequency;}
        }
      // (3.-289) the square this receiver heard the sender send - not ours (our own reports returned above: they are what
      // we heard, the fill's first source), not its own of itself, not of our call (the fill never asks it, and the
      // stations that hear us are many)
      if (tx != rx && tx != m_ownCall && messageGrid (s.senderLocator)) see (tx, rx, upper (s.senderLocator.substr (0, 4)), s.time);
      return true;
    }

    // a receiver's details, or their end (the feed's empty message)
    void setInfo (std::string const& call, Info const& info) {m_info[upper (call)] = info;}
    void dropInfo (std::string const& call) {m_info.erase (upper (call));}

    // the stations that hear us whose details were not asked for yet - asked for now
    // (3.-262, the operator: "let's also put more info in the tooltip of other stations, e.g. just like from the ones
    // that hear us") A receiver's details wanted when the mouse comes to it: of any station that reports, once - not
    // asked of the server for every station of the band, there are over a thousand.
    bool infoWanted (std::string const& call)
    {
      std::string const c {upper (call)};
      return m_reporting.find (c) != m_reporting.end () && m_asked.insert (std::make_pair (c, true)).second;
    }
    std::vector<std::string> infoToAsk (long long now)
    {
      std::vector<std::string> ask;
      for (auto const& kv : m_hearsUs)
        if (now - kv.second.time <= m_window && m_asked.insert (std::make_pair (kv.first, true)).second) ask.push_back (kv.first);
      return ask;
    }

    // What is older than the 30 minutes kept goes. (the second review, TODO.md 3.-264) And with a station that reports
    // no longer goes what the server was asked about it: its details, and that they were asked for - each is a
    // subscription of its own, and none was ever let go while band, mode and call stayed. The calls let go are handed
    // back, for the feed to end their subscriptions; a station that reports again is asked about again.
    std::vector<std::string> prune (long long now)
    {
      for (auto it = m_reporting.begin (); it != m_reporting.end ();)
        if (now - it->second.lastReport > keptSeconds) it = m_reporting.erase (it); else ++it;
      for (auto it = m_hearsUs.begin (); it != m_hearsUs.end ();)
        if (now - it->second.time > keptSeconds) it = m_hearsUs.erase (it); else ++it;
      for (auto it = m_transmits.begin (); it != m_transmits.end ();)
        if (now - it->second.time > keptSeconds) it = m_transmits.erase (it); else ++it;
      for (auto it = m_sightings.begin (); it != m_sightings.end ();)   // (3.-289)
        {
          for (auto s = it->second.begin (); s != it->second.end ();)
            if (now - s->second.time > keptSeconds) s = it->second.erase (s); else ++s;
          if (it->second.empty ()) it = m_sightings.erase (it); else ++it;
        }
      for (auto it = m_info.begin (); it != m_info.end ();)
        if (m_reporting.find (it->first) == m_reporting.end ()) it = m_info.erase (it); else ++it;
      std::vector<std::string> letGo;
      for (auto it = m_asked.begin (); it != m_asked.end ();)
        if (m_reporting.find (it->first) == m_reporting.end ())
          {
            letGo.push_back (it->first);
            it = m_asked.erase (it);
          }
        else ++it;
      return letGo;
    }

    // The map's stations at `now`, in the order they are drawn (see the top). `ours` is what we hear; `localGrid` what
    // the program knows of a call when its own messages gave no grid.
    std::vector<Station> stations (HeardLog const& ours, GridLookup const& localGrid, long long now, Counts * counts = nullptr) const
    {
      std::map<std::string, Station> all;
      for (auto const& kv : m_reporting)
        {
          if (now - kv.second.lastReport > m_window) continue;
          world_map::Place const p {world_map::place (kv.second.locator)};
          if (!p.ok) continue;
          Station s;
          s.call = kv.first;
          s.locator = kv.second.locator; s.lat = p.lat; s.lon = p.lon;
          all[kv.first] = s;
        }
      Counts c;
      c.reporting = static_cast<int> (all.size ());
      for (auto const& kv : m_hearsUs)
        {
          if (now - kv.second.time > m_window) continue;
          auto const it = all.find (kv.first);
          if (it == all.end ()) continue;   // a report of us is a report: its station is there
          Station& s = it->second;
          s.kind = HearsUs; s.hearsUs = true; s.heardUs = kv.second.time; s.hasSnr = kv.second.hasSnr; s.snr = kv.second.snr;
          s.ourFrequency = kv.second.frequency;
          ++c.heardBy;
        }
      for (auto const& h : ours.heard (m_band, m_mode, now, m_window))
        {
          if (h.call == m_ownCall) continue;
          ++c.hears;
          auto it = all.find (h.call);
          if (it != all.end () && HearsUs == it->second.kind)
            {
              it->second.bothWays = true;
              ++c.bothWays; ++c.hearsOnMap;
            }
          else
            {
              // where the program itself places it: the grid of its own messages, else what it knows of the call
              std::string grid {h.grid};
              if (!placed (grid) && localGrid) grid = localGrid (h.call);
              world_map::Place const p {world_map::place (grid)};
              if (placed (grid))
                {
                  if (it == all.end ())
                    {
                      Station s;
                      s.call = h.call;
                      it = all.insert (std::make_pair (h.call, s)).first;
                    }
                  it->second.kind = WeHear;
                  // (3.-262) it is on the map by what the program itself knows; where the feed has the same place more
                  // exactly - the six or more characters of a station that reports - that is taken for it, and shown
                  if (!refines (it->second.locator, grid)) {it->second.locator = grid; it->second.lat = p.lat; it->second.lon = p.lon;}
                  ++c.hearsOnMap;
                }
              else if (it == all.end ()) continue;   // it does not report and has no place: counted, not on the map
              // else it reports, not us, and has no place of ours: it stays where the feed has it
            }
          Station& s = it->second;
          s.weHear = true; s.weHeard = h.time; s.ourSnr = h.snr; s.itsFrequency = h.frequency;
        }
      std::vector<Station> out;
      out.reserve (all.size ());
      for (auto& kv : all)
        {
          Station& s = kv.second;
          auto const tx = m_transmits.find (kv.first);
          if (tx != m_transmits.end () && now - tx->second.time <= m_window)
            {
              s.hasTx = true; s.txFrequency = tx->second.frequency; s.txTime = tx->second.time;
            }
          if (s.weHear && (!s.hasTx || s.weHeard >= s.txTime))   // our own decode of it is the newer word on where it transmits
            {
              s.hasTx = true; s.txFrequency = s.itsFrequency; s.txTime = s.weHeard;
            }
          auto const info = m_info.find (kv.first);
          if (info != m_info.end ()) {s.hasInfo = true; s.info = info->second;}
          out.push_back (s);
        }
      std::sort (out.begin (), out.end (), [] (Station const& a, Station const& b) {
          int const ra {rank (a)}, rb {rank (b)};
          if (ra != rb) return ra < rb;
          long long const ta {HearsUs == a.kind ? a.heardUs : WeHear == a.kind ? a.weHeard : 0}, tb {HearsUs == b.kind ? b.heardUs : WeHear == b.kind ? b.weHeard : 0};
          return ta != tb ? ta < tb : a.call < b.call;
        });
      if (counts) *counts = c;
      return out;
    }

  private:
    static int rank (Station const& s) {return HearsUs == s.kind ? (s.bothWays ? 3 : 2) : WeHear == s.kind ? 1 : 0;}

    struct Reporter
    {
      std::string locator;
      long long lastReport {0};
    };
    struct Hearing
    {
      long long time {0};
      bool hasSnr {false};
      int snr {0};
      long long frequency {0};
    };
    struct Transmit
    {
      long long time {0};
      long long frequency {0};
    };
    struct Sighting                // (3.-289) a receiver's newest square of a call
    {
      std::string square;          // four characters, upper case
      long long time {0};
    };

    // (3.-289) a receiver's square of a call: its newer spot replaces its older; a receiver new to the call takes the
    // place of the oldest when spottedKept are kept, unless its spot is older than all of them
    void see (std::string const& tx, std::string const& rx, std::string const& square, long long time)
    {
      auto& seen = m_sightings[tx];
      auto const it = seen.find (rx);
      if (it != seen.end ())
        {
          if (time >= it->second.time) {it->second.square = square; it->second.time = time;}
          return;
        }
      if (seen.size () >= spottedKept)
        {
          auto oldest = seen.begin ();
          for (auto i = seen.begin (); i != seen.end (); ++i) if (i->second.time < oldest->second.time) oldest = i;
          if (time <= oldest->second.time) return;   // older than every one kept: nothing to learn
          seen.erase (oldest);
        }
      Sighting& w = seen[rx];
      w.square = square;
      w.time = time;
    }

    std::string m_ownCall, m_band, m_mode;

    long long m_window {windowSeconds};
    std::map<std::string, Reporter> m_reporting;
    std::map<std::string, Hearing> m_hearsUs;
    std::map<std::string, Transmit> m_transmits;
    std::map<std::string, Info> m_info;
    std::map<std::string, bool> m_asked;
    std::map<std::string, std::map<std::string, Sighting>> m_sightings;   // (3.-289) sender -> receiver -> its square
  };

  // ---------------------------------------------------------------------------------------------- what is drawn
  // the step of the SNR a station gave us: 0 for -18 and below (and for a report without one), 1 to -10, 2 to -1, 3
  // from 0 up
  inline int snrStep (bool hasSnr, int snr) {return !hasSnr ? 0 : snr >= 0 ? 3 : snr >= -9 ? 2 : snr >= -17 ? 1 : 0;}

  typedef world_map::Box Box;   // (3.-261) the one box of worldmap.h

  // Which labels of a crowd are drawn. `boxes` are the labels the stations of the map would have, in the order the
  // stations are drawn (a box of no width: no label wanted). They are given out from the LAST to the first - both
  // ways before the others, the newest before the older - and one that would meet a label already given, or reach
  // out of the map, is left out: its mark stays, and a zoom brings the label.
  inline std::vector<bool> placeLabels (std::vector<Box> const& boxes, double mapWidth, double mapHeight)
  {
    return world_map::placeBoxes (boxes, true, 1.0, true, mapWidth, mapHeight);   // from the last, a pixel free round each, inside the map
  }

  double const pickRadius {8.0};     // how near the mouse must be to a mark
  double const samePlace {4.0};      // marks this near the one picked are under the mouse with it

  // The stations under the mouse: indices into the map's stations, whose marks stand at `xs`, `ys`. The nearest mark
  // within 8 px - of a station that hears us before one we hear, before one that only reports - and with it the
  // others of its kind within 4 px of it (two receivers in one locator), the latest drawn first. None when no mark is
  // near.
  inline std::vector<std::size_t> pick (std::vector<Station> const& stations, std::vector<double> const& xs, std::vector<double> const& ys, double mx, double my)
  {
    std::size_t best {stations.size ()};
    double bestD {0};
    for (std::size_t i = 0; i < stations.size () && i < xs.size () && i < ys.size (); ++i)
      {
        double const d {(xs[i] - mx) * (xs[i] - mx) + (ys[i] - my) * (ys[i] - my)};
        if (d > pickRadius * pickRadius) continue;
        if (best == stations.size () || stations[i].kind > stations[best].kind || (stations[i].kind == stations[best].kind && d <= bestD))
          {
            best = i; bestD = d;
          }
      }
    std::vector<std::size_t> under;
    if (best == stations.size ()) return under;
    for (std::size_t i = stations.size (); i-- > 0;)
      {
        if (i >= xs.size () || i >= ys.size () || stations[i].kind != stations[best].kind) continue;
        double const d {(xs[i] - xs[best]) * (xs[i] - xs[best]) + (ys[i] - ys[best]) * (ys[i] - ys[best])};
        if (i == best || d <= samePlace * samePlace) under.push_back (i);
      }
    return under;
  }
}

#endif
