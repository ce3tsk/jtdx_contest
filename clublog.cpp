// CE3TSK 2026-11-21 (TODO.md 3.-280): see clublog.h
#include "clublog.h"
#include "obfuscated.h"   // (3.-288) the token's file

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>

#include "logfields.h"   // adif_field: the reader ADIF::load uses

namespace club_log
{
  QString api_base ()
  {
    if (!qEnvironmentVariableIsSet (api_env)) return QString::fromLatin1 (real_api);
    QUrl const url {qEnvironmentVariable (api_env), QUrl::StrictMode};
    auto const host = url.host ().toLower ();
    if (!url.isValid () || "http" != url.scheme () || !url.userInfo ().isEmpty () || url.port () <= 0
        || url.hasQuery () || url.hasFragment () || ("127.0.0.1" != host && "localhost" != host))
      return {};
    auto base = url.toString ();
    while (base.endsWith ('/')) base.chop (1);
    return base;
  }

  QString export_url (QString const& base, QString const& call)
  {
    auto const c = call.trimmed ().toUpper ();
    // \A and \z: "$" would also match before a line break at the end (the test found it)
    static QRegularExpression const callsign {R"(\A[A-Z0-9]+(/[A-Z0-9]+)*\z)"};
    if (base.isEmpty () || !callsign.match (c).hasMatch ()) return {};
    return base + "/logs/" + c + "/adif-export";
  }

  bool plausible_token (QString const& token)
  {
    // \A and \z, not ^ and $: "$" matches before a final line break too, and a line break must never reach a header
    static QRegularExpression const shape {R"(\Acl_[A-Za-z0-9_-]{16,256}\z)"};
    return shape.match (token).hasMatch ();
  }

  QString token_path (QString const& settings_file)
  {
    QFileInfo const ini {settings_file};
    return ini.absoluteDir ().absoluteFilePath (ini.completeBaseName () + "_clublog.token");
  }

  QString read_token (QString const& path)
  {
    // (3.-288) the owner's alone first, decoded, an older program's plain file saved obfuscated (obfuscated::read)
    QByteArray const content {obfuscated::read (path)};
    QByteArray line {content.left (1024)};
    if (line.indexOf ('\n') >= 0) line.truncate (line.indexOf ('\n') + 1);
    if (line.startsWith ("\xEF\xBB\xBF")) line.remove (0, 3);   // UTF-8 byte-order mark: Notepad writes one
    auto const token = QString::fromLatin1 (line).trimmed ();
    return plausible_token (token) ? token : QString {};
  }

  QString token_id (QString const& token)
  {
    return QString::fromLatin1 (QCryptographicHash::hash (token.toUtf8 (), QCryptographicHash::Sha256).toHex ());
  }

  QString write_token (QString const& path, QString const& token)
  {
    QFile f {path};
    if (token.isEmpty ())
      {
        if (f.exists () && !f.remove ()) return f.errorString ();
        return {};
      }
    // (3.-288) the owner's alone before the token goes in, never in clear text; a failed save leaves the old file
    return obfuscated::save (path, token.toLatin1 () + '\n');
  }

  Export read_export (QByteArray const& body, QDateTime const& downloaded)
  {
    Export e;
    // positions found in an upper-case copy: Latin-1, so the same length as the body
    QByteArray const upper {body.toUpper ()};
    auto const eoh = upper.indexOf ("<EOH>");
    if (eoh < 0 || !upper.left (eoh).contains ("<PROGRAMID:8>CLUB LOG"))
      {
        e.error = "not a Club Log ADIF export";
        return e;
      }
    QByteArray records;
    QSet<QString> entities;
    int from = eoh + 5;
    for (;;)
      {
        auto const eor = upper.indexOf ("<EOR>", from);
        if (eor < 0) break;
        QByteArray const record {body.mid (from, eor - from).trimmed ()};
        from = eor + 5;
        QString const fields {QString::fromLatin1 (record)};
        if (adif_field (fields, "CALL:").isEmpty ()) continue;
        ++e.qsos;
        auto const qsl = adif_field (fields, "QSL_RCVD:").toUpper ();
        if ("Y" != qsl && "V" != qsl) continue;   // ADIF: Y yes, V verified; N, R and I are not confirmations
        ++e.confirmed_qsos;
        records += record + "<EOR>\n";
        auto const dxcc = adif_field (fields, "DXCC:");
        if (!dxcc.isEmpty () && "0" != dxcc) entities.insert (dxcc);
      }
    e.entities = entities.size ();
    e.confirmed = "LoTW-confirmed QSOs from Club Log (QSL_RCVD Y), downloaded "
      + downloaded.toUTC ().toString (Qt::ISODate).toLatin1 () + "\n<PROGRAMID:12>JTDX_contest\n<EOH>\n" + records;
    return e;
  }

  bool due (QDateTime const& last_success, QDateTime const& now)
  {
    return !last_success.isValid () || now < last_success || last_success.secsTo (now) >= qint64 {refresh_hours} * 3600;
  }

  After after_status (int http_status)
  {
    switch (http_status)
      {
      case 401: return After::TokenRefused;
      case 403: case 422: return After::CallRefused;   // 2026-11-22: "the callsign ... does not belong to the account" (403)
      case 429: case 503: return After::WaitRetryAfter;
      default: return After::Retry;
      }
  }

  QDateTime next_try (After after, QDateTime const& retry_after, QDateTime const& now)
  {
    switch (after)
      {
      case After::TokenRefused: case After::CallRefused: return {};
      case After::WaitRetryAfter:
        if (retry_after.isValid ()) return qMin (qMax (retry_after, now.addSecs (60)), now.addSecs (qint64 {max_wait_hours} * 3600));
        return now.addSecs (3600);
      case After::Retry: break;
      }
    return now.addSecs (3600);
  }
}
