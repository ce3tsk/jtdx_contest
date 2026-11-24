// CE3TSK 2026-11-24 (TODO.md 3.-288): the login files never in clear text - obfuscation, not encryption: see obfuscated.h
#include "obfuscated.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QSaveFile>

namespace obfuscated
{
  namespace
  {
    // in the program, and so in its public source: what makes this obfuscation and not encryption
    char const secret[] {"JTDX_contest login files: obfuscation, not encryption - the source is public (TODO.md 3.-288)"};
    int const salt_size {16};
    int const check_size {8};

    QByteArray keystream (QByteArray const& salt, int size)
    {
      QByteArray s;
      for (quint32 n = 0; s.size () < size; ++n)
        {
          QCryptographicHash h {QCryptographicHash::Sha256};
          h.addData (secret);
          h.addData (salt);
          char const count[4] {char (n >> 24), char (n >> 16), char (n >> 8), char (n)};
          h.addData (count, 4);
          s += h.result ();
        }
      return s.left (size);
    }

    QByteArray mixed (QByteArray data, QByteArray const& salt)
    {
      QByteArray const k {keystream (salt, data.size ())};
      for (int i = 0; i < data.size (); ++i) data[i] = char (data[i] ^ k[i]);
      return data;
    }

    QByteArray check (QByteArray const& salt, QByteArray const& plain)
    {
      QCryptographicHash h {QCryptographicHash::Sha256};
      h.addData ("check");
      h.addData (secret);
      h.addData (salt);
      h.addData (plain);
      return h.result ().left (check_size);
    }

    QByteArray header () {return QByteArray {magic} + '\n';}
  }

  QByteArray encode (QByteArray const& plain)
  {
    QByteArray salt (salt_size, '\0');
    for (int i = 0; i < salt_size; ++i) salt[i] = char (QRandomGenerator::system ()->bounded (256));
    return header () + (salt + check (salt, plain) + mixed (plain, salt)).toBase64 () + '\n';
  }

  bool decode (QByteArray const& content, QByteArray& plain)
  {
    plain.clear ();
    QByteArray body {content};
    if (body.startsWith (QByteArray {magic} + "\r\n")) body.remove (int (QByteArray {magic}.size ()), 1);   // a CRLF editor's save
    if (!body.startsWith (header ())) return false;
    QByteArray const raw {QByteArray::fromBase64 (body.mid (header ().size ()).trimmed ())};
    if (raw.size () < salt_size + check_size) return false;
    QByteArray const salt {raw.left (salt_size)};
    QByteArray const data {mixed (raw.mid (salt_size + check_size), salt)};
    if (check (salt, data) != raw.mid (salt_size, check_size)) return false;
    plain = data;
    return true;
  }

  QByteArray read (QString const& path, qint64 max)
  {
    QFile f {path};
    if (!f.open (QIODevice::ReadOnly)) return {};
    // a file made by hand, readable by others: made the user's alone before anything is taken from it
    auto const others = QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ReadOther | QFileDevice::WriteOther;
    if (f.permissions () & others) f.setPermissions (QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    QByteArray const content {f.read (max)};
    f.close ();
    if (!content.startsWith (magic))   // an older program's, or one made by hand: as it stands, and from now on ours
      {
        if (!content.trimmed ().isEmpty ()) save (path, content);
        return content;
      }
    QByteArray plain;
    decode (content, plain);   // damaged: nothing
    return plain;
  }

  QString save (QString const& path, QByteArray const& plain)
  {
    QByteArray const content {encode (plain)};
    QByteArray back;
    if (!decode (content, back) || back != plain) return QStringLiteral ("the file's content could not be made");
    QDir {}.mkpath (QFileInfo {path}.absolutePath ());
    QSaveFile f {path};
    if (!f.open (QIODevice::WriteOnly)) return f.errorString ();
    // the owner's alone before the content goes in
    if (!f.setPermissions (QFileDevice::ReadOwner | QFileDevice::WriteOwner))
      {
        QString const why {f.errorString ()};
        f.cancelWriting ();
        return why;
      }
    if (f.write (content) != content.size () || !f.commit ()) return f.errorString ();
    return {};
  }
}
