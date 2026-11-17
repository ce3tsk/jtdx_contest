#ifndef FEEDCALLS_H
#define FEEDCALLS_H

// CE3TSK 2026-11-16 (TODO.md 3.-263, RECEPTION_MAP_PLAN.md 7f): the feed's compound calls, for the decoder.
//
// The operator: "is it possible to check the reports for compound call signs and add them to the local compound call
// sign data so that we get more hashes and reduce <...> place holders?" - and, to the plan put to him: "two receiver
// rule sounds good, 30 days is OK, and learning only while map is open is enough for now."
//
// A station with a nonstandard call - PJ4/K1ABC, F/DC6JN, K1ABC/7, TC31COP - sends its call in full only in some of
// its messages; in the others it is a 22-bit hash, and the decoder writes <...> until it has heard the call in full
// itself (and remembers it then: packjt77.f90 remember_*, compound_calls.txt). PSK Reporter's feed has those calls in
// full, from every receiver that heard them. So, while the Reception map is open and listening:
//
//   - a SENDER's call that is one a transmitter sends as a hash (hashedCall (): the decoder's own test, ported), and
//     that TWO DIFFERENT receivers have reported, is taken - one receiver's word is not: a phantom decode is a call
//     nobody else has (the recorded quarter of an hour had "7G2" and "UD6", each from one receiver);
//   - it is kept for 30 days from the day it was last reported;
//   - the calls are written to the data directory's compound_calls_feed.txt - a call and its day a line, as the
//     decoder's own list - which the decoder reads when it is new and asks LAST, after what it has heard itself.
//
// Nothing here is the decoder's own list: that stays what this station decoded in full. Deleting the file undoes
// all of it - while the program runs as well (the second review, TODO.md 3.-264): the window lets its list go with
// the file. No Qt: test_spotfeed drives it; the window (receptionmap.cpp) feeds it and writes the file.

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace feed_calls
{
  int const keptDays {30};                   // packjt77.f90 NFEEDAGE
  std::size_t const maxCalls {5000};         // packjt77.f90 MAXFEED: the decoder takes the file's first so many
  std::size_t const maxWaiting {4000};       // calls one receiver has reported, waiting for a second
  char const * const fileName {"compound_calls_feed.txt"};

  inline std::string upper (std::string s)
  {
    for (auto& c : s) if (c >= 'a' && c <= 'z') c = static_cast<char> (c - 'a' + 'A');
    return s;
  }

  // Is this a call a transmitter sends as a 22-bit hash in a standard message: packjt77.f90's hashed_call (), step for
  // step - 3 to 11 capitals, digits and slashes that are NOT a standard call (one or two characters with a letter
  // among them, a digit, up to three letters; /P and /R are a flag of a standard call, not part of it). The decoder
  // asks its own function again of every line of the file.
  inline bool hashedCall (std::string const& call)
  {
    std::string cw {call};
    int n {static_cast<int> (cw.size ())};
    if (n < 3 || n > 11) return false;
    auto digit = [] (char c) {return c >= '0' && c <= '9';};
    auto letter = [] (char c) {return c >= 'A' && c <= 'Z';};
    for (char c : cw) if (!(digit (c) || letter (c) || '/' == c)) return false;
    if (cw.compare (static_cast<std::size_t> (n - 2), 2, "/P") == 0 || cw.compare (static_cast<std::size_t> (n - 2), 2, "/R") == 0)
      {
        cw.erase (static_cast<std::size_t> (n - 2));
        n -= 2;
      }
    int area {1};                            // the place of the last digit, counted from 1 as the decoder counts; 1 when there is none after the first
    for (int i = n; i >= 2; --i) if (digit (cw[static_cast<std::size_t> (i - 1)])) {area = i; break;}
    int prefixDigits {0}, prefixLetters {0}, suffixLetters {0};
    for (int i = 1; i <= area - 1; ++i)
      {
        if (digit (cw[static_cast<std::size_t> (i - 1)])) ++prefixDigits;
        if (letter (cw[static_cast<std::size_t> (i - 1)])) ++prefixLetters;
      }
    for (int i = area + 1; i <= n; ++i) if (letter (cw[static_cast<std::size_t> (i - 1)])) ++suffixLetters;
    return area < 2 || area > 3 || 0 == prefixLetters || prefixDigits >= area - 1 || suffixLetters > 3 || cw.find ('/') != std::string::npos;
  }

  // a day yyyymmdd that is one, and its number in the run of days (the decoder's remjday)
  inline bool goodDay (int ymd)
  {
    return ymd >= 19000101 && ymd <= 99991231 && ymd / 100 % 100 >= 1 && ymd / 100 % 100 <= 12 && ymd % 100 >= 1 && ymd % 100 <= 31;
  }
  inline long dayNumber (int ymd)
  {
    int const y {ymd / 10000}, m {ymd / 100 % 100}, d {ymd % 100};
    int const a {(14 - m) / 12}, yy {y + 4800 - a}, mm {m + 12 * a - 3};
    return d + (153L * mm + 2) / 5 + 365L * yy + yy / 4 - yy / 100 + yy / 400 - 32045;
  }

  class Collector
  {
  public:
    // A report of the feed: who was heard, by whom, on which day (yyyymmdd). True when the list is another for it -
    // a call taken, or a call of the list reported on a later day than it has.
    bool note (std::string const& sender, std::string const& receiver, int day)
    {
      std::string const call {upper (sender)}, by {upper (receiver)};
      if (!goodDay (day) || by.empty () || by == call || !hashedCall (call)) return false;
      auto const have = m_calls.find (call);
      if (have != m_calls.end ())
        {
          if (have->second >= day) return false;
          have->second = day;
          return true;
        }
      auto const waiting = m_waiting.find (call);
      if (waiting == m_waiting.end ())
        {
          if (m_waiting.size () >= maxWaiting) m_waiting.clear ();   // thousands of calls nobody confirms: begun anew
          m_waiting[call] = by;
          return false;
        }
      if (waiting->second == by) return false;   // the same receiver again is still one receiver
      m_waiting.erase (waiting);
      m_calls[call] = day;
      return true;
    }

    // what has not been reported for more than 30 days goes; true when something went
    bool expire (int today)
    {
      bool gone {false};
      for (auto it = m_calls.begin (); it != m_calls.end ();)
        if (dayNumber (today) - dayNumber (it->second) > keptDays) {it = m_calls.erase (it); gone = true;} else ++it;
      return gone;
    }

    // the file as it was written before: its calls into the list - a line is a call and its day; what is no such
    // line, no call that travels as a hash, or older than 30 days is passed over; of a call twice, the later day
    void load (std::string const& text, int today)
    {
      std::size_t at {0};
      while (at < text.size ())
        {
          std::size_t end {text.find ('\n', at)};
          if (end == std::string::npos) end = text.size ();
          std::string line {text.substr (at, end - at)};
          at = end + 1;
          while (!line.empty () && ('\r' == line.back () || ' ' == line.back ())) line.pop_back ();
          std::size_t const first {line.find_first_not_of (' ')};
          if (first == std::string::npos || '#' == line[first]) continue;
          line.erase (0, first);
          std::size_t const space {line.find (' ')};
          if (space == std::string::npos) continue;
          std::string const call {line.substr (0, space)};
          std::size_t const second {line.find_first_not_of (' ', space)};
          if (second == std::string::npos) continue;
          std::string const rest {line.substr (second)};
          if (rest.size () != 8 || rest.find_first_not_of ("0123456789") != std::string::npos) continue;
          int day {0};
          for (char c : rest) day = day * 10 + (c - '0');
          if (!goodDay (day) || !hashedCall (call) || dayNumber (today) - dayNumber (day) > keptDays) continue;
          auto const have = m_calls.find (call);
          if (have == m_calls.end () || have->second < day) m_calls[call] = day;
        }
    }

    // the file: two lines that say what it is, then the calls - the latest reported first, 5000 at most
    std::string text () const
    {
      std::vector<std::pair<std::string, int>> all (m_calls.begin (), m_calls.end ());
      std::sort (all.begin (), all.end (), [] (std::pair<std::string, int> const& a, std::pair<std::string, int> const& b) {
          return a.second != b.second ? a.second > b.second : a.first < b.first;
        });
      if (all.size () > maxCalls) all.resize (maxCalls);
      std::string out {"# JTDX_contest: nonstandard calls that two receivers or more reported to PSK Reporter while the\n"
                       "# Reception map was open, kept 30 days - a 22-bit hash the decoder has not heard in full itself is\n"
                       "# read from here, after its own list (compound_calls.txt). Call, day last reported. Deleting this\n"
                       "# file is safe.\n"};
      for (auto const& c : all)
        {
          char day[16];
          std::snprintf (day, sizeof day, "%08d", c.second);
          out += c.first + " " + day + "\n";
        }
      return out;
    }

    std::size_t size () const {return m_calls.size ();}
    std::size_t waiting () const {return m_waiting.size ();}
    bool has (std::string const& call) const {return m_calls.find (upper (call)) != m_calls.end ();}
    int day (std::string const& call) const
    {
      auto const it = m_calls.find (upper (call));
      return it == m_calls.end () ? 0 : it->second;
    }

  private:
    std::map<std::string, int> m_calls;             // call -> the day it was last reported
    std::map<std::string, std::string> m_waiting;   // call -> the one receiver that has reported it
  };
}

#endif
