#ifndef CONTESTRULES_H
#define CONTESTRULES_H

/* CE3TSK 2026-11-03 (TODO.md 3.-226): what a contest asks of the program, one question at a time. These were one yes or no, "WW Digi",
   and a contest with the same exchange and another score, or the normal exchange and Tx1, needs some of them and not others. A site in
   the program asks the question it means. A contest is one row here, chosen by Configuration's contest_rules_of (). What a QSO is worth
   and what the log file is called are asked of the activity itself, in Configuration.cpp's switches. */
namespace contest_rules
{
  struct Rules
  {
    bool running = false;              // a contest is on: its own log and score, the lamp, the controls it locks
    bool gridExchange = false;         // the exchange is the 4 character grid: Tx2 and Tx3 carry it, and it stands where a report would
    bool rejectsReports = false;       // a line ending in a signal report is not contest traffic: dropped; one answering our call ends the QSO
    bool forcesSkipTx1 = false;        // Tx1 would say what Tx2 says: Skip Tx1 forced on and the Tx1 row greyed
    bool humanChoosesPartner = false;  // the operator starts each QSO: no wanted lists and no search through CQ messages (WW Digi's rule XI.12)
    bool fieldMultipliers = false;     // the 2 character field on each band is the multiplier: the new-grid tiers ask for it, at the head of the chain
    bool workedRanksLast = false;      // a station already worked on the band scores nothing: its points are shown, it ranks below every new call whatever its field, and the contest's lock holds AnsB4 on, so it is still answered
    bool finishesQsoFirst = false;     // a QSO under way comes before any ranking: a later step of his shows he heard us and is answered first, and "sent RR73/73" also counts our RR73 to his repeated R+grid
    bool prefixCountryMultipliers = false;  // the WPX prefix and the DXCC country on each band are the multipliers, and a QSO's points come from the call
    bool ft8Only = false;              // FT4 is not allowed: greyed, and left on entering
    bool cqYbUndirected = false;       // CQ YB is the contest's too: it directs nothing
    bool forbidsSkipTx1 = false;       // (2026-11-07) Tx1 carries the grid, half of the exchange: Skip Tx1 forced off, greyed, the Tx1 row offered
    bool missingGridScores = false;    // (2026-11-07) a QSO with no grid still counts, 1 point and no multiplier; the contest log says ZZ00 for it
  };

  inline Rules ww_digi ()
  {
    Rules r;
    r.running = r.gridExchange = r.rejectsReports = r.forcesSkipTx1 = r.humanChoosesPartner = r.fieldMultipliers = r.workedRanksLast = r.finishesQsoFirst = true;
    return r;
  }

  /* The ARRL International Digital Contest (rules version 1.1): the same 4 character grid and the same four messages as WW Digi, and
     "fully automated contacts are prohibited" (OPRG.4). The score is the sum of the QSO points with no multipliers (5.3), so a new
     grid is worth nothing and a QSO with a station already worked on the band is worth nothing (2.3, LGCK.1). */
  inline Rules arrl_digi ()
  {
    Rules r;
    r.running = r.gridExchange = r.rejectsReports = r.forcesSkipTx1 = r.humanChoosesPartner = r.workedRanksLast = r.finishesQsoFirst = true;
    return r;
  }

  /* The YBDXPI FT8 contest (the DX version: https://contest.ybdxpi.net/ft8/rules/): the 4 character grid, FT8 alone; points by member,
     country and continent, multipliers the WPX prefix and the DXCC country on each band (contestpoints.h). The rules name no CQ word
     and no rule on automation. Its guide puts participants in WSJT-X's WW Digi mode, so they call CQ WW; some call CQ YB. */
  inline Rules ybdxpi ()
  {
    Rules r;
    r.running = r.gridExchange = r.rejectsReports = r.forcesSkipTx1 = r.humanChoosesPartner = r.workedRanksLast = r.finishesQsoFirst = true;
    r.prefixCountryMultipliers = r.ft8Only = r.cqYbUndirected = true;
    return r;
  }

  /* CE3TSK 2026-11-07 (TODO.md 3.-231, FT_CHALLENGE_PLAN.md): the International FT Challenge (https://www.rttycontesting.com/ft-challenge/rules/):
     the normal FT8 and FT4 QSO, the grid in Tx1 and the signal reports after it; points and multipliers as WW Digi's, from the 4 character
     grid. Its FAQ: "you should always call other stations with your Grid Square message", so Skip Tx1 is off; a station that sends no grid
     still counts, 1 point and no multiplier, logged as ZZ00 ("the log submittal process will fill in any missing Grid Squares with ZZ00").
     Rule 7.13: "the operator must take some action for each QSO". Not gridExchange, rejectsReports or forcesSkipTx1: reports are its traffic. */
  inline Rules ft_challenge ()
  {
    Rules r;
    r.running = r.finishesQsoFirst = r.workedRanksLast = r.humanChoosesPartner = true;   // as every contest
    r.fieldMultipliers = true;                                                         // WW Digi's score
    r.forbidsSkipTx1 = r.missingGridScores = true;
    return r;
  }
}

#endif
