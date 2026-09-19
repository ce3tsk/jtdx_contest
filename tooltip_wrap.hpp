#ifndef TOOLTIP_WRAP_HPP_
#define TOOLTIP_WRAP_HPP_

#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>
#include <QAction>
#include <QFontMetrics>
#include <QTextDocument>
#include <QToolTip>

/* CE3TSK: fold plain text into lines that FIT, measured in pixels.

   Counting characters, which this did first, is only right for a Latin alphabet: a Han
   ideograph is about twice the width of an average Latin letter, so the same 72 characters
   came out at 1078 px in ja_JP against 565 px in en_US - the Japanese tooltips were half
   again as wide as the widest English one and still counted as "folded".

   So the budget is a pixel width, given as a number of columns of the tooltip font's average
   character - the count therefore still means roughly what it used to for Latin text, and it
   follows the application font when the operator picks a bigger one.

   The other half of the same problem: Chinese and Japanese put spaces almost nowhere, so
   folding on spaces alone leaves one 673 px run that cannot be broken at all. CJK does allow a
   break between characters, so this also breaks after an ideograph, a kana, a hangul syllable
   or a clause-ending mark - but never before a closing bracket, which must not start a line. */
namespace tooltip_wrap_detail
{
  inline QString const& cjk_clause_end ()
  {
    static QString const marks {QString::fromUtf8 (u8"、。，．：；！？")};
    return marks;
  }
  inline bool breakable_after (QChar c)
  {
    switch (c.script ())
      {
      case QChar::Script_Han:
      case QChar::Script_Hiragana:
      case QChar::Script_Katakana:
      case QChar::Script_Hangul:
      case QChar::Script_Bopomofo: return true;
      default: break;
      }
    return cjk_clause_end ().contains (c);
  }
  /* Kinsoku, the CJK rule that some characters may never BEGIN a line: the clause marks (a line
     starting with a comma is what folding on width alone produced 16 times), the closing
     brackets, and in Japanese the prolonged sound mark, the small kana and the iteration marks
     - without those last a break landed inside a word, パイプライン最大デコ / ード数.
     The opening brackets need no rule: nothing may break after them because breakable_after
     does not accept them. */
  inline bool never_starts_a_line (QChar c)
  {
    static QString const forbidden {cjk_clause_end () + QString::fromUtf8
      (u8"）］》」』〉｝ーゝゞヽヾっゃゅょぁぃぅぇぉッャュョァィゥェォ")};
    return forbidden.contains (c);
  }
  struct Piece { QString text; bool spaced; };
}

inline QString fold_plain_text (QString const& text, QFontMetrics const& fm, int columns)
{
  using namespace tooltip_wrap_detail;
  int const budget = columns * fm.averageCharWidth ();
  QStringList out;
  for (auto const& paragraph : text.split (QChar {'\n'}))
    {
      if (fm.horizontalAdvance (paragraph) <= budget) { out << paragraph; continue; }

      // cut the paragraph where a line is allowed to break, remembering which cuts ate a space
      QVector<Piece> pieces;
      QString current;
      bool current_spaced = false, pending_space = false;
      auto const push = [&] { if (!current.isEmpty ()) { pieces.append ({current, current_spaced}); current.clear (); } };
      for (int i = 0; i < paragraph.size (); ++i)
        {
          QChar const c = paragraph.at (i);
          if (c == QChar {' '}) { push (); pending_space = true; continue; }
          // a CJK break eats no space, and pending_space is false here: a space would have
          // pushed the piece already, and then current would not be non-empty
          if (!current.isEmpty () && breakable_after (paragraph.at (i - 1)) && !never_starts_a_line (c)) push ();
          if (current.isEmpty ()) { current_spaced = pending_space; pending_space = false; }
          current += c;
        }
      push ();

      QString line;
      for (auto const& piece : pieces)
        {
          QString const candidate = line.isEmpty () ? piece.text
            : line + (piece.spaced ? QStringLiteral (" ") : QString {}) + piece.text;
          /* a piece that may not begin a line hangs off the end of this one instead, over
             budget - the one case left was a paragraph broken at a space in front of a lone
             fullwidth question mark, which then sat on a line of its own */
          bool const hangs = never_starts_a_line (piece.text.at (0));
          if (!line.isEmpty () && !hangs && fm.horizontalAdvance (candidate) > budget)
            {
              out << line;
              line = piece.text;
            }
          else line = candidate;
        }
      if (!line.isEmpty ()) out << line;
    }
  return out.join (QChar {'\n'});
}

/* CE3TSK: wrap over-long tooltips.

   Qt only word-wraps a tooltip it believes is rich text - QTipLabel does
   setWordWrap(Qt::mightBeRichText(text)) - so a PLAIN tooltip is drawn as a single line however
   long it is. Three of ours ran 930 to 1492 px, two thirds of a 2240 px screen, in one
   unbroken line. Rich-text tooltips are left alone, Qt already wraps those.

   64 columns is the width the app already had: the widest of its 109 folded English tooltip
   lines measured 573 px, and 64 average characters of the tooltip font is 576. */
inline QString wrap_tooltip (QString const& text, int columns = 64)
{
  if (text.isEmpty () || Qt::mightBeRichText (text)) return text;
  return fold_plain_text (text, QFontMetrics {QToolTip::font ()}, columns);
}

/* CE3TSK: has this action a tooltip of its own, or only the one QAction::toolTip () invents
   from its text?

   Asked of Qt itself, with a throwaway action carrying the same text, rather than by
   reimplementing its qt_strippedText: that helper also removes "..." anywhere in the string and
   trims the result, so a hand-written "strip the & " comparison says NO for `Settings...` and
   for `Messages with my callsign to RX frequency window ` (a trailing space in the .ui) - and
   the caller would then store the invented tooltip on exactly the entries this exists to spare.
   The text is what Qt looks at first and the icon text only when the text is empty, so the
   probe carries both. */
inline bool has_own_tooltip (QAction const * action)
{
  QAction probe {action->text (), nullptr};
  probe.setIconText (action->iconText ());
  return action->toolTip () != probe.toolTip ();
}

/* Apply it to every tooltip a window owns, widgets and menu actions alike - an action is not a
   widget, and the wav-converter menu entry was one of the offenders.

   An action with no tooltip of its own is left ALONE, and that matters: QAction::toolTip ()
   falls back to the stripped text, but QMenu decides whether to pop a tooltip by looking at the
   stored tooltip, not at that accessor. Assigning the fallback therefore turns "no tooltip"
   into "a tooltip that repeats the entry's own label", and every entry of a menu with
   setToolTipsVisible would echo itself - measured on View > Waterfall. See has_own_tooltip for
   why the test is not a hand-written mnemonic strip. */
template <class Window>
void wrap_tooltips (Window * w, int columns = 64)
{
  for (auto * child : w->template findChildren<QWidget *> ())
    child->setToolTip (wrap_tooltip (child->toolTip (), columns));
  for (auto * action : w->template findChildren<QAction *> ())
    {
      if (!has_own_tooltip (action)) continue;
      action->setToolTip (wrap_tooltip (action->toolTip (), columns));
    }
}

#endif
