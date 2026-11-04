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
  };

  inline Rules ww_digi ()
  {
    Rules r;
    r.running = r.gridExchange = r.rejectsReports = r.forcesSkipTx1 = r.humanChoosesPartner = r.fieldMultipliers = true;
    return r;
  }
}

#endif
