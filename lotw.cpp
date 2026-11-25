// CE3TSK 2026-11-25 (TODO.md 3.-299): see lotw.h
#include "lotw.h"
#include "loginfile.h"

#include <QFile>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>

namespace lotw
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

  QString login_path (QString const& settings_file)
  {
    return login_file::path (settings_file, "lotw.login");
  }

  QString request_url (QString const& base, QString const& user, QString const& password, QString const& since,
                       QString const& own_call)
  {
    auto const e = [] (QString const& v) {return QString::fromLatin1 (QUrl::toPercentEncoding (v));};
    return base + "?login=" + e (user) + "&password=" + e (password) + "&qso_query=1&qso_qsl=yes&qso_qslsince="
      + e (since.isEmpty () ? QString::fromLatin1 (first_day) : since) + "&qso_owncall=" + e (own_call)
      + "&qso_withown=yes&qso_qsldetail=yes";
  }

  QString scrubbed (QString text, QString const& password)
  {
    // any address first: Qt's network errors name the request's, the password in its query
    static QRegularExpression const address {R"([A-Za-z][A-Za-z0-9+.-]*://\S*)"};
    text.replace (address, QStringLiteral ("LoTW"));
    if (!password.isEmpty ())
      {
        text.replace (password, QStringLiteral ("***"));
        text.replace (QString::fromLatin1 (QUrl::toPercentEncoding (password)), QStringLiteral ("***"));
      }
    return text;
  }

  namespace
  {
    // the fields of one ADIF record, by lower-case name; a value never runs into the next tag (as qrzlogbook.cpp's)
    QHash<QByteArray, QByteArray> fields_of (QByteArray const& record)
    {
      static QRegularExpression const tag {R"(<([A-Za-z0-9_]+):(\d+)(?::[^>]*)?>)"};
      QHash<QByteArray, QByteArray> f;
      QString const r {QString::fromLatin1 (record)};   // Latin-1: one character a byte, so the positions are the bytes'
      auto it = tag.globalMatch (r);
      QList<QRegularExpressionMatch> tags;
      while (it.hasNext ()) tags << it.next ();
      for (int i = 0; i < tags.size (); ++i)
        {
          auto const& m = tags[i];
          int const from = m.capturedEnd ();
          int const next = i + 1 < tags.size () ? tags[i + 1].capturedStart () : record.size ();
          int const length = qMin (m.captured (2).toInt (), next - from);
          f.insert (m.captured (1).toLower ().toLatin1 (), record.mid (from, qMax (0, length)));
        }
      return f;
    }

    // what is kept of a QSL: what the log's reader uses (the grid, the state and county for the US states) and the count
    char const * const kept_fields[] {"call", "station_callsign", "qso_date", "time_on", "band", "mode", "submode",
                                      "gridsquare", "vucc_grids", "state", "cnty", "dxcc", "qsl_rcvd", "qslrdate"};

    QByteArray kept_record (QHash<QByteArray, QByteArray> const& f)
    {
      QByteArray r;
      for (auto const* name : kept_fields)
        {
          auto const v = f.value (name).trimmed ();
          if (!v.isEmpty ()) r += '<' + QByteArray {name}.toUpper () + ':' + QByteArray::number (v.size ()) + '>' + v;
        }
      return r + "<EOR>\n";
    }

    // one QSO: its call, band, mode, submode, date, time and own call
    QByteArray key_of (QHash<QByteArray, QByteArray> const& f)
    {
      return f.value ("call").trimmed ().toUpper () + '|' + f.value ("band").trimmed ().toLower () + '|'
        + f.value ("mode").trimmed ().toUpper () + '|' + f.value ("submode").trimmed ().toUpper () + '|'
        + f.value ("qso_date").trimmed () + '|' + f.value ("time_on").trimmed () + '|'
        + f.value ("station_callsign").trimmed ().toUpper ();
    }

    QList<QByteArray> records_of (QByteArray const& adif)
    {
      static QRegularExpression const eor {R"(<eor>)", QRegularExpression::CaseInsensitiveOption};
      QList<QByteArray> out;
      for (auto const& r : QString::fromLatin1 (adif).split (eor)) out << r.toLatin1 ();
      return out;
    }

    bool confirmed (QHash<QByteArray, QByteArray> const& f)
    {
      auto const q = f.value ("qsl_rcvd").trimmed ().toUpper ();
      return "Y" == q || "V" == q;   // ADIF: Y yes, V verified
    }
  }

  Report read_report (QByteArray const& body)
  {
    Report r;
    auto const eoh = body.toUpper ().indexOf ("<EOH>");
    if (eoh < 0)
      {
        // LoTW's own way of saying no: an HTML page - a wrong username or password, or LoTW busy, not told apart
        r.reason = QStringLiteral ("no LoTW report");
        return r;
      }
    auto const head = fields_of (body.left (eoh));
    if (0 != QString::fromLatin1 (head.value ("programid").trimmed ()).compare ("LoTW", Qt::CaseInsensitive))
      {
        r.reason = QStringLiteral ("not a LoTW report");
        return r;
      }
    bool numrec_ok {false};
    r.numrec = head.value ("app_lotw_numrec").trimmed ().toInt (&numrec_ok);
    if (!numrec_ok || r.numrec < 0)
      {
        r.reason = QStringLiteral ("a LoTW report without its count");
        return r;
      }
    r.last_qsl = QString::fromLatin1 (head.value ("app_lotw_lastqsl").trimmed ());
    int records {0};
    for (auto const& record : records_of (body.mid (eoh + 5)))
      {
        auto const f = fields_of (record);
        if (f.value ("call").trimmed ().isEmpty ()) continue;   // what follows the last <EOR>
        ++records;
        if (confirmed (f)) r.records.insert (key_of (f), kept_record (f));
      }
    if (records != r.numrec)
      {
        r.reason = QString {"a LoTW report cut short: %1 of %2 QSOs"}.arg (records).arg (r.numrec);
        r.records.clear ();
        return r;
      }
    r.good = true;
    return r;
  }

  QMap<QByteArray, QByteArray> read_kept (QByteArray const& file)
  {
    QMap<QByteArray, QByteArray> kept;
    auto const eoh = file.toUpper ().indexOf ("<EOH>");
    for (auto const& record : records_of (eoh < 0 ? file : file.mid (eoh + 5)))
      {
        auto const f = fields_of (record);
        if (!f.value ("call").trimmed ().isEmpty () && confirmed (f)) kept.insert (key_of (f), kept_record (f));
      }
    return kept;
  }

  QByteArray write_kept (QMap<QByteArray, QByteArray> const& kept, QDateTime const& downloaded)
  {
    QByteArray out {"LoTW confirmations from LoTW (QSL_RCVD Y), downloaded " + downloaded.toUTC ().toString (Qt::ISODate).toLatin1 ()
                    + "\n<PROGRAMID:12>JTDX_contest\n<EOH>\n"};
    for (auto const& r : kept) out += r;
    return out;
  }

  void merge (QMap<QByteArray, QByteArray>& kept, QMap<QByteArray, QByteArray> const& more)
  {
    for (auto i = more.cbegin (); i != more.cend (); ++i)
      {
        auto const was = kept.constFind (i.key ());
        // YYYYMMDD: the text's order is the date's
        if (was == kept.cend () || fields_of (i.value ()).value ("qslrdate") >= fields_of (was.value ()).value ("qslrdate"))
          kept.insert (i.key (), i.value ());
      }
  }

  int entities (QMap<QByteArray, QByteArray> const& kept)
  {
    QSet<QByteArray> dxcc;
    for (auto const& r : kept)
      {
        auto const d = fields_of (r).value ("dxcc").trimmed ();
        if (!d.isEmpty () && "0" != d) dxcc.insert (d);
      }
    return dxcc.size ();
  }

  bool due (QDateTime const& last_success, QDateTime const& now)
  {
    return !last_success.isValid () || now < last_success || last_success.secsTo (now) >= qint64 {refresh_hours} * 3600;
  }

  bool full_due (QDateTime const& last_full, QDateTime const& now)
  {
    return !last_full.isValid () || now < last_full || last_full.secsTo (now) >= qint64 {full_days} * 86400;
  }

  Download::Download (QObject * parent)
    : QObject {parent}
  {
    connect (&download_, &FileDownload::complete, this, [this] (QString const&) {
        Q_EMIT finished (true, FileDownload::Failure::Network, QString {});
      });
    connect (&download_, &FileDownload::error, this, [this] (FileDownload::Failure failure, QString const& detail) {
        Q_EMIT finished (false, failure, scrubbed (detail, password_));   // never the address, never the password
      });
  }

  bool Download::running () const
  {
    return download_.running ();
  }

  void Download::start (QNetworkAccessManager * manager, QString const& user, QString const& password, QString const& own_call,
                        QString const& since, QString const& file, QString const& user_agent)
  {
    if (download_.running ()) return;
    file_ = file;
    password_ = password;
    full_ = since.isEmpty ();
    since_ = full_ ? QString::fromLatin1 (first_day) : since;
    last_qsl_.clear ();
    report_ = Report {};
    confirmed_ = entities_ = received_ = 0;
    started_ = QDateTime::currentDateTimeUtc ();
    FileDownload::Request r;
    // a header of its own: FileDownload then follows no redirect - the password is in the address
    r.headers = {{"Accept", "text/plain, application/octet-stream, */*"}};
    r.max_body_size = max_size;
    r.timeout_ms = timeout_ms;
    download_.configure (manager, request_url (api_base (), user, password, since_, own_call), file_, user_agent,
                         [this] (QByteArray const& body) -> QString {
                           report_ = read_report (body);
                           return report_.good ? QString {} : report_.reason;
                         },
                         r,
                         [this] (QByteArray const&) -> QByteArray {
                           // what is written: a full report alone, else its QSLs merged into the file there now
                           QMap<QByteArray, QByteArray> kept;
                           if (!full_)
                             {
                               QFile f {file_};
                               if (f.open (QIODevice::ReadOnly)) kept = read_kept (f.readAll ());
                             }
                           merge (kept, report_.records);
                           received_ = report_.records.size ();
                           confirmed_ = kept.size ();
                           entities_ = lotw::entities (kept);   // the namespace's: the member of that name hides it
                           last_qsl_ = report_.last_qsl.isEmpty () ? since_ : report_.last_qsl;   // no QSL since: the same again
                           return write_kept (kept, started_);
                         });
    download_.start_download ();
  }
}
