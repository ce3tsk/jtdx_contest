// CE3TSK 2026-11-24 (TODO.md 3.-283, GRID_FILL_PLAN.md stage 4): Club Log's users file - see clublogusers.h
#include "clublogusers.h"

#include <cstring>
#include <QRegularExpression>
#include "contrib/zlib/zlib.h"   // zlib 1.4, built in with Z_PREFIX (contrib/zlib/JTDX_CONTEST.txt)

namespace club_log_users
{
  namespace
  {
    quint32 u16 (QByteArray const& b, qint64 at) {return static_cast<quint8> (b[int (at)]) | (static_cast<quint8> (b[int (at + 1)]) << 8);}
    quint32 u32 (QByteArray const& b, qint64 at) {return u16 (b, at) | (u16 (b, at + 2) << 16);}
  }

  QByteArray zip_entry (QByteArray const& zip, char const * name, qint64 max_size, QString * error)
  {
    auto const fail = [error] (char const * why) {if (error) *error = QString::fromLatin1 (why); return QByteArray {};};
    qint64 const n {zip.size ()};
    // the end of the central directory: its signature within the last 64 kB and 22 bytes
    qint64 eocd {-1};
    for (qint64 at = n - 22; at >= 0 && at >= n - 22 - 65535; --at)
      if (0x06054b50u == u32 (zip, at)) {eocd = at; break;}
    if (eocd < 0) return fail ("not a ZIP");
    quint32 const entries {u16 (zip, eocd + 10)};
    qint64 cd {u32 (zip, eocd + 16)};
    std::size_t const name_len {std::strlen (name)};
    for (quint32 e = 0; e < entries; ++e)
      {
        if (cd + 46 > n || 0x02014b50u != u32 (zip, cd)) return fail ("a broken ZIP directory");
        quint32 const method {u16 (zip, cd + 10)}, crc {u32 (zip, cd + 16)}, csize {u32 (zip, cd + 20)}, usize {u32 (zip, cd + 24)};
        quint32 const nlen {u16 (zip, cd + 28)}, xlen {u16 (zip, cd + 30)}, clen {u16 (zip, cd + 32)};
        qint64 const local {u32 (zip, cd + 42)};
        if (cd + 46 + nlen > n) return fail ("a broken ZIP directory");
        if (nlen == name_len && 0 == std::memcmp (zip.constData () + cd + 46, name, name_len))
          {
            if (0xffffffffu == csize || 0xffffffffu == usize) return fail ("a ZIP64 entry");
            if (static_cast<qint64> (usize) > max_size) return fail ("the entry is larger than any users file");
            if (local + 30 > n || 0x04034b50u != u32 (zip, local)) return fail ("a broken ZIP entry");
            qint64 const data {local + 30 + u16 (zip, local + 26) + u16 (zip, local + 28)};
            if (data + csize > n) return fail ("a cut ZIP");
            QByteArray out;
            if (0 == method)
              out = zip.mid (int (data), int (csize));
            else if (8 == method)
              {
                out.resize (int (usize));
                z_stream z {};
                if (Z_OK != inflateInit2 (&z, -MAX_WBITS)) return fail ("zlib would not start");
                z.next_in = reinterpret_cast<Bytef *> (const_cast<char *> (zip.constData () + data));
                z.avail_in = csize;
                z.next_out = reinterpret_cast<Bytef *> (out.data ());
                z.avail_out = usize;
                int const r {inflate (&z, Z_FINISH)};
                bool const whole {Z_STREAM_END == r && z.total_out == usize};
                inflateEnd (&z);
                if (!whole) return fail ("the ZIP entry did not inflate");
              }
            else return fail ("a ZIP method other than deflate");
            if (crc != crc32 (0L, reinterpret_cast<Bytef const *> (out.constData ()), static_cast<uInt> (out.size ())))
              return fail ("the ZIP entry's CRC does not match");
            return out;
          }
        cd += 46 + nlen + xlen + clen;
      }
    return fail ("no clublog_users.json in the ZIP");
  }

  namespace
  {
    // a scanner of the users file's one shape: {"CALL":{"key":value,...},...}
    struct Scan
    {
      Scan (char const * b, char const * e) : p {b}, end {e} {}   // C++11: a default member value makes no aggregate
      char const * p;
      char const * end;
      bool ok {true};
      void ws () {while (p < end && (' ' == *p || '\n' == *p || '\r' == *p || '\t' == *p)) ++p;}
      bool take (char c) {ws (); if (p < end && c == *p) {++p; return true;} return false;}
      // a string; its text kept when `out` (escapes: the ASCII ones, \u as '?')
      bool string (QByteArray * out)
      {
        ws ();
        if (p >= end || '"' != *p) return ok = false;
        ++p;
        while (p < end && '"' != *p)
          {
            if ('\\' == *p)
              {
                if (++p >= end) return ok = false;
                char c {*p};
                if ('u' == c) {p += 4; c = '?'; if (p >= end) return ok = false;}
                else if ('n' == c) c = '\n'; else if ('t' == c) c = '\t'; else if ('r' == c) c = '\r'; else if ('b' == c || 'f' == c) c = ' ';
                if (out) *out += c;
              }
            else if (out) *out += *p;
            ++p;
          }
        if (p >= end) return ok = false;
        ++p;
        return true;
      }
      /* any value. (the review 2026-11-24, 2) Nested no deeper than max_depth: each level is a call of its own, and a ZIP of
         4 kB holding a value nested two million deep ran the stack out - the program died. The users file nests three. */
      static int const max_depth {32};
      bool skip (int depth = 0)
      {
        ws ();
        if (p >= end) return ok = false;
        if ('"' == *p) return string (nullptr);
        if ('{' == *p || '[' == *p)
          {
            if (depth >= max_depth) return ok = false;
            char const close {'{' == *p ? '}' : ']'};
            bool const object {'{' == *p};
            ++p;
            if (take (close)) return true;
            do
              {
                if (object && (!string (nullptr) || !take (':'))) return ok = false;
                if (!skip (depth + 1)) return false;
              }
            while (take (','));
            return take (close) || (ok = false);
          }
        while (p < end && ',' != *p && '}' != *p && ']' != *p && ' ' != *p && '\n' != *p && '\r' != *p && '\t' != *p) ++p;   // a number, true, false, null
        return true;
      }
    };

  }

  /* "YYYY-MM-DD..." or invalid - from its digits: a format parse of a summer-time start day fails in the AppImage's Qt
     5.15.3 (memory: qdate-fromstring-dst-trap) */
  QDate date_of (QByteArray const& d)
  {
    if (d.size () < 10 || '-' != d[4] || '-' != d[7]) return {};
    int v[3] {0, 0, 0};
    int const at[3] {0, 5, 8}, len[3] {4, 2, 2};
    for (int k = 0; k < 3; ++k)
      for (int i = at[k]; i < at[k] + len[k]; ++i)
        {
          if (d[i] < '0' || d[i] > '9') return {};
          v[k] = v[k] * 10 + (d[i] - '0');
        }
    return QDate {v[0], v[1], v[2]};
  }

  QByteArray small_file (QByteArray const& zip, QDate const& today, QString * error, int * kept)
  {
    if (kept) *kept = 0;
    QByteArray const json {zip_entry (zip, entry_name, max_json, error)};
    if (json.isEmpty ()) return {};
    static QRegularExpression const locator {QStringLiteral ("^[A-R]{2}[0-9]{2}([A-X]{2}([0-9]{2})?)?$"), QRegularExpression::CaseInsensitiveOption};
    static QRegularExpression const call_shape {QStringLiteral ("^[A-Z0-9/]{3,20}$")};
    QByteArray out {"# Club Log users, " + today.toString (Qt::ISODate).toLatin1 () + ": call,locator,last upload\n"};
    int n {0};
    Scan s {json.constData (), json.constData () + json.size ()};
    if (!s.take ('{'))
      {
        if (error) *error = QStringLiteral ("not the users file's JSON");
        return {};
      }
    if (!s.take ('}'))
      do
        {
          QByteArray call, loc, upload;
          if (!s.string (&call) || !s.take (':')) break;
          s.ws ();
          if (s.p < s.end && '{' == *s.p)
            {
              ++s.p;
              if (!s.take ('}'))
                do
                  {
                    QByteArray key;
                    if (!s.string (&key) || !s.take (':')) break;
                    s.ws ();
                    bool const text {s.p < s.end && '"' == *s.p};
                    if (text && "locator" == key) s.string (&loc);
                    else if (text && "lastupload" == key) s.string (&upload);
                    else s.skip ();
                  }
                while (s.ok && s.take (','));
              if (!s.ok || !s.take ('}')) {s.ok = false; break;}
            }
          else if (!s.skip ()) break;
          QString const c {QString::fromLatin1 (call).trimmed ().toUpper ()};
          QString const g {QString::fromLatin1 (loc).trimmed ()};
          QDate const d {date_of (upload)};
          // a placeholder (an underscore or a hyphen) is no call; a locator, an upload within keep_days (a day ahead allowed)
          if (call_shape.match (c).hasMatch () && locator.match (g).hasMatch () && d.isValid () && d.daysTo (today) <= keep_days && d.daysTo (today) >= -1)
            {
              out += c.toLatin1 () + ',' + g.left (4).toUpper ().toLatin1 () + ',' + d.toString (Qt::ISODate).toLatin1 () + '\n';
              ++n;
            }
        }
      while (s.ok && s.take (','));
    if (!s.ok || !s.take ('}'))
      {
        if (error) *error = QStringLiteral ("not the users file's JSON");
        return {};
      }
    if (n < minimum_members)
      {
        if (error) *error = QStringLiteral ("%1 members with a locator: not Club Log's users file").arg (n);
        return {};
      }
    if (kept) *kept = n;
    return out;
  }

  QHash<QString, Member> read (QByteArray const& small)
  {
    QHash<QString, Member> members;
    for (auto const& line : small.split ('\n'))
      {
        if (line.startsWith ('#')) continue;
        auto const f = line.trimmed ().split (',');
        if (3 != f.size ()) continue;
        QDate const d {date_of (f[2])};
        if (!d.isValid () || 4 != f[1].size ()) continue;
        members.insert (QString::fromLatin1 (f[0]).toUpper (), Member {QString::fromLatin1 (f[1]).toUpper (), d});
      }
    return members;
  }

  QString grid (QHash<QString, Member> const& members, QString const& call, QDate const& today)
  {
    auto const it = members.constFind (call.trimmed ().toUpper ());
    return it != members.constEnd () && it->uploaded.daysTo (today) <= recent_days ? it->grid : QString {};
  }
}
