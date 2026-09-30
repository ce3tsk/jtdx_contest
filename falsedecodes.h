// -*- Mode: C++ -*-
/* CE3TSK 2026-09-30: decodes that are likely false - marked, never rejected.

   Each detector is a row of Settings > Filters > False decodes, with two settings: mark it, and do
   not answer it automatically (the second greyed while the first is off). All four are on by
   default, so any of them can be switched off if it misbehaves.

     grid   the grid in the message does not lie in the DXCC entity of the call that sent it. The
            entity is the one the country column shows - cty.dat through the program's own lookup,
            so DL/CE3TSK and CE3TSK/DL are both judged as Germany, and /MM, /AM or an unknown prefix
            are never judged at all - and geodata::gridFitsEntity decides, with its tolerances: the
            eight neighbouring squares and the entity's administration count as inside.
     rover  a call signing /R below 30 MHz. /R is one bit per call in a standard FT8/FT4 message,
            so a false decode sets it by chance, while a real rover is a VHF station - on HF this
            station has one confirmed /R QSO in its whole log.

   A marked decode is shown with a red wave underline under what is in doubt (the grid, the /R call)
   and its country as ?Chile? (or ?CE?), carries the tag "lc:grid CE" / "lc:rover" in ALL.TXT and
   the low-confidence bit in the UDP Decode message, rings no bell and raises no window, and - with
   the second setting of its row - is not picked by the auto-sequencer on its own. A double click
   still works it.

   One exception to "not picked": a call heard with the SAME doubtful grid in two different periods
   is most likely a station operating away from home without a location prefix (DP0POL in GC45,
   RI0SP in BR77) - false decodes do not repeat. It stays marked, and may be answered. A hint or
   a-priori decode is not a second hearing: the a-priori decoder is fed with the callsigns already
   heard, so it can reproduce the very call it was given - and neither is any decode of the TX
   background, where an a-priori decode is printed '|' like the others. /R has no such exception.

   Every check reads the line's messageField (), never DecodedText::message (), which is shortened.

   Qt Core only, and no cty.dat of its own: the country lookup is handed in, so the harness
   (test_falsedecodes) can drive all of it. */

#ifndef FALSEDECODES_H
#define FALSEDECODES_H

#include <QChar>
#include <QHash>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include "geodata/geodata.h"

namespace false_decodes
{
  enum Reason : unsigned {None = 0u, Grid = 1u, Rover = 2u};

  struct Settings
  {
    bool gridMark {true};
    bool gridNoAnswer {true};    // read only together with gridMark
    bool roverMark {true};
    bool roverNoAnswer {true};   // read only together with roverMark
  };

  struct Verdict
  {
    unsigned reasons {None};
    QString sender;          // the call the message is from, as printed ("CE3TSK", "<DL/CE3TSK>")
    QString grid;            // with Grid: the grid in doubt
    QString entity;          // with Grid: the cty.dat prefix it was judged against ("CE", "VP8/O")
    QStringList rovers;      // with Rover: the /R calls, as printed
    bool gridFits {false};   // the grid was judged and lies in the entity
    bool repeated {false};   // the same doubtful grid from this call in an earlier period
    bool noAnswer {false};   // the auto-sequencer must not pick the sender on its own
    bool reliable {true};    // an ordinary decode: only such a one may release a station held back
    bool marked () const {return None != reasons;}
    // Whether it goes out as doubtful: the UDP clients' low-confidence bit, and no PSK Reporter spot. After
    // the second hearing a grid-only mark is trusted there - the station is answered, and a spot at the grid
    // it sends is most likely right (the operator, 2026-09-30, C). The underline and the ALL.TXT tag stay, as
    // the record of the conflict; /R is never trusted.
    bool lowConfidence () const {return marked () && !(repeated && !(reasons & Rover));}
  };

  // a call as printed, without the brackets of a resolved hash; "" for an unresolved one (<...>)
  inline QString bareCall (QString call)
  {
    if (call.startsWith ('<') && call.endsWith ('>')) call = call.mid (1, call.size () - 2);
    return "..." == call ? QString {} : call;
  }

  // a grid worth judging: a 4-character square, never RR73 (it shares the field) and never the
  // null AA00 that some software sends when no locator is set
  inline bool judgeableGrid (QString const& g)
  {
    if (4 != g.size () || "RR73" == g || "AA00" == g) return false;
    return g[0] >= 'A' && g[0] <= 'R' && g[1] >= 'A' && g[1] <= 'R'
        && g[2] >= '0' && g[2] <= '9' && g[3] >= '0' && g[3] <= '9';
  }

  // a call signing /R: "K1ABC/R", "<K1ABC/R>". What stands before the /R must hold a digit and a
  // letter, so free text such as "TNX/R" is not read as a call.
  inline bool isRover (QString const& printed)
  {
    QString const c = bareCall (printed);
    if (!c.endsWith ("/R") || c.size () < 5) return false;
    bool digit = false, letter = false;
    for (QChar const ch : c.left (c.size () - 2))
      {
        if (ch >= '0' && ch <= '9') digit = true;
        else if (ch >= 'A' && ch <= 'Z') letter = true;
        else if ('/' != ch) return false;
      }
    return digit && letter;
  }

  inline bool isHf (double dialHz) {return dialHz > 0. && dialHz < 30.e6;}

  // A sender whose country cty.dat does not know ("where?"): not spotted to PSK Reporter, whatever the
  // message (the operator, 2026-09-30, A3) - an unallocated prefix is a false decode. /MM and /AM have no
  // country by design and are not counted: a maritime or aeronautical mobile station is real, and its spot
  // says where it is. An unresolved hash names nobody and is not counted either.
  template<typename EntityOf>
  bool unknownCountry (QString const& printed, EntityOf entityOf)
  {
    QString const call = bareCall (printed);
    if (call.isEmpty ()) return false;
    for (auto const& part : call.split ('/').mid (1)) if ("MM" == part || "AM" == part) return false;
    return "?" == entityOf (call).section (',', 1, 1).trimmed ();
  }

  /* What the checks read from a standard message: the sender, the grid it sent, every call.
       CQ [DX|NA|POTA|123] CALL [GRID]     the sender is CALL
       CALL1 CALL2 [R] [GRID]              the sender is CALL2
     The grid is taken only as the LAST word, so free text never offers one by accident. The
     decoder's marker, printed after the message, is a one-character last word: dropped. */
  struct Parts
  {
    QString sender;
    QString grid;
    QStringList calls;
  };
  inline Parts parts (QString const& message)
  {
    Parts p;
    QString const m = message.simplified ();   // single spaces only, so split () yields no empty word
    if (m.isEmpty ()) return p;
    QStringList w = m.split (' ');
    if (w.size () > 1 && 1 == w.last ().size ()) w.removeLast ();
    if (w.size () < 2) return p;
    int s = 1;
    if ("CQ" == w[0] || "DE" == w[0] || "QRZ" == w[0])
      {
        static QRegularExpression const modifier {"^([A-Z]{1,4}|[0-9]{3})$"};
        if (w.size () > 2 && modifier.match (w[1]).hasMatch ()) s = 2;
      }
    else p.calls << w[0];
    p.sender = w[s];
    p.calls << w[s];
    int g = s + 1;
    if (g < w.size () && "R" == w[g]) ++g;
    if (g == w.size () - 1) p.grid = w[g];
    return p;
  }

  // The ALL.TXT tag. Lower case on purpose: an FT8/FT4 message can hold only upper-case letters,
  // digits, space and + - . / ?, so the tag can never be mistaken for message text, and a reader
  // drops it with one pattern: "\s+lc:\S+( \S+)?$".
  inline QString tag (Verdict const& v)
  {
    QStringList r;
    if (v.reasons & Grid) r << "grid";
    if (v.reasons & Rover) r << "rover";
    if (r.isEmpty ()) return {};
    QString t {"lc:" + r.join (',')};
    if (v.reasons & Grid) t += ' ' + v.entity;
    return t;
  }

  // The decoder's marker column in a decode line, by DecodedText's own rule: 47, two more when the
  // time carries seconds ("hhmmss", FT8/FT4/FT2) rather than "hhmm" (JT65, JT9)
  inline int markerColumn (QString const& line) {return 47 + (line.indexOf (' ') > 4 ? 2 : 0);}

  // The message field of a decode line, as the decoder printed it: the 26 characters before the marker
  // column, padding dropped - brackets kept, nothing cut. What the checks read. DecodedText::message () is NOT
  // this: it drops the < > of a hashed call, keeps 24 characters, and cuts a long CQ after position 16,
  // which loses its grid ("CQ POTA XQ3SKX/P JO62" arrives there as "CQ POTA XQ3SKX/P ").
  inline QString messageField (QString const& line)
  {
    int const column = markerColumn (line);
    return line.mid (column - 26, 26).trimmed ();
  }

  // The decode line with its tag, two spaces after the marker column: the line arrives trimmed, so
  // it is padded first, and the tag starts in the same column whether a marker was printed or not.
  inline QString tagged (QString line, Verdict const& v)
  {
    QString const t = tag (v);
    if (t.isEmpty ()) return line;
    int const column = markerColumn (line);
    if (line.size () < column + 1) line = line.leftJustified (column + 1, ' ');
    return line + "  " + t;
  }

  class Judge
  {
  public:
    /* The mark alone. No memory is read or written, so it can be asked again at any time (the UDP
       replay of the window and a double click do). `message` is the line's messageField (); entityOf is
       the country lookup of the windows: cty.dat's "continent,prefix,name,cqz,ituz" for a call, with "?"
       as the prefix when it is unknown. */
    template<typename EntityOf>
    static Verdict mark (QString const& message, double dialHz, Settings const& s, EntityOf entityOf)
    {
      Verdict v;
      Parts const p = parts (message);
      v.sender = p.sender;
      QString const call = bareCall (p.sender);
      if (s.gridMark && judgeableGrid (p.grid) && !call.isEmpty ())
        {
          QString const prefix = entityOf (call).section (',', 1, 1).trimmed ();
          if (!prefix.isEmpty () && "?" != prefix)
            {
              if (geodata::gridFitsEntity (prefix.toLatin1 ().constData (), p.grid.toLatin1 ().constData ()))
                v.gridFits = true;
              else
                {
                  v.reasons |= Grid;
                  v.grid = p.grid;
                  v.entity = prefix;
                }
            }
        }
      if (s.roverMark && isHf (dialHz))
        {
          for (auto const& c : p.calls) if (isRover (c)) v.rovers << c;
          if (!v.rovers.isEmpty ()) v.reasons |= Rover;
        }
      v.noAnswer = ((v.reasons & Grid) && s.gridNoAnswer) || ((v.reasons & Rover) && s.roverNoAnswer);
      return v;
    }

    /* The mark, with the answer to "may the sequencer pick the sender?" - which remembers what it
       has seen. `period` is the decode's period (its time of day in seconds); `unreliable` says the
       decode is no evidence of a second hearing: a hint or a-priori decode, or any decode of the TX
       background - where the decoders print '|' over an a-priori decode's '*', so the two cannot be
       told apart there. */
    template<typename EntityOf>
    Verdict judge (QString const& message, double dialHz, unsigned period, bool unreliable, Settings const& s, EntityOf entityOf)
    {
      Verdict v = mark (message, dialHz, s, entityOf);
      v.reliable = !unreliable;   // a hint, a-priori or background decode releases no one (Verdict::reliable)
      QString const key = bareCall (v.sender);
      if (key.isEmpty ()) return v;
      if (v.gridFits)            // his grid fits now: whatever was remembered about him is moot
        {
          if (!unreliable) seen_.remove (key);
          return v;
        }
      if (!(v.reasons & Grid)) return v;
      if (!unreliable)
        {
          auto it = seen_.find (key);
          if (seen_.end () == it || it->grid != v.grid)
            {
              if (seen_.size () >= 4096) seen_.clear ();   // a bound - real traffic never nears it
              Seen const first {v.grid, period, false};
              seen_.insert (key, first);
            }
          else if (it->period != period) it->repeated = true;
        }
      remembered (v, s);
      return v;
    }

    /* The mark with what the memory says, and nothing written: a decode looked up again (the UDP replay of the
       window, a double click) is no hearing, but a station already heard twice is reported as it was live. */
    template<typename EntityOf>
    Verdict recall (QString const& message, double dialHz, Settings const& s, EntityOf entityOf) const
    {
      Verdict v = mark (message, dialHz, s, entityOf);
      v.reliable = false;         // a line read again is no new message: it holds, and releases, no one
      if (v.reasons & Grid) remembered (v, s);
      return v;
    }

  private:
    // a doubtful grid this call has sent in two different periods: marked still, but it may be answered
    void remembered (Verdict& v, Settings const& s) const
    {
      auto const it = seen_.constFind (bareCall (v.sender));
      if (seen_.cend () != it && it->grid == v.grid && it->repeated)
        {
          v.repeated = true;
          v.noAnswer = (v.reasons & Rover) && s.roverNoAnswer;
        }
    }

    struct Seen
    {
      QString grid;
      unsigned period;
      bool repeated;
    };
    QHash<QString, Seen> seen_;
  };
}

#endif
