#include "tqsl.h"
// CE3TSK 2026-11-25 (TODO.md 3.-303, LOTW_PLAN.md section 5): TQSL's command line - tqsl.h has the rules

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QXmlStreamReader>

namespace tqsl
{
  QStringList candidates ()
  {
    QStringList c;
#if defined (Q_OS_WIN)
    for (auto const * env : {"ProgramFiles(x86)", "ProgramFiles"})
      if (qEnvironmentVariableIsSet (env)) c << QDir::fromNativeSeparators (qEnvironmentVariable (env)) + "/TrustedQSL/tqsl.exe";
#elif defined (Q_OS_MAC)
    auto const home = QDir::homePath ();
    for (auto const& top : {QString {"/Applications"}, home + "/Applications", home + "/Desktop", home + "/Applications/Ham Radio"})
      c << top + "/TrustedQSL/tqsl.app/Contents/MacOS/tqsl" << top + "/tqsl.app/Contents/MacOS/tqsl";
#else
    c << "/usr/bin/tqsl" << "/usr/local/bin/tqsl";
#endif
    auto const in_path = QStandardPaths::findExecutable ("tqsl");
    if (!in_path.isEmpty () && !c.contains (in_path)) c << in_path;
    return c;
  }

  bool runnable (QString const& path)
  {
    QFileInfo const f {path};
    return !path.isEmpty () && f.isFile () && f.isExecutable ();
  }

  bool switched_off ()
  {
    return qEnvironmentVariableIsSet (program_env) && qEnvironmentVariable (program_env).isEmpty ();
  }

  QString program (QString const& chosen)
  {
    if (qEnvironmentVariableIsSet (program_env))
      {
        auto const p = qEnvironmentVariable (program_env);
        return runnable (p) ? p : QString {};
      }
    if (runnable (chosen)) return chosen;
    for (auto const& c : candidates ()) if (runnable (c)) return c;
    return {};
  }

  int test_timeout_ms ()
  {
    bool ok {false};
    int const ms {qEnvironmentVariableIntValue ("JTDX_TQSL_TEST_MS", &ok)};
    return ok && ms > 0 ? ms : 30 * 1000;
  }

  QString program_in (QString const& chosen)
  {
    if (chosen.endsWith (".app", Qt::CaseInsensitive) && QFileInfo {chosen}.isDir ()) return chosen + "/Contents/MacOS/tqsl";
    return chosen;
  }

  QString station_file ()
  {
    // (3.-304) TQSL's own TQSLDIR first, on every system, as tqsllib's tqsl_init takes it
    if (!qEnvironmentVariable ("TQSLDIR").isEmpty ()) return QDir::fromNativeSeparators (qEnvironmentVariable ("TQSLDIR")) + "/station_data";
#if defined (Q_OS_WIN)
    for (auto const * env : {"APPDATA", "LOCALAPPDATA"})
      if (qEnvironmentVariableIsSet (env))
        {
          auto const f = QDir::fromNativeSeparators (qEnvironmentVariable (env)) + "/TrustedQSL/station_data";
          if (QFileInfo::exists (f)) return f;
        }
    return QDir::fromNativeSeparators (qEnvironmentVariable ("APPDATA")) + "/TrustedQSL/station_data";
#else
    return QDir::homePath () + "/.tqsl/station_data";
#endif
  }

  QList<QPair<QString, QString>> station_locations (QString const& file)
  {
    QList<QPair<QString, QString>> out;
    QFile f {file};
    if (!f.open (QIODevice::ReadOnly)) return out;
    QXmlStreamReader x {&f};
    QString name, call;
    bool in {false};
    while (!x.atEnd () && !x.hasError ())
      {
        x.readNext ();
        if (x.isStartElement () && x.name () == QLatin1String ("StationData"))
          {
            in = true;
            name = x.attributes ().value ("name").toString ();
            call.clear ();
          }
        else if (in && x.isStartElement () && x.name () == QLatin1String ("CALL"))
          call = x.readElementText ().trimmed ().toUpper ();
        else if (in && x.isEndElement () && x.name () == QLatin1String ("StationData"))
          {
            if (!name.isEmpty ()) out << qMakePair (name, call);
            in = false;
          }
      }
    return out;
  }

  QStringList arguments (QString const& location, QString const& file)
  {
    return {"-d", "-u", "-a", "all", "-x", "-l", location, file};   // ARRL's own example's order, `all` for `compliant`
  }

  namespace
  {
    QStringList lines_of (QByteArray const& output)
    {
      // each line without TQSL's time ("06:05:56 PM: ", "17:21:33 PM: /") - the words alone
      static QRegularExpression const time {R"(^\s*\d{1,2}:\d{2}:\d{2}(\s*[AP]M)?\s*:?\s*/?)"};
      QStringList out;
      for (auto l : QString::fromLocal8Bit (output).split (QRegularExpression {"[\r\n]+"}))
        {
          l.remove (time);
          l = l.trimmed ();
          if (!l.isEmpty ()) out << l;
        }
      return out;
    }
  }

  Status final_status (QByteArray const& output)
  {
    Status s;
    static QRegularExpression const final {R"(^Final Status:\s*(.*?)\s*\((\d+)\)$)"};
    auto const lines = lines_of (output);
    for (int i = lines.size () - 1; i >= 0; --i)
      {
        auto const m = final.match (lines[i]);
        if (!m.hasMatch ()) continue;
        s.code = m.captured (2).toInt ();
        s.text = m.captured (1);
        QStringList before;
        for (int k = i - 1; k >= 0 && before.size () < 2; --k) before.prepend (lines[k]);
        s.detail = before.join (" / ").left (300);
        break;
      }
    return s;
  }

  QString version (QByteArray const& output)
  {
    auto const lines = lines_of (output);
    return lines.isEmpty () ? QString {} : lines.last ().left (200);
  }
}
