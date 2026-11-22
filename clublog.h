#ifndef CLUBLOG_H
#define CLUBLOG_H

/* CE3TSK 2026-11-21 (TODO.md 3.-280, QSL_CONFIRMATIONS_PLAN.md step 1): the user's confirmed QSOs from Club Log.

   Club Log's REST API v1 gives a log as ADIF in one request (POST /logs/{call}/adif-export) to a program holding a
   token the user created in Club Log (Settings > API). There is no application key, so it passes rule 6b. A QSO
   Club Log has confirmed carries QSL_RCVD Y, and in Club Log those are the LoTW QSLs it has fetched. Only those
   records are kept, as clublog_qsl.adi in the data directory: ADIF that ADIF::load reads as it reads the log, so a
   confirmed DXCC, zone, grid or call is judged by the same cty.dat lookup as a worked one.

   Club Log's rules, kept here and in Configuration: a User-Agent naming the product, never a retry after 401, 403 or
   422 (the user must act: 401 the token is wrong or revoked; since 2026-11-22 403 - the call is not on the token's
   account - and 422 - a request to correct - refuse that call with that token alone), on 429 wait until its Retry-After, and
   far below 60 requests a minute and 500 a day: one request at most every refresh_hours, or when the user asks.

   The token is a password. It lives in a file of its own beside the settings file, readable by the user alone
   (the operator, 2026-11-21: "Own file, mode 600"), never in the ini, a log, ALL.TXT or an error message. */

#include <QByteArray>
#include <QDateTime>
#include <QString>

namespace club_log
{
  constexpr char const * real_api {"https://clublog.org/api/v1"};
  constexpr char const * file_name {"clublog_qsl.adi"};
  constexpr char const * api_env {"JTDX_CLUBLOG_API"};
  constexpr int refresh_hours {12};
  constexpr int max_wait_hours {24};   // (review 2026-11-21) the longest wait a Retry-After may impose: a day
  constexpr qint64 max_export_size {256 * 1024 * 1024};   // 46,549 QSOs are 11 MB
  constexpr int timeout_ms {120000};                       // Club Log builds the export before the first byte

  /* The API's address. JTDX_CLUBLOG_API is for tests and may only name a server on this machine,
     http://127.0.0.1:<port> or http://localhost:<port> (a path after it is kept); any other value gives an empty
     string, which switches the download off - so no setting of it reaches the real server or another one. */
  QString api_base ();

  /* base + "/logs/<call>/adif-export"; empty when base is empty or the call is not one (letters, digits and slashes:
     Club Log keeps a slash in the path, "/logs/VP8/G7VJR/qsos") */
  QString export_url (QString const& base, QString const& call);

  // "cl_" and the rest of the token's own characters - a pasted space or line break is not part of it
  bool plausible_token (QString const& token);

  // the token's file beside the settings file: JTDX.ini -> JTDX_clublog.token
  QString token_path (QString const& settings_file);
  /* the token in the file, or none: a byte-order mark in front (Notepad) is dropped, and what is not a token
     (plausible_token) is none - it would only reach a header and be refused there (review 2026-11-21) */
  QString read_token (QString const& path);
  // what the settings keep of a token Club Log refused: its SHA-256 in hex - never the token itself (review 2026-11-21)
  QString token_id (QString const& token);
  /* written readable and writable by the owner alone before the token goes in; an empty token removes the file.
     Returns an empty string, or what went wrong (never the token). */
  QString write_token (QString const& path, QString const& token);

  /* What a Club Log export holds. error: empty, or why the body is not an export (an error page, another file).
     confirmed: the QSL_RCVD Y records as one ADIF file, with a header naming its origin and time. */
  struct Export
  {
    QString error;
    QByteArray confirmed;
    int qsos {0};
    int confirmed_qsos {0};
    int entities {0};      // DXCC entities among the confirmed, by Club Log's numbers (for the status line)
  };
  Export read_export (QByteArray const& body, QDateTime const& downloaded);

  // an automatic download is due: none yet, or the last good one refresh_hours or more ago (or in the future)
  bool due (QDateTime const& last_success, QDateTime const& now);

  // how a failed download's HTTP status leaves the automatic ones
  enum class After {Retry, WaitRetryAfter, TokenRefused, CallRefused};   // CallRefused: 403, 422 - that call with that token
  After after_status (int http_status);   // 0: no answer from the server at all
  /* when the next automatic download may start after a failure: an hour later (the network was down, the server
     failed), at Club Log's Retry-After (an hour when it named none, never sooner than a minute nor later than
     max_wait_hours - a broken proxy's "years" must not stop the downloads for good), or never while the token is
     refused - until the user saves another */
  QDateTime next_try (After after, QDateTime const& retry_after, QDateTime const& now);
}

#endif
