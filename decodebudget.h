#ifndef DECODEBUDGET_H
#define DECODEBUDGET_H

/* CE3TSK: the TX background's time window (PIPELINED_DECODE_PLAN.md P7, DECODE_RECIPE_PLAN.md).
   The decoder's background phase runs after the period's decode until the next decode is
   due. The GUI used to decode every period, its own TX periods included (the rig's muted RX
   audio), which aborted the background about 14 s into our own transmission and replaced
   the retained band with that empty period. Now, while the TX background is enabled, the
   decode of a period in which we transmitted is skipped, and the decode that precedes our
   TX period hands the decoder a two-period budget: our 15 s TX plus the other station's
   12.64 s transmission are the background's window, about 27 s less the margin. If the
   TX does not happen after all, the next period's decode removes .lock and the background
   stops as it always did - one unit wasted at most. */

/* the index of the period a moment belongs to; a decode trigger sits at 14.1 s of its
   period, so a moment inside the period is taken a few seconds earlier to stay clear of
   the boundary when the trigger is late (audio delay, a lost block) */
inline int period_index (double seconds_of_day, double period)
{
  if (period <= 0) return 0;
  return static_cast<int> (seconds_of_day / period);
}

/* CE3TSK: the 5 s step back is sized for FT8's 15 s and FT4's 7.5 s, where a trigger sits at
   14.1 s and 6.05 s of its period. A period shorter than that would be stepped back past its
   own start and the trigger would be counted as the previous period's, which is what decides
   whether our own TX period's decode is skipped. Below 7.5 s the back-off is a third of the
   period instead - 1.25 s for FT2's 3.75 s, against a trigger at 3.17 s. */
inline int period_index_of_trigger (double seconds_of_day, double period)
{
  return period_index (seconds_of_day - (period >= 7.5 ? 5.0 : period / 3.0), period);
}

/* whether the period after the given one is a period of ours: "TX first" transmits in the
   even periods of every two */
inline bool tx_period_next (bool tx_first, int index)
{
  return (((index + 1) % 2) == 0) == tx_first;
}

/* the budget handed to the decoder, tenths of a second: two periods when we transmit next
   or when replaying from disk (a replay has no next period until the next file is loaded,
   and the whole unit list needs about 20 s - the .lock removal of the next decode still
   aborts it), one period otherwise; 0 would mean "the period" to the decoder and is never
   produced */
inline int background_budget_tenths (bool tx_enabled, bool tx_next, double period, bool replay = false)
{
  int const p = period > 0 ? static_cast<int> (period * 10 + 0.5) : 150;
  return (replay || (tx_enabled && tx_next) ? 2 : 1) * p;
}

/* the decode of a period in which we transmitted is skipped - only for live audio and only
   while the TX background is enabled (with it off the decode is harmless and JTDX's usual
   behaviour is kept) */
inline bool skip_own_tx_decode (bool disk_data, bool background_enabled, int tx_period, int this_period)
{
  return !disk_data && background_enabled && tx_period == this_period;
}

#endif
