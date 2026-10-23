#ifndef STATIONSYNC_H
#define STATIONSYNC_H

/* CE3TSK 2026-10-23: sync to one station (STATION_SYNC_PLAN.md, TODO.md 3.-186) - asked for by JP1LRT, whose JTDX
   2.2.160-rc10 has it; the operator: "to be able to dial in on a few DX stations that have bigger than normal time
   deviations, e.g. +2s, and in such cases these stations get only decoded sporadically".

   JTDX's internal clock (JTDXDateTime) is the system clock plus an offset, and everything that keeps time reads it: the
   receive window, our transmissions, the scheduler. The offset now has two parts: the average Sync's BASE, as before,
   and a STATION part. Ctrl + click on Sync takes the station in DX Call: the part that puts his newest line (decision
   7: from the last 2 minutes) at DT 0 - measured against the offset that line was received under, so a second click
   does not add his DT twice (decision 1 A). It is a one-time thing: it ends (decision 3) with a right click on Sync (the
   station part alone; the next right click resets the base, as before), another DX call, the QSO logged, another band
   or mode, and quitting (nothing is saved) - and the clock falls back to the base, 0 when the average Sync was never
   used ("it either falls back to 0 or the active average sync if enabled"). While it lasts the average Sync is refused
   (decision 4), and the Sync button and the band label are light blue (decisions 2, 5). FT8 only for now (decision 6).

   Every change of the clock is a JUMP, and step 1 of the plan measured it live (plan 5c): receiving loses nothing, but
   the scheduler raises the PTT whenever the clock is inside our transmit window (0 to 13.64 s of our period in FT8,
   tx_step::tx_window (), tx_step::assert_ptt ()) and drops it once the clock leaves it. A jump back while we transmit
   cuts the transmission and keys it again late; a jump forward past our transmission's start starts it late, its
   beginning missing; a jump back after it ended keys a second, truncated one. And the receive buffer restarts when the
   clock wraps into a new period (Detector::writeData ()): a jump forward past a period's end starts the next period's
   buffer late, every DT in it short by the overshoot (the station sync's own on-screen check met it: a +1.5 s jump at a
   decode's start put the next period's DTs 0.6 s short, and a sync asked for from such a line was 0.6 s wrong); a jump
   back in a period not yet decoded restarts its buffer midway, that period's decode lost or its DTs shifted. So no jump
   is made at once: it waits for a safe moment (decision 8) - a decode's start (the audio handed to the decoder),
   DecodeFinished, a request - and is made there only when it leaves no transmission cut, doubled or begun late
   (safe ()) and every period's audio whole (step_ms ()): forward only in a period already handed over, and only as far
   as its end, in steps one a period; back in such a period, or into the period before. The average Sync's own jump
   goes the same way (decision 9); the decoder is not told (decision 10).

   The decisions are here: what it is (Run), what happens (Event), and step (), which moves it and says what the window
   must do (Do). MainWindow gathers the events (stationSyncRequest (), stationSyncCheck (), stationSyncMoment (), ...)
   and carries Do out (stationSyncDo ()); test_stationsync holds them. */

#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <cmath>
#include "Radio.hpp"
#include "decodebudget.h"

namespace station_sync
{
  // decision 6: FT8 first (the button is FT8's)
  inline bool mode_has_it (QString const& mode) { return mode == "FT8"; }

  // decision 7: his newest line, if it is from the last 2 minutes
  qint64 const recent_ms = 120000;

  // the clock offset's two parts, in seconds; JTDXDateTime uses their sum
  struct Offsets
  {
    double base = 0.0;      // the average Sync's
    double station = 0.0;   // a station sync's
  };
  inline double total (Offsets const& o) { return o.base + o.station; }
  inline bool same (Offsets const& a, Offsets const& b)
  {
    return qAbs (a.base - b.base) < 0.0005 && qAbs (a.station - b.station) < 0.0005;
  }
  // a decoded line of a station: its DT, the offset (the sum, s) its period was received under, and when it came (the
  // system clock, ms)
  struct Heard
  {
    double dt = 0.0;
    double offset = 0.0;
    qint64 ms = 0;
    bool valid = false;
  };

  // the newest line of each station heard, by base call - kept for recent_ms
  class Book
  {
  public:
    void hear (QString const& call, double dt, double offset, qint64 ms)
    {
      QString const c = Radio::base_callsign (call);
      if (c.isEmpty ()) return;
      Heard h; h.dt = dt; h.offset = offset; h.ms = ms; h.valid = true;
      book_[c] = h;
      if (book_.size () > 4000) prune (ms);
    }
    Heard newest (QString const& call, qint64 nowMs) const
    {
      auto const i = book_.constFind (Radio::base_callsign (call));
      if (i == book_.constEnd () || nowMs - i->ms > recent_ms) return {};
      return *i;
    }
    void prune (qint64 nowMs)
    {
      for (auto i = book_.begin (); i != book_.end (); )
        if (nowMs - i->ms > recent_ms) i = book_.erase (i); else ++i;
    }
    int size () const { return book_.size (); }
  private:
    QHash<QString, Heard> book_;
  };

  /* (the review 2026-10-23, 3) the clock offset each recent period was received under, by the period's index in the
     day (decodebudget.h): a decode's lines are measured against their own period's - the background's last lines of a
     period come after the next decode began, and a jump made at its start */
  class PeriodOffsets
  {
  public:
    void set (int period, double offset)
    {
      for (int i = 0; i < offsets_.size (); ++i) if (offsets_[i].first == period) { offsets_.removeAt (i); break; }
      offsets_.append (qMakePair (period, offset));
      if (offsets_.size () > 8) offsets_.removeFirst ();   // the oldest - an index starts again at 0 after midnight
    }
    double of (int period, double fallback) const
    {
      for (int i = offsets_.size () - 1; i >= 0; --i) if (offsets_[i].first == period) return offsets_[i].second;
      return fallback;
    }
  private:
    QList<QPair<int, double>> offsets_;
  };

  /* (the review 2026-10-23, 2) the period - its index in the day - the clock is in, and the one a decode starting now
     hands over: a decode is due at 14.1 s of its period (FT8), and one started late, past the period's end (the window
     busy), still hands over the period it was due for - the one decode () names it by, period_index_of_trigger () */
  inline double seconds_of_day (qint64 clockMs) { return (((clockMs % 86400000) + 86400000) % 86400000) / 1000.0; }
  inline int period_now (qint64 clockMs, double period) { return period_index (seconds_of_day (clockMs), period); }
  inline int handed_period (qint64 clockMs, double period) { return period_index_of_trigger (seconds_of_day (clockMs), period); }

  /* (the review 2026-10-23, 4) whether a decode's start and DecodeFinished are coming: not with Monitor off, nor in
     WSPR (its decoder is not jt9); then no period's audio waits for a decode, and any second is a moment */
  inline bool decodes_come (bool monitoring, QString const& mode) { return monitoring && !mode.startsWith ("WSPR"); }

  struct Run
  {
    QString call;        // the station synced to, his base call; empty: none
    QString band;        // what it was started on: another band or mode ends it
    QString mode;
    Offsets now;         // in force
    bool pending = false;
    Offsets next;        // to be made at the next safe moment
  };
  inline bool on (Run const& r) { return !r.call.isEmpty (); }
  // the parts once the pending change is made
  inline Offsets target (Run const& r) { return r.pending ? r.next : r.now; }

  // where the clock stands against our transmissions, at a moment a jump may be made
  struct Moment
  {
    bool handedOver = false;     // this period's audio is the decoder's already (its decode has started)
    qint64 msIntoPeriod = 0;     // the clock's ms into its period
    bool transmitting = false;   // keyed, or the modulator playing
    bool txEnabled = false;      // Enable Tx (or Tune): the scheduler keys whenever the clock is in our window
    qint64 fromTxStartMs = 0;    // the clock's ms from the start of our transmit period - the nearest one, before
                                 // (negative) or since it began, within a period
    qint64 periodMs = 15000;
    qint64 windowMs = 13640;     // our transmit window from the period's start (tx_step::tx_window (): tx2)
    qint64 startMs = 500;        // our transmission's first symbol within the period (txstart.h)
    bool receiving = true;       // decodes come (decodes_come ()): the receive buffer's rule holds
  };
  qint64 const margin_ms = 100;

  /* a jump of jumpMs (the clock moves by it, + forward) leaves no transmission of ours cut, keyed twice or begun late:
     never while we transmit; with Enable Tx off, always; with it on, only if the clock lands before our transmission's
     start (less a margin) or past our window - where it lands counted from the start of our transmit period, in
     (-period, +period] around it */
  inline bool safe (Moment const& m, qint64 jumpMs)
  {
    if (m.transmitting) return false;
    if (!m.txEnabled) return true;
    qint64 const pair = 2 * m.periodMs;
    qint64 at = m.fromTxStartMs + jumpMs;
    while (at < -m.periodMs) at += pair;
    while (at >= m.periodMs) at -= pair;
    return at < m.startMs - margin_ms || at >= m.windowMs;
  }

  /* the part of a pending jump that may be made now, 0 for none - with no decode to come, all of it when safe () (no
     period's audio waits for a decode); otherwise the receive buffer (Detector::writeData ()) restarts whenever the
     clock's position in its period goes down, a wrap into the next period or a jump back:
     - forward: only in a period whose audio is the decoder's already, and only as far as its end less the margin - past
       it the next period's buffer would begin late, its DTs short by the overshoot; in a period not yet decoded it would
       cut that period short of its decode. A longer jump forward goes in steps, one a period;
     - back: in a period handed over already (its buffer restarts, then again at the wrap - nothing lost), or into the
       period before (the clock goes up within it, no restart until the wrap); within a period not yet decoded it would
       restart that period's buffer midway, its decode lost or its DTs shifted;
     and in either case only when safe () for our transmissions */
  inline qint64 step_ms (Moment const& m, qint64 jumpMs)
  {
    if (!m.receiving) return safe (m, jumpMs) ? jumpMs : 0;
    if (jumpMs > 0)
      {
        if (!m.handedOver) return 0;
        jumpMs = qMin (jumpMs, m.periodMs - m.msIntoPeriod - margin_ms);
        if (jumpMs <= 0) return 0;
      }
    else if (!m.handedOver && m.msIntoPeriod + jumpMs >= 0) return 0;
    return safe (m, jumpMs) ? jumpMs : 0;
  }

  // the ms the clock moves when `next` is put in force over `now`
  inline qint64 jump_ms (Offsets const& now, Offsets const& next)
  {
    return static_cast<qint64> (std::llround ((total (next) - total (now)) * 1000.0));
  }

  enum class Kind {Request, AverageArmed, AverageResult, RightClick, Check, Logged, Moment};
  enum class Why {RightClick, DxCall, Logged, Band, Mode};
  // what the status bar says (MainWindow has the words, translated)
  enum class Say {Nothing, Started, NoCall, NoDecode, NotHere, AverageRefused, Ended};

  struct Event
  {
    explicit Event (Kind k) : kind {k} {}   // the tree is C++11: no brace initialisation with member defaults
    Kind kind;
    QString hisCall;     // Request, Check: DX Call; Logged: the call logged
    QString band;        // Request, Check
    QString mode;        // Request, Check
    Heard heard;         // Request: his newest line (Book::newest ())
    double avexdt = 0.0; // AverageResult: the band's average DT - the base moves by -avexdt
    Moment moment;       // Moment
  };

  struct Do
  {
    bool apply = false;          // put `offsets` in force (JTDXDateTime::SetOffsets ())
    Offsets offsets;
    qint64 moved = 0;            // the ms the clock moves by it (a decode's lag leaves them out)
    bool uncheck = false;        // AverageArmed refused: the Sync button unchecked again
    bool grey = false;           // a right click reset the base: the button greyed until the next decode, as before
    bool repaint = false;        // the look changed: the button, the band label, the tooltip
    Say say = Say::Nothing;
    Why why = Why::RightClick;   // Say::Ended
    QString call;                // Say::Started, NoDecode, Ended
    double station = 0.0;        // Say::Started
    QStringList debug;           // lines for ALL.TXT (debug lines on)
  };

  inline QString why_text (Why w)   // the debug line's (English, as the other debug lines)
  {
    switch (w)
      {
      case Why::RightClick: return "right click";
      case Why::DxCall: return "another DX call";
      case Why::Logged: return "QSO logged";
      case Why::Band: return "another band";
      case Why::Mode: return "another mode";
      }
    return {};
  }

  inline QString secs (double s) { return QString::asprintf ("%+.2f", s); }

  namespace detail
  {
    // ended: the station part goes, the base stays - the jump waits for a safe moment like any other
    inline Do end (Run& r, Why why)
    {
      Do d;
      Offsets const t = target (r);
      d.say = Say::Ended; d.why = why; d.call = r.call;
      d.debug << QString {"Station sync to %1 ended: %2"}.arg (r.call, why_text (why));
      r.call.clear (); r.band.clear (); r.mode.clear ();
      r.next = t; r.next.station = 0.0;
      r.pending = !same (r.next, r.now);
      d.repaint = true;
      return d;
    }
  }

  inline Do step (Run& r, Event const& e)
  {
    Do d;
    switch (e.kind)
      {
      case Kind::Request:   // Ctrl + click on Sync
        {
          if (!mode_has_it (e.mode)) { d.say = Say::NotHere; break; }
          if (e.hisCall.isEmpty ()) { d.say = Say::NoCall; break; }
          if (!e.heard.valid) { d.say = Say::NoDecode; d.call = e.hisCall; break; }
          Offsets const t = target (r);
          // his line was at DT dt under the offset `offset`: under offset - dt it is at DT 0; the base stays
          double const station = e.heard.offset - e.heard.dt - t.base;   // JTDXDateTime rounds the sum to the ms
          r.call = Radio::base_callsign (e.hisCall); r.band = e.band; r.mode = e.mode;
          r.next = t; r.next.station = station;
          r.pending = !same (r.next, r.now);
          d.say = Say::Started; d.call = r.call; d.station = station; d.repaint = true;
          d.uncheck = true;   // an average Sync armed before is not wanted now (decision 4)
          d.debug << QString {"Station sync to %1: his line at DT %2 s under the offset %3 s - the station part %4 s, the base %5 s"}
                       .arg (r.call, secs (e.heard.dt), secs (e.heard.offset), secs (station), secs (t.base));
          break;
        }
      case Kind::AverageArmed:   // a left click checked Sync: refused while a station sync lasts (decision 4)
        if (on (r)) { d.uncheck = true; d.say = Say::AverageRefused; }
        break;
      case Kind::AverageResult:   // DecodeFinished with Sync checked and the band's average DT past +-0.19 s
        {
          if (on (r)) break;
          Offsets const t = target (r);
          r.next = t; r.next.base = t.base - e.avexdt;
          r.pending = !same (r.next, r.now);
          d.debug << QString {"Average sync: the band's DT %1 s - the base %2 s"}.arg (secs (e.avexdt), secs (r.next.base));
          break;
        }
      case Kind::RightClick:
        if (on (r)) { d = detail::end (r, Why::RightClick); break; }
        if (qAbs (target (r).base) >= 0.0005)   // as before: back to the system's time, the button grey until a decode
          {
            r.next = target (r); r.next.base = 0.0;
            r.pending = !same (r.next, r.now);
            d.grey = true;
            d.debug << "Average sync: the base back to 0";
          }
        break;
      case Kind::Check:   // DX Call, the band or the mode may have changed
        if (!on (r)) break;
        if (e.hisCall.isEmpty () || Radio::base_callsign (e.hisCall) != r.call) d = detail::end (r, Why::DxCall);
        else if (e.band != r.band) d = detail::end (r, Why::Band);
        else if (e.mode != r.mode) d = detail::end (r, Why::Mode);
        break;
      case Kind::Logged:   // (the review 2026-10-23, 1) his QSO only - an earlier QSO's Log QSO window may be accepted now
        if (on (r) && Radio::base_callsign (e.hisCall) == r.call) d = detail::end (r, Why::Logged);
        break;
      case Kind::Moment:
        {
          if (!r.pending) break;
          qint64 const jump = jump_ms (r.now, r.next);
          qint64 const made = jump ? step_ms (e.moment, jump) : 0;   // the parts alone changed (the sum the same): no jump
          if (jump && !made) break;
          if (made == jump) { r.now = r.next; r.pending = false; }
          else   // a step of a jump forward: both parts moved alike, by the share made
            {
              double const f = double (made) / double (jump);
              r.now.base += (r.next.base - r.now.base) * f;
              r.now.station += (r.next.station - r.now.station) * f;
            }
          d.apply = true; d.offsets = r.now; d.moved = made; d.repaint = true;
          d.debug << QString {"Clock offset %1 s (base %2 s, station %3 s): moved %4 ms%5"}
                       .arg (secs (total (r.now)), secs (r.now.base), secs (r.now.station)).arg (made)
                       .arg (r.pending ? QString {", %1 ms to come"}.arg (jump - made) : QString {});
          break;
        }
      }
    return d;
  }
}

#endif
