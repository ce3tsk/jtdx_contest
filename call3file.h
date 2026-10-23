// -*- Mode: C++ -*-
/* CE3TSK 2026-10-23 (TODO.md 3.-199): CALL3.TXT - the calls and grid squares behind the JT65/JT9/JT10 Hint decoders
   (lib/call3list.f90) and the Lookup button - and Settings > General > Data files' download of it, from where WSJT-X
   3.0 and WS take theirs (Settings > Colors there). The copy in use is always the one in the log directory: the
   program's first start copies the bundled one there, the Add button writes to it, and a download replaces it, the
   copy replaced kept beside it as CALL3_backup.TXT.

   Its version is the first comment line, "// Version: 2026-09-20_18:12Z"; the bundled list of 2016-2017 has none.
   A download is taken only as a real list (1000 entries at least, a line "call,square," with a call of 3 characters
   or more - capitals, digits, '/' - and a grid square): a captive portal's page, a truncated file or another data file
   is refused, the copy in use left as it was.
   (3.-200) That version line is not required: WSJT-X reads it and never checks it, so a change of its form on the
   server must not refuse every download here - a list of 1000 entries is taken, its date then "version unknown". The
   Lookup button takes the line of the call itself, not the first one that starts with it (AA0I took AA0IJ's EN34),
   and a copy that is the bundled list of 2016-2017 byte for byte is replaced by the bundled one at the start.
   Qt Core only: test_call3file drives it. */

#ifndef CALL3FILE_H
#define CALL3FILE_H

#include <QByteArray>
#include <QCryptographicHash>
#include <QDate>
#include <QString>

namespace call3_file
{
  char const * const url {"https://wsjt-x-improved.sourceforge.io/CALL3.TXT"};
  char const * const file_name {"CALL3.TXT"};
  char const * const backup_name {"CALL3_backup.TXT"};
  int const minimum_entries {1000};
  // (3.-200) the bundled list of 2016-2017 (JTAlert's HamSpots spots, 34 323 calls; ad08d18:contrib/CallDB/CALL3.TXT)
  char const * const old_bundled_md5 {"8251f5ca98d2462c3fbabfeed9a49ec9"};

  // the date of the "// Version:" line among the leading comment lines; invalid without one
  inline QDate version (QByteArray const& content)
  {
    QByteArray const tag {"// Version:"};
    int pos {0};
    while (pos < content.size ())
      {
        int end = content.indexOf ('\n', pos);
        if (end < 0) end = content.size ();
        QByteArray const line {content.mid (pos, end - pos).trimmed ()};
        if (!line.startsWith ("//")) break;   // the comments come first
        if (line.startsWith (tag))
          {
            QByteArray const v {line.mid (tag.size ()).trimmed ()};
            if (v.size () < 10 || '-' != v[4] || '-' != v[7]) return {};
            bool y, m, d;
            int const year {v.mid (0, 4).toInt (&y)}, month {v.mid (5, 2).toInt (&m)}, day {v.mid (8, 2).toInt (&d)};
            return y && m && d ? QDate {year, month, day} : QDate {};   // from its digits (QDate::fromString trips on a summer-time day)
          }
        pos = end + 1;
      }
    return {};
  }

  // the entry lines: "call,square," - a call of 3 characters at least, capitals, digits and '/' only, then a grid
  // square (two letters A-R, two digits). (3.-200) That strict since the version line is no longer required: the Hint
  // readers' looser rule (3 and 4 characters) took 3264 lines of cty.dat, served in its place
  inline int entries (QByteArray const& content)
  {
    auto const is_call = [] (QByteArray const& c) {
      if (c.size () < 3) return false;
      for (char ch : c) if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || '/' == ch)) return false;
      return true;
    };
    auto const is_square = [] (QByteArray const& g) {
      return g.size () >= 4 && g[0] >= 'A' && g[0] <= 'R' && g[1] >= 'A' && g[1] <= 'R' && g[2] >= '0' && g[2] <= '9' && g[3] >= '0' && g[3] <= '9';
    };
    int n {0}, pos {0};
    while (pos < content.size ())
      {
        int end = content.indexOf ('\n', pos);
        if (end < 0) end = content.size ();
        QByteArray const line {content.mid (pos, end - pos)};
        int const c1 = line.indexOf (','), c2 = c1 < 0 ? -1 : line.indexOf (',', c1 + 1);
        if (!line.startsWith ("//") && c2 > c1 && is_call (line.left (c1)) && is_square (line.mid (c1 + 1, c2 - c1 - 1))) ++n;
        pos = end + 1;
      }
    return n;
  }

  // a download's verdict, as the other data files': invalid when it is not a CALL3.TXT call list (fewer than 1000
  // entries); else its version - or 1970-01-01 when it has no readable one, taken all the same (3.-200)
  inline QDate checked_version (QByteArray const& content)
  {
    if (entries (content) < minimum_entries) return {};
    QDate const v {version (content)};
    return v.isValid () ? v : QDate {1970, 1, 1};
  }

  // (3.-200) the Lookup button's line: the call's own - the call and its comma - not one that starts with it
  inline bool entry_of (QString const& line, QString const& call)
  {
    return !call.isEmpty () && line.startsWith (call + ',');
  }

  // (3.-200) the copy is the bundled list of 2016-2017, byte for byte: nothing added with Add, nothing downloaded
  inline bool is_old_bundled (QByteArray const& content)
  {
    return QCryptographicHash::hash (content, QCryptographicHash::Md5).toHex () == old_bundled_md5;
  }
}

#endif
