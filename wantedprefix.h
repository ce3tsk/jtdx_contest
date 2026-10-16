#ifndef WANTEDPREFIX_H
#define WANTEDPREFIX_H
// CE3TSK 2026-10-15 (TODO 3.-94): is a decoded call one of a wanted prefix (the main window's Wanted prefix box,
// DisplayText's priorities and autoselection)? Stock asked only where the call begins, so AA1AAA/VP9 - a station in
// Bermuda - was not one of VP9. Now also a compound call's effective prefix, the one cty.dat takes the country from
// (Radio::effective_prefix: the part after the slash when it is a prefix; /P and /A leave the call as it is). /MM and /AM
// give no prefix here: effective_prefix stands "1B1ABCD" in for them, which a wanted 1B matched (TODO 3.-105, the
// review). The beginning still counts as before, so VP9/AA1AAA and AA1AAA/VP9 both match VP9.
#include <QString>
#include "Radio.hpp"

namespace wanted_prefix
{
  inline bool matches (QString const& call, QString const& wanted)
  {
    if (wanted.size () < 2) return false;   // as before: a single letter is no prefix here
    if (call.startsWith (wanted)) return true;
    if (!call.contains ('/')) return false;
    for (auto const& part : call.split ('/'))
      if (part == "MM" || part == "AM") return false;   // maritime and aeronautical mobile: no country
    return Radio::effective_prefix (call).startsWith (wanted);
  }
}
#endif
