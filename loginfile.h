#ifndef LOGINFILE_H
#define LOGINFILE_H

/* CE3TSK 2026-11-24 (TODO.md 3.-283): a login kept in a file of its own beside the settings file - readable by its owner
   alone, never in the settings file - as the Club Log token and the QRZ.com key are (clublog.h's write_token). The
   operator: "Let's also save the eqsl data in an extra file only read by me just like the new qrz.com one, and add a code
   that takes the existing data from the ini and saves it to the new file on first run and from then on use the new file."

   One value a line, UTF-8 (a password may hold any character), the file made the owner's alone before anything goes in;
   all values empty: the file removed. A file made by hand and readable by others is made the owner's before anything is
   taken from it. Used by eQSL's login (username, password, QTH nickname: JTDX_eqsl.login) and QRZ.com's XML login
   (username, password: JTDX_qrz_xml.login). test_gridfill has the rules. */

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>

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
    QFile f {path};
    if (f.open (QIODevice::ReadOnly))
      {
        auto const others = QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ReadOther | QFileDevice::WriteOther;
        if (f.permissions () & others) f.setPermissions (QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        QByteArray const all {f.read (64 * 1024)};   // a byte-order mark (Notepad writes one): QString::fromUtf8 drops a
                                                     // leading one (Qt 5.15) - test_gridfill's row holds it, whoever drops it
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
    QDir {}.mkpath (QFileInfo {path}.absolutePath ());
    if (!f.open (QIODevice::WriteOnly | QIODevice::Truncate)) return f.errorString ();
    if (!f.setPermissions (QFileDevice::ReadOwner | QFileDevice::WriteOwner))
      {
        auto const why = f.errorString ();
        f.close ();
        f.remove ();
        return why;
      }
    QByteArray text;
    for (auto const& v : values) text += QString {v}.remove ('\r').remove ('\n').toUtf8 () + '\n';
    if (f.write (text) != text.size () || !f.flush ())
      {
        auto const why = f.errorString ();
        f.close ();
        f.remove ();
        return why;
      }
    f.close ();
    return {};
  }
}

#endif
