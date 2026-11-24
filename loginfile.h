#ifndef LOGINFILE_H
#define LOGINFILE_H

/* CE3TSK 2026-11-24 (TODO.md 3.-283): a login kept in a file of its own beside the settings file - readable by its owner
   alone, never in the settings file - as the Club Log token and the QRZ.com key are (clublog.h's write_token). The
   operator: "Let's also save the eqsl data in an extra file only read by me just like the new qrz.com one, and add a code
   that takes the existing data from the ini and saves it to the new file on first run and from then on use the new file."

   One value a line, UTF-8 (a password may hold any character), the file made the owner's alone before anything goes in;
   all values empty: the file removed. A file made by hand and readable by others is made the owner's before anything is
   taken from it. Used by eQSL's login (username, password, QTH nickname: JTDX_eqsl.login) and QRZ.com's XML login
   (username, password: JTDX_qrz_xml.login). test_gridfill has the rules.

   (2026-11-24, TODO.md 3.-288) Never in clear text: read and saved through obfuscated.h - obfuscation, not encryption (the
   source is public); an older program's plain file read as it stands and saved obfuscated by the read. */

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include "obfuscated.h"

namespace login_file
{
  // the file beside the settings file: JTDX.ini -> JTDX_<name>
  inline QString path (QString const& settings_file, QString const& name)
  {
    QFileInfo const ini {settings_file};
    return ini.absoluteDir ().absoluteFilePath (ini.completeBaseName () + '_' + name);
  }

  // its `count` values, each "" when it has none; none at all without a file
  inline QStringList read (QString const& path, int count)
  {
    QStringList values;
    // (3.-288) the owner's alone first, decoded, an older program's plain file saved obfuscated (obfuscated::read)
    QByteArray const all {obfuscated::read (path)};   // a byte-order mark (Notepad writes one): QString::fromUtf8 drops a
                                                     // leading one (Qt 5.15) - test_gridfill's row holds it, whoever drops it
    if (!all.isEmpty ())
      {
        for (auto const& line : all.split ('\n'))
          {
            QByteArray l {line};
            if (l.endsWith ('\r')) l.chop (1);
            values << QString::fromUtf8 (l);
          }
      }
    while (values.size () < count) values << QString {};
    return values.mid (0, count);
  }

  // written, the owner's alone before a value goes in; all empty: removed. "" or what went wrong - never a value.
  inline QString write (QString const& path, QStringList const& values)
  {
    QFile f {path};
    bool any {false};
    for (auto const& v : values) any = any || !v.isEmpty ();
    if (!any)
      {
        if (f.exists () && !f.remove ()) return f.errorString ();
        return {};
      }
    QByteArray text;
    for (auto const& v : values) text += QString {v}.remove ('\r').remove ('\n').toUtf8 () + '\n';
    return obfuscated::save (path, text);   // (3.-288) the owner's alone, never in clear text; a failed save leaves the old file
  }
}

#endif
