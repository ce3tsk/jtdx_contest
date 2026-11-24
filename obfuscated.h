#ifndef OBFUSCATED_H
#define OBFUSCATED_H

/* CE3TSK 2026-11-24 (TODO.md 3.-288, the operator: "how about obfuscation only to avoid having the data be there in clear
   text?", then "let's add the proposed obfuscation"): the login files - eQSL's and QRZ.com's XML logins (loginfile.h),
   the Club Log token and the QRZ.com key (clublog.cpp's write_token, read_token; qrzlogbook.cpp's read_key) - never in
   clear text.

   OBFUSCATION, NOT ENCRYPTION. The program reads them unattended, so their key is in the program, and the program's
   source is public: whoever has a file and the source has the login. What it gives: no login as plain text in a file -
   for cat, grep, a desktop search, a support zip, a secret scanner. The files stay their owner's alone.

   A file of ours: "JTDX-OBF1" and a line break, then one line of base64 - 16 random bytes (each save its own: the same
   login never looks the same twice), an 8-byte check, and the content mixed with a SHA-256 keystream of the program's
   constant and those 16 bytes. One whose check fails is damaged: refused, nothing taken from it, left as it is. One
   without the header is an older program's or made by hand: read as it stands, and saved as ours by the read.

   save (): through QSaveFile - the owner's alone before the content goes in, the content decoded again in memory before
   it replaces the old file; a failed save leaves the old file as it was. test_gridfill has the rules. */

#include <QByteArray>
#include <QString>

namespace obfuscated
{
  constexpr char const * magic {"JTDX-OBF1"};

  QByteArray encode (QByteArray const& plain);
  // a file of ours decoded into plain: false, plain empty, when it is none or damaged
  bool decode (QByteArray const& content, QByteArray& plain);

  /* a login file's content, at most max bytes: ours decoded; an older program's plain one as it stands - and saved as
     ours at once (left as it was when that fails); "" without a file, or when it is damaged. A file readable by others is
     made its owner's alone before anything is taken from it. */
  QByteArray read (QString const& path, qint64 max = 64 * 1024);

  // plain saved as ours, the file its owner's alone: "" or why not - never the content
  QString save (QString const& path, QByteArray const& plain);
}

#endif
