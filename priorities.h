// -*- Mode: C++ -*-
/* CE3TSK 2026-10-03: the priorities DisplayText gives a decode and the autoselect ranks stations by - named, with the
   rules the autoselect reads from them, so a tier can be added or moved in one place and every rule says which tiers it
   covers. Until now the levels were bare numbers in displaytext.cpp and the rules bare thresholds in qsohistory.cpp and
   mainwindow.cpp ("> 16", "< 20", "> 19"), so a tier inherited whatever behaviour the range it landed in carried - the
   new US state did, ranked at 20-23 (review 2026-10-03).

   The order, highest first: WW Digi field, contest points, CQ zone, ITU zone, DXCC, the wanted lists, new US state,
   grid, prefix, call, the wanted lists over no new tier, LoTW. Each "new" tier is a pair: the odd value is a LoTW user's.

   Part of JTDX_CONTEST.  Copyright (C) 2026 Tihomir Sokcevic CE3TSK.  This program is free software: you may
   redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 or (at your option) any later version. It is distributed WITHOUT ANY WARRANTY. */
#ifndef JTDX_PRIORITIES_H
#define JTDX_PRIORITIES_H

namespace decode_priority
{
  enum Level : int
  {
    None = 0,
    LoTW = 1,
    // a wanted call, prefix or grid, or country, on a decode with no new tier
    WantedCountryOld = 2, WantedPrefixOld = 3, WantedCallOld = 4,
    NewCallBand = 5, NewCall = 7,
    NewPxBand = 9, NewPx = 11,
    NewGridBand = 13, NewGrid = 15,
    NewStateBand = 17, NewState = 19,                 // Worked All States: below the wanted lists, above grid (the operator)
    // a wanted call, prefix or grid, or country, lifting a decode whose tier is below DXCC
    WantedCountry = 21, WantedPrefix = 22, WantedCall = 23,
    NewDXCCBand = 24, NewDXCC = 26,
    NewITUZBand = 28, NewITUZ = 30,
    NewCQZBand = 32, NewCQZ = 34,
    ContestPointsBase = 34,                            // 34 + 2 * points (+1 LoTW): 1 point 36/37 .. 7 points 48/49
    NewField = 52                                      // WW Digi multiplier, the head of the chain
  };

  // the wanted band over a lower tier: such a station's CQ is called even while we call CQ
  inline bool wanted (int p) {return p >= WantedCountry && p <= WantedCall;}
  // the wanted lists over no new tier
  inline bool wantedOld (int p) {return p >= WantedCountryOld && p <= WantedCallOld;}
  // the chased class - DXCC and everything above it: the answer counter never gives up on it, and unless "Strict
  // directional CQ operation" is ON the direction of its CQ is not held against it - the 'new DXCC' exception the
  // option's tooltip names, which applies by default and which the option removes. NOTE: QsoHistory keeps the option
  // inverted - its _strictdirCQ is true when the option is OFF (owndata () stores !strictdirCQ).
  inline bool dxccClass (int p) {return p >= NewDXCCBand;}
  // a station the answer counter gives up on after its tries: every tier below the wanted band - a new US state included
  // - and no tier at all; never a wanted one, never the chased class
  inline bool counterGivesUp (int p) {return !wanted (p) && !wantedOld (p) && !dxccClass (p);}
  // unless "Strict directional CQ operation" is ON, OUR directed CQ is not held against a wanted station or one of the
  // chased class (QsoHistory's inverted _strictdirCQ, as above)
  inline bool ourDirectionWaived (int p) {return wanted (p) || wantedOld (p) || dxccClass (p);}
  // may the wanted lists lift this decode into the wanted band? Anything below the chased class - the new US state too,
  // so a wanted station in a new state keeps the wanted beep and is called while we call CQ
  inline bool wantedMayLift (int p) {return !dxccClass (p);}
  // no new tier at all: the wanted lists then use their own low band
  inline bool noNewTier (int p) {return p < NewCallBand;}
}

#endif
