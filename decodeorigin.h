#ifndef DECODEORIGIN_H
#define DECODEORIGIN_H

// CE3TSK 2026-10-20: a decode's lines carry the dial and the mode the decode BEGAN on (IMPROVED_FEATURES_PLAN.md F6).
//
// FT8 starts decoding 14.1 s into its period and its passes run on past the period's end, so a band or mode changed at
// the top of the minute - by hand, or by band hopping at every hop - comes while the old period is still being
// decoded. Its later lines were taken with the dial and mode of NOW: the band in ALL.TXT (under the header of the new
// dial), the judgement of a /R call (a rover below 30 MHz is a likely phantom), worked-before by band and mode, the
// band's name on the period's blank line, and - the band change having stopped spotting until the next decode ended -
// no spot to PSK Reporter at all. Reproduced 2026-10-20 before the fix (tools/ui-check/latedecode.sh: a 20 m
// decode's header named 50.313 MHz and its /R line lost its tag). The deleted-decodes record (deletedlog.h) already
// kept the dial a decode began on, for the same reason.
//
// These are the rules; MainWindow keeps the origin of the decode running and asks them for every line.

#include <QString>
#include <QtGlobal>

namespace decode_origin
{
  struct Origin
  {
    // constructors rather than a member default: the tree is C++11, where a default stops brace initialisation
    Origin () : dial {0} {}
    Origin (qint64 d, QString const& m) : dial {d}, mode {m} {}
    qint64 dial;   // Hz; 0 before the first decode
    QString mode;
  };

  // the origin a decode is given as it begins: new data is of the dial and mode of now; a decode of the same data
  // again (the Decode button, a double click - newdat 0) is of the data's own origin, which a band change since has not
  // changed. Before any decode there is no older origin to keep.
  inline Origin at_begin (Origin const& previous, bool new_data, qint64 dial_now, QString const& mode_now)
  {
    if (new_data || 0 == previous.dial) return {dial_now, mode_now};
    return previous;
  }

  // a line of a decode that began before the dial or the mode last changed
  inline bool late (Origin const& o, qint64 dial_now, QString const& mode_now)
  {
    return o.dial != dial_now || o.mode != mode_now;
  }

  // ALL.TXT files a line under the last dial-and-mode header above it: a line whose origin is not that header's gets
  // one of its own first (the next line of the new dial then gets the new dial's header back)
  inline bool needs_header (Origin const& o, qint64 header_dial, QString const& header_mode)
  {
    return o.dial != header_dial || o.mode != header_mode;
  }

  // PSK Reporter: a late line is spotted, with its own dial and mode - it is no longer the new band's; any other line
  // as before (ok_to_post: spotting starts once a decode has ended after the program starts or the band changes)
  inline bool may_spot (bool ok_to_post, bool is_late) { return ok_to_post || is_late; }

  // the UDP clients take a decode's band from the dial of the program's last Status message, which is the new dial
  // after a change: a line of another band or mode is held back from them (WS holds back the decodes for 0.6 of a
  // period after a band change for the same reason). A dial moved within the band - a wheel step, the rig's knob -
  // leaves their band right: those lines go, as they always did (review 3.-155 3)
  inline bool to_udp (Origin const& o, QString const& origin_band, QString const& band_now, QString const& mode_now)
  {
    return origin_band == band_now && o.mode == mode_now;
  }
}

#endif
