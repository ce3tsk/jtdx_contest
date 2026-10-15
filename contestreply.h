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

// CE3TSK 2026-10-14 (TODO 3.-85, review 3.-82 item 16): the second tab's "Answer caller" - the answer to a station that
// answered my CQ, chosen by the same rule as above. Stock cut the "R" out of Tx3 ("K1ABC CE3TSK R-10" -> "K1ABC CE3TSK
// -10", Tx2's text) and cut at -1 when there was none: "1AB" in the WW Digi contest, "O" for the JT65 VHF "RO". In WW
// Digi his call carried his exchange (the grid), so the answer is Tx3 itself, "K1ABC CE3TSK R FF46" - Tx2 there is Tx1's
// text, as if I had called him (on the air 2026-08-29). Elsewhere a Tx3 with no signed report has no "R" to cut, and the
// report message is Tx2. tx: the Tx whose stage it is - 3 (a roger report) in WW Digi, else 2.
struct CallerAnswer { QString text; int tx; };
inline CallerAnswer answer_caller (bool wwDigi, QString const& tx2, QString const& tx3)
{
  if (wwDigi) return {tx3, 3};
  int i0 = tx3.indexOf (" R-");
  if (i0 < 0) i0 = tx3.indexOf (" R+");
  if (i0 < 0) return {tx2, 2};
  return {tx3.left (i0 + 1) + tx3.mid (i0 + 2, 3), 2};
}
#endif
