#ifndef CONTESTIGNORE_H
#define CONTESTIGNORE_H

#include <QHash>
#include <QString>
#include <QStringList>

/* CE3TSK: WW Digi contest - stations to leave alone for a while.

   The contest exchange is the 4 character grid. A station that answers our exchange with a
   signal report ("CE3TSK DL6FKR -10", "CE3TSK DL6FKR R-10") is running ordinary FT8/FT4, not
   the contest: the QSO cannot complete, because neither side ever sends what the other is
   waiting for. Answering him again the next period only repeats the deadlock, and the
   autoselect would do exactly that - he is calling us, which is the highest priority there
   is. So the QSO is dropped and he is skipped for five minutes.

   Keyed by base callsign. A double click on the station clears him again - the operator
   overrules this. A clock that jumps backwards keeps a station ignored rather than releasing
   him early; five minutes later he is free either way. */
class ContestIgnore
{
public:
  static qint64 const window_ms = 300000;   /* five minutes */

  void add (QString const& base, qint64 now) { if (!base.isEmpty ()) m_until.insert (base, now); }
  void remove (QString const& base) { m_until.remove (base); }
  void clear () { m_until.clear (); }
  int size () const { return m_until.size (); }

  bool has (QString const& base, qint64 now) const
  {
    auto const it = m_until.constFind (base);
    return it != m_until.constEnd () && (now - it.value ()) < window_ms;
  }

  /* the stations still inside their window - process_Auto takes them out of the QSO history
     every pass, so the autoselect never offers one and never wastes a period's pick on him */
  QStringList calls (qint64 now) const
  {
    QStringList out;
    for (auto it = m_until.constBegin (); it != m_until.constEnd (); ++it)
      if ((now - it.value ()) < window_ms) out << it.key ();
    return out;
  }

  /* drop what has expired - only to keep the table small, has() is already time aware */
  void prune (qint64 now)
  {
    for (auto it = m_until.begin (); it != m_until.end ();)
      {
        if ((now - it.value ()) >= window_ms) it = m_until.erase (it);
        else ++it;
      }
  }

private:
  QHash<QString, qint64> m_until;
};

#endif
