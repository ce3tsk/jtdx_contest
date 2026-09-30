#ifndef CONTESTPROFILE_H
#define CONTESTPROFILE_H

// CE3TSK 2026-09-29: JTDX_contest's notification colours kept apart from stock JTDX's, and a copy of
// a stock profile taken before JTDX_contest first writes to it.
//
// Both programs are called JTDX, so they share one ini - with the same 34 notification colour keys
// and the same UseDarkStyle. Users who took the recommended colours on the one-time offer (rc04 to
// rc09) found their stock JTDX colours gone. Now JTDX_contest keeps its own copy of those keys in
// [Configuration] under JTDX_contest\ and never writes the shared ones, so a stock JTDX run and a
// JTDX_contest run each find their own colours and their own style.
//
// Where that copy starts from, the first time a profile meets this version:
//   own              - the copy is there already: it is read
//   earlier_contest  - an earlier JTDX_contest has used the profile, so the shared keys hold what that
//                      version showed - the recommended set if the offer was taken, the operator's own
//                      colours otherwise: they are copied, and nothing changes on screen
//   first_run        - JTDX_contest has never run on the profile (a stock JTDX profile, or none at
//                      all): the recommended colours and the dark style, with no question asked
//
// On a first run main.cpp copies the ini file before anything is saved in it (backup_first_run), and
// does not start without that copy (the operator, 2026-09-30: always, before any change is saved).
//
// "Has used" is read from keys stock JTDX has never written: ContestFrequencies (written on every
// exit since rc02), RecommendedColorsOffered (since rc04), SpecialOpActivity, and the older
// Common/WWDigiContest. The same test decides the copy of the ini file that main.cpp takes on a
// first run, so the two can never disagree about which run is the first.

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QSettings>
#include <QString>
#include "JTDXMessageBox.hpp"

namespace contest_profile
{
  // a subgroup of [Configuration]. A function, not a namespace-scope QString: that would be one object
  // per translation unit, which the inline functions below would then each name - an ODR violation
  // before C++17's inline variables (this build is C++11)
  inline QString own_group () { return QStringLiteral ("JTDX_contest"); }

  inline QString own_key (QString const& key) { return own_group () + '/' + key; }

  enum class Origin {own, earlier_contest, first_run};

  // `s` must be at the top level, inside no group
  inline Origin origin (QSettings const& s)
  {
    if (s.contains ("Configuration/" + own_key ("UseDarkStyle"))) return Origin::own;   // written with every colour
    for (auto const * key : {"Configuration/ContestFrequencies", "Configuration/RecommendedColorsOffered",
                             "Configuration/SpecialOpActivity", "Common/WWDigiContest"})
      {
        if (s.contains (key)) return Origin::earlier_contest;
      }
    return Origin::first_run;
  }

  // "JTDX.ini" -> "JTDX_original_20260929_225900.ini", beside it; "JTDX - G90.ini" (-r G90) likewise.
  // The digits are ASCII whatever the locale: QDateTime::toString (format) writes the system locale's
  // own digits in Qt 5 - "JTDX_original_۲۰۲۶۰۹۲۹_۲۲۵۹۰۰.ini" under fa_IR (review 2026-09-30).
  inline QString backup_path (QString const& ini, QDateTime const& when)
  {
    QFileInfo const file {ini};
    return file.dir ().filePath (file.completeBaseName () + "_original_"
                                 + QLocale::c ().toString (when, "yyyyMMdd_HHmmss") + '.' + file.suffix ());
  }

  enum class Backup {not_needed, made, failed};

  // the copy's file name, written into the ini the moment the copy exists: the first run is decided by
  // the own colour keys, which Configuration writes much later, and a start that ends before them (a
  // shared memory segment that cannot be made) would otherwise copy the file again on every retry. It
  // also tells whoever reads the ini where the copy went.
  inline QString copy_key () { return "Configuration/" + own_key ("OriginalCopy"); }

  // Copy the ini file as it is, before this program writes a single key, when it is a profile
  // JTDX_contest has never used and has never copied. `s` is the QSettings open on `ini`, at the top
  // level. On failure `error` says why; a copy of that name already there is a failure too, since
  // QFile::copy never overwrites. A copy that was made is recorded in the ini (copy_key) and synced at
  // once - and a record that cannot be written is a failure as well, with the copy taken back, since
  // the next start would otherwise copy again (review 2026-09-30).
  // A missing or empty file is a fresh install - nothing to keep, and that is recorded too (an empty
  // copy_key): a first start that ends before Configuration has run leaves this program's own first
  // keys behind (the Language seed), and the next start must not take them for a stock profile.
  inline Backup backup_first_run (QSettings& s, QString const& ini, QDateTime const& when,
                                  QString * copy, QString * error)
  {
    if (s.contains (copy_key ()) || Origin::first_run != origin (s)) return Backup::not_needed;
    QFileInfo const file {ini};
    if (!file.exists () || !file.size ())
      {
        s.setValue (copy_key (), QString {});
        s.sync ();
        return Backup::not_needed;
      }
    *copy = backup_path (ini, when);
    QFile source {ini};
    if (!source.copy (*copy))
      {
        *error = source.errorString ();
        return Backup::failed;
      }
    s.setValue (copy_key (), QFileInfo {*copy}.fileName ());
    s.sync ();
    if (QSettings::NoError != s.status ())
      {
        QFile::remove (*copy);
        s.remove (copy_key ());
        *error = QCoreApplication::translate ("main", "The copy could not be recorded in the settings file.");
        return Backup::failed;
      }
    return Backup::made;
  }

  // a button's label as the operator sees it, without its accelerator ("&Retry" -> "Retry", the CJK
  // "重试(&R)" -> "重试"), for a sentence that names that button: taken from the button's own
  // translation, so the two cannot disagree (the Italian Retry read "Indietro" until 2026-09-30)
  inline QString button_label (char const * source)
  {
    return QCoreApplication::translate ("JTDXMessageBox", source)
      .remove (QRegularExpression {QStringLiteral ("\\(&.\\)")}).remove ('&');
  }

  // the copy could not be made: say why and how to go on. Two buttons and no third way (the operator,
  // 2026-09-30: never start without the copy) - Retry, the default, tries again; Close, or leaving the
  // box any other way, ends the program with nothing written. true = Retry.
  inline bool ask_retry (QString const& title, QString const& ini, QString const& copy, QString const& error)
  {
    return JTDXMessageBox::Retry == JTDXMessageBox::critical_message (nullptr, title
        , QCoreApplication::translate ("main", "The settings file could not be copied. This program keeps "
                                       "a copy of it before it first saves anything in it, and does "
                                       "not start without one.")
        , QCoreApplication::translate ("main", "%1\ncould not be copied to\n%2:\n%3\n\nFix the cause "
                                       "and press %4, or press %5 to quit.")
            .arg (QDir::toNativeSeparators (ini), QDir::toNativeSeparators (copy), error
                  , button_label ("&Retry"), button_label ("Close"))
        , QString {}, JTDXMessageBox::Retry | JTDXMessageBox::Close, JTDXMessageBox::Retry);
  }
}

#endif
