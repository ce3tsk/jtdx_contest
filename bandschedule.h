#ifndef BANDSCHEDULE_H
#define BANDSCHEDULE_H

// CE3TSK 2026-09-26: which entry of the band scheduler fires at a given time.
//
// Settings holds five entries (time, band and mode, mixed JT9+JT65), and guiUpdate checked them in a
// chain of nested ifs - where the fifth compared the FOURTH entry's time, inside the fourth's own
// else, so it could never fire, for as long as the chain had existed. Taken out of guiUpdate so
// test_schedule can walk all five, with the chain's rules otherwise kept:
//
//   - from the second entry on, the list ends at the first entry with no band - a later entry is
//     not reached, whatever its time says;
//   - the first entry does not end the list. The chain let it fire on its time alone, and with no
//     band set, set_scheduler () parsed frequency 0 and QSYed towards it (review 2026-09-26) - a
//     first entry with no band is now skipped, and the entries after it still fire as they did.
//
// guiUpdate keeps the gate around it (minutes divisible by 5, second 01, scheduler on, Enable Tx
// off).

#include <QString>
#include <QVector>

struct ScheduleEntry
{
  // constructors rather than member defaults: the tree is C++11, where a default would stop the
  // brace initialisation guiUpdate uses - and a default-built entry must not carry a random mixed
  ScheduleEntry () : mixed {false} {}
  ScheduleEntry (QString h, QString m, QString b, bool x = false) : hh {h}, mm {m}, band {b}, mixed {x} {}
  QString hh;     // "00".."23"
  QString mm;     // "00".."55"
  QString band;   // "14.074 FT8" and the like; empty ends the list
  bool mixed;     // JT9+JT65 - part of the entry, so the chosen entry carries its own (review 2026-09-26)
};

// the index of the entry due at hour:minute, or -1 for none
inline int scheduled_entry (QVector<ScheduleEntry> const& entries, QString const& hour, QString const& minute)
{
  auto const due = [&] (int i) { return entries[i].hh == hour && entries[i].mm == minute; };
  if (entries.isEmpty ()) return -1;
  if (!entries[0].band.isEmpty () && due (0)) return 0;
  for (int i = 1; i < entries.size (); ++i)
    {
      if (entries[i].band.isEmpty ()) return -1;
      if (due (i)) return i;
    }
  return -1;
}

#endif
