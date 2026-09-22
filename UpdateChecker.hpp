#ifndef UPDATE_CHECKER_HPP
#define UPDATE_CHECKER_HPP
/* CE3TSK 2026-09-22: "Check for updates" - Help menu, and a silent background check that only ever shows the
   update icon beside the Ko-fi cup (the operator's design). One GET of
       https://ce3tsk.com/download/latest.json
   on HttpFetch (the Fox verifier's transport: the shared QNetworkAccessManager, WinHTTP on Windows, which has no
   OpenSSL). The file is published with the installers (public/download/latest.json in the work tree). JSON on the
   operator's word - easier to extend and to check than a text file - with a version per platform if wanted:
       {
         "version": "3.0.0-rc08",              required unless every platform has its own; as version(true)
                                               prints it, 3.0.0 for a release, a leading "v" allowed
         "date": "2026-09-22",                 optional, for the reader of the file - not shown
         "changes": ["a short line", "..."],   optional, one line per change (a single string is accepted too)
         "url": "https://ce3tsk.com/download/",  optional, the download page (this is the default)
         "platforms": {                        optional; "windows", "linux", "macos" - each may carry its own
           "windows": {"version": "3.0.0-rc08", "changes": [...], "url": "..."},
           "linux":   {"version": "3.0.0-rc07"}
         }
       }
   What this build reads is its own platform's entry, key by key, falling back on the top level. A file that
   cannot be read - not JSON, or no readable version for this platform - is a FAILURE, never "an update".
   Test hook: the environment variable JTDX_UPDATE_URL replaces the address (a local server, to see the icon and
   the dialog before a version is published).
   No tr() here: the outcome is a kind plus data, MainWindow says it in the operator's language. */
#include <QObject>
#include <QString>
#include <QByteArray>

class QNetworkAccessManager;
class HttpFetch;

class UpdateChecker final
  : public QObject
{
  Q_OBJECT

public:
  enum class Trouble {Network, NoSsl, Timeout, TooLarge, HttpStatus, Unreadable};
  Q_ENUM (Trouble)

  struct Latest
  {
    QString version;
    QString changelog;   // "- line" per change, joined by newlines
    QString url;         // the download page
  };

  static char const * url ();        // https://ce3tsk.com/download/latest.json
  static char const * platform ();   // "windows", "linux" or "macos" - this build's key in "platforms"
  static char const * download_page ();   // https://ce3tsk.com/download/ - when the file names none

  UpdateChecker (QNetworkAccessManager *, QString const& user_agent, QObject * parent = nullptr);

  // a check; silent is handed back with the outcome (the background check says nothing but "update")
  void check (bool silent);
  bool running () const;

  // the pure parts, for the unit test
  static bool parse (QByteArray const& body, QString const& platform, Latest * latest);
  // -1 a < b, 0 equal, 1 a > b; false when either is not a version (major.minor.patch[-rcN], "v" allowed)
  static bool compare (QString const& a, QString const& b, int * result);

  Q_SIGNAL void newer (UpdateChecker::Latest const&, bool silent);
  Q_SIGNAL void current (UpdateChecker::Latest const&, bool silent);
  Q_SIGNAL void failed (UpdateChecker::Trouble, QString const& detail, bool silent);

private:
  HttpFetch * fetch_;
  QString user_agent_;
  bool silent_ {false};
};

#endif
