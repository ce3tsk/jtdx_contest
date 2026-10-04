// CE3TSK 2026-10-03: see licensestates.h
#include "licensestates.h"

#include <algorithm>
#include <cstring>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>

namespace
{
  char const magic[] = "# us-license-states.txt";

  // one call line, "AA0AI CO": the call in [A-Z0-9], one space, two capital letters
  bool callLine (char const * p, char const * end, char const ** space)
  {
    char const * s = static_cast<char const *> (std::memchr (p, ' ', end - p));
    if (!s || s == p || s - p > 6 || end - s != 3 || s[1] < 'A' || s[1] > 'Z' || s[2] < 'A' || s[2] > 'Z') return false;
    for (char const * c = p; c < s; ++c) if (!((*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9'))) return false;
    *space = s;
    return true;
  }

  // calls each line, without its end of line, until f returns false
  template<typename F> void eachLine (QByteArray const& content, F f)
  {
    char const * p = content.constData ();
    char const * const end = p + content.size ();
    while (p < end)
      {
        char const * eol = static_cast<char const *> (std::memchr (p, '\n', end - p));
        if (!eol) eol = end;
        char const * le = eol;
        if (le > p && '\r' == le[-1]) --le;
        if (!f (p, le)) return;
        p = eol + 1;
      }
  }

  int digits (char const * p, int n)
  {
    int v = 0;
    for (int i = 0; i < n; ++i) {if (p[i] < '0' || p[i] > '9') return -1; v = v * 10 + (p[i] - '0');}
    return v;
  }
}

QDate LicenseStates::version (QByteArray const& content)
{
  if (!content.startsWith (magic)) return QDate {};
  QDate date;
  long declared = -1, calls = 0;
  bool bad = false;
  eachLine (content, [&] (char const * p, char const * le) {
      if (le == p) return true;
      if ('#' == *p)
        {
          QByteArray const line {p, int (le - p)};
          if (line.startsWith ("# version ") && line.size () == 20)   // "# version yyyy-mm-dd" - built from its digits (a
            {                                                          // format parse fails on a DST start day in Qt 5.15.3)
              char const * d = p + 10;
              int const y = digits (d, 4), m = digits (d + 5, 2), dd = digits (d + 8, 2);
              if (y > 0 && '-' == d[4] && '-' == d[7]) date = QDate {y, m, dd};
            }
          else if (line.startsWith ("# calls ")) declared = line.mid (8).toLong ();
          return true;
        }
      char const * space;
      if (!callLine (p, le, &space)) {bad = true; return false;}
      ++calls;
      return true;
    });
  return !bad && declared > 0 && calls == declared && date.isValid () ? date : QDate {};
}

void LicenseStates::load (QString const& path)
{
  QFile file {path};
  if (file.open (QIODevice::ReadOnly)) load (file.readAll ());
  else entries_.clear ();
}

void LicenseStates::load (QByteArray const& content)
{
  entries_.clear ();
  if (!version (content).isValid ()) return;
  QVector<quint64> entries;
  eachLine (content, [&] (char const * p, char const * le) {
      char const * space;
      if (le > p && '#' != *p && callLine (p, le, &space))
        if (quint64 const v = pack (QByteArray::fromRawData (p, int (space - p))))
          entries.append ((v << 16) | (quint64 (quint8 (space[1])) << 8) | quint8 (space[2]));
      return true;
    });
  std::sort (entries.begin (), entries.end ());
  entries_ = entries;
}

/* bijective base 37 - digits 1-10, letters 11-36, no zero digit - so calls of different lengths never meet: six
   characters at most is below 37^6 = 2 565 726 409, inside 32 bits */
quint64 LicenseStates::pack (QByteArray const& call)
{
  if (call.isEmpty () || call.size () > 6) return 0;
  quint64 v = 0;
  for (char const c : call)
    {
      int const d = c >= '0' && c <= '9' ? c - '0' + 1 : c >= 'A' && c <= 'Z' ? c - 'A' + 11 : 0;
      if (!d) return 0;
      v = v * 37 + d;
    }
  return v;
}

QString LicenseStates::usBaseCall (QString const& call)
{
  static QRegularExpression const us {R"(^[AKNW][A-Z]?[0-9][A-Z]{1,3}$)"};   // tools/make_license_states.py: US_CALL
  QString best;
  for (auto const& part : call.toUpper ().split ('/'))
    if (part.size () > best.size () && us.match (part).hasMatch ()) best = part;
  return best;
}

QString LicenseStates::state (QString const& call) const
{
  if (entries_.isEmpty ()) return {};
  quint64 const key = pack (usBaseCall (call).toLatin1 ());
  if (!key) return {};
  auto const it = std::lower_bound (entries_.cbegin (), entries_.cend (), key << 16);
  if (it == entries_.cend () || (*it >> 16) != key) return {};
  char const s[2] {char ((*it >> 8) & 0xff), char (*it & 0xff)};
  return QString::fromLatin1 (s, 2);
}
