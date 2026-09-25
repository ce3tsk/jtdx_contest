// last time modified by Arvo ES1JA on 20200127 (JTDX)
// and rewritten in places for JTDX_contest by Tihomir Sokcevic CE3TSK since 2025

#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>

#include <locale.h>

#include <QApplication>
#include <QIcon>        /* CE3TSK: the explicit application icon */
#include <QFile>        /* CE3TSK */
#include <QTranslator>
#include <QNetworkAccessManager>
#include <QRegularExpression>
#include <QObject>
#include <QSettings>
#include <QLibraryInfo>
#include <QLocale>     /* CE3TSK: first-run UI language from the operating system */
#include <QSysInfo>
#include <QDir>
#include <QStandardPaths>
#include <QStringList>
#include <QLockFile>

#if QT_VERSION >= 0x050200
#include <QCommandLineParser>
#include <QCommandLineOption>
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#include <QRandomGenerator>
#endif

#if defined (Q_OS_MAC)
#include <QProcess>
#include "mac_shared_memory.hpp"
#endif

#include "JTDXMessageBox.hpp"
#include "revision_utils.hpp"
#include "MetaDataRegistry.hpp"
#include "SettingsGroup.hpp"
#include "TraceFile.hpp"
#include "mainwindow.h"
#include "commons.h"
#include "lib/init_random_seed.h"

namespace
{
  /* CE3TSK: the UI language on a first run.

     JTDX used to start in English for everybody and stay there until the operator found the
     Language menu. QLocale::system() gives the operating system's language on all three
     platforms - Qt reads LC_ALL/LC_MESSAGES/LANG on Linux, GetUserDefaultLocaleName on Windows
     and NSLocale on macOS - so the sensible default is simply that, when we ship a catalogue
     for it. Only used when the ini has no Language key at all; once the key exists, whatever it
     says wins, including a deliberate "en_US".

     Matching is: exact tag first (so pt_BR, zh_CN and zh_HK land on themselves), then the
     language alone, mapped to the country variant we actually ship - a Chilean or Mexican
     operator gets es_ES, an Austrian de_DE, an Angolan pt_PT. Chinese is decided by script
     rather than country, because zh_TW and zh_MO ship no catalogue of their own but are
     traditional. Anything we have no catalogue for falls back to English. */
  QString default_ui_language ()
  {
    // the catalogues JTDX ships - keep in step with LANGUAGES in CMakeLists.txt
    static QStringList const shipped {
      "ca_ES", "da_DK", "de_DE", "en_US", "es_ES", "et_EE", "fr_FR", "hr_HR", "hu_HU", "it_IT",
      "ja_JP", "ko_KR", "lv_LV", "nl_NL", "pl_PL", "pt_BR", "pt_PT", "ru_RU", "sv_SE",
      "zh_CN", "zh_HK"};
    // the variant a bare language falls back to
    static QList<QPair<QString, QString>> const by_language {
      {"ca", "ca_ES"}, {"da", "da_DK"}, {"de", "de_DE"}, {"en", "en_US"}, {"es", "es_ES"},
      {"et", "et_EE"}, {"fr", "fr_FR"}, {"hr", "hr_HR"}, {"hu", "hu_HU"}, {"it", "it_IT"},
      {"ja", "ja_JP"}, {"ko", "ko_KR"}, {"lv", "lv_LV"}, {"nl", "nl_NL"}, {"pl", "pl_PL"},
      {"pt", "pt_PT"}, {"ru", "ru_RU"}, {"sv", "sv_SE"}};

    auto const system = QLocale::system ();
    auto const name = system.name ();               // "es_CL", "pt_BR", "zh_TW", "C" ...
    if (shipped.contains (name)) return name;

    if (QLocale::Chinese == system.language ())
      {
        return QLocale::TraditionalHanScript == system.script () ? "zh_HK" : "zh_CN";
      }

    auto const code = name.left (name.indexOf (QChar {'_'}));
    for (auto const& pair : by_language)
      {
        if (pair.first == code) return pair.second;
      }
    return "en_US";
  }

  struct RNGSetup
  {
    RNGSetup ()
    {
      // one time seed of pseudo RNGs from current time
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      QRandomGenerator();
#else
      auto seed = QDateTime::currentMSecsSinceEpoch ();
      qsrand (seed);            // this is good for rand() as well
#endif
    }
  } seeding;

  // We  can't use  the GUI  after QApplication::exit()  is called  so
  // uncaught exceptions can  get lost on Windows  systems where there
  // is    no    console    terminal,     so    here    we    override
  // QApplication::notify() and  wrap the base  class call with  a try
  // block to catch and display exceptions in a message box.
  class ExceptionCatchingApplication final
    : public QApplication
  {
  public:
    explicit ExceptionCatchingApplication (int& argc, char * * argv)
      : QApplication {argc, argv}
    {
    }
    bool notify (QObject * receiver, QEvent * e) override
    {
      try
        {
          return QApplication::notify (receiver, e);
        }
      catch (std::exception const& e)
        {
          JTDXMessageBox::critical_message (nullptr, "", translate ("main", "Fatal error"), e.what ());
          throw;
        }
      catch (...)
        {
          JTDXMessageBox::critical_message (nullptr, "", translate ("main", "Unexpected fatal error"));
          throw;
        }
    }
  };

#if defined (Q_OS_MAC)
  /* CE3TSK: macOS allows 4 MB of System V shared memory per segment, JTDX needs sizeof (dec_data),
     about 13 MB. The launch daemon com.jtdx.sysctl.plist raises the limits at every boot, but it
     used to be in effect only for users who copied it into /Library/LaunchDaemons by hand, with
     sudo, from the DMG - everybody else got "Unable to create shared memory segment" and nothing
     more. Now, when the limits are what is too small, JTDX offers to install it: macOS asks for an
     administrator password, a script compiled into this program (mac_shared_memory.hpp) writes
     the daemon and raises the limits at once, and JTDX goes on starting. True when that worked. */
  bool install_mac_shared_memory_setting (quint64 bytes)
  {
    auto const megabytes = QString::number ((bytes + 1048575u) / 1048576u);
    if (JTDXMessageBox::Yes != JTDXMessageBox::query_message (nullptr
            , QCoreApplication::translate ("main", "Shared memory")
            , QCoreApplication::translate ("main", "JTDX_contest needs %1 MB of shared memory, more than macOS allows by default.").arg (megabytes)
            , QCoreApplication::translate ("main", "It can raise the limit now, and at every start of this Mac, by installing a small "
                                           "system setting (/Library/LaunchDaemons/com.jtdx.sysctl.plist). macOS will ask for an "
                                           "administrator password.\n\nInstall the setting?")
            , QString {}, JTDXMessageBox::Yes | JTDXMessageBox::No, JTDXMessageBox::Yes))
      {
        return false;
      }
    // The script and the password dialog's text go to osascript as arguments, the script quoted by
    // AppleScript, so nothing in them needs escaping here. Without "with prompt" the dialog would
    // say "osascript wants to make changes" - no mention of JTDX_contest, which looks suspicious.
    QProcess osascript;
    osascript.start ("/usr/bin/osascript", {
        "-e", "on run argv"
      , "-e", "do shell script \"/bin/sh -c \" & quoted form of (item 1 of argv) with prompt (item 2 of argv) with administrator privileges"
      , "-e", "end run"
      , QString::fromStdString (mac_shared_memory::install_script ())
      , QCoreApplication::translate ("main", "JTDX_contest wants to install its shared memory setting.")});
    if (!osascript.waitForFinished (-1)) return false;   // no timeout: the user is typing a password
    if (osascript.exitStatus () == QProcess::NormalExit && osascript.exitCode () == 0) return true;
    auto const error = QString::fromLocal8Bit (osascript.readAllStandardError ()).trimmed ();
    if (!error.contains ("-128"))   // -128: the user cancelled the password prompt
      {
        JTDXMessageBox::warning_message (nullptr, QCoreApplication::translate ("main", "Shared memory")
            , QCoreApplication::translate ("main", "Installing the shared memory setting failed."), error);
      }
    return false;
  }
#endif
}

int main(int argc, char *argv[])
{
#if defined(Q_OS_WIN)
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
  }
#endif

  bool has_style = true;
  int result = 0;
  for (auto i = 0; i < argc; i++) {if (std::string(argv[i]).find("-style") != std::string::npos && std::string(argv[i]).find("-stylesheet") == std::string::npos) has_style = false;}

  init_random_seed ();

  register_types ();            // make the Qt magic happen

  // Multiple instances:
  QSharedMemory mem_jtdxjt9;

  auto const env = QProcessEnvironment::systemEnvironment ();

  ExceptionCatchingApplication a(argc, argv);
  if (has_style) a.setStyle("Fusion");
  try
    {

      setlocale (LC_NUMERIC, "C"); // ensure number forms are in
                                   // consistent format, do this after
                                   // instantiating QApplication so
                                   // that GUI has correct l18n

      // Override programs executable basename as application name.
      a.setApplicationName ("JTDX");
      a.setApplicationVersion (version ());

      /* CE3TSK: set the application icon explicitly. JTDX never called setWindowIcon, so Qt
         had to fall back on the desktop-entry lookup by WM_CLASS - which finds nothing when
         the program runs uninstalled, from a build directory, or under `-r NAME`, and the
         window then shows the toolkit's blank default in the task bar and the switcher. The
         icon is searched for in the icon theme first, then where CMake installs it. */
      {
        /* "jtdx_contest_icon", not "jtdx_icon": stock JTDX installs its own jtdx_icon into
           /usr/share/icons, and when both are present the theme lookup finds that one and this
           build shows the stock artwork. A distinct name keeps the two apart. It is also the
           name jtdx_contest.desktop declares and the name CMake installs under, so the theme lookup can
           only ever match ours. hasThemeIcon is the reliable existence test - fromTheme returns
           a non-null QIcon even when nothing matches. */
        QIcon base {QIcon::hasThemeIcon ("jtdx_contest_icon") ? QIcon::fromTheme ("jtdx_contest_icon") : QIcon {}};
        if (base.isNull ())
          {
            for (auto const& path : {QCoreApplication::applicationDirPath () + "/../share/pixmaps/jtdx_contest_icon.png",
                                     QString {"/usr/share/pixmaps/jtdx_contest_icon.png"}})
              if (QFile::exists (path)) { base = QIcon {path}; break; }
          }
        if (!base.isNull ()) a.setWindowIcon (base);
      }
  if (version().replace("_32A","").indexOf("_") > 1) {
    #include <QDate>
    auto expire_date = QLocale(QLocale::English).toDate(QString(__DATE__).replace("  "," "),"MMM d yyyy").addMonths(3);
    if (QDate().currentDate() > expire_date) {
      JTDXMessageBox::critical_message (nullptr, a.applicationName(), "Release candidate expired on " + expire_date.toString("dd.MM.yyyy"));
          return -1;
    } else if (QDate().currentDate().addDays(5) > expire_date) {
      JTDXMessageBox::information_message (nullptr, a.applicationName(), "Release candidate will expire on " + expire_date.toString("dd.MM.yyyy"));
    }
  }
      bool multiple {false};

#if QT_VERSION >= 0x050200
      QCommandLineParser parser;
      parser.setApplicationDescription ("\nFT8,JT65A,JT9 & T10 Weak Signal Communications Program.");
      auto help_option = parser.addHelpOption ();
      auto version_option = parser.addVersionOption ();

      // support for multiple instances running from a single installation
      QCommandLineOption style_option (QString {"style"}
                                     , a.translate ("main", "<style> can be Fusion (default) or Windows")
                                     , a.translate ("main" , "style"));
      parser.addOption (style_option);

      QCommandLineOption rig_option (QStringList {} << "r" << "rig-name"
                                     , a.translate ("main", "Where <rig-name> is for multi-instance support.")
                                     , a.translate ("main", "rig-name"));
      parser.addOption (rig_option);

      QCommandLineOption test_option (QStringList {} << "test-mode"
                                      , a.translate ("main", "Writable files in test location.  Use with caution, for testing only."));
      parser.addOption (test_option);

      if (!parser.parse (a.arguments ()))
        {
          JTDXMessageBox::critical_message (nullptr, a.applicationName (), parser.errorText ());
          return -1;
        }
      else
        {
          if (parser.isSet (help_option))
            {
              JTDXMessageBox::information_message (nullptr, a.applicationName (), parser.helpText ());
              return 0;
            }
          else if (parser.isSet (version_option))
            {
              JTDXMessageBox::information_message (nullptr, a.applicationName (), a.applicationVersion ());
              return 0;
            }
        }

      QStandardPaths::setTestModeEnabled (parser.isSet (test_option));

      // support for multiple instances running from a single installation
      if (parser.isSet (rig_option) || parser.isSet (test_option))
        {
          auto temp_name = parser.value (rig_option);
          if (!temp_name.isEmpty ())
            {
              if (temp_name.contains (QRegularExpression {R"([\\/,])"}))
                {
                  std::cerr << QObject::tr ("Invalid rig name - \\ & / not allowed").toLocal8Bit ().data () << std::endl;
                  parser.showHelp (-1);
                }
                
              a.setApplicationName (a.applicationName () + " - " + temp_name);
            }

          if (parser.isSet (test_option))
            {
              a.setApplicationName (a.applicationName () + " - test");
            }

          multiple = true;
        }

      // disallow multiple instances with same instance key
      QLockFile instance_lock {QDir {QStandardPaths::writableLocation (QStandardPaths::TempLocation)}.absoluteFilePath (a.applicationName () + ".lock")};
      instance_lock.setStaleLockTime (0);
      while (!instance_lock.tryLock ())
        {
          if (QLockFile::LockFailedError == instance_lock.error ())
            {
              auto button = JTDXMessageBox::query_message (nullptr
                                                   , QApplication::applicationName ()
                                                   , QObject::tr ("Another instance may be running, try to remove stale lock file?")
                                                   , "" ,""
                                                   , JTDXMessageBox::Yes | JTDXMessageBox::Retry | JTDXMessageBox::No
                                                   , JTDXMessageBox::Yes);
              switch (button)
                {
                case JTDXMessageBox::Yes:
                  instance_lock.removeStaleLockFile ();
                  break;

                case JTDXMessageBox::Retry:
                  break;

                default:
                  throw std::runtime_error {"Multiple instances must have unique rig names"};
                }
            }
          else
            {
              throw std::runtime_error {"Failed to access lock file"};
            }
        }
#endif

      auto config_directory = QStandardPaths::writableLocation (QStandardPaths::ConfigLocation);
      QDir config_path {config_directory}; // will be "." if config_directory is empty
      if (!config_path.mkpath ("."))
        {
          throw std::runtime_error {"Cannot find a usable configuration path \"" + config_path.path ().toStdString () + '"'};
        }

      auto settings_file = config_path.absoluteFilePath (a.applicationName () + ".ini");
      QSettings settings(settings_file, QSettings::IniFormat);
      if (!settings.isWritable ())
        {
          throw std::runtime_error {QString {"Cannot access \"%1\" for writing"}.arg (settings_file).toStdString ()};
        }

#if WSJT_QDEBUG_TO_FILE
      // // open a trace file
      TraceFile trace_file {QDir {QStandardPaths::writableLocation (QStandardPaths::TempLocation)}.absoluteFilePath (a.applicationName () + "_trace.log")};

      // announce to trace file
      qDebug () << program_title (revision ()) + " - Program startup";
#endif
      QString lang;
      QLocale localeUsedToDeterminateTranslators;
      QTranslator translator_from_qt;
      QTranslator translator_from_resources;
      QTranslator translator_from_files;
      bool qt_OK = false;
      bool resources_OK = false;
      bool files_OK = false;
      /* CE3TSK: this used to be `do { ... } while (result == 1337)` - the language menu set
         exit code 1337 and the whole GUI was torn down and rebuilt inside the one process.
         That re-zeroed the decode shared memory, restarted jtdxjt9, emptied both decode
         windows, and re-enumerated the rigs mid-session, which lost the configured rig when
         hamlib had renamed its entry. A language change now just closes the program and the
         operator starts it again, so this block runs exactly once. */
      {
        settings.beginGroup("Common");
        /* CE3TSK: no Language key means this ini has never chosen one - take the operating
           system's language and write it down, so the next start and the Language menu both
           see a settled value. An empty key counts as never chosen too. A key that is present
           is never second-guessed. */
        if (settings.value ("Language").toString ().isEmpty ())
          {
            settings.setValue ("Language", default_ui_language ());
          }
        lang = settings.value ("Language","en_US").toString();
        settings.endGroup();
        if (files_OK) has_style = a.removeTranslator (&translator_from_files);
        if (resources_OK) has_style = a.removeTranslator (&translator_from_resources);
        if (qt_OK) has_style = a.removeTranslator (&translator_from_qt);
        if (lang != "en_US") {
          //
          // Enable i18n
          //
          localeUsedToDeterminateTranslators = QLocale (lang);
          
          /* load the system translations provided by Qt Currently None useable*/
          has_style = translator_from_qt.load("qt_" + localeUsedToDeterminateTranslators.name(),QLibraryInfo::location(QLibraryInfo::TranslationsPath));
          if (has_style) {
              qt_OK = a.installTranslator(&translator_from_qt);
          }

          // Default translations for releases  use translations stored in
          // the   resources   file    system   under   the   Translations
          // directory. These are built by the CMake build system from .ts
          // files in the translations source directory. New languages are
          // added by  enabling the  UPDATE_TRANSLATIONS CMake  option and
          // building with the  new language added to  the LANGUAGES CMake
          // list  variable.  UPDATE_TRANSLATIONS  will preserve  existing
          // translations  but   should  only  be  set   when  adding  new
          // languages.  The  resulting .ts  files should be  checked info
          // source control for translators to access and update.
          has_style = translator_from_resources.load (localeUsedToDeterminateTranslators, "jtdx", "_", ":/Translations");
          if (has_style) {
              resources_OK = a.installTranslator (&translator_from_resources);
          } 

          // Load  any matching  translation  from  the current  directory
          // using the locale name. This allows translators to easily test
          // their translations  by releasing  (lrelease) a .qm  file into
          // the    current    directory     with    a    suitable    name
          // (e.g.  jtdx_et_EE.qm),  then  running   wsjtx  to  view  the
          // results. Either the system  locale setting or the environment
          // variable LANG can be used to select the target language.
          has_style = translator_from_files.load (QString {"jtdx_"} + localeUsedToDeterminateTranslators.name ());
          if (has_style) {
              files_OK = a.installTranslator (&translator_from_files);
          }
        }
        // Create and initialize shared memory segment
        // Multiple instances: use rig_name as shared memory key

        mem_jtdxjt9.setKey(a.applicationName ());

        // CE3TSK: a segment left over by a crashed run - of stock JTDX, whose key "JTDX" is the same,
        // or of an older JTDX_contest - can be smaller than this program's dec_data (this fork added
        // nsftol), and the memset below would write past its end. Let it go instead: Qt removes it
        // when nobody else is attached any more, and a fresh one is created below. Should another
        // process still hold it, that create fails and says so.
        if (mem_jtdxjt9.attach () && mem_jtdxjt9.size () < static_cast<int> (sizeof (struct dec_data))) {
          mem_jtdxjt9.detach ();
        }
        if (!mem_jtdxjt9.isAttached ()) {
          bool created = mem_jtdxjt9.create(sizeof(struct dec_data));
#if defined (Q_OS_MAC)
          // CE3TSK: offer to raise the macOS limits, see install_mac_shared_memory_setting
          // ... and only when the limits are what stopped it: a segment another process still
          // holds fails with AlreadyExists, which no system setting can help (review 2026-09-25)
          if (!created && mem_jtdxjt9.error () != QSharedMemory::AlreadyExists
              && mac_shared_memory::limits_too_small (sizeof (struct dec_data))
              && install_mac_shared_memory_setting (sizeof (struct dec_data))) {
            created = mem_jtdxjt9.create(sizeof(struct dec_data));
          }
#endif
          if (!created) {
#if defined (Q_OS_MAC)
            JTDXMessageBox::critical_message (nullptr, QCoreApplication::translate ("main", "Error")
                , QCoreApplication::translate ("main", "Unable to create shared memory segment.")
                , mem_jtdxjt9.error () != QSharedMemory::AlreadyExists
                  && mac_shared_memory::limits_too_small (sizeof (struct dec_data))
                  ? QCoreApplication::translate ("main", "macOS allows too little shared memory. To raise the limit permanently, copy "
                                                 "com.jtdx.sysctl.plist from the JTDX_contest installer (DMG) to "
                                                 "/Library/LaunchDaemons and restart this Mac - see ReadMe.txt in the DMG.")
                  : QString {}
                , mem_jtdxjt9.errorString ());
#else
            JTDXMessageBox::critical_message (nullptr, QCoreApplication::translate ("main", "Error")
                , QCoreApplication::translate ("main", "Unable to create shared memory segment."));
#endif
            exit(1);
          }
        }
        memset(mem_jtdxjt9.data(),0,sizeof(struct dec_data)); //Zero all decoding params in shared memory

        unsigned downSampleFactor;
        {
          SettingsGroup {&settings, "Tune"};

          // deal with Windows Vista and earlier input audio rate
          // converter problems
          downSampleFactor = settings.value ("Audio/DisableInputResampling",
  #if defined (Q_OS_WIN)
                                             // default to true for
                                             // Windows Vista and older
                                             QSysInfo::WV_VISTA >= QSysInfo::WindowsVersion ? true : false
  #else
                                             false
  #endif
                                             ).toBool () ? 1u : 4u;
        }

        MainWindow w(multiple, &settings, &mem_jtdxjt9, downSampleFactor, new QNetworkAccessManager {&a}, env);
        w.show();

        QObject::connect (&a, SIGNAL (lastWindowClosed()), &a, SLOT (quit()));
        result = a.exec();
      }
      return result;
    }
  catch (std::exception const& e)
    {
      JTDXMessageBox::critical_message (nullptr, a.applicationName (), e.what ());
      std::cerr << "Error: " << e.what () << '\n';
    }
  catch (...)
    {
      JTDXMessageBox::critical_message (nullptr, a.applicationName (), QObject::tr ("Unexpected error"));
      std::cerr << "Unexpected error\n";
      throw;			// hoping the runtime might tell us more about the exception
    }
  return -1;
}
