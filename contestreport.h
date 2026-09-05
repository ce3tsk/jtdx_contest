#ifndef CONTESTREPORT_H
#define CONTESTREPORT_H

#include <QString>
#include <QStringList>
#include <QRegularExpression>

/* CE3TSK: WW Digi contest - the exchange is the 4 character grid, never a signal report.
   A standard message that ends in a report ("K1ABC W9XYZ -10", "K1ABC W9XYZ R-10", also
   "+05" / "R+05") is therefore either a station not in the contest or a false decode: an
   a-priori codeword's random g15 field decodes as a report about as often as it decodes as
   a grid, and the report form has no grid consistency check to fail (chkgrid). In contest
   mode such a line is dropped before ALL.TXT, the windows, the UDP clients and the
   sequencer see it. RRR / RR73 / 73, grids, CQs, hashed and free text lines pass. */

/* CE3TSK: the message out of a decoder line - "163015 -14  0.2 1023 ~ K1ABC W9XYZ R-10  ^".
   The separator before the message is the mode's own character and FT4 does NOT use FT8's
   tilde: it prints "163015 -14  0.2 1023 : K1ABC W9XYZ R-10". Matching only " ~ " left this
   whole filter inert in FT4 - which is the mode most of WW Digi is worked in (found
   2026-08-29 from ALL.TXT: nine reports addressed to us survived the filter that evening).
   So the prefix is recognised by its shape, whatever the separator, and anything that does
   not look like a decoder line is taken as the bare message text. */
inline QString contest_message_text (QString const& line)
{
  static QRegularExpression const prefix {
    QStringLiteral (R"(^\s*(?:\d{8}_)?\d{6}(?:\.\d+)?\s+[-+]?\d+\s+[-+]?\d*\.?\d+\s+\d+\s+\S\s+)")};
  auto const m = prefix.match (line);
  if (m.hasMatch ()) return line.mid (m.capturedLength ());
  int const tilde = line.indexOf (" ~ ");
  if (tilde >= 0) return line.mid (tilde + 3);
  return line;
}

/* CE3TSK: the message's words, markers dropped. One copy: this scan sat in both functions
   below, and the marker set is decoder-defined - the FT4 separator case was already missed
   once. A future marker rule applied to one loop only would let a line be rejected by the
   filter while the abort path failed to identify the partner, dropping the message without
   arming the five minute skip. */
inline QStringList contest_message_tokens (QString const& line)
{
  QStringList tokens;
  for (auto const& tok : contest_message_text (line).simplified ().split (' ', Qt::SkipEmptyParts))
    {
      /* the decoder's trailing markers (^ * | ┼ ? • ° ...) carry no letter or digit; every
         message word does - so a token without one is a marker, whatever its byte length */
      bool word = false;
      for (auto const& ch : tok) if (ch.isLetterOrNumber ()) { word = true; break; }
      if (!word) continue;
      tokens << tok;
    }
  return tokens;
}

/* the decoder line as jtdxjt9 prints it (any trailing marker characters are ignored), or
   just the message text */
inline bool contest_report_message (QString const& line)
{
  QStringList const tokens = contest_message_tokens (line);
  if (tokens.size () < 3 || tokens.size () > 4) return false;   /* CALL CALL RPT, or with a hash/compound */
  if (tokens[0] == "CQ" || tokens[0] == "QRZ" || tokens[0] == "DE") return false;
  static QRegularExpression const report {QStringLiteral ("^R?[-+]\\d\\d$")};
  return report.match (tokens.last ()).hasMatch ();
}

/* CE3TSK: the same line, asked the other way round - is this report message addressed to me?
   "CE3TSK DL6FKR -10" / "CE3TSK DL6FKR R-10" means the station I am working answers with a
   signal report instead of his grid, i.e. he is not in the contest and the QSO cannot
   complete. Returns his callsign, or empty for anything else (a report between two other
   stations is only dropped, never acted on). Deliberately only the plain three token form:
   a hashed or compound variant is rare here and not worth a wrong abort. */
inline QString contest_report_to_me (QString const& line, QString const& myBaseCall)
{
  if (myBaseCall.size () < 3 || !contest_report_message (line)) return QString {};
  QStringList const tokens = contest_message_tokens (line);
  if (tokens.size () != 3) return QString {};
  QString first = tokens[0]; first.remove ('<').remove ('>');
  bool mine = false;
  for (auto const& part : first.split ('/', Qt::SkipEmptyParts))
    if (0 == part.compare (myBaseCall, Qt::CaseInsensitive)) { mine = true; break; }
  if (!mine) return QString {};
  QString his = tokens[1]; his.remove ('<').remove ('>');
  return his;
}

/* CE3TSK: the message column of a line as the RX window shows it, which is what a double
   click hands us - not a decoder line. displaytext.cpp lays the message out in a fixed 26
   character field (appendText: text.mid(ft,26)) and follows it with the service character
   and, when "show country name" is on, the country; Configuration.cpp defaults that to true.
   A line ending in a multi-word country therefore ran to seven tokens, so the "at most four
   tokens" test in contest_report_message() never held and the double-click guard could not
   fire on a real window line - the operator could still start the deadlocking QSO by hand.
   The column offsets are the ones the handler already uses to cut the message out. */
constexpr int CONTEST_MSG_COL_FT = 23;    /* FT modes: the message starts here */
constexpr int CONTEST_MSG_COL = 21;       /* the other modes */
constexpr int CONTEST_MSG_WIDTH = 26;     /* appendText's fixed message field */
inline QString contest_displayed_message (QString const& line, bool ft_mode)
{
  int const col = ft_mode ? CONTEST_MSG_COL_FT : CONTEST_MSG_COL;
  return line.size () > col ? line.mid (col, CONTEST_MSG_WIDTH) : QString {};
}

#endif
