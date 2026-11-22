// CE3TSK 2026-11-22 (TODO.md 3.-280): see qrzlogbook.h
#include "qrzlogbook.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QHash>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QTimer>
#include <QUrl>

namespace qrz_logbook
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

  bool plausible_key (QString const& key)
  {
    // \A and \z, not ^ and $: "$" matches before a final line break too, and a line break must never reach a request
    static QRegularExpression const shape {R"(\A[A-Za-z0-9]{4}(-?[A-Za-z0-9]{4}){3}\z)"};
    return shape.match (key).hasMatch ();
  }

  QString key_path (QString const& settings_file)
  {
    QFileInfo const ini {settings_file};
    return ini.absoluteDir ().absoluteFilePath (ini.completeBaseName () + "_qrz.key");
  }

  QString read_key (QString const& path)
  {
    QFile f {path};
    if (!f.open (QIODevice::ReadOnly)) return {};
    // a file made by hand, readable by others: made the user's alone before anything is taken from it
    auto const others = QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ReadOther | QFileDevice::WriteOther;
    if (f.permissions () & others) f.setPermissions (QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    auto line = f.readLine (1024);
    if (line.startsWith ("\xEF\xBB\xBF")) line.remove (0, 3);   // UTF-8 byte-order mark: Notepad writes one
    auto const key = QString::fromLatin1 (line).trimmed ();
    return plausible_key (key) ? key : QString {};
  }

  QByteArray request_body (QString const& key, QString const& option)
  {
    return "KEY=" + QUrl::toPercentEncoding (key) + "&ACTION=FETCH&OPTION=" + QUrl::toPercentEncoding (option);
  }

  QString full_option (qint64 after_logid)
  {
    return QString {"STATUS:CONFIRMED,MAX:%1,AFTERLOGID:%2"}.arg (page_size).arg (after_logid);
  }

  QString changes_option (QDate const& since, qint64 after_logid)
  {
    return QString {"MODSINCE:%1,MAX:%2,AFTERLOGID:%3"}.arg (since.toString ("yyyy-MM-dd")).arg (page_size).arg (after_logid);
  }

  namespace
  {
    // the fields of one ADIF record, by lower-case name; a value never runs into the next tag (a length in bytes or in
    // characters gives the same for the plain ASCII fields kept)
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

    // what is kept of a record: what ADIF::load reads, the LoTW confirmation, QRZ's logid and DXCC (for the count)
    char const * const kept_fields[] {"call", "station_callsign", "qso_date", "time_on", "band", "mode", "submode",
                                      "gridsquare", "dxcc", "lotw_qsl_rcvd", "lotw_qslrdate", "app_qrzlog_logid"};

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

    QList<QByteArray> records_of (QByteArray const& adif)
    {
      static QRegularExpression const eor {R"(<eor>)", QRegularExpression::CaseInsensitiveOption};
      QList<QByteArray> out;
      for (auto const& r : QString::fromLatin1 (adif).split (eor)) out << r.toLatin1 ();
      return out;
    }

    QByteArray unescaped (QByteArray s)
    {
      // as PHP's htmlspecialchars escapes them; &amp; last, so "&amp;lt;" stays "&lt;"
      s.replace ("&lt;", "<").replace ("&gt;", ">").replace ("&quot;", "\"").replace ("&#039;", "'").replace ("&#39;", "'");
      return s.replace ("&amp;", "&");
    }
  }

  Page read_page (QByteArray const& body)
  {
    Page p;
    // name=value pairs: a '&' before NAME= starts the next one - inside the ADIF a '&' only ever starts an entity
    static QRegularExpression const pair_start {R"(&(?=[A-Z_]+=))"};
    QHash<QString, QByteArray> pairs;
    for (auto const& part : QString::fromLatin1 (body).split (pair_start))
      {
        auto const eq = part.indexOf ('=');
        if (eq > 0) pairs.insert (part.left (eq).trimmed ().toUpper (), part.mid (eq + 1).toLatin1 ());
      }
    auto const result = QString::fromLatin1 (pairs.value ("RESULT")).trimmed ().toUpper ();
    p.reason = QString::fromUtf8 (pairs.value ("REASON")).trimmed ();
    if ("AUTH" == result)
      {
        p.kind = Page::Refused;
        return p;
      }
    if ("FAIL" == result)
      {
        // a page past the last record: RESULT=FAIL&COUNT=0 and no reason (seen 2026-11-22)
        p.kind = p.reason.isEmpty () && 0 == pairs.value ("COUNT").trimmed ().toInt () ? Page::End : Page::Error;
        return p;
      }
    if ("OK" != result)
      {
        p.kind = Page::Error;
        p.reason = "not a QRZ.com Logbook API answer";
        return p;
      }
    for (auto const& record : records_of (unescaped (pairs.value ("ADIF"))))
      {
        auto const f = fields_of (record);
        bool id_ok {false};
        qint64 const id {f.value ("app_qrzlog_logid").trimmed ().toLongLong (&id_ok)};
        if (f.value ("call").trimmed ().isEmpty () || !id_ok) continue;
        ++p.records;
        p.last_logid = qMax (p.last_logid, id);
        auto const lotw = f.value ("lotw_qsl_rcvd").trimmed ().toUpper ();
        if ("Y" == lotw || "V" == lotw) p.lotw.insert (id, kept_record (f));   // ADIF: Y yes, V verified
        else p.others << id;
      }
    p.kind = p.records ? Page::Records : Page::End;
    return p;
  }

  QMap<qint64, QByteArray> read_kept (QByteArray const& file)
  {
    QMap<qint64, QByteArray> kept;
    auto const eoh = file.toUpper ().indexOf ("<EOH>");
    for (auto const& record : records_of (eoh < 0 ? file : file.mid (eoh + 5)))
      {
        auto const f = fields_of (record);
        bool id_ok {false};
        qint64 const id {f.value ("app_qrzlog_logid").toLongLong (&id_ok)};
        auto const lotw = f.value ("lotw_qsl_rcvd").toUpper ();
        if (!f.value ("call").isEmpty () && id_ok && ("Y" == lotw || "V" == lotw)) kept.insert (id, kept_record (f));
      }
    return kept;
  }

  QByteArray write_kept (QMap<qint64, QByteArray> const& kept, QDateTime const& downloaded)
  {
    QByteArray out {"LoTW-confirmed QSOs from QRZ.com (LOTW_QSL_RCVD Y), downloaded "
                    + downloaded.toUTC ().toString (Qt::ISODate).toLatin1 () + "\n<PROGRAMID:12>JTDX_contest\n<EOH>\n"};
    for (auto const& r : kept) out += r;
    return out;
  }

  int entities (QMap<qint64, QByteArray> const& kept)
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

  Fetch::Fetch (QObject * parent)
    : QObject {parent}
  {
    connect (&page_, &FileDownload::complete, this, [this] (QString const&) {if (running_) page_done ();});
    connect (&page_, &FileDownload::error, this, [this] (FileDownload::Failure failure, QString const& detail) {
        if (!running_) return;   // aborted
        if (FileDownload::Failure::HttpStatus == failure && (401 == detail.toInt () || 403 == detail.toInt ())) refused_ = true;
        fail (failure, detail);
      });
  }

  void Fetch::start (QNetworkAccessManager * manager, QString const& key, bool full, QDate const& since,
                     QString const& file, QString const& user_agent)
  {
    if (running_) return;
    manager_ = manager;
    key_ = key;
    file_ = file;
    user_agent_ = user_agent;
    since_ = since;
    full_ = full || !since.isValid ();
    kept_.clear ();
    if (!full_)
      {
        QFile f {file_};
        if (f.open (QIODevice::ReadOnly)) kept_ = read_kept (f.readAll ());
        else full_ = true;   // nothing to apply the changes to
      }
    started_ = QDateTime::currentDateTimeUtc ();
    after_ = 0;
    pages_ = 0;
    entities_ = 0;
    refused_ = false;
    running_ = true;
    request ();
  }

  void Fetch::abort ()
  {
    if (!running_) return;
    running_ = false;   // nothing more is signalled for this start
    page_.abort ();
    kept_.clear ();
    QFile::remove (file_ + ".part");
  }

  void Fetch::request ()
  {
    FileDownload::Request r;
    r.method = "POST";
    // a header of its own: FileDownload then follows no redirect, and the key in the body goes nowhere else
    r.headers = {{"Content-Type", "application/x-www-form-urlencoded"}};
    r.body = request_body (key_, full_ ? full_option (after_) : changes_option (since_, after_));
    r.max_body_size = max_page_size;
    r.timeout_ms = timeout_ms;
    page_.configure (manager_, api_base (), file_ + ".part", user_agent_,
                     [this] (QByteArray const& body) -> QString {
                       last_ = read_page (body);
                       if (Page::Refused == last_.kind) refused_ = true;
                       if (Page::Records == last_.kind || Page::End == last_.kind) return {};
                       return last_.reason.isEmpty () ? QString {"RESULT=AUTH"} : last_.reason;
                     },
                     r,
                     [this] (QByteArray const&) {return "page " + QByteArray::number (pages_ + 1) + '\n';});   // the pages stay in memory
    page_.start_download ();
  }

  void Fetch::page_done ()
  {
    ++pages_;
    if (Page::Records == last_.kind)
      {
        if (last_.last_logid < after_)
          {
            fail (FileDownload::Failure::Rejected, "QRZ.com's pages did not advance");
            return;
          }
        for (auto i = last_.lotw.cbegin (); i != last_.lotw.cend (); ++i) kept_.insert (i.key (), i.value ());
        for (auto const id : last_.others) kept_.remove (id);   // a change: no longer LoTW-confirmed
        after_ = last_.last_logid + 1;
        if (last_.records >= page_size)
          {
            if (pages_ >= max_pages)
              {
                fail (FileDownload::Failure::Rejected, QString {"more than %1 pages"}.arg (max_pages));
                return;
              }
            QTimer::singleShot (page_gap_ms, this, [this] {if (running_) request ();});
            return;
          }
      }
    // the last page: the file replaced at once, and only now
    QFile::remove (file_ + ".part");
    entities_ = qrz_logbook::entities (kept_);   // the namespace's: the member of that name hides it
    QByteArray const content {write_kept (kept_, started_)};
    QSaveFile out {file_};
    if (!out.open (QIODevice::WriteOnly) || out.write (content) != content.size () || !out.commit ())
      {
        fail (FileDownload::Failure::Write, out.errorString ());
        return;
      }
    running_ = false;
    Q_EMIT finished (true, FileDownload::Failure::Network, QString {});
  }

  void Fetch::fail (FileDownload::Failure failure, QString const& detail)
  {
    running_ = false;
    kept_.clear ();
    QFile::remove (file_ + ".part");
    Q_EMIT finished (false, failure, detail);
  }
}
