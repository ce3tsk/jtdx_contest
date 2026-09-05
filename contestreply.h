#ifndef CONTESTREPLY_H
#define CONTESTREPLY_H
// CE3TSK: which Tx message answers a decoded message addressed to me (double click on a line,
// MainWindow::processMessage). The message's last field decides:
//   RRR / RR73 / 73        -> Tx5, sign off
//   R<report> or "R"       -> Tx4, RR73
//   a report               -> Tx3, roger report
//   a grid                 -> Tx2 in normal operation (send my report)
//                             Tx3 in the WW Digi contest: the grid *is* his exchange, so the answer
//                             is "HISCALL MYCALL R MYGRID" - the same as the auto-sequencer sends.
//                             Tx2 there would repeat a bare "HISCALL MYCALL MYGRID" without the R,
//                             i.e. behave as if I had called him (seen on air 2026-08-29).
//   nothing after his call -> Tx2
// hasLast: a field follows his call; last: that field; lastIsGrid: it passes gridOK().
#include <QString>

inline int reply_tx_to_me (bool wwDigi, bool hasLast, QString const& last, bool lastIsGrid)
{
  if (!hasLast) return 2;
  if (lastIsGrid) return wwDigi ? 3 : 2;
  if (last.left (3) == "RRR" || last.toInt () == 73 || last.left (4) == "RR73") return 5;
  if (last.left (1) == "R") return 4;
  return 3;   // a report; toInt() of anything else is 0, which JTDX has always treated as a report
}
#endif
