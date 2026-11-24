#ifndef EQSLMEMBERS_H
#define EQSLMEMBERS_H

/* CE3TSK 2026-11-23 (TODO.md 3.-283, GRID_FILL_PLAN.md stage 2): eQSL's member list - each member's grid and last
   upload - for a QSO that gave no grid (gridfill.h, source 3). The operator: "For eQSL there is a full member list file
   that can be downloaded without eQSL account (login/password) https://www.eqsl.cc/DownloadedFiles/eQSLMemberList.csv
   and the benefit is it could be downloaded similar to the lotw_user list on settings > general (add its download and
   use the data) and it has the grid and when last uploaded."

   The file, as fetched for the plan (2026-11-23): CRLF lines under the header "# Callsign,GridSquare,AG,Last_Upload_Date",
   the date YYYYMMDD or empty; 363,889 rows, 153,897 with a date, 55,645 within the last 12 months; grids self-reported,
   8,607 no locator; calls with a slash are rows of their own; a few calls twice; not strictly sorted. Kept in the log
   directory as eQSLMemberList.csv, downloaded from Settings > General > Data files, never by itself.

   What is taken (G3): a member's grid only while it uploaded to eQSL within the last 12 months - a profile not used for
   longer may be an old QTH - and only a locator; of a call listed twice, its newest upload's row. The rest of G3 - four
   characters, the country, /MM - is gridfill.h's usable ().

   test_gridfill has the reading, test_datadownload the download. */

#include <cstring>
#include <QByteArray>
#include <QDate>
#include <QHash>
#include <QRegularExpression>
#include <QString>

namespace eqsl_members
{
  char const * const url {"https://www.eqsl.cc/DownloadedFiles/eQSLMemberList.csv"};
  char const * const file_name {"eQSLMemberList.csv"};
  char const * const header {"# Callsign,GridSquare,AG,Last_Upload_Date"};
  int const minimum_rows {1000};     // fewer: a cut or a wrong file, not eQSL's list
  int const recent_days {365};       // the last 12 months (G3)

  // where the download comes from: eQSL's, or JTDX_EQSL_MEMBERS_URL's for a test
  inline QByteArray source_url ()
  {
    QByteArray const set {qgetenv ("JTDX_EQSL_MEMBERS_URL")};
    return set.isEmpty () ? QByteArray {url} : set;
  }

  // the date of a row's last upload, YYYYMMDD, or an invalid date
  inline QDate upload_date (char const * p, int n)
  {
    if (8 != n) return {};
    for (int i = 0; i < 8; ++i) if (p[i] < '0' || p[i] > '9') return {};
    auto const num = [p] (int at, int len) {int v {0}; for (int i = at; i < at + len; ++i) v = v * 10 + (p[i] - '0'); return v;};
    return QDate {num (0, 4), num (4, 2), num (6, 2)};
  }

  /* The list's version - its newest upload, as the LoTW file's - or an invalid date when the content is not eQSL's member
     list: its header first (a byte-order mark allowed), and minimum_rows rows of four fields at least. What a download
     must be to replace the copy in use (Configuration's start_data_file_download). */
  inline QDate version (QByteArray const& content)
  {
    char const * p {content.constData ()};
    char const * const end {p + content.size ()};
    if (end - p >= 3 && 0 == std::memcmp (p, "\xEF\xBB\xBF", 3)) p += 3;
    std::size_t const h {std::strlen (header)};
    if (end - p < static_cast<long> (h) || 0 != std::memcmp (p, header, h)) return {};
    QDate newest;
    int rows {0};
    while (p < end)
      {
        auto eol = static_cast<char const *> (std::memchr (p, '\n', end - p));
        if (!eol) eol = end;
        auto line_end = eol;
        if (line_end > p && '\r' == line_end[-1]) --line_end;
        if (line_end > p && '#' != *p)
          {
            char const * f[4] {p, nullptr, nullptr, nullptr};
            int n {1};
            for (char const * q = p; q < line_end && n < 4; ++q) if (',' == *q) f[n++] = q + 1;
            if (4 == n)
              {
                ++rows;
                auto const c = static_cast<char const *> (std::memchr (f[3], ',', line_end - f[3]));
                QDate const d {upload_date (f[3], static_cast<int> ((c ? c : line_end) - f[3]))};
                if (d.isValid () && (!newest.isValid () || d > newest)) newest = d;
              }
          }
        p = eol + 1;
      }
    return rows >= minimum_rows ? newest : QDate {};
  }

  struct Member
  {
    QString grid;        // its first four characters, upper case
    QDate uploaded;
  };

  /* The members the log may take a grid from (G3), by call in upper case: a locator for a grid, an upload within
     recent_days before `today` (none from the future beyond a day); of a call listed twice, the newest upload's row. */
  inline QHash<QString, Member> read (QByteArray const& content, QDate const& today)
  {
    QHash<QString, Member> members;
    if (!version (content).isValid ()) return members;
    static QRegularExpression const locator {QStringLiteral ("^[A-R]{2}[0-9]{2}([A-X]{2}([0-9]{2})?)?$"), QRegularExpression::CaseInsensitiveOption};
    char const * p {content.constData ()};
    char const * const end {p + content.size ()};
    // (a byte-order mark before the header makes that line no member either: its date is none)
    while (p < end)
      {
        auto eol = static_cast<char const *> (std::memchr (p, '\n', end - p));
        if (!eol) eol = end;
        auto line_end = eol;
        if (line_end > p && '\r' == line_end[-1]) --line_end;
        if (line_end > p && '#' != *p)
          {
            char const * f[4] {p, nullptr, nullptr, nullptr};
            int n {1};
            for (char const * q = p; q < line_end && n < 4; ++q) if (',' == *q) f[n++] = q + 1;
            if (4 == n)
              {
                auto const c = static_cast<char const *> (std::memchr (f[3], ',', line_end - f[3]));
                QDate const d {upload_date (f[3], static_cast<int> ((c ? c : line_end) - f[3]))};
                QString const grid {QString::fromLatin1 (f[1], static_cast<int> (f[2] - 1 - f[1])).trimmed ()};
                if (d.isValid () && d.daysTo (today) <= recent_days && d.daysTo (today) >= -1 && locator.match (grid).hasMatch ())
                  {
                    QString const call {QString::fromLatin1 (f[0], static_cast<int> (f[1] - 1 - f[0])).trimmed ().toUpper ()};
                    auto it = members.find (call);
                    if (!call.isEmpty () && (it == members.end () || d > it->uploaded)) members[call] = Member {grid.left (4).toUpper (), d};
                  }
              }
          }
        p = eol + 1;
      }
    return members;
  }

  // a call's grid in the list, or "": while its last upload is within recent_days before `today` - a program that runs
  // for days asks again each time
  inline QString grid (QHash<QString, Member> const& members, QString const& call, QDate const& today)
  {
    auto const it = members.constFind (call.trimmed ().toUpper ());
    return it != members.constEnd () && it->uploaded.daysTo (today) <= recent_days ? it->grid : QString {};
  }
}

#endif
