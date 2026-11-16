#ifndef TRACESPOTS_H
#define TRACESPOTS_H

// CE3TSK 2026-11-15 (TODO.md 3.-259; the operator: "show the decoded call signs in the waterfall itself ... putting the
// mouse atop a trace and it shows in the tooltip which signals are there", each line "the senders call sign and the SNR,
// e.g. K1ABC -12", the call signs of the Band Activity window).
//
// The waterfall moves a row down at each spectrum (0.288 s in FT8 and FT4, 0.144 s in FT2, times the Avg setting). The
// rows of one period hold that period's traces, and its decodes - which come only after its end, when its rows have
// moved on down - name them. The rules are here, once, in plain C++ without Qt, so they can be run without a window;
// CPlotter feeds them its rows and the decoded lines and asks them what lies under the mouse:
//   - each row drawn is counted with the scale it was drawn with (the frequency at its left edge, the Hz of a pixel): a
//     change of Start or of bins/pixel leaves the rows drawn before as they were, so a row keeps its own scale
//   - live, a row of a period other than the newest one's - by the clock's period start, the second of the UTC day its
//     decoded lines carry (hhmmss) - begins a new period: at a period's start, and after any time without rows (Monitor
//     off, then on again in the middle of a period). A recording's first row begins its own period
//   - each period is of an air (cqspots.h's Origin: dial, mode, live or a recording). The air changing within a period -
//     a QSY, band hopping's at the period's start (its dial reported a moment after the period began), another mode,
//     Monitor on after a recording - begins a new part of it, of the same time and the new air: no row is of two airs
//     (TODO.md 3.-259's review, 2026-11-16: it had been "the first second's change taken, a later one leaves the period
//     unnamed", and with Avg 4 or more no row fell in the first second)
//   - a live decoded line goes to the newest part of the period of its time (a second either way) and its air; a
//     recording's line to the newest part of the recording's air, whatever its time - a recording's lines are decoded
//     before the next one is read, and the JT modes stamp them with the clock, not the file's time. A line decoded again
//     (the same call within half a signal's width) replaces the first; a Fox's streams, 60 Hz apart, stay apart
//   - at a row and a column: the part of the row, and those of its signals whose trace - from the decoded frequency up
//     one signal's width, a little either side - the column falls on, the lowest frequency first
//   - what has scrolled off the waterfall is let go

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <string>
#include <vector>
#include "cqspots.h"

namespace trace_spots
{
  std::size_t const max_signals {256};   // a period's decodes at most; a busy FT8 band gives about a hundred
  int const tolerance_px {2};            // the mouse may miss a trace by this much either side
  int const time_tolerance_s {1};        // a decoded line's time against its period's start
  int const day_s {86400};

  struct Scale           // a row's: the frequency at x = 0 and the Hz of a pixel
  {
    Scale () : left_hz {0.}, hz_per_px {1.} {}
    Scale (double l, double h) : left_hz {l}, hz_per_px {h} {}
    double left_hz;
    double hz_per_px;
  };
  inline bool same_scale (Scale const& a, Scale const& b) {return a.left_hz == b.left_hz && a.hz_per_px == b.hz_per_px;}

  struct Signal
  {
    std::string call;    // the sender
    int snr;
    int freq;            // audio frequency in Hz, as in the decoded line: the trace's lowest tone
  };

  // what lies at a row and a column: the signals, lowest frequency first (none: nothing there), and the columns of their
  // traces in the asked row's scale
  struct Hit
  {
    Hit () : left_px {0}, right_px {0} {}
    std::vector<Signal> found;
    int left_px;
    int right_px;
  };

  // "K1ABC -12": the sender and the SNR, signed
  inline std::string label (Signal const& s)
  {
    char snr[16];
    std::snprintf (snr, sizeof snr, "%+d", s.snr);
    return s.call + " " + snr;
  }

  namespace detail
  {
    inline bool has_letter_and_digit (std::string const& w)
    {
      bool letter {false}, digit {false};
      for (char c : w)
        {
          if (c >= 'A' && c <= 'Z') letter = true;
          if (c >= '0' && c <= '9') digit = true;
        }
      return letter && digit;
    }

    inline bool callsign_like (std::string const& c)
    {
      if (c.size () < 3 || c.size () > 13 || c == "RR73" || c.front () == '/' || c.back () == '/') return false;
      for (char ch : c) if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '/')) return false;
      if (c.size () == 4 && c[0] >= 'A' && c[0] <= 'R' && c[1] >= 'A' && c[1] <= 'R'
          && c[2] >= '0' && c[2] <= '9' && c[3] >= '0' && c[3] <= '9') return false;   // a grid
      return has_letter_and_digit (c);
    }

    inline std::vector<std::string> words (std::string const& s)
    {
      std::vector<std::string> w;
      std::string cur;
      for (char c : s)
        if (c == ' ' || c == '\t') {if (!cur.empty ()) w.push_back (cur); cur.clear ();}
        else cur += c;
      if (!cur.empty ()) w.push_back (cur);
      return w;
    }
  }

  // The sender of a decoded message: K1ABC of "CQ K1ABC FN42", "CQ DX K1ABC FN42", "CQ POTA K1ABC/P FN42",
  // "W9XYZ K1ABC -12", "<PJ4/W9XYZ> K1ABC R-12", "W9XYZ <PJ4/K1ABC> RR73", "TU; W9XYZ K1ABC R 579 MA" (the part after
  // a "TU;"), "W9XYZ RR73; K2DEF <K1ABC> -08" (a Fox's message for two: the part after the ';'), without its brackets;
  // "" where there is none - a hash not known ("<...>"), free text with no call second, a report or RR73 where the call
  // would be. Whatever follows the message on its line (a hint's '*') is left alone. The message must be WHOLE: the
  // decoded line's own (DecodedText::wholeMessage ()), not its message () - cut at 24 characters, a Fox's second call cut.
  inline std::string sender (std::string const& message)
  {
    std::vector<std::string> const w {detail::words (message)};
    std::size_t i {0};
    for (std::size_t k = 0; k < w.size (); ++k)
      if (w[k].back () == ';') i = k + 1;   // TU; or a Fox's first half
    if (i >= w.size ()) return {};
    std::string c;
    if (w[i] == "CQ" || w[i] == "QRZ" || w[i] == "DE")
      {
        // the word of a CQ - DX, NA, POTA, TEST, 290 - has no letter and digit both, as any call has
        std::size_t j {i + 1};
        while (j < w.size () && j < i + 3 && !detail::has_letter_and_digit (w[j])) ++j;
        if (j < w.size ()) c = w[j];
      }
    else if (i + 1 < w.size ()) c = w[i + 1];
    if (c.size () >= 2 && c.front () == '<' && c.back () == '>') c = c.substr (1, c.size () - 2);
    return detail::callsign_like (c) ? c : std::string {};
  }

  // seconds between two seconds of the day, across midnight too
  inline int apart_s (int a, int b)
  {
    int const d {std::abs (((a - b) % day_s + day_s) % day_s)};
    return std::min (d, day_s - d);
  }

  class Board
  {
  public:
    void clear () {rows_ = 0; scales_.clear (); periods_.clear ();}
    long long rows () const {return rows_;}
    std::size_t periods () const {return periods_.size ();}   // parts of periods, each of one air

    // a row drawn at the top, with its scale
    void row (Scale const& s)
    {
      ++rows_;
      if (scales_.empty () || !same_scale (scales_.back ().scale, s)) scales_.push_back (Scaled {rows_ - 1, s});
    }

    // the row just drawn begins a period, of time_s (the second of the UTC day) on air; one of its signals is width_hz wide
    void begin_period (int time_s, cq_spots::Origin const& air, double width_hz)
    {
      if (!rows_) return;
      periods_.push_back (Period {rows_ - 1, ((time_s % day_s) + day_s) % day_s, air, width_hz});
    }

    // the row just drawn is of air, its signals width_hz wide: another air than its period's begins a new part of it
    void air (cq_spots::Origin const& now, double width_hz)
    {
      if (periods_.empty () || cq_spots::shows_on (now, periods_.back ().air)) return;
      begin_period (periods_.back ().time_s, now, width_hz);
    }

    // the row just drawn is live audio, of the period that began at time_s by the clock: a new period if the newest
    // is not that one, else as air ()
    void live (int time_s, cq_spots::Origin const& now, double width_hz)
    {
      if (periods_.empty () || periods_.back ().time_s != ((time_s % day_s) + day_s) % day_s) begin_period (time_s, now, width_hz);
      else air (now, width_hz);
    }

    // a decoded line, decoded on air: its sender's call, SNR and frequency, and - live - its period's time_s. False if
    // no part of a period on the waterfall is its
    bool add (int time_s, cq_spots::Origin const& air, std::string const& call, int snr, int freq)
    {
      if (call.empty ()) return false;
      for (auto p = periods_.rbegin (); p != periods_.rend (); ++p)
        {
          if (!cq_spots::shows_on (air, p->air)) continue;
          if (!air.replay && apart_s (p->time_s, time_s) > time_tolerance_s) continue;
          for (auto& s : p->sigs)
            if (s.call == call && 2.0 * std::abs (s.freq - freq) <= p->width_hz)
              {
                s.snr = snr;
                s.freq = freq;
                return true;
              }
          if (p->sigs.size () >= max_signals) return false;
          p->sigs.push_back (Signal {call, snr, freq});
          return true;
        }
      return false;
    }

    // lets go of what lies wholly below the newest `visible` rows
    void keep (int visible)
    {
      long long const oldest {rows_ - visible};   // the oldest row still there
      while (periods_.size () > 1 && periods_[1].first <= oldest) periods_.pop_front ();
      while (scales_.size () > 1 && scales_[1].first <= oldest) scales_.pop_front ();
    }

    // what lies at column x of the row r rows below the newest (0: the newest, at the top)
    Hit at (int r, int x) const
    {
      Hit h;
      long long const row {rows_ - 1 - r};
      if (r < 0 || row < 0) return h;
      std::size_t pi {periods_.size ()};
      for (std::size_t k = periods_.size (); k-- > 0;) if (periods_[k].first <= row) {pi = k; break;}
      if (pi == periods_.size ()) return h;
      std::size_t si {scales_.size ()};
      for (std::size_t k = scales_.size (); k-- > 0;) if (scales_[k].first <= row) {si = k; break;}
      if (si == scales_.size () || scales_[si].scale.hz_per_px <= 0.) return h;
      Period const& p = periods_[pi];
      Scale const& sc = scales_[si].scale;
      double const hz {sc.left_hz + x * sc.hz_per_px};
      double const slack {tolerance_px * sc.hz_per_px};
      double lo {0.}, hi {0.};
      for (auto const& s : p.sigs)
        if (hz >= s.freq - slack && hz <= s.freq + p.width_hz + slack)
          {
            if (h.found.empty () || s.freq < lo) lo = s.freq;
            if (h.found.empty () || s.freq + p.width_hz > hi) hi = s.freq + p.width_hz;
            h.found.push_back (s);
          }
      if (h.found.empty ()) return h;
      std::stable_sort (h.found.begin (), h.found.end (), [] (Signal const& a, Signal const& b) {return a.freq < b.freq;});
      h.left_px = static_cast<int> ((lo - sc.left_hz) / sc.hz_per_px) - tolerance_px;
      h.right_px = static_cast<int> ((hi - sc.left_hz) / sc.hz_per_px + 0.999) + tolerance_px;
      return h;
    }

  private:
    struct Scaled {long long first; Scale scale;};
    struct Period
    {
      Period (long long f, int t, cq_spots::Origin const& a, double w) : first {f}, time_s {t}, air {a}, width_hz {w} {}
      long long first;   // its first row
      int time_s;
      cq_spots::Origin air;
      double width_hz;
      std::vector<Signal> sigs;
    };
    long long rows_ {0};
    std::deque<Scaled> scales_;
    std::deque<Period> periods_;
  };
}

#endif // TRACESPOTS_H
