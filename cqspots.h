#ifndef CQSPOTS_H
#define CQSPOTS_H

// CQ calls on the waterfall.
//
// The callsigns of the stations calling CQ are written at the bottom of the waterfall, over the audio
// frequency each of them was decoded on, and stay there for a time the operator sets (Wide Graph > "CQ calls",
// in seconds; 0 switches it off). A glance at the waterfall then tells who is where - a call is no longer to be
// looked for in the Band Activity window by its frequency column.
//
// The rules are here, once, in plain C++ without Qt, so they can be run without a window; CPlotter draws what
// they decide:
//   - one entry per callsign: a station that calls CQ again - on another frequency, say - moves its entry
//     and starts its time afresh
//   - an entry lives for the set time, and fades out over the last 30 % of it
//   - the labels of stations whose frequencies lie close together would print over each other (an FT8 signal
//     is 50 Hz wide, a call is wider than that on any screen), so they are stacked in rows above the waterfall's
//     bottom edge; the newest call has the first choice of row, and a call for which no row is free is not
//     written (its frequency bracket is still drawn)

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace cq_spots
{
  int const max_rows {4};         // label rows at most, above the bracket line
  std::size_t const max_entries {256};   // a busy band holds fewer in any period; this only bounds the memory

  struct Spot
  {
    std::string call;
    int freq;            // audio frequency in Hz, as in the decode line
    long long when;      // milliseconds on a monotonic clock
  };

  struct Item            // an entry as it is drawn now
  {
    std::string call;
    int freq;
    long long age;       // milliseconds since it was heard
  };

  class Board
  {
  public:
    int duration_s () const {return duration_s_;}

    // seconds an entry stays; 0 switches the board off and empties it
    void set_duration_s (int seconds)
    {
      duration_s_ = seconds < 0 ? 0 : seconds;
      if (!duration_s_) spots_.clear ();
    }

    bool empty () const {return spots_.empty ();}
    void clear () {spots_.clear ();}

    // the callsign was heard calling CQ on freq Hz at now_ms
    void add (std::string const& call, int freq, long long now_ms)
    {
      if (!duration_s_ || call.empty () || freq <= 0) return;
      auto const same = std::find_if (spots_.begin (), spots_.end ()
                                      , [&call] (Spot const& s) {return s.call == call;});
      if (same != spots_.end ()) spots_.erase (same);   // spots_ stays in the order the calls were heard
      if (spots_.size () >= max_entries) spots_.erase (spots_.begin ());
      spots_.push_back (Spot {call, freq, now_ms});
    }

    // drops what has run out; true if anything was dropped
    bool purge (long long now_ms)
    {
      auto const keep_end = std::remove_if (spots_.begin (), spots_.end ()
                                            , [this, now_ms] (Spot const& s) {return expired (s, now_ms);});
      bool const dropped {keep_end != spots_.end ()};
      spots_.erase (keep_end, spots_.end ());
      return dropped;
    }

    // what is on the board at now_ms, the newest first
    std::vector<Item> active (long long now_ms) const
    {
      std::vector<Item> items;
      for (auto s = spots_.rbegin (); s != spots_.rend (); ++s)
        {
          if (expired (*s, now_ms)) continue;
          items.push_back (Item {s->call, s->freq, std::max (0LL, now_ms - s->when)});
        }
      return items;
    }

  private:
    bool expired (Spot const& s, long long now_ms) const
    {
      return now_ms - s.when >= static_cast<long long> (duration_s_) * 1000;
    }

    int duration_s_ {0};
    std::vector<Spot> spots_;
  };

  // opacity, 0..255, of an entry of the given age: full for the first 70 % of its time, then down to about a
  // third of it by the time it goes
  inline int fade (long long age_ms, long long duration_ms)
  {
    if (duration_ms <= 0) return 0;
    double const f {static_cast<double> (age_ms) / static_cast<double> (duration_ms)};
    if (f <= 0.7) return 255;
    if (f >= 1.0) return 80;
    return 255 - static_cast<int> ((f - 0.7) / 0.3 * 175.0 + 0.5);
  }

  struct Box             // a label to be placed: where its signal is, and how wide the label is, in pixels
  {
    int centre;
    int width;
  };

  struct Slot            // where it went: row 0 is the lowest; row -1 means no row had room
  {
    int row;
    int left;
  };

  // Places the labels in the order given, each in the lowest of the rows (0 .. rows-1) where it keeps gap
  // pixels clear of the labels already there. A label is centred on its signal, and pushed inwards where
  // that would take it past the edges of the view (0 .. view_w).
  inline std::vector<Slot> stack (std::vector<Box> const& boxes, int view_w, int rows, int gap)
  {
    std::vector<Slot> slots (boxes.size (), Slot {-1, 0});
    std::vector<std::vector<std::pair<int, int>>> used (rows > 0 ? rows : 0);   // [left, right) of each placed label
    for (std::size_t i = 0; i < boxes.size (); ++i)
      {
        int left {boxes[i].centre - boxes[i].width / 2};
        left = std::max (0, std::min (left, view_w - boxes[i].width));
        int const right {left + boxes[i].width};
        slots[i].left = left;
        for (std::size_t r = 0; r < used.size (); ++r)
          {
            bool free_here {true};
            for (auto const& u : used[r])
              {
                if (left < u.second + gap && u.first < right + gap) {free_here = false; break;}
              }
            if (free_here)
              {
                used[r].push_back (std::make_pair (left, right));
                slots[i].row = static_cast<int> (r);
                break;
              }
          }
      }
    return slots;
  }
}

#endif // CQSPOTS_H
