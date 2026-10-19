#ifndef TXCHECK_H
#define TXCHECK_H

/* CE3TSK 2026-10-18 (TODO.md 3.-135): does a Tx message go out as it is written?
 *
 * In FT8, FT4 and FT2 the 77-bit packer decides what goes out, and the Tx fields' colour came from the 72-bit JT9 packer
 * (gen9), which knows no brackets: every bracketed message the program builds itself was pink, and a message the 77-bit
 * packer changes was white - "CE3TSK KH6/K1ABC RR73" goes out as "CE3TSK K1ABC RR73" (Type 1 holds two standard calls,
 * the prefix is dropped, as in WSJT-X), "W6/UT5UF AF0G TNX" cannot be sent at all. The main window packs the text as
 * genft8 does (the TX hash tables left alone) and hands the result here, which compares it word by word with what was
 * typed: upper and lower case, extra blanks, a report written -5 or -05 are the same; a call in brackets is the same
 * whether the table knows its hash ("<KH6/K1ABC>") or not ("<...>"). test/harness/test_txcheck. */

#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace tx_check
{
  enum class Kind
  {
    AsWritten,     // a structured message, sent as typed
    FreeText,      // free text, sent as typed (13 characters at most)
    Changed,       // sent otherwise: a call cut to its base, a word dropped, free text cut at 13 characters
    Unencodable    // its own packing refused by unpack77 - nothing can be sent ("*** bad message ***")
  };

  struct Result
  {
    Kind kind;
    QString sent;   // what goes out, as the packer's own unpack gives it back
  };

  inline bool same_word (QString const& typed, QString const& sent)
  {
    if (typed == sent) return true;
    // a hashed call: the packer sends its hash, and unpack77 shows the call only when its table holds it
    if (typed.size () > 2 && typed.startsWith ('<') && typed.endsWith ('>') && sent.startsWith ('<') && sent.endsWith ('>'))
      return true;
    static QRegularExpression const report {R"(^(R?)([+-])(\d{1,2})$)"};
    auto const a = report.match (typed), b = report.match (sent);
    return a.hasMatch () && b.hasMatch () && a.captured (1) == b.captured (1)
      && (a.captured (2) + a.captured (3)).toInt () == (b.captured (2) + b.captured (3)).toInt ();
  }

  // typed: the field's text; i3, n3: what pack77 made of it; sent: the text genft8 gives back for it
  inline Result of (QString const& typed, int i3, int n3, QString const& sent)
  {
    QString const s {sent.simplified ()};
    if (s.isEmpty () || s.startsWith ("*** bad message ***")) return {Kind::Unencodable, s};
    QStringList const t {typed.toUpper ().split (' ', Qt::SkipEmptyParts)}, u {s.split (' ', Qt::SkipEmptyParts)};
    bool same = t.size () == u.size ();
    for (int i = 0; same && i < t.size (); ++i) same = same_word (t[i], u[i]);
    if (!same) return {Kind::Changed, s};
    return {(0 == i3 && 0 == n3) ? Kind::FreeText : Kind::AsWritten, s};
  }
}

#endif
