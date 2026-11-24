#include "displaytext.h"
#include "contestpoints.h"   // CE3TSK 2026-11-04
#include "swatchcontrast.h"   // CE3TSK 2026-11-12 (TODO.md 3.-246): the wave's colour against its background

#include <algorithm>
#include <QtGlobal>
#include <QApplication>
#include <QMouseEvent>
#include <QTextCharFormat>
#include <QFont>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextLayout>
#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QScrollBar>
#include <QFontMetricsF>

#include "Configuration.hpp"
#include "qt_helpers.hpp"
#include "usstates.h"   /* CE3TSK 2026-10-02 */
#include "priorities.h"   /* CE3TSK 2026-10-03 */
#include "cqdirection.h"   /* CE3TSK 2026-10-12 */
#include "wantedprefix.h"   /* CE3TSK 2026-10-15 */
#include "distancefield.h"   /* CE3TSK 2026-10-20 */
#include "stdcall.h"   /* CE3TSK 2026-11-13 */
#include "modetiming.h"   /* CE3TSK 2026-11-18: lines_have_seconds () */
namespace dp = decode_priority;

#include "moc_displaytext.cpp"

/* CE3TSK: WW Digi contest - true for a plain 4 character Maidenhead grid.
   "RR73" is deliberately excluded, it occupies the same field in the protocol. */
static bool isGrid4 (QString const& w)
{
  if (w.length () != 4 || w == "RR73") return false;
  return w[0] >= 'A' && w[0] <= 'R' && w[1] >= 'A' && w[1] <= 'R'
      && w[2] >= '0' && w[2] <= '9' && w[3] >= '0' && w[3] <= '9';
}

/* CE3TSK 2026-09-30: what a word in doubt looks like (phantomdecodes.h) - the format it would have had,
   with a red wave underline, the spell checker's mark. Its own colour-table row: the dark style's
   red for a background is too dark to see as a line.
   2026-10-01, the operator: the wave is drawn by the window itself, 2 px thick, in both styles - Qt's own
   WaveUnderline is one pixel, and in the dark style it all but vanished on the category colours (option C of
   the comparison: the same reds, twice as thick). So the span carries DoubtProperty and its colour, Qt draws
   no underline under it (any underline the segment had gives way to the wave, as it did before), and
   paintEvent () draws the wave over Qt's text.
   2026-11-12 (TODO.md 3.-246, its review): the red stays red down to 2.0:1 against the background under it, the floor
   the operator set for it (Radio.cpp fixed_colors); below that the wave is black or white, whichever stands out
   (swatchcontrast.h) - its shape still says "in doubt". That is the dark style inverted, where the line's own colour is
   the background: #ff7878 on CQ 1.89:1, on a call to us 1.14:1, on a standard call 1.15:1, and since 3.-246 on a wanted
   message part (the my-call colour). With the recommended colours every other background keeps the red (the lowest,
   new CQ zone etc. in the light style, 2.04:1). */
QTextCharFormat DisplayText::doubtFormat (QTextCharFormat f) const
{
  f.setUnderlineStyle (QTextCharFormat::NoUnderline);
  double constexpr waveFloor {2.0};
  QColor const red {Radio::convert_dark ("#e60000", useDarkStyle_)};
  QColor const bg {Qt::NoBrush == f.background ().style () ? QColor {} : f.background ().color ()};
  f.setUnderlineColor (bg.isValid () && swatch_contrast::ratio (red, bg) < waveFloor ? swatch_contrast::frame_colour (bg) : red);
  f.setProperty (DoubtProperty, true);
  return f;
}

namespace
{
  qreal const waveWidth = 2.;    // the pen
  qreal const waveAmp = 1.5;     // above and below the centre line
  qreal const waveHalf = 3.;     // half a period

  void drawWave (QPainter& p, qreal x0, qreal x1, qreal y, QColor const& c)
  {
    QPainterPath path {QPointF {x0, y}};
    bool up = true;
    for (qreal x = x0; x < x1; x += waveHalf, up = !up)
      {
        qreal const xe = qMin (x + waveHalf, x1);
        path.quadTo ((x + xe) / 2., y + (up ? -2. : 2.) * waveAmp, xe, y);   // the curve reaches half its control
      }
    QPen pen {c, waveWidth};
    pen.setCapStyle (Qt::RoundCap);
    pen.setJoinStyle (Qt::RoundJoin);
    p.setPen (pen);
    p.setBrush (Qt::NoBrush);
    p.drawPath (path);
  }
}

void DisplayText::paintEvent (QPaintEvent *e)
{
  QTextEdit::paintEvent (e);
  QPainter p {viewport ()};
  p.setRenderHint (QPainter::Antialiasing, true);
  p.setClipRect (e->rect ());
  // where QTextEdit itself draws the document: in a right-to-left layout the horizontal offset is maximum - value
  // (QTextEditPrivate::horizontalOffset), not value - else a sideways-scrolled window draws the waves off their words
  QScrollBar const* const hbar = horizontalScrollBar ();
  QPointF const offset {-qreal (isRightToLeft () ? hbar->maximum () - hbar->value () : hbar->value ()),
                        -qreal (verticalScrollBar ()->value ())};
  qreal const top = e->rect ().top (), bottom = e->rect ().bottom ();
  auto* const docLayout = document ()->documentLayout ();
  // a wave reaches a little into the line below (there is no room under the letters inside a line - it is ascent
  // plus descent, 3 px at 11 pt), so a repaint of that line alone must redraw the wave of the line above it
  QTextBlock b = cursorForPosition (QPoint {0, qMax (0, int (top))}).block ();
  if (b.previous ().isValid ()) b = b.previous ();
  for (; b.isValid (); b = b.next ())
    {
      QRectF const r = docLayout->blockBoundingRect (b).translated (offset);
      if (r.top () > bottom) break;
      if (r.bottom () + waveAmp + waveWidth < top || !b.layout ()) continue;
      for (auto it = b.begin (); !it.atEnd (); ++it)
        {
          QTextFragment const fr = it.fragment ();
          if (!fr.isValid () || !isDoubt (fr.charFormat ())) continue;
          int const from = fr.position () - b.position (), to = from + fr.length ();
          QTextLine const line = b.layout ()->lineForTextPosition (from);
          if (!line.isValid ()) continue;
          // just under the letters - its foot reaches ~1.5 px into the next line, which is painted by now (inside
          // the line it would cut through the letters, measured 2026-10-01). The line's own descent, as laid out: a
          // format without a font of its own, or another screen's DPI, would put the font's metrics elsewhere
          qreal const y = r.top () + line.y () + line.ascent () + line.descent () - 1.;
          drawWave (p, r.left () + line.cursorToX (from), r.left () + line.cursorToX (to), y, fr.charFormat ().underlineColor ());
        }
    }
}

/* CE3TSK 2026-10-19: why a line is a likely phantom decode, as its tooltip (the operator: anywhere on the line) - a
   sentence for each reason the judge found (phantomdecodes.h), then what is done with the decode: held back from the
   sequencer, and how it goes out (Verdict::lowConfidence). `country` names the entity the grid was judged against as
   the country column does (cty.dat's name, translated when Settings says so); `snr` is the decode's, the level the
   /P rule judged. Rich text, a paragraph a sentence, so a long one wraps; each escaped, as a country name may hold an '&'. */
QString DisplayText::doubtToolTip (phantom_decodes::Verdict const& v, QString const& country, int snr)
{
  using namespace phantom_decodes;
  QString const call = bareCall (v.sender);
  QStringList s;
  if (v.reasons & Grid)
    {
      s << tr ("Likely phantom decode: %1 lies outside %2, the country of %3.").arg (v.grid, country, call);
      if (v.repeated) s << tr ("%1 has sent this grid in an earlier period too: most likely a station operating away from home.").arg (call);
    }
  if (v.reasons & Rover)
    for (auto const& r : v.rovers) s << tr ("Likely phantom decode: %1 signs /R (rover) below 30 MHz, and /R is a single bit of the message.").arg (bareCall (r));
  if (v.reasons & Portable)
    s << tr ("Likely phantom decode: %1 and %2 both sign /P (portable), at %3 dB below 30 MHz; genuine park-to-park contacts are stronger.")
         .arg (bareCall (v.portables.value (0)), bareCall (v.portables.value (1)), QString::number (snr));
  if (v.reasons & Where) s << tr ("Likely phantom decode: the prefix of %1 belongs to no country in cty.dat.").arg (call);
  if (s.isEmpty ()) return {};
  if (v.noAnswer) s << tr ("Not answered automatically; a double click still answers it.");
  s << (v.lowConfidence () ? tr ("Sent to UDP clients as a low-confidence decode, and never spotted to PSK Reporter.")
                           : tr ("No longer of low confidence: it goes to UDP clients and PSK Reporter like any other decode."));
  QString tip;
  for (auto const& x : s) tip += "<p>" + x.toHtmlEscaped () + "</p>";
  return tip;
}

/* CE3TSK 2026-11-22 (TODO.md 3.-280, the operator): why a line has the colour of a LoTW user worked but not LoTW-confirmed -
   what was worked in the tier that fired, that it therefore counts as never worked (for the sequencer too), and, for a QSO
   confirmed in LoTW already, where Club Log fetches its confirmations (Club Log's labels in English, as Club Log shows
   them). Rich text, a paragraph a sentence, each escaped: a country name may hold an '&'. */
QString DisplayText::unconfirmedToolTip (QString const& call, QString const& worked)
{
  QStringList const s {
    tr ("%1 is a LoTW user, and you have worked %2, but none of those QSOs is LoTW-confirmed.").arg (call, worked),
    tr ("So it counts as never worked, and the auto sequencer treats it as new."),
    tr ("Confirmed in LoTW already? Club Log and QRZ.com have only the LoTW confirmations they have fetched: in Club Log, Tools > LoTW Sync > Download confirmations; in your QRZ.com Logbook, Settings > Logbook of the World Import/Export > Import. The program downloads them every 12 hours.")};
  QString tip;
  for (auto const& x : s) tip += "<p>" + x.toHtmlEscaped () + "</p>";
  return tip;
}

/* CE3TSK 2026-11-22 (the operator: "show the caller's call sign followed by the DXCC, and then give more information in
   the next line, e.g. coming from the notification color ... and then in the next lines also add the information that
   comes encoded in the hint chars"; the first line bold): a paragraph a sentence, each escaped - a country name may hold
   an '&'. Nothing to say, no tooltip. */
QString DisplayText::lineToolTip (QString const& head, QString const& tier, QString const& unconfirmed, QStringList const& colours,
                                  QStringList const& marks, QString const& phantom)
{
  QString tip;
  if (!head.isEmpty ()) tip += "<p><b>" + head.toHtmlEscaped () + "</b></p>";
  if (!tier.isEmpty ()) tip += "<p>" + tier.toHtmlEscaped () + "</p>";
  tip += unconfirmed;
  for (auto const& x : colours) tip += "<p>" + x.toHtmlEscaped () + "</p>";
  for (auto const& x : marks) tip += "<p>" + x.toHtmlEscaped () + "</p>";
  return tip + phantom;
}

QString DisplayText::tierSentence (int level, QString const& value, QString const& where)
{
  namespace dp = decode_priority;
  bool const on {!where.isEmpty ()};
  switch (level)
    {
    case dp::NewField: return on ? tr ("New Maidenhead field on %1: %2").arg (where, value) : tr ("New Maidenhead field: %1").arg (value);
    case dp::NewCQZ: return tr ("New CQ zone: %1").arg (value);
    case dp::NewCQZBand: return on ? tr ("New CQ zone on %1: %2").arg (where, value) : tr ("New CQ zone: %1").arg (value);
    case dp::NewITUZ: return tr ("New ITU zone: %1").arg (value);
    case dp::NewITUZBand: return on ? tr ("New ITU zone on %1: %2").arg (where, value) : tr ("New ITU zone: %1").arg (value);
    case dp::NewDXCC: return tr ("New DXCC: %1").arg (value);
    case dp::NewDXCCBand: return on ? tr ("New DXCC on %1: %2").arg (where, value) : tr ("New DXCC: %1").arg (value);
    case dp::NewState: return tr ("New US state: %1").arg (value);
    case dp::NewStateBand: return on ? tr ("New US state on %1: %2").arg (where, value) : tr ("New US state: %1").arg (value);
    case dp::NewGrid: return tr ("New grid: %1").arg (value);
    case dp::NewGridBand: return on ? tr ("New grid on %1: %2").arg (where, value) : tr ("New grid: %1").arg (value);
    case dp::NewPx: return tr ("New prefix: %1").arg (value);
    case dp::NewPxBand: return on ? tr ("New prefix on %1: %2").arg (where, value) : tr ("New prefix: %1").arg (value);
    case dp::NewCall: return tr ("New call: %1").arg (value);
    case dp::NewCallBand: return on ? tr ("New call on %1: %2").arg (where, value) : tr ("New call: %1").arg (value);
    default: return {};
    }
}

/* the mark the decoder wrote (lib/ft8_decode.f90, ft4b.f90, the JT decoders; BACKGROUND_DECODES_SEQUENCER.md section 2):
   ^ a hint decode, * an a-priori one - the window draws both as * (° for a LoTW user) - | the Tx background, where an
   a-priori decode is marked | as well, the cross a hint decode there, ? JT65's likely false one, 1 a DXpedition-mode
   message read without help (the window underlines it), , and . free text */
QStringList DisplayText::markSentences (QString const& mark, bool lotw, bool windowCopy, bool jt)
{
  QStringList s;
  // (review) JT65 and JT9 mark their hint decodes * (lib/extract.f90, decode9.f90): no a-priori decoder there
  if (jt && (mark == "*" || mark == QString::fromUtf8 ("°"))) s << tr ("Hint decode: the decoder matched it to a message it expected - of the QSO in progress, or one heard before - instead of reading it bit by bit.");
  // (review) a window's copy of the line: * or the degree sign stands for both kinds - the decoder's own mark is gone
  else if (windowCopy && (mark == "*" || mark == QString::fromUtf8 ("°"))) s << tr ("Hint or a-priori decode: the decoder had help with this message; the window's mark does not tell which kind.");
  else if (mark == "^") s << tr ("Hint decode: the decoder matched it to a message it expected - of the QSO in progress, or one heard before - instead of reading it bit by bit.");
  else if (mark == "*") s << tr ("A-priori decode: the decoder took part of the message as known - CQ, My Call, the DX call, or RRR, RR73 or 73 - and decoded the rest.");
  else if (mark == QString::fromUtf8 ("┼")) s << tr ("Hint decode in the TX background: found while you transmitted, by matching it to a message the decoder expected.");
  else if (mark == "|") s << tr ("Decoded in the TX background: the decoder went on with the last receive period while you transmitted. The mark does not tell a plain decode from an a-priori one.");
  else if (mark == "?") s << tr ("Likely a false decode: many of its symbols had to be corrected.");
  else if (mark == "1") s << tr ("A DXpedition-mode message (Fox and Hound), decoded without help.");
  else if (mark == "," || mark == ".") s << tr ("Free text.");
  if (lotw) s << tr ("A LoTW user: uploaded to LoTW within the last year.");
  return s;
}

void DisplayText::setConfirmations (LogBook * confirmed, LogBook * pending)
{
  confirmed_ = confirmed && confirmed->get_qso_count () > 0 ? confirmed : nullptr;
  pending_ = confirmed_ ? pending : nullptr;   // (2026-11-22, D12) no confirmations: nothing pending either
}

DisplayText::DisplayText(QWidget *parent) :
    QTextEdit(parent)
{
    setReadOnly (true);
    viewport ()->setCursor (Qt::ArrowCursor);
    setWordWrapMode (QTextOption::NoWrap);
    setStyleSheet ("");
    // max lines to limit heap usage
    document ()->setMaximumBlockCount (10000);
}

void DisplayText::setConfiguration(Configuration const * config)
{
  scroll_ = config->scroll();
  useDarkStyle_ = config->useDarkStyle();
  myGrid_ = config->my_grid();            /* CE3TSK */
  specialOp_ = config->special_op_id();   /* CE3TSK */
  displayCountryName_ = config->countryName();
  displayCountryPrefix_ = config->countryPrefix();
  displayUSStates_ = config->usStates();   /* CE3TSK */
  showDistance_ = config->showDistance ();   /* CE3TSK 2026-10-20: distancefield.h */
  showAzimuth_ = config->showAzimuth ();
  miles_ = config->miles ();
  displayNewCQZ_ = config->newCQZ();
  displayNewCQZBand_ = config->newCQZBand();
  displayNewCQZBandMode_ = config->newCQZBandMode();
  displayNewITUZ_ = config->newITUZ();
  displayNewITUZBand_ = config->newITUZBand();
  displayNewITUZBandMode_ = config->newITUZBandMode();
  displayNewDXCC_ = config->newDXCC();
  displayNewDXCCBand_ = config->newDXCCBand();
  displayNewDXCCBandMode_ = config->newDXCCBandMode();
  displayNewState_ = config->newState();   /* CE3TSK 2026-10-03: Worked All States */
  displayNewStateBand_ = config->newStateBand();
  displayNewStateBandMode_ = config->newStateBandMode();
  /* CE3TSK 2026-10-03 (review): the tier list once - the decode path read it in three copies. The call tiers need
     nothing but the call, so they judge every decode; grid and the new US state judge only a decode they can place
     (displayDecodedText, "judged") */
  callTiers_ = config->newCQZ () || config->newITUZ () || config->newDXCC () || config->newPx () || config->newCall ();
  anyTier_ = callTiers_ || config->newGrid () || config->newState ();
  displayNewGrid_ = config->newGrid();
  contest_ = config->contest (); /* CE3TSK: what the contest being run asks (contestrules.h) */
  displayNewGridBand_ = config->newGridBand();
  displayNewGridBandMode_ = config->newGridBandMode();
  displayNewPx_ = config->newPx();
  displayNewPxBand_ = config->newPxBand();
  displayNewPxBandMode_ = config->newPxBandMode();
  displayNewCall_ = config->newCall();
  displayNewCallBand_ = config->newCallBand();
  displayNewCallBandMode_ = config->newCallBandMode();
  displayPotential_ = config->newPotential();
  displayTxtColor_ = config->txtColor();
  displayWorkedColor_ = config->workedColor();
  displayWorkedStriked_ = config->workedStriked();
  displayWorkedUnderlined_ = config->workedUnderlined();
  displayWorkedDontShow_ = config->workedDontShow();
  beepOnNewCQZ_ = config->beepOnNewCQZ();
  beepOnNewITUZ_ = config->beepOnNewITUZ();
  beepOnNewDXCC_ = config->beepOnNewDXCC();
  beepOnNewState_ = config->beepOnNewState();   /* CE3TSK 2026-10-03: Worked All States */
  beepOnNewGrid_ = config->beepOnNewGrid();
  beepOnNewPx_ = config->beepOnNewPx();
  beepOnNewCall_ = config->beepOnNewCall();
  beepOnMyCall_ = config->beepOnMyCall();
  RR73Marker_ = config->RR73Marker();
  otherMessagesMarker_ = config->otherMessagesMarker();
  enableCountryFilter_ = config->enableCountryFilter();
  enableCallsignFilter_ = config->enableCallsignFilter();
  hidefree_ = config->hidefree();
  enableMyConinentFilter_ = config->hideOwnContinent();
  showcq_ = config->showcq();
  showcqrrr73_ = config->showcqrrr73();
  showcq73_ = config->showcq73();
  redMarker_ = config->redMarker();
  blueMarker_ = config->blueMarker();
  hidehintMarker_ = config->hidehintMarker();
  hide_TX_messages_ = config->hide_TX_messages();
  color_MyCall_ = config->color_MyCall().name();
  color_CQ_ = config->color_CQ().name();
  color_StandardCall_ = config->color_StandardCall().name();
  color_WorkedCall_ = config->color_WorkedCall().name();
  color_NewCQZ_ = config->color_NewCQZ().name();
  color_NewCQZBand_ = config->color_NewCQZBand().name();
  color_NewITUZ_ = config->color_NewITUZ().name();
  color_NewITUZBand_ = config->color_NewITUZBand().name();
  color_NewDXCC_ = config->color_NewDXCC().name();
  color_NewDXCCBand_ = config->color_NewDXCCBand().name();
  color_NewState_ = config->color_NewState().name();   /* CE3TSK */
  color_NewStateBand_ = config->color_NewStateBand().name();
  color_NewGrid_ = config->color_NewGrid().name();
  color_NewGridBand_ = config->color_NewGridBand().name();
  color_NewPx_ = config->color_NewPx().name();
  color_NewPxBand_ = config->color_NewPxBand().name();
  color_NewCall_ = config->color_NewCall().name();
  color_Unconfirmed_ = config->color_Unconfirmed ().name ();   /* CE3TSK 2026-11-21 (TODO.md 3.-280) */
  countUnconfirmed_ = config->countUnconfirmedAsNew ();
  color_Confirmed_ = config->color_Confirmed ().name ();   /* CE3TSK 2026-11-22 (TODO.md 3.-280, D12) */
  highlightConfirmed_ = config->highlightConfirmed ();
  color_NewCallBand_ = config->color_NewCallBand().name();
  hideContinents_ = config->hideContinents();
  countries_ = config->countries();
  callsigns_ = config->callsigns();
  myCall_ = config->my_callsign();   
}

void DisplayText::setMyContinent(QString const& mycontinet)
{
    myContinent_ = mycontinet;
}

void DisplayText::setContentFont(QFont const& font)
{
//  setFont (font);
  m_charFormat.setFont (font);
  bold_ = font.bold();
  m_charFormat.setFontItalic(false);
//  selectAll ();
/*
  auto cursor = textCursor ();
  cursor.select(QTextCursor::Document);
  cursor.mergeCharFormat (m_charFormat);
  cursor.clearSelection ();
  cursor.movePosition (QTextCursor::End);

  // position so viewport scrolled to left
  cursor.movePosition (QTextCursor::Up);
  cursor.movePosition (QTextCursor::StartOfLine);

  setTextCursor (cursor);
  ensureCursorVisible (); */
}

void DisplayText::mouseDoubleClickEvent(QMouseEvent *e)
{
  bool ctrl = (e->modifiers() & Qt::ControlModifier);
  bool alt = (e->modifiers() & Qt::AltModifier);
  QTextEdit::mouseDoubleClickEvent(e);
  emit(selectCallsign(alt,ctrl));
}

void DisplayText::insertLineSpacer(QString const& line)
{
    appendText (line, Radio::convert_dark("#d3d3d3",useDarkStyle_), Radio::convert_dark("#000000",useDarkStyle_), 0, " ", Radio::convert_dark("#000000",useDarkStyle_), " ", true);
}

void DisplayText::appendText(QString const& text, QString const& bg, QString const& color, int std_type, QString const& servis, QString const& servis_color, QString const& cntry, bool forceBold, bool strikethrough, bool underlined, bool DXped, bool overwrite, bool wanted, QList<QPair<int, int>> const& doubtful, int cntryDoubtFrom, QString const& toolTip, QString const& field, int fieldColumns, bool inverted)
{
    QString servbg, s;
    if (std_type == 2) servbg = Radio::convert_dark("#ff0000",useDarkStyle_);
    else if (std_type == 5) servbg = Radio::convert_dark("#0000ff",useDarkStyle_);
    else if (servis == "?") servbg = Radio::convert_dark("#ffff00",useDarkStyle_);
    else if (std_type == 3 && servis.length()>1) servbg = servis.mid(1);
    else servbg = Radio::convert_dark("#ffffff",useDarkStyle_);
    /* CE3TSK 2026-10-19: the line's tooltip on every part of it, and none on any other line - m_charFormat serves them all */
    if (toolTip.isEmpty ()) m_charFormat.clearProperty (QTextFormat::TextToolTip);
    else m_charFormat.setToolTip (toolTip);
    auto cursor = textCursor ();
    if (scroll_) {
        if (document ()->blockCount() == 10000) {
            cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, 9998);
            cursor.select(QTextCursor::LineUnderCursor);
            cursor.removeSelectedText();
            cursor.deleteChar();
        }
        cursor.movePosition (QTextCursor::Start);
        if (overwrite) {
            cursor.select(QTextCursor::LineUnderCursor);
            cursor.removeSelectedText();
        }
    } else {
        cursor.movePosition (QTextCursor::End);
        if (0 == cursor.position ())
            cursor.setCharFormat (m_charFormat);
        else if (overwrite) {
            cursor.select(QTextCursor::LineUnderCursor);
            cursor.removeSelectedText();
        } else
            cursor.insertText ("\n");
    }    
    if (forceBold) {
        if (bold_) { 
            m_charFormat.setFontWeight(QFont::Black);
        } else {
            m_charFormat.setFontWeight(QFont::Bold);
        }
    } else {
        if (bold_) { 
            m_charFormat.setFontWeight(QFont::Bold);
        } else {
            m_charFormat.setFontWeight(QFont::Normal);
        }
    }
    m_charFormat.setForeground(QColor(color));
    m_charFormat.setBackground (QColor(bg));
    if (text.length() < 50) {
        int ft = 23; 
        if (text.mid(4,1) == " ") ft = 21;
        cursor.insertText (text.left(ft),m_charFormat);
        m_charFormat.setFontStrikeOut(strikethrough);
        if (DXped) {
            if (underlined)  m_charFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            else  m_charFormat.setUnderlineStyle(QTextCharFormat::DashUnderline);
        } 
        else if (underlined) m_charFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        else m_charFormat.setUnderlineStyle(QTextCharFormat::NoUnderline);
        if (wanted) {
            /* CE3TSK 2026-11-12 (TODO.md 3.-246): the wanted colour follows the inversion, as the line's other two do -
               its text, or with Inverse text/background color its background. It stayed the text (stock JTDX), on the
               line's own colour turned background: 2.00:1 light / 1.66:1 dark on a CQ line, 1.0:1 on a standard call's,
               with the recommended colours. Swapped, every pair is one the recommended palette is checked for. The
               servis column below sets both colours again, so the background ends with the message part. */
            m_charFormat.setFontItalic(true); m_charFormat.setFontOverline(true);
            if (inverted) m_charFormat.setBackground (QColor(color_MyCall_));
            else m_charFormat.setForeground(QColor(color_MyCall_));
            if (!underlined && !DXped) m_charFormat.setFontUnderline(true);
            }
        /* CE3TSK 2026-09-30: the words in doubt get a red wave underline over whatever this segment
           already carries - the spell checker's mark (phantomdecodes.h). A copy of the format, so
           nothing of it leaks into the rest of the line. */
        {
          int pos = ft;
          int const end = qMin (ft + 26, text.size ());
          for (auto const& d : doubtful) {
            int const a = qMax (d.first, pos), b = qMin (d.first + d.second, end);
            if (a >= b) continue;
            cursor.insertText (text.mid (pos, a - pos), m_charFormat);
            cursor.insertText (text.mid (a, b - a), doubtFormat (m_charFormat));
            pos = b;
          }
          cursor.insertText (text.mid (pos, qMax (0, end - pos)), m_charFormat);
        }
        if (wanted) {
            m_charFormat.setFontItalic(false); m_charFormat.setFontOverline(false);
            if (!underlined && !DXped) m_charFormat.setFontUnderline(false);
            }
        m_charFormat.setFontStrikeOut(false);
        m_charFormat.setUnderlineStyle(QTextCharFormat::NoUnderline);
        m_charFormat.setBackground (QColor(servbg));
        m_charFormat.setForeground(QColor(servis_color));
        cursor.insertText (servis.left(1),m_charFormat);
        m_charFormat.setBackground (QColor(Radio::convert_dark("#ffffff",useDarkStyle_)));
        m_charFormat.setForeground(QColor(Radio::convert_dark("#000000",useDarkStyle_)));
        if (cntryDoubtFrom >= 0 && cntryDoubtFrom < cntry.size ()) {   // CE3TSK: ?Chile?
            cursor.insertText (cntry.left (cntryDoubtFrom), m_charFormat);
            cursor.insertText (cntry.mid (cntryDoubtFrom), doubtFormat (m_charFormat));
        } else
            cursor.insertText (cntry,m_charFormat);
        /* CE3TSK 2026-10-20: the distance and azimuth, a part of its own after the country - no wave of the country's
           runs on into it - in the country's colours, at the column (distancefield.h): measured in the font the country
           was just drawn in, the padding's spaces widened by what they fall short of the column */
        if (!field.isEmpty ()) {
            /* (review 3.-161 3) measured on the window's own paint device - its DPI is the one the line is laid out at
               (a second monitor of another DPI gave a CJK name's correction at the wrong scale) */
#if QT_VERSION >= QT_VERSION_CHECK (5, 13, 0)
            QFontMetricsF const fm {m_charFormat.font (), viewport ()};
#else
            QFontMetricsF const fm {m_charFormat.font ()};
#endif
            auto const pad = distance_field::padding (fm.horizontalAdvance (cntry), fm.horizontalAdvance (QLatin1Char (' ')), fieldColumns);
            QTextCharFormat spacing {m_charFormat};
            if (pad.extraPx > 0) {
                spacing.setFontLetterSpacingType (QFont::AbsoluteSpacing);
                spacing.setFontLetterSpacing (pad.extraPx);
            }
            cursor.insertText (QString (pad.spaces, QLatin1Char (' ')), spacing);
            cursor.insertText (field, m_charFormat);
        }
    } else {
        cursor.insertText (text.trimmed(),m_charFormat);
    }
    if (scroll_ && !overwrite) cursor.insertText ("\n");
    else cursor.movePosition (QTextCursor::StartOfLine);
    setTextCursor (cursor);
    ensureCursorVisible ();
    document ()->setMaximumBlockCount (document ()->maximumBlockCount ());
}

int DisplayText::displayDecodedText(DecodedText* decodedText, QString myCall, QString hisCall, QString hisGrid,
                            bool once_notified, LogBook logBook, QsoHistory& qsoHistory,
                            QsoHistory& qsoHistory2, double dialFreq, const QString app_mode,
                            bool bypassRxfFilters,bool bypassAllFilters, int rx_frq,
                            QStringList wantedCallList, QStringList wantedPrefixList, QStringList wantedGridList, 
                            QStringList wantedCountryList, bool windowPopup, QWidget* window)
{
    QString bgColor = Radio::convert_dark("#ffffff",useDarkStyle_);
    QString txtColor = Radio::convert_dark("#000000",useDarkStyle_);
    QString swpColor = "";
    QString servisColor = Radio::convert_dark("#000000",useDarkStyle_);
    QString messageText;
    bool forceBold = false;
    bool strikethrough = false;
    bool underlined = false;
    bool inverted = false;   // CE3TSK 2026-11-12 (TODO.md 3.-246): Inverse text/background color applied to this line
    bool beep = false;
    bool actwind = false;
    bool show_line = true;
    bool jt65bc = false;
    bool notified = false;
    bool new_marker = false;
    int inotified = 0;
    int std_type = 0;
    bool bwantedCall = false;
    bool bwantedPrefix = false;
    bool bwantedGrid = false;
    bool bwantedCountry = false;
    /* CE3TSK 2026-11-17 (MSK144_PLAN.md step 2): MSK144's lines are the FT modes' - hhmmss, the message two columns
       further right. Step 1 left this one out: a message of more than 17 characters stood cut in both windows. */
    if (lines_have_seconds (app_mode)) messageText = decodedText->string().left(49);
    else if (app_mode == "WSPR-2") messageText = decodedText->string().trimmed();
    else messageText = decodedText->string().left(40);
    QString servis = " ";
    QString cntry = " ";
    QString checkCall;
    QString unconfirmedTip;   // CE3TSK 2026-11-22 (TODO.md 3.-280): why the line is in the colour of a LoTW user worked, not confirmed
    QString tierTip;          // CE3TSK 2026-11-22 (the operator): the tier the line's colour shows, for its tooltip
    bool workedTip = false;   // and "worked before": a tier could judge it, and none fired
    bool confirmedTip = false;   // (2026-11-22, D12) worked, and LoTW-confirmed in every tier that is on
    QString lineHead;         // the tooltip's first line: the caller and its DXCC
    QString checkCall2;
    QString grid;
    QString tyyp="";
    QString countryName;
    QString countryName2;
    QString mpx="";
    QString lotw="";
    QsoHistory::Status status = QsoHistory::NONE;
    QsoHistory::Status dummy = QsoHistory::NONE;
    int priority = 0;
    QString param;
    QString report;
    QString checkMode;
    QString rep_type;
    unsigned c_time = 0;
    phantom_decodes::Verdict const& doubt = decodedText->verdict ();   /* CE3TSK 2026-09-30 */
    int cntryDoubtFrom = -1;
    bool workedOnBand = false;   // CE3TSK 2026-11-04: asked of the log below, for a contest where that QSO scores nothing
    bool newCountryOnBand = false, newPrefixOnBand = false;   // the same: the multipliers of a contest that has those
    if (!decodedText->isDebug() && app_mode != "WSPR-2") {
        c_time = decodedText->timeInSeconds();
        if (c_time != 0 && c_time != max_r_time) {
            max_r_time = c_time;
            qsoHistory.time(max_r_time);
            if (!hisCall.isEmpty ()) {
                mystatus_ = qsoHistory2.status(hisCall,mygrid_);
                if (mygrid_.isEmpty ()) mygrid_ = hisGrid;
                myhisCall_ = hisCall;
                }
        }
        QStringList parts = decodedText->message().split (' ', SkipEmptyParts);
        if (!hisCall.isEmpty () && messageText.contains(Radio::base_callsign (hisCall))) txtColor = color_StandardCall_;
        checkCall = decodedText->CQersCall(grid,tyyp);
        checkCall2 = decodedText->call();
        if(!lines_have_seconds (app_mode) && (messageText.contains("2nd-h") || messageText.contains("3rd-h"))) jt65bc = true;
        if (!checkCall.isEmpty ()) {
            if (grid.isEmpty ()) dummy = qsoHistory2.status(checkCall,grid);
            if (grid.isEmpty () && Radio::base_callsign (checkCall) == hisCall) grid = hisGrid;
            if (decodedText->message().left(3) == "DE "){
                tyyp = "";
                if (qAbs(rx_frq - decodedText->frequencyOffset()) < 10 ) {
                    std_type = 2;
                    txtColor = color_MyCall_;
                     
                    if (!grid.isEmpty () && grid != "RR73" && hisCall.isEmpty ()) {
                        status = QsoHistory::RCALL;
                        param = grid;
                    }
                    else if (hisCall.isEmpty () && decodedText->message().right(4) == checkCall) {
                        status = QsoHistory::RCALL;
                    }
                    else if (checkCall.contains(hisCall)  && mystatus_ > QsoHistory::SCQ  && mystatus_ != QsoHistory::FIN) {
                        if (decodedText->report(myCall,Radio::base_callsign (checkCall),report,rep_type,contest_.gridExchange) && !report.isEmpty ()) {
                            if (rep_type == "R")
                                status = QsoHistory::RRREPORT;
                            else
                                status = QsoHistory::RREPORT;
                            param = report;
                        }
                        else if (decodedText->message().contains(" RRR") && mystatus_ > QsoHistory::SREPORT) {
                            status = QsoHistory::RRR;
                        }
                        else if (decodedText->message().contains("RR73") && mystatus_ > QsoHistory::SREPORT) {
                            status = QsoHistory::RRR73;
                        }
                        else if (decodedText->message().contains(" 73") && mystatus_ >= QsoHistory::RRR && mystatus_ != QsoHistory::FIN) { // DE call 73 case
                            status = QsoHistory::R73;
                        }
                        else {
                            std_type = 3;
                            txtColor = Radio::convert_dark("#000000",useDarkStyle_);
                        }
                    } else {
                        std_type = 3;
                        txtColor = Radio::convert_dark("#000000",useDarkStyle_);
                    }
                } else {
                    std_type = 3;
                }
            } else {
                std_type = 1;
                txtColor = color_CQ_;
                status = QsoHistory::RCQ;
                param = grid;
            }
        }
        else if (!myCall.isEmpty () && Radio::base_callsign (checkCall2) == myCall) {
                std_type = 2;
                txtColor = color_MyCall_;
                actwind = true;
                if (beepOnMyCall_) {
                    beep = true;
                }
                decodedText->deCallAndGrid(checkCall, grid);
                /* CE3TSK: WW Digi contest - "MYCALL HISCALL R GRID" is a roger plus his
                   exchange, not a fresh incoming call. Classified as RCALL the sequencer
                   keeps answering with Tx2 and the QSO never advances to RR73. */
                if (contest_.gridExchange && !checkCall.isEmpty () && parts.length() == 4
                    && parts[2] == "R" && isGrid4 (parts[3])) {
                    status = QsoHistory::RRREPORT;
                    param = parts[3];
                    if (grid.isEmpty ()) grid = parts[3];
                }
                else if (!grid.isEmpty () || (!checkCall.isEmpty () && parts.length() == 2)) {
                    status = QsoHistory::RCALL;
                    if (grid.isEmpty ()) dummy = qsoHistory2.status(checkCall,grid);
                    if (grid.isEmpty () && Radio::base_callsign (checkCall) == hisCall) grid = hisGrid;
                    param = grid;
                }
                else {
                    if (grid.isEmpty ()) dummy = qsoHistory2.status(checkCall,grid);
                    if (grid.isEmpty () && Radio::base_callsign (checkCall) == hisCall) grid = hisGrid;
                    if (decodedText->report(myCall,Radio::base_callsign (checkCall),report,rep_type,contest_.gridExchange)) {
                        if (!checkCall.isEmpty ()) {
                            if (!report.isEmpty ()) {
                                if (rep_type == "R")
                                    status = QsoHistory::RRREPORT;
                                else
                                    status = QsoHistory::RREPORT;
                                param = report;
                            }
                            else if (decodedText->message().contains(" RRR")) {
                                status = QsoHistory::RRR;
                            }
                            else if (decodedText->message().contains("RR73")) {
                                status = QsoHistory::RRR73;
                            }
                            else if (decodedText->message().contains(" 73")) {
                                status = QsoHistory::R73;
                            }
                            else {
                                status = QsoHistory::RCALL;
                            }
                        }
                    }            
                    else if (decodedText->message().contains("73") && !hisCall.isEmpty () && mystatus_ >= QsoHistory::RRR && mystatus_ != QsoHistory::FIN) { // nonstandard73 with myCall
                        status = QsoHistory::R73;
                        checkCall = hisCall;
                    }
                    if (!checkCall.isEmpty ()) {
                        /* CE3TSK 2026-10-02: the same station by its base call, not a call that merely contains it -
                           with K1AB in the QSO, K1ABC calling took K1AB's status and grid, and showed K1AB's state (review) */
                        QString const base = Radio::base_callsign (checkCall);
                        if (hisCall.isEmpty () && (myhisCall_.isEmpty () || base != myhisCall_)) {
                            mystatus_ = qsoHistory2.status(base,mygrid_);
                            myhisCall_ = base;
                        }
                        if ((!hisCall.isEmpty () && base == hisCall) || (!myhisCall_.isEmpty () && base == myhisCall_)) {
                            mystatus_ = status;
                            if (grid.isEmpty () && !mygrid_.isEmpty ()) {
                                grid = mygrid_;
                            }
                        }
                    }
                }
        }
        else {
                decodedText->deCallAndGrid(checkCall, grid);
                if (!checkCall.isEmpty ()) {
                    if (grid.isEmpty ()) dummy = qsoHistory2.status(checkCall,grid);
                    if (grid.isEmpty () && Radio::base_callsign (checkCall) == hisCall) grid = hisGrid;
                    if (!decodedText->isNonStd1() && !decodedText->isNonStd2()) { 
                        std_type = 3;
                        if (!grid.isEmpty ()) param = grid;
                        if (!hisCall.isEmpty () && checkCall.contains(hisCall)) qsoHistory.rx(checkCall,decodedText->frequencyOffset());
                    } else if (!hisCall.isEmpty () && checkCall.contains(hisCall) && qAbs(rx_frq - decodedText->frequencyOffset()) < 10 && decodedText->message().contains("73") && mystatus_ >= QsoHistory::RRR && mystatus_ != QsoHistory::FIN) { //nonstandard73 with hisCall
                        std_type = 2;
                        txtColor = color_MyCall_;
                        status = QsoHistory::R73;
                        mystatus_ = status;
                    } else if (!hisCall.isEmpty () && checkCall.contains(hisCall)) {
                        qsoHistory.rx(checkCall,decodedText->frequencyOffset());
                        checkCall = "";
                    } else if (!decodedText->isNonStd2()) {
                        std_type = 3;
                        if (!grid.isEmpty ()) param = grid;
                        if (!hisCall.isEmpty () && checkCall.contains(hisCall)) qsoHistory.rx(checkCall,decodedText->frequencyOffset());
                    } else {
                        checkCall = "";
                    }
                    if (!checkCall.isEmpty () && RR73Marker_ && (decodedText->message().contains("RR73") || decodedText->message().contains(" 73"))) {
                        std_type = 4;
                        txtColor = color_CQ_;
                        status = QsoHistory::RFIN;
                    }
                } else if (std_type == 0 && !hisCall.isEmpty () && qAbs(rx_frq - decodedText->frequencyOffset()) < 10 && decodedText->message().contains("73") && mystatus_ >= QsoHistory::RRR && mystatus_ != QsoHistory::FIN) { // nonstandard 73 in my rx
                    std_type = 2;
                    txtColor = color_MyCall_;
                    checkCall = hisCall;
                    status = QsoHistory::R73;
                    mystatus_ = status;
                }
        }
    } else checkCall = "";
    if (!checkCall.isEmpty ()) {

        bool cqzB4 = true;
        bool ituzB4 = true;
        bool countryB4 = true;
        bool pxB4 = true;
        bool callB4 = true;
        bool cqzB4BandMode = true;
        bool ituzB4BandMode = true;
        bool countryB4BandMode = true;
        bool pxB4BandMode = true;
        bool callB4BandMode = true;
        bool gridB4 = true;
        bool gridB4BandMode = true;
        bool stateB4 = true;        /* CE3TSK 2026-10-03: Worked All States */
        bool stateB4Band = true;
        bool stateJudged = false;   // a US call in a square of a single state: the tier has a state to judge
        QString judgedState;        // CE3TSK 2026-11-22: that state (matchState), named by the unconfirmed tooltip
        /* CE3TSK 2026-10-02: the grid a US station's state is read from - the one this line carries, from its sender,
           or else the one this very call sent before. Not the window's own grid: that one comes from the history of
           the BASE call (and from the DX boxes), so VE3ABC's FN03 would put VE3ABC/W1 in New York (review). */
        // parsed once, and only for a US entity's call - asked for by the state lookup and the state display (review)
        QString stateGridValue;
        bool stateGridParsed = false;
        auto stateGrid = [&] () -> QString const& {
            if (!stateGridParsed) {
                stateGridParsed = true;
                auto const said = phantom_decodes::parts (phantom_decodes::messageField (decodedText->string ()));
                stateGridValue = phantom_decodes::bareCall (said.sender) == checkCall && phantom_decodes::judgeableGrid (said.grid)
                                 ? said.grid : qsoHistory2.gridSentBy (checkCall);
            }
            return stateGridValue;
        };
        logBook.getLOTW(/*in*/ checkCall, /*out*/ lotw);
        if (!lotw.isEmpty ()) {
            priority = dp::LoTW;
        }
        if (displayPotential_ && std_type == 3) {
            txtColor = color_StandardCall_;
        }
        if (app_mode == "JT9+JT65") {
            if (decodedText->isJT9()) {
                checkMode = "JT9";
            } else if (decodedText->isJT65()) { // TODO: is this if-condition necessary?
                checkMode = "JT65";
            }
        } else {
            checkMode = app_mode;
        }
        if (!jt65bc && (displayCountryName_ || anyTier_)) {
            if (!displayNewCQZ_ && !displayNewITUZ_ && !displayNewDXCC_ && displayCountryName_ && !displayNewCall_ && !displayNewPx_) {
                        logBook.getDXCC(/*in*/ checkCall, /*out*/ countryName);
                    }
            if (displayNewCQZ_) {
                if (displayNewCQZBand_ || displayNewCQZBandMode_) {
                    if (displayNewCQZBand_ && displayNewCQZBandMode_) {
                        logBook.matchCQZ(/*in*/checkCall,/*out*/countryName,cqzB4,cqzB4BandMode,/*in*/dialFreq,checkMode);
                    } else if (displayNewCQZBand_){
                        logBook.matchCQZ(/*in*/checkCall,/*out*/countryName,cqzB4,cqzB4BandMode,/*in*/dialFreq);
                    } else {
                        logBook.matchCQZ(/*in*/checkCall,/*out*/countryName,cqzB4,cqzB4BandMode,/*in*/0,checkMode);
                    }
                } else {
                    logBook.matchCQZ(/*in*/ checkCall, /*out*/ countryName, cqzB4 ,cqzB4BandMode);
                }
            }
            if (displayNewITUZ_) {
                if (displayNewITUZBand_ || displayNewITUZBandMode_) {
                    if (displayNewITUZBand_ && displayNewITUZBandMode_) {
                        logBook.matchITUZ(/*in*/checkCall,/*out*/countryName,ituzB4,ituzB4BandMode,/*in*/dialFreq,checkMode);
                    } else if (displayNewITUZBand_){
                        logBook.matchITUZ(/*in*/checkCall,/*out*/countryName,ituzB4,ituzB4BandMode,/*in*/dialFreq);
                    } else {
                        logBook.matchITUZ(/*in*/checkCall,/*out*/countryName,ituzB4,ituzB4BandMode,/*in*/0,checkMode);
                    }
                } else {
                    logBook.matchITUZ(/*in*/ checkCall, /*out*/ countryName, ituzB4 ,ituzB4BandMode);
                }
            }
            if (displayNewDXCC_) {
                if (displayNewDXCCBand_ || displayNewDXCCBandMode_) {
                    if (displayNewDXCCBand_ && displayNewDXCCBandMode_) {
                        logBook.matchDXCC(/*in*/checkCall,/*out*/countryName,countryB4,countryB4BandMode,/*in*/dialFreq,checkMode);
                    } else if (displayNewDXCCBand_){
                        logBook.matchDXCC(/*in*/checkCall,/*out*/countryName,countryB4,countryB4BandMode,/*in*/dialFreq);
                    } else {
                        logBook.matchDXCC(/*in*/checkCall,/*out*/countryName,countryB4,countryB4BandMode,/*in*/0,checkMode);
                    }
                } else {
                    logBook.matchDXCC(/*in*/ checkCall, /*out*/ countryName, countryB4 ,countryB4BandMode);
                }
            }
            /* CE3TSK 2026-10-03: Worked All States - a state only from a square of a single state, here and in the log
               (us_states::clearState). Never in WW Digi, whatever the profile says. The country is fetched when nothing
               above did: the code below reads it. */
            if (displayNewState_ && !contest_.running) {
                if (countryName.isEmpty ()) logBook.getDXCC (checkCall, countryName);
                QString const prefix = countryName.section (',', 1, 1).trimmed ();
                if (us_states::usEntity (prefix))   // the state worked out once (matchState answers with it)
                    stateJudged = !(judgedState = logBook.matchState (prefix, checkCall, stateGrid (), stateB4, stateB4Band, displayNewStateBand_ ? dialFreq : 0,
                                                                      displayNewStateBandMode_ ? checkMode : QString {})).isEmpty ();
            }
            /* CE3TSK: WW Digi contest - contest_.fieldMultipliers switches these lookups from the 4 character
               square to the 2 character field, which is the contest multiplier. The square
               scores nothing in WW Digi, so the two are mutually exclusive and the existing
               new grid settings, colours and beep simply describe a field instead. */
            // CE3TSK 2026-10-03: one square asked as the options count it - the decode's own, and below (item 6) one heard before
            auto matchGridAt = [&] (QString const& square, bool& b4, bool& b4BandMode) {
                if (displayNewGridBand_ || displayNewGridBandMode_) {
                    if (displayNewGridBand_ && displayNewGridBandMode_) {
                        logBook.matchGrid(/*in*/square,/*out*/b4,b4BandMode,/*in*/dialFreq,checkMode,contest_.fieldMultipliers);
                    } else if (displayNewGridBand_) {
                        logBook.matchGrid(/*in*/square,/*out*/b4,b4BandMode,/*in*/dialFreq,"",contest_.fieldMultipliers);
                    } else {
                        logBook.matchGrid(/*in*/square,/*out*/b4,b4BandMode,/*in*/0,checkMode,contest_.fieldMultipliers);
                    }
                } else {
                    logBook.matchGrid(/*in*/ square, /*out*/ b4 ,b4BandMode,/*in*/0,"",contest_.fieldMultipliers);
                }
            };
            if (displayNewGrid_) matchGridAt (grid.trimmed (), gridB4, gridB4BandMode);
            if (displayNewPx_) {
                if (displayNewPxBand_ || displayNewPxBandMode_) {
                    if (displayNewPxBand_ && displayNewPxBandMode_) {
                        logBook.matchPX(/*in*/checkCall,/*out*/countryName,pxB4,pxB4BandMode,/*in*/dialFreq,checkMode);
                    } else if (displayNewPxBand_) {
                        logBook.matchPX(/*in*/checkCall,/*out*/countryName,pxB4,pxB4BandMode,/*in*/dialFreq);
                    } else {
                        logBook.matchPX(/*in*/checkCall,/*out*/countryName,pxB4,pxB4BandMode,/*in*/0,checkMode);
                    }
                } else {
                    logBook.matchPX(/*in*/ checkCall, /*out*/ countryName, pxB4 ,pxB4BandMode);
                }
            }
            if (displayNewCall_) {
                if (displayNewCallBand_ || displayNewCallBandMode_) {
                    if (displayNewCallBand_ && displayNewCallBandMode_) {
                        logBook.matchCall(/*in*/checkCall,/*out*/countryName,callB4,callB4BandMode,/*in*/dialFreq,checkMode);
                    } else if (displayNewCallBand_) {
                        logBook.matchCall(/*in*/checkCall,/*out*/countryName,callB4,callB4BandMode,/*in*/dialFreq);
                    } else {
                        logBook.matchCall(/*in*/checkCall,/*out*/countryName,callB4,callB4BandMode,/*in*/0,checkMode);
                    }
                } else {
                    logBook.matchCall(/*in*/ checkCall, /*out*/ countryName, callB4 ,callB4BandMode);
                }
                workedOnBand = callB4BandMode;
                newCountryOnBand = !countryB4BandMode;
                newPrefixOnBand = !pxB4BandMode;
            }

            /* CE3TSK 2026-11-21 (TODO.md 3.-280, QSL_CONFIRMATIONS_PLAN.md D3, D4, D7, D8, D9): a LoTW user worked but not
               LoTW-confirmed counts as never worked in every tier that is on - the decode ranks as that tier, so the
               sequencer calls it as new, and shows the colour of its own (the chain's end). Only a station in the LoTW users
               file (D8), never in a contest, and nothing while no confirmations were downloaded. Each tier is asked of the
               confirmations as of the log - band and mode as the options count them - and starts at "confirmed": a lookup
               that cannot judge (no call, no grid, an unknown country) leaves it so, as the log side starts at "worked".
               workedOnBand and the two above stay the log's: they are about having worked the station (dupes, Hide). */
            bool unconfirmedAt[dp::NewField + 2] = {};   // by the level a tier's new decode ranks at, without the LoTW step
            if (countUnconfirmed_ && confirmed_ && !lotw.isEmpty () && !contest_.running) {
                QString confirmedName {countryName};   // the country the log side found, so each lookup judges the same
                auto const confirm = [&] (bool worked, bool workedBandMode, bool confirmedAll, bool confirmedBandMode,
                                          bool& b4, bool& b4BandMode, int level, int bandLevel) {
                    if (worked && !confirmedAll) {b4 = false; unconfirmedAt[level] = true;}
                    if (workedBandMode && !confirmedBandMode) {b4BandMode = false; unconfirmedAt[bandLevel] = true;}
                };
                auto const tier = [&] (void (LogBook::* match) (QString, QString&, bool&, bool&, double, QString),
                                       bool on, bool band, bool bandMode, bool& b4, bool& b4BandMode, int level, int bandLevel) {
                    if (!on || (!b4 && !b4BandMode)) return;
                    bool c4 = true, c4BandMode = true;
                    (confirmed_->*match) (checkCall, confirmedName, c4, c4BandMode, band ? dialFreq : 0, bandMode ? checkMode : QString {});
                    if (pending_ && (!c4 || !c4BandMode)) {   // (2026-11-22) or confirmed for now (D10)
                        bool p4 = false, p4BandMode = true;   // (the review) a lookup given no band leaves its band answer: as one book did
                        (pending_->*match) (checkCall, confirmedName, p4, p4BandMode, band ? dialFreq : 0, bandMode ? checkMode : QString {});
                        c4 = c4 || p4; c4BandMode = c4BandMode || p4BandMode;
                    }
                    confirm (b4, b4BandMode, c4, c4BandMode, b4, b4BandMode, level, bandLevel);
                };
                tier (&LogBook::matchCQZ, displayNewCQZ_, displayNewCQZBand_, displayNewCQZBandMode_, cqzB4, cqzB4BandMode, dp::NewCQZ, dp::NewCQZBand);
                tier (&LogBook::matchITUZ, displayNewITUZ_, displayNewITUZBand_, displayNewITUZBandMode_, ituzB4, ituzB4BandMode, dp::NewITUZ, dp::NewITUZBand);
                tier (&LogBook::matchDXCC, displayNewDXCC_, displayNewDXCCBand_, displayNewDXCCBandMode_, countryB4, countryB4BandMode, dp::NewDXCC, dp::NewDXCCBand);
                tier (&LogBook::matchPX, displayNewPx_, displayNewPxBand_, displayNewPxBandMode_, pxB4, pxB4BandMode, dp::NewPx, dp::NewPxBand);
                tier (&LogBook::matchCall, displayNewCall_, displayNewCallBand_, displayNewCallBandMode_, callB4, callB4BandMode, dp::NewCall, dp::NewCallBand);
                if (displayNewGrid_ && !grid.trimmed ().isEmpty () && (gridB4 || gridB4BandMode)) {
                    bool c4 = true, c4BandMode = true;
                    confirmed_->matchGrid (grid.trimmed (), c4, c4BandMode, (displayNewGridBand_ ? dialFreq : 0), (displayNewGridBandMode_ ? checkMode : QString {}));
                    if (pending_ && (!c4 || !c4BandMode)) {   // (2026-11-22) or confirmed for now (D10)
                        bool p4 = false, p4BandMode = true;   // (the review) as above
                        pending_->matchGrid (grid.trimmed (), p4, p4BandMode, (displayNewGridBand_ ? dialFreq : 0), (displayNewGridBandMode_ ? checkMode : QString {}));
                        c4 = c4 || p4; c4BandMode = c4BandMode || p4BandMode;
                    }
                    confirm (gridB4, gridB4BandMode, c4, c4BandMode, gridB4, gridB4BandMode, dp::NewGrid, dp::NewGridBand);
                }
                if (displayNewState_ && stateJudged && (stateB4 || stateB4Band)) {
                    bool c4 = true, c4Band = true;
                    confirmed_->matchState (countryName.section (',', 1, 1).trimmed (), checkCall, stateGrid (), c4, c4Band,
                                            displayNewStateBand_ ? dialFreq : 0, displayNewStateBandMode_ ? checkMode : QString {});
                    if (pending_ && (!c4 || !c4Band)) {   // (2026-11-22) or confirmed for now (D10)
                        bool p4 = false, p4Band = false;
                        pending_->matchState (countryName.section (',', 1, 1).trimmed (), checkCall, stateGrid (), p4, p4Band,
                                              displayNewStateBand_ ? dialFreq : 0, displayNewStateBandMode_ ? checkMode : QString {});
                        c4 = c4 || p4; c4Band = c4Band || p4Band;
                    }
                    confirm (stateB4, stateB4Band, c4, c4Band, stateB4, stateB4Band, dp::NewState, dp::NewStateBand);
                }
            }

            if (anyTier_) {
//Worked
                /* CE3TSK 2026-10-03 (review items 7 and 6, the operator): a decode is "worked" only when a tier that is on
                   could judge it. The call tiers - zones, DXCC, prefix, call - judge every decode; the new US state only a
                   US call in a square of a single state; the grid tier one with a grid - its own or the one the history
                   holds for the call (what matchGrid above was given), or else the grid this call last sent on this band
                   in any line. That last one because a hidden line never reaches the history (so the autoselect never
                   picks it): with "don't show it", a station whose CQ in a worked square was hidden would have no grid
                   for its reports and RR73 after it. Heard in a worked square, the decode is worked; in a new one it is
                   not judged - the ranking and the history never see that grid. A decode no tier on could judge - a
                   report from a call never heard with a grid when only grid and state are on, a DX call when only the
                   state is - is shown plainly and never hidden by "don't show it". */
                bool gridJudged = false;
                QString judgedSquare;   // (the review 2026-11-22, D12) the square the grid tier judged by, for the confirmations
                if (displayNewGrid_ && !checkCall.isEmpty ()) {
                    QString const key = checkCall + QLatin1Char ('@') + ADIF::bandFromFrequency (dialFreq / 1.e6);
                    if (!grid.trimmed ().isEmpty ()) {
                        if (gridsHeard_.size () > 20000) gridsHeard_.clear ();   // a bound a session never reaches
                        gridsHeard_.insert (key, grid.trimmed ());
                        gridJudged = true;
                        judgedSquare = grid.trimmed ();
                    } else if (gridsHeard_.contains (key)) {
                        bool b4 = true, b4BandMode = true;
                        matchGridAt (gridsHeard_.value (key), b4, b4BandMode);
                        gridJudged = b4 && b4BandMode;
                        if (gridJudged) judgedSquare = gridsHeard_.value (key);
                    }
                }
                bool const judged = callTiers_ || gridJudged || (displayNewState_ && stateJudged);
                if (!judged) {
                } else if ((displayPotential_ && std_type == 3) || (std_type != 3)) {
                    if (displayWorkedColor_) {
                        bgColor = color_WorkedCall_;
                    }
                    if (displayWorkedStriked_) {
                        strikethrough = true;
                    } else if (displayWorkedUnderlined_) {
                        underlined = true;
                    }
                } else if (displayWorkedColor_ && otherMessagesMarker_) servis += color_WorkedCall_;

                /* CE3TSK: WW Digi contest - the 2 character field is the only multiplier, so
                   it is placed at the head of the chain, above DXCC and above the zones.
                   None of those score anything in this contest, and leaving the tier in its
                   usual place below DXCC would let a new DXCC or a new zone mask a new
                   multiplier. The matching grid tiers further down carry !contest_.fieldMultipliers so they
                   cannot fire as well. */
                /* CE3TSK: never worked, or not yet worked on this band - one multiplier either
                   way, so one block. WW Digi counts a field once per band, so "new on this band"
                   is a multiplier exactly as "never worked" is: painted in the new-grid colour
                   rather than the new-grid-band one (two colours for one fact only made a fresh
                   band look like everyday operation) and ranked equal, so between the two the
                   SNR tie-break decides. The two conditions carried identical bodies as separate
                   branches; a later fix to one would have left the other behind. */
                /* (2026-11-04) a station already worked on the band in a field not worked there: a repeat scores nothing and brings no
                   multiplier (contest_score::sum ()), so it does not head the chain. It may only be a phantom's wrong grid anyway. */
                if (contest_.fieldMultipliers && !(contest_.workedRanksLast && workedOnBand) && ((displayNewGrid_ && !gridB4)
                                || ((displayNewGridBand_ || displayNewGridBandMode_) && !gridB4BandMode))) {
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = color_NewGrid_;
                        if (!lotw.isEmpty ()) priority = dp::NewField + 1;
                        else priority = dp::NewField;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewGrid_) {
                            beep = true;
                        }
                    }
                    else if (otherMessagesMarker_) {
                        servis = servis.left(1) + color_NewGrid_;
                        if (!lotw.isEmpty ()) priority = dp::NewField + 1;
                        else priority = dp::NewField;
                        new_marker = true;
                    }
                } else if (displayNewCQZ_ && !cqzB4) {
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = color_NewCQZ_;
                        if (!lotw.isEmpty ()) priority = dp::NewCQZ + 1;
                        else priority = dp::NewCQZ;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewCQZ_) {
                            beep = true;
                        }
                    }
                    else if (otherMessagesMarker_) {
                        servis = servis.left(1) + color_NewCQZ_;
                        if (!lotw.isEmpty ()) priority = dp::NewCQZ + 1;
                        else priority = dp::NewCQZ;
                        new_marker = true;
                    }
                } else if ((displayNewCQZBand_ || displayNewCQZBandMode_) && !cqzB4BandMode) {
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = color_NewCQZBand_;
                        if (!lotw.isEmpty ()) priority = dp::NewCQZBand + 1;
                        else priority = dp::NewCQZBand;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewCQZ_) {
                            beep = true;
                        }
                    }
                    else if (otherMessagesMarker_) {
                        servis = servis.left(1) + color_NewCQZBand_;
                        if (!lotw.isEmpty ()) priority = dp::NewCQZBand + 1;
                        else priority = dp::NewCQZBand;
                        new_marker = true;
                    }
                } else if (displayNewITUZ_ && !ituzB4) {
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = color_NewITUZ_;
                        if (!lotw.isEmpty ()) priority = dp::NewITUZ + 1;
                        else priority = dp::NewITUZ;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewITUZ_) {
                            beep = true;
                        }
                    }
                    else if (otherMessagesMarker_) {
                        servis = servis.left(1) + color_NewITUZ_;
                        if (!lotw.isEmpty ()) priority = dp::NewITUZ + 1;
                        else priority = dp::NewITUZ;
                        new_marker = true;
                    }
                } else if ((displayNewITUZBand_ || displayNewITUZBandMode_) && !ituzB4BandMode) {
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = color_NewITUZBand_;
                        if (!lotw.isEmpty ()) priority = dp::NewITUZBand + 1;
                        else priority = dp::NewITUZBand;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewITUZ_) {
                            beep = true;
                        }
                    }
                    else if (otherMessagesMarker_) {
                        servis = servis.left(1) + color_NewITUZBand_;
                        if (!lotw.isEmpty ()) priority = dp::NewITUZBand + 1;
                        else priority = dp::NewITUZBand;
                        new_marker = true;
                    }
                } else if (displayNewDXCC_ && !countryB4) {
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = color_NewDXCC_;
                        if (!lotw.isEmpty ()) priority = dp::NewDXCC + 1;
                        else priority = dp::NewDXCC;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewDXCC_) {
                            beep = true;
                        }
                    }
                    else if (otherMessagesMarker_) {
                        servis = servis.left(1) + color_NewDXCC_;
                        if (!lotw.isEmpty ()) priority = dp::NewDXCC + 1;
                        else priority = dp::NewDXCC;
                        new_marker = true;
                    }
                } else if ((displayNewDXCCBand_ || displayNewDXCCBandMode_) && !countryB4BandMode) {
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = color_NewDXCCBand_;
                        if (!lotw.isEmpty ()) priority = dp::NewDXCCBand + 1;
                        else priority = dp::NewDXCCBand;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewDXCC_) {
                            beep = true;
                        }
                    }
                    else if (otherMessagesMarker_) {
                        servis = servis.left(1) + color_NewDXCCBand_;
                        if (!lotw.isEmpty ()) priority = dp::NewDXCCBand + 1;
                        else priority = dp::NewDXCCBand;
                        new_marker = true;
                    }
                /* CE3TSK 2026-10-03: Worked All States, below new DXCC and above new grid, with a level of its own - below the
                   wanted lists too (the operator). A state counts only from a square of a single state (us_states::clearState).
                   Its behaviour is chosen in priorities.h, not inherited from the range it sits in: the answer counter gives up
                   on it as on a new grid, it is not called while we call CQ, the strict directional CQ exception stays DXCC's,
                   and the wanted lists lift it into their band. Off in a contest: the dialog forces it off with the other
                   unscored tiers, and the lookup above is skipped in WW Digi even where a profile was never through it. */
                } else if (displayNewState_ && !stateB4) {
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = color_NewState_;
                        if (!lotw.isEmpty ()) priority = dp::NewState + 1;
                        else priority = dp::NewState;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewState_) {
                            beep = true;
                        }
                    }
                    else if (otherMessagesMarker_) {
                        servis = servis.left(1) + color_NewState_;
                        if (!lotw.isEmpty ()) priority = dp::NewState + 1;
                        else priority = dp::NewState;
                        new_marker = true;
                    }
                } else if ((displayNewStateBand_ || displayNewStateBandMode_) && !stateB4Band) {   // CE3TSK 2026-10-04: or per mode
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = color_NewStateBand_;
                        if (!lotw.isEmpty ()) priority = dp::NewStateBand + 1;
                        else priority = dp::NewStateBand;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewState_) {
                            beep = true;
                        }
                    }
                    else if (otherMessagesMarker_) {
                        servis = servis.left(1) + color_NewStateBand_;
                        if (!lotw.isEmpty ()) priority = dp::NewStateBand + 1;
                        else priority = dp::NewStateBand;
                        new_marker = true;
                    }
                /* CE3TSK: !contest_.fieldMultipliers - in contest mode this tier is handled at the head of the
                   chain instead, at a priority above DXCC */
                } else if (!contest_.fieldMultipliers && displayNewGrid_ && !gridB4) {
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = color_NewGrid_;
                        if (!lotw.isEmpty ()) priority = dp::NewGrid + 1;
                        else priority = dp::NewGrid;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewGrid_) {
                            beep = true;
                        }
                    }
                    else  if (otherMessagesMarker_) {
                        servis = servis.left(1) + color_NewGrid_;
                        if (!lotw.isEmpty ()) priority = dp::NewGrid + 1;
                        else priority = dp::NewGrid;
                        new_marker = true;
                    }
                } else if (!contest_.fieldMultipliers && (displayNewGridBand_ || displayNewGridBandMode_) && !gridB4BandMode) { /* CE3TSK: see above */
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = color_NewGridBand_;
                        if (!lotw.isEmpty ()) priority = dp::NewGridBand + 1;
                        else priority = dp::NewGridBand;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewGrid_) {
                            beep = true;
                        }
                    }
                    else if (otherMessagesMarker_) {
                        servis = servis.left(1) + color_NewGridBand_;
                        if (!lotw.isEmpty ()) priority = dp::NewGridBand + 1;
                        else priority = dp::NewGridBand;
                        new_marker = true;
                    }
                } else  if (displayNewPx_ && !pxB4) {
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = color_NewPx_;
                        if (!lotw.isEmpty ()) priority = dp::NewPx + 1;
                        else priority = dp::NewPx;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewPx_) {
                            beep = true;
                        }
                    }
                    else  if (otherMessagesMarker_) {
                        servis = servis.left(1) + color_NewPx_;
                        if (!lotw.isEmpty ()) priority = dp::NewPx + 1;
                        else priority = dp::NewPx;
                        new_marker = true;
                    }
                } else if ((displayNewPxBand_ || displayNewPxBandMode_) && !pxB4BandMode) {
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = color_NewPxBand_;
                        if (!lotw.isEmpty ()) priority = dp::NewPxBand + 1;
                        else priority = dp::NewPxBand;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewPx_) {
                            beep = true;
                        }
                    }
                    else  if (otherMessagesMarker_) {
                        servis = servis.left(1) + color_NewPxBand_;
                        if (!lotw.isEmpty ()) priority = dp::NewPxBand + 1;
                        else priority = dp::NewPxBand;
                        new_marker = true;
                    }
                } else  if (displayNewCall_ && !callB4) {
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = color_NewCall_;
                        if (!lotw.isEmpty ()) priority = dp::NewCall + 1;
                        else priority = dp::NewCall;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewCall_) {
                            beep = true;
                        }
                    }
                    else  if (otherMessagesMarker_) {
                        servis = servis.left(1) + color_NewCall_;
                        if (!lotw.isEmpty ()) priority = dp::NewCall + 1;
                        else priority = dp::NewCall;
                        new_marker = true;
                    }
                } else if ((displayNewCallBand_ || displayNewCallBandMode_) && !callB4BandMode) {
                    if ((displayPotential_ && std_type == 3) || std_type != 3) {
                        forceBold = true;
                        bgColor = contest_.running ? color_NewCall_ : color_NewCallBand_;   /* CE3TSK: one colour in a contest - once per band is the dupe rule itself */
                        if (!lotw.isEmpty ()) priority = dp::NewCallBand + 1;
                        else priority = dp::NewCallBand;
                        strikethrough = false;
                        underlined = false;
                        actwind = true;
                        if (beepOnNewCall_) {
                            beep = true;
                        }
                    }
                    else  if (otherMessagesMarker_) {
                        servis = servis.left(1) + (contest_.running ? color_NewCall_ : color_NewCallBand_);
                        if (!lotw.isEmpty ()) priority = dp::NewCallBand + 1;
                        else priority = dp::NewCallBand;
                        new_marker = true;
                    }
                } 
                /* 2026-11-22 (the operator): what the tier that fired counts - the zone, the country as the column names it,
                   the prefix, the call, the square, the state, WW Digi's field - and, for an "on band/mode" tier, the band
                   and the mode it was asked for: the line's tooltip names it (tierTip), and the unconfirmed one's below */
                int const level {priority - (lotw.isEmpty () ? 0 : 1)};   // a LoTW user's tier ranks one above it
                QString worked, value;
                bool band = false, mode = false;
                switch (level) {
                case dp::NewField:   // (review) one tier for never worked and new on this band: the band named for the second
                    value = worked = grid.trimmed ().left (2);
                    if (!(displayNewGrid_ && !gridB4)) {band = displayNewGridBand_; mode = displayNewGridBandMode_;}
                    break;
                case dp::NewCQZBand: band = displayNewCQZBand_; mode = displayNewCQZBandMode_; Q_FALLTHROUGH ();
                case dp::NewCQZ: value = countryName.section (',', 3, 3).trimmed (); worked = tr ("CQ zone %1").arg (value); break;
                case dp::NewITUZBand: band = displayNewITUZBand_; mode = displayNewITUZBandMode_; Q_FALLTHROUGH ();
                case dp::NewITUZ: value = countryName.section (',', 4, 4).trimmed (); worked = tr ("ITU zone %1").arg (value); break;
                case dp::NewDXCCBand: band = displayNewDXCCBand_; mode = displayNewDXCCBandMode_; Q_FALLTHROUGH ();
                case dp::NewDXCC: value = worked = countryName.section (',', 2, 2).trimmed (); break;
                case dp::NewPxBand: band = displayNewPxBand_; mode = displayNewPxBandMode_; Q_FALLTHROUGH ();
                case dp::NewPx: value = Radio::striped_prefix (Radio::effective_prefix (checkCall)); worked = tr ("prefix %1").arg (value); break;
                case dp::NewCallBand: band = displayNewCallBand_; mode = displayNewCallBandMode_; Q_FALLTHROUGH ();
                case dp::NewCall: value = worked = checkCall; break;
                case dp::NewGridBand: band = displayNewGridBand_; mode = displayNewGridBandMode_; Q_FALLTHROUGH ();
                case dp::NewGrid: value = grid.trimmed ().left (4); worked = tr ("grid %1").arg (value); break;
                case dp::NewStateBand: band = displayNewStateBand_; mode = displayNewStateBandMode_; Q_FALLTHROUGH ();
                case dp::NewState: value = judgedState; worked = tr ("US state %1").arg (value); break;
                }
                QStringList where;
                if (band) where << ADIF::bandFromFrequency (dialFreq / 1.e6);
                if (mode) where << checkMode;
                where.removeAll (QString {});
                if (!value.isEmpty ()) tierTip = tierSentence (level, value, where.join (' '));
                else workedTip = judged && dp::noNewTier (priority)   // (review) where a tier could be drawn at all: else a new
                                 && ((displayPotential_ && std_type == 3) || std_type != 3 || otherMessagesMarker_);   // one goes unseen
                /* CE3TSK 2026-11-22 (TODO.md 3.-280, D12; the operator: "only if the call sign appears as worked considering
                   all the selected notification settings does the LotW confirmed highlight come into play as lowest priority"):
                   worked in every tier that is on - nothing fired - and LoTW-confirmed in each, as that tier counts it (its
                   band and mode): the colour of its own where the worked state is drawn, when Settings asks (off by default),
                   and the tooltip says so. A tier that cannot judge (no grid, no state) is left out, as on the log's side.
                   Never in a contest; nothing while no confirmations were downloaded; the rank is the worked one's. */
                if (workedTip && confirmed_ && !contest_.running) {
                    confirmedTip = true;
                    int asked = 0;   // (the review 2026-11-22) the tiers that could judge it: none, and nothing is proved
                    QString confirmedName {countryName};
                    /* (the review) a call of no country is "worked" in the country tiers by the log's lookups (inherited) and by
                       the confirmations' alike: they prove nothing there - left out, as a tier that cannot judge */
                    bool const placed {countryName.section (',', 1, 1).trimmed () != "?"};
                    auto const held = [&] (void (LogBook::* match) (QString, QString&, bool&, bool&, double, QString), bool on, bool band, bool bandMode,
                                           bool needsCountry) {
                        if (!on || !confirmedTip || (needsCountry && !placed)) return;
                        ++asked;
                        bool c4 = false, c4BandMode = false;
                        (confirmed_->*match) (checkCall, confirmedName, c4, c4BandMode, band ? dialFreq : 0, bandMode ? checkMode : QString {});
                        if (!c4 || ((band || bandMode) && !c4BandMode)) confirmedTip = false;
                    };
                    held (&LogBook::matchCQZ, displayNewCQZ_, displayNewCQZBand_, displayNewCQZBandMode_, true);
                    held (&LogBook::matchITUZ, displayNewITUZ_, displayNewITUZBand_, displayNewITUZBandMode_, true);
                    held (&LogBook::matchDXCC, displayNewDXCC_, displayNewDXCCBand_, displayNewDXCCBandMode_, true);
                    held (&LogBook::matchPX, displayNewPx_, displayNewPxBand_, displayNewPxBandMode_, false);
                    held (&LogBook::matchCall, displayNewCall_, displayNewCallBand_, displayNewCallBandMode_, false);
                    // the square the grid tier judged by - the decode's own, or (a report without one) the one heard before
                    if (confirmedTip && displayNewGrid_ && !judgedSquare.isEmpty ()) {
                        ++asked;
                        bool c4 = false, c4BandMode = false;
                        confirmed_->matchGrid (judgedSquare, c4, c4BandMode, (displayNewGridBand_ ? dialFreq : 0), (displayNewGridBandMode_ ? checkMode : QString {}));
                        if (!c4 || ((displayNewGridBand_ || displayNewGridBandMode_) && !c4BandMode)) confirmedTip = false;
                    }
                    if (confirmedTip && displayNewState_ && stateJudged) {
                        ++asked;
                        bool c4 = false, c4Band = false;
                        confirmed_->matchState (countryName.section (',', 1, 1).trimmed (), checkCall, stateGrid (), c4, c4Band,
                                                displayNewStateBand_ ? dialFreq : 0, displayNewStateBandMode_ ? checkMode : QString {});
                        if (!c4 || ((displayNewStateBand_ || displayNewStateBandMode_) && !c4Band)) confirmedTip = false;
                    }
                    if (0 == asked) confirmedTip = false;
                    if (confirmedTip && highlightConfirmed_) {
                        if ((displayPotential_ && std_type == 3) || std_type != 3) bgColor = color_Confirmed_;
                        else if (otherMessagesMarker_) servis = servis.left (1) + color_Confirmed_;
                    }
                }
                /* CE3TSK 2026-11-21 (TODO.md 3.-280, D3): the tier that fired is new only because the LoTW user worked in it
                   has not LoTW-confirmed - the colour of its own, the tier's rank kept (a LoTW user's: the level + 1) */
                if (priority > 0 && priority - 1 < int (sizeof unconfirmedAt) && unconfirmedAt[priority - 1]) {
                    if (new_marker) servis = servis.left (1) + color_Unconfirmed_;
                    else bgColor = color_Unconfirmed_;
                    // 2026-11-22 (the operator): and the tooltip says why - what was worked in that tier, where it was asked
                    if (!where.isEmpty ()) worked += " (" + where.join (' ') + ")";
                    unconfirmedTip = unconfirmedToolTip (checkCall, worked);
                }
                if (judged && displayWorkedDontShow_ && std_type != 2 && ((!forceBold && ((displayPotential_ && std_type == 3) || std_type != 3)) || (!new_marker && otherMessagesMarker_ && std_type == 3))) {
                    show_line = false;
                }
            }
        } else {
            logBook.getDXCC(/*in*/ checkCall, /*out*/ countryName);
        }

        /* CE3TSK 2026-10-03 (review): no lookup above fills the country when only grid (or state in WW Digi) is on and no
           country names are shown - items[1] then read past an empty list */
        if (countryName.isEmpty ()) logBook.getDXCC (checkCall, countryName);
        QStringList items = countryName.split(',');
        mpx = items[1];

        if (!wantedCallList.isEmpty() && (wantedCallList.indexOf(Radio::base_callsign (checkCall)) >= 0 || wantedCallList.indexOf(checkCall) >= 0)) {
            bwantedCall = true; show_line = true;
        }
        for (int i=0; i<wantedPrefixList.size(); i++) {
            if (wanted_prefix::matches (checkCall, wantedPrefixList.at(i))) {   // CE3TSK 2026-10-15: AA1AAA/VP9 is one of VP9 (wantedprefix.h)
                bwantedPrefix = true;
                break;
            }
        }
        for (int i=0; i<wantedGridList.size(); i++) {
            if (wantedGridList.at(i).size() > 0 && wantedGridList.at(i).left(4) == grid.left(4)) {
                bwantedGrid = true;
                break;
            }
        }
        for (int i=0; i<wantedCountryList.size(); i++) {
            if (wantedCountryList.at(i).size() > 0 && wantedCountryList.at(i) == mpx.toUpper()) {
                bwantedCountry = true;
                break;
            }
        }
        /* CE3TSK 2026-10-03: the wanted lists by the named levels (priorities.h) - over no new tier their low band, over a
           tier below the chased class the wanted band, which ranks above a new US state since the same day */
        if (bwantedCall && dp::noNewTier (priority)) {
            priority = dp::WantedCallOld;
            beep = true;
        } else if ((bwantedPrefix || bwantedGrid) && dp::noNewTier (priority)) {
            priority = dp::WantedPrefixOld;
//            beep = true;
        } else if (bwantedCountry && dp::noNewTier (priority)) {
            priority = dp::WantedCountryOld;
//            beep = true;
        } else if (bwantedCall && dp::wantedMayLift (priority)) {
            priority = dp::WantedCall;
            beep = true;
        } else if ((bwantedPrefix || bwantedGrid) && dp::wantedMayLift (priority)) {
            priority = dp::WantedPrefix;
            beep = true;
        } else if (bwantedCountry && dp::wantedMayLift (priority)) {
            priority = dp::WantedCountry;
            beep = true;
        }
         
            
        inverted = displayTxtColor_ && (displayPotential_ || std_type != 3);
        if (inverted) {
            swpColor = bgColor;
            bgColor = txtColor;
            txtColor = swpColor;
        }
        if (displayCountryName_) {
            if (displayCountryPrefix_) {
                cntry = items[1];
                
            } else {
                // do some obvious abbreviations, don't care if we using just prefixes here, not big deal to run some replace's
                cntry = items[2];
            }
            /* CE3TSK 2026-10-02: a US station's state or states from its grid square - "USA, CA", "K, NY/MA"
               (usstates.h), from stateGrid above; since 2026-10-03 a shared square the call's licence resolves names
               that state alone, "USA, MA". A grid in doubt below never names a state: every state square fits a US call
               (test/geodata_tables_check.py), and no other call is placed. */
            if (displayUSStates_ && us_states::usEntity (items[1])) cntry += us_states::suffix (items[1], stateGrid (), logBook.licenseState (checkCall));
            /* CE3TSK 2026-09-30: the grid does not lie in the country of this call (phantomdecodes.h) -
               ?Chile?, the leading mark first so a window narrowed by the splitter still shows it; a call
               of no country has its where? (or ?) underlined as it stands - it is a question already.
               Only when the country shown is the call that was judged. */
            if (checkCall == phantom_decodes::bareCall (doubt.sender)) {
                if (doubt.reasons & phantom_decodes::Grid) {
                    cntry = '?' + cntry + '?';
                    cntryDoubtFrom = 0;
                } else if (doubt.reasons & phantom_decodes::Where) cntryDoubtFrom = 0;
            }
        }
        // CE3TSK 2026-11-22 (the operator): the tooltip's first line - the caller, its DXCC by name, a US station's state;
        // (2026-11-24, TODO.md 3.-287, the operator: "add to the message tooltips the SNR value of the call signs") the
        // line's SNR after the call, signed as the Wide Graph's traces have it ("K1ABC -12 dB - United States")
        lineHead = checkCall + ' ' + QString::asprintf ("%+d dB", decodedText->snr ()) + ' ' + QChar (0x2013) + ' ' + items.value (2).trimmed ();
        if (us_states::usEntity (items.value (1))) lineHead += us_states::suffix (items[1], stateGrid (), logBook.licenseState (checkCall));
        if (!bwantedCall && !bwantedPrefix && !bwantedGrid && !bwantedCountry) {
            if (hideContinents_.contains(items[0]) && std_type != 2 && !jt65bc) {
                show_line = false;
            } else if (enableCountryFilter_ && std_type != 2 && !jt65bc) {
                QStringList countries = countries_.split(',');
                if (countries.contains(items[1].toUpper()))
                    show_line = false;
            }
            if (show_line && enableCallsignFilter_ && std_type != 2 && !jt65bc) {
                QStringList callsigns = callsigns_.split(',');
                if (callsigns.contains(Radio::base_callsign (checkCall)))
                    show_line = false;
            }
        }
        else if (!bwantedCall && enableCallsignFilter_ && std_type != 2 && !jt65bc) {
            QStringList callsigns = callsigns_.split(',');
            if (callsigns.contains(Radio::base_callsign (checkCall)))
                show_line = false;
        }
        if (enableMyConinentFilter_ && std_type != 2 && !jt65bc) {
            logBook.getDXCC(/*in*/ checkCall2, /*out*/ countryName2);
            QString continent2 =  countryName2.split(',')[0];
            if (continent2 == "  ") continent2 = myContinent_;
            if ((std_type == 1 && myContinent_ == items[0]) || (continent2 == myContinent_ && (items[0] == myContinent_ || items[0] == "  ")) || (continent2 != myContinent_ && items[0] != myContinent_)) show_line = false;
        }
    } else if (enableMyConinentFilter_ && !jt65bc) {
        logBook.getDXCC(/*in*/ checkCall2, /*out*/ countryName2);
        QString continent2 =  countryName2.split(',')[0];
        if (continent2 == "  ") continent2 = myContinent_;
        if (continent2 == myContinent_) show_line = false;
    }
    
    if (show_line && decodedText->isNonStd2() && hidefree_ && !decodedText->message().contains(myCall) && std_type != 1 && !jt65bc) {
        show_line = false;
    } else if (show_line && showcq_ && std_type != 1 && std_type != 2 && qAbs(rx_frq-decodedText->frequencyOffset()) >10 && !jt65bc) {
        show_line = false;
    } else if (show_line && showcqrrr73_ && std_type != 1 && std_type != 2 && !decodedText->isEnd() && qAbs(rx_frq-decodedText->frequencyOffset()) >10 && !jt65bc) {
        show_line = false;
    } else if (show_line && showcq73_ && std_type != 1 && std_type != 2 && !decodedText->isFin() && qAbs(rx_frq-decodedText->frequencyOffset()) >10 && !jt65bc) {
        show_line = false;
    } else if (!hidehintMarker_ && decodedText->isHint()) {
        if(decodedText->isPipeline())
            servis = QString::fromUtf8("┼") + servis.mid(1); // CE3TSK: hinted decode in the TX background (pipeline)
        else if(lotw.isEmpty ())
            servis = "*" + servis.mid(1); // hinted decode
        else
            servis = "°" + servis.mid(1); // lotw hinted decode
    } else if (decodedText->isWrong()) {
        servis = "?" + servis.mid(1); // error decode
    } else if (decodedText->isPipeline()) {
        servis = "|" + servis.mid(1); // CE3TSK: a pipeline message, decoded in the TX background
    } else if (!lotw.isEmpty ()) {
        servis = "•" + servis.mid(1); // lotw 
    }
    /* CE3TSK 2026-10-23: Misc > Hide clear phantom messages - a /R call on HF, a call of no country, a grid outside the
       country not yet heard twice (Verdict::clearPhantom ()) is not drawn, with our call in it too. Only the window leaves
       it out (the operator: "hide it only from the GUI") - show_line stays as the filters left it, so the history the
       sequencer reads, the UDP clients and the Rx frequency window's routing take it as shown. The bypasses below do not
       draw it: they are about the filters of Settings, and are on by default - the station runs with both on */
    bool const drawn {!(hideClearPhantoms_ && doubt.clearPhantom ())};
    if (bypassAllFilters || bypassRxfFilters) {
            show_line = true;
    }
    /* CE3TSK 2026-09-30: a likely phantom decode (phantomdecodes.h) keeps its colour, but rings no bell
       and raises no window, and what is in doubt - the grid, a /R call, the two /P calls, a call of no country - is
       underlined in red. The words are found as whole words from the message column on, the last one of each; a word
       found twice (a /R call of no country) is drawn once - appendText () skips a span it has passed. */
    QList<QPair<int, int>> doubtful;
    QString doubtTip;
    if (doubt.marked ()) {
        beep = false;
        actwind = false;
        int const from = messageText.mid (4, 1) == " " ? 21 : 23;   // where appendText () starts the message
        auto underline = [&] (QString const& word) {
            int at = -1;
            for (int i = messageText.indexOf (word, from); i >= 0 && !word.isEmpty (); i = messageText.indexOf (word, i + 1)) {
                int const e = i + word.size ();
                if (messageText.at (i - 1) == ' ' && (e == messageText.size () || messageText.at (e) == ' ')) at = i;
            }
            if (at >= 0) doubtful << qMakePair (at, word.size ());
        };
        for (auto const& r : doubt.rovers) underline (r);
        for (auto const& c : doubt.portables) underline (c);
        if (doubt.reasons & phantom_decodes::Grid) underline (doubt.grid);
        if (doubt.reasons & phantom_decodes::Where) underline (doubt.sender);
        std::sort (doubtful.begin (), doubtful.end ());
        QString country;   // CE3TSK 2026-10-19: the entity the grid was judged against, by the name the country column gives it
        if (doubt.reasons & phantom_decodes::Grid) logBook.getDXCC (phantom_decodes::bareCall (doubt.sender), country);
        doubtTip = doubtToolTip (doubt, country.section (',', 2, 2).trimmed (), decodedText->snr ());
    }
    if (show_line) {
        if (actwind) {
            if (windowPopup && window != NULL) {
                window->showNormal();
				window->raise();
				QApplication::setActiveWindow(window);
			}
		}
        if (beep && !once_notified) {
            QApplication::beep();
			notified = true;
        }
    }
    if (jt65bc) {
        bgColor = Radio::convert_dark("#ffffff",useDarkStyle_);
        txtColor = Radio::convert_dark("#000000",useDarkStyle_);
        inverted = false;   // CE3TSK 2026-11-12 (TODO.md 3.-246, review): drawn plain - a wanted colour is its text again
    }

    /* CE3TSK: contest points as a ranking tier, sitting below the new field tier (52/53, both
       the never-worked and the new-on-this-band field since 2026-08-29) and above New CQ zone (34/35): 1 point ranks 36/37, 7 points 48/49
       (four higher since 2026-10-03, when the new US state got a level of its own - 17-20, the wanted band moving to 21-23; priorities.h), the odd
       value being the LOTW variant as everywhere else in this ladder.

       Applied as a maximum AFTER the chain rather than as another "else if" inside it. Every
       station with a grid scores at least one point, so a tier in the chain would match
       almost every decode and, first match winning, would suppress the new DXCC, zone,
       wanted call and new call colours for the whole contest. This way the colour still
       comes from whichever tier matched and only the ranking changes. */
    int contestPts = 0;
    if (contest_.prefixCountryMultipliers) {
        /* CE3TSK 2026-11-04 (TODO.md 3.-230): its points come from the call, so a line with no grid shows them too. The order the
           operator chose: a new DXCC on the band, a new prefix on the band, the points, a repeat last. New overall is new on the band. */
        if (!checkCall.isEmpty ()) {
            if (countryName.isEmpty ()) logBook.getDXCC (checkCall, countryName);
            QString myCountry;
            logBook.getDXCC (myCall_, myCountry);   // (TODO.md 3.-232) the whole call, as the score asks: K1ABC/KH6 is in Hawaii, myCall is K1ABC
            contestPts = contest_qso::ybdxpi_points (logBook.isContestMember (checkCall), myCountry, countryName);
            if (workedOnBand) priority = lotw.isEmpty () ? dp::None : dp::LoTW;
            else {
                int pp = contestPts > 0 ? dp::ContestPointsBase + 2 * contestPts : 0;
                if (newCountryOnBand && !contest_qso::country_key (countryName).isEmpty ()) pp = dp::ContestCountry;
                else if (newPrefixOnBand) pp = dp::ContestPrefix;
                if (pp > 0 && !lotw.isEmpty ()) pp += 1;
                if (pp > priority) priority = pp;
            }
        }
    } else if (specialOp_ != Configuration::SpecialOperatingActivity::NONE
               && (!grid.isEmpty() || (contest_.missingGridScores && !checkCall.isEmpty () && !std_call::grid_can_come (checkCall, myCall_)))) {
        /* CE3TSK 2026-11-07 (TODO.md 3.-231): a contest where a station with no grid still counts, the FT Challenge: 1 point on his line,
           where the others show " --", and his rank as any 1 point station's (grid_distance_km () says -1 without his grid).
           (2026-11-13, TODO.md 3.-251, the operator) only a station whose grid cannot come (stdcall.h, grid_can_come ()): a
           compound call (V5/G0RIB, G0RIB/MM), or a pair the messages carry with no grid (his /P call to our compound one, a
           /R and a /P call); a standard call's line without one shows " --" and ranks with no points, as in the other
           contests - his grid comes in his CQ or his first call. The score and the log are not this: a QSO with no grid
           still counts 1 point, ZZ00 in the contest log. */
        contestPts = contest_points (specialOp_, grid_distance_km (myGrid_, grid, true));
        /* (2026-11-04, the operator) a station already worked on the band: his points are shown, the rank alone falls, "simply because it was answered before" */
        if (contestPts > 0 && !(contest_.workedRanksLast && workedOnBand)) {
            int const pp = dp::ContestPointsBase + 2 * contestPts + (lotw.isEmpty () ? 0 : 1);
            if (pp > priority) priority = pp;
        }
    }
    if (show_line) {
        if (!checkCall.isEmpty () && (std_type == 1 || std_type == 2 || std_type == 4 || (std_type == 3 && !param.isEmpty()))) {
            /* CE3TSK 2026-09-30: the sequencer's side of a likely phantom decode - set or cleared by
               the message it is judged on, and only when this entry is the call that was judged. Only an
               ordinary decode clears it: a hint, a-priori or TX-background one can reproduce the very call
               it was fed, so it holds a station back but never releases one (Verdict::reliable). */
            QsoHistory::Doubt const d = checkCall != phantom_decodes::bareCall (doubt.sender) ? QsoHistory::DOUBT_KEEP
                                      : doubt.noAnswer ? QsoHistory::DOUBT_SET
                                      : doubt.reliable ? QsoHistory::DOUBT_CLEAR : QsoHistory::DOUBT_KEEP;
            /* CE3TSK 2026-10-24 (TODO.md 3.-205): a hint or a-priori decode of his CQ (isHint: * ° ^ and the Tx
               background's cross) in the period of his message to us that was taken is the decoder's, not his - one
               station, one message a period - and does not take him back to calling CQ. JA5CEX 2026-09-25: "CQ VK8SSB
               PH57" from the Tx background, on his frequency in the very period he sent us R-07, and the sequencer
               called him from the start again. Passed on as NONE: his step stays as it is. A hint CQ of his in a period
               of its own still counts (a DXpedition's ^ is mostly his), and a plain one in the same period too (MSHV's
               multi-slot sends two). */
            QsoHistory::Status step = status;
            if (step == QsoHistory::RCQ && decodedText->isHint () && qsoHistory.answeredIn (checkCall, c_time)) step = QsoHistory::NONE;
            if (contest_.workedRanksLast && workedOnBand) qsoHistory.rankAsWorked (checkCall);   // CE3TSK 2026-11-04: logged since his last line
            qsoHistory.message(checkCall,step,priority,param,tyyp,countryName.left(2),mpx,c_time,decodedText->report(),decodedText->frequencyOffset(),checkMode,d,decodedText->isHint ());
        } 
        bool const toMe {std_type == 2};   // CE3TSK 2026-11-22: for the tooltip, the red marker cell or not
        if (std_type == 2) {
            if(!redMarker_) std_type = 0;
            else if(blueMarker_ && !hisCall.isEmpty () && checkCall.contains(hisCall)) std_type = 5;
        }
        /* CE3TSK: contest points for this station, scored from its own decoded grid rather
           than the QSO partner's - hisGrid above is the current DX station. Appended to the
           country annotation so it rides the same insertion path and neutral colours.
           grid_distance_km() returns -1 when either grid is missing and contest_points()
           scores 0 for that, so an unknown distance prints no points - " --" of the tag's
           width instead (contest_points_tag()), so the country column stays aligned on
           the lines without a grid. (2026-11-07) Except in a contest that counts a QSO
           with no grid (missingGridScores, the FT Challenge): there a line with no grid is
           worth 1 point and shows " 1p" - since 2026-11-13 (3.-251) only where his grid
           cannot come (grid_can_come () above: a compound call, a pair sent without one);
           a standard call's line, and a line with no call, show " --". */
        int tagColumns {0};   // CE3TSK 2026-10-20: the points tag's width - the distance field's column moves with it
        if (specialOp_ != Configuration::SpecialOperatingActivity::NONE) {
            /* directly after the hint character and before the country, which is what
               appendText() inserts next: "CQ XQ3SK FF46  * 1p Chile". The separator is
               added only when the country string does not already carry one, since the
               country table's leading spacing is not uniform. */
            /* (2026-11-04) as wide as the contest's largest score, the antipodes': ARRL Digi goes to 42 points */
            QString p = contest_points_tag (contestPts, contest_points (specialOp_, 20400) > 9 ? 2 : 1);
            if (!cntry.isEmpty () && !cntry.startsWith (' ')) p += ' ';
            cntry = p + cntry;
            if (cntryDoubtFrom >= 0) cntryDoubtFrom += p.size ();   // CE3TSK: ?Chile? moved along
            tagColumns = p.size ();
        }
        /* CE3TSK 2026-10-20: the distance and azimuth to the grid the MESSAGE carries (distancefield.h) - not a grid
           remembered for the call or the DX Call box's, which `grid` above may hold; none for a grid in doubt (a likely
           phantom decode's grid), none without My Grid (grid_distance_azimuth () says false). In a contest both grids
           cut to 4 characters, as the score measures them. */
        QString field;
        if (distanceField_ && (showDistance_ || showAzimuth_)) {
            /* (review 3.-161 5) the grid as the phantom judge reads it - the whole message field, 26 characters:
               DecodedText's message is cut at 24 before a hash's brackets go, and <VP2E/KA1ABC> KA9XYZ FN42 lost its grid.
               JT65's "CQ K1ABC FN42 OOO" only DecodedText reads. (3.-161 4) A grid in doubt shows none - until it is
               trusted: the same grid heard from the call again (Verdict::lowConfidence (), as the spot and the tooltip) */
            QString msgGrid {phantom_decodes::parts (phantom_decodes::messageField (decodedText->string ())).grid};
            if (!distance_field::usable (msgGrid)) {
                QString deCall;
                decodedText->deCallAndGrid (deCall, msgGrid);
            }
            bool const gridInDoubt {(doubt.reasons & phantom_decodes::Grid) && doubt.lowConfidence ()};
            int km, mi, az;
            if (distance_field::usable (msgGrid) && !gridInDoubt
                && grid_distance_azimuth (myGrid_, msgGrid, specialOp_ != Configuration::SpecialOperatingActivity::NONE, km, mi, az))
                field = distance_field::text (showDistance_, showAzimuth_, miles_, km, mi, az);
        }
        int const fieldColumns {(displayCountryName_ ? (displayCountryPrefix_ ? (displayUSStates_ ? distance_field::prefixStateColumns : distance_field::prefixColumns)
                                                                              : distance_field::nameColumns) : 0) + tagColumns};
        /* CE3TSK 2026-11-22 (the operator): the line's tooltip - what its colours say after the tier (the text colour, the
           marker cell's, the wanted lists), what the decoder's mark says (lineToolTip ()) */
        QStringList colours;
        if (workedTip) colours << (confirmedTip ? tr ("Worked before and LoTW-confirmed: nothing new in the notifications that are on.")   // (D12)
                                                : tr ("Worked before: nothing new in the notifications that are on."));
        if (std_type == 1) colours << tr ("Calls CQ.");
        else if (std_type == 5) colours << tr ("A message to you from your DX call.");
        else if (toMe) colours << tr ("A message to you.");
        else if (std_type == 4) colours << tr ("Ends a QSO: RR73 or 73.");
        if (!toMe && !hisCall.isEmpty () && messageText.contains (Radio::base_callsign (hisCall))) colours << tr ("Has the call of your DX Call box.");
        if (bwantedCall) colours << tr ("On your list of wanted calls.");
        if (bwantedPrefix) colours << tr ("Its prefix is on your list of wanted prefixes.");
        if (bwantedGrid) colours << tr ("Its grid is on your list of wanted grids.");
        if (bwantedCountry) colours << tr ("Its country is on your list of wanted countries.");
        QString const tip {lineToolTip (lineHead, tierTip, unconfirmedTip, colours, markSentences (decodedText->marker (), !lotw.isEmpty (), decodedText->windowCopy (),
                                                            !lines_have_seconds (app_mode) && app_mode != "WSPR-2"), doubtTip)};
        if (drawn) {   // CE3TSK 2026-10-23: Misc > Hide clear phantom messages, above
            appendText(messageText, bgColor, txtColor, std_type, servis, servisColor, cntry, forceBold, strikethrough, underlined, decodedText->isDXped(), false, bwantedCall||bwantedGrid||bwantedPrefix||bwantedCountry, doubtful, cntryDoubtFrom, tip, field, fieldColumns, inverted);
            wastx_ = false;
        }
    }
        if (notified) inotified |= 1;
        if (show_line) inotified |= 2;
        if (bwantedCall) inotified |= 8;
        if (bwantedPrefix) inotified |= 16;
        if (bwantedGrid) inotified |= 32;
        if (bwantedCountry) inotified |= 64;
	return inotified;
}


void DisplayText::displayTransmittedText(QString text, QString myCall, QString hisCall, QString skip_tx1, QString modeTx, qint32 txFreq,
                                         QColor color_TxMsg, QsoHistory& qsoHistory)
{
    QsoHistory::Status status = QsoHistory::S73;
    QString bg=color_TxMsg.name();
    QString t;
    QString t1=" @ ";
    QString t2;
    QString tyyp = "";
    unsigned ttime=0;
    t = text;
    int dxped = t.indexOf("; ");
    if (dxped >0) {
        auto next_ws = t.indexOf(' ',dxped+2);
        t2 = t.mid(next_ws,t.indexOf(' ',next_ws+1)-next_ws);
        next_ws = t.indexOf(' ',1);
        t =  t.left(next_ws) + t2 + t.mid(next_ws,32);
    }
    t2 = QString::asprintf("%4d",txFreq);
    if(modeTx=="FT8") t1=" ~ ";
    else if(modeTx=="FT4") t1=" : ";
    else if(modeTx=="FT2") t1=" ; ";   /* CE3TSK: FT2's own character, as the decoder prints it
                                          (decoder.f90's ft4_decoded). Without this arm FT2 fell
                                          through to the default " @ ", which is JT9's marker, and
                                          the first FT2 QSO showed one character on the lines
                                          received and another on the lines sent. */
    else if(modeTx=="MSK144") t1=" & ";   // CE3TSK 2026-11-17: WSJT-X's character for MSK144, as the decoder prints it
    else if(modeTx=="JT65") t1=" # ";
    else if(modeTx=="T10") t1=" + ";
    
    QStringList txs = t.split ("; ");
    for (int i=0; i<txs.size(); i++) {
        if(lines_have_seconds (modeTx)) {   // CE3TSK 2026-11-17: MSK144's lines have seconds too (modetiming.h)
          t = QDateTime::currentDateTimeUtc().toString("hhmmss") + \
            "  Tx      " + t2 + t1 + txs.at(i).left(24);
          t = t.leftJustified(49,' ');
        } else {
          t = QDateTime::currentDateTimeUtc().toString("hhmm") + \
            "  Tx      " + t2 + t1 + txs.at(i).left(19);
        }
        ttime = 3600 * t.mid (0, 2).toUInt () + 60 * t.mid (2, 2).toUInt();
        if (t.mid (4, 2) != "  ") ttime += t.mid (4, 2).toUInt();        

        /* CE3TSK 2026-11-18 (MSK144_PLAN.md section 17): a short message of MSK144 is shown as it is sent, "<K1ABC
           CE3TSK> R+03", and read here as the long one it stands for - every other text arrives without brackets */
        QStringList parts = QString {txs.at(i)}.remove (QChar {'<'}).remove (QChar {'>'}).split (' ', SkipEmptyParts);
        if (parts.size () > 1) 
          {
            QString param="";
            QString call=parts[0];
            if (call == "DE ") call = hisCall;
            if (parts.size () > 2)
              {
                if (parts[0] == "CQ")
                  {
                    status = QsoHistory::SCQ;
                    if (parts.size () > 3) {
                        /* CE3TSK 2026-10-12: read as the line received is (cqdirection.h). The word went
                           in as it was sent, and the autoselect holds every caller against it: after a
                           "CQ TEST" of ours no caller was from "TEST", and it passed over all of them. */
                        tyyp = cq_direction::meaning (parts [1]).region;
                        call=parts[2];
                        param=parts[3];
                    } else {
                        call=parts[1];
                        param=parts[2];
                    }
                  }            
                /* CE3TSK: WW Digi contest - "HISCALL MYCALL R GRID" is our roger plus
                   exchange (SRREPORT). Without this it matched no branch at all and was
                   recorded as the initial S73, ending the QSO in the history.
                   param is our sent exchange, it lands in QSO history as s_rep which the
                   RRR/RR73/73 states require to be non empty before they will advance. */
                else if (contest_.gridExchange && parts.size() == 4 && parts[2] == "R" && isGrid4 (parts[3]))
                  {
                    param = parts[3];
                    status = QsoHistory::SRREPORT;
                    tyyp = skip_tx1;
                  }
                else if (parts[2].size() == 4)
                  {
                    if (parts[2] == "RR73")
                      status = QsoHistory::SRR73;
                    else if (parts[2].left(2) == "R+" || parts[2].left(2) == "R-") {
                      param = parts[2].mid(1,3);
                      status = QsoHistory::SRREPORT;
                      tyyp = skip_tx1;
                    }
                    /* CE3TSK: WW Digi contest - "HISCALL MYCALL GRID" carries our exchange.
                       Keep it as SCALL but pass the grid along, QSO history stores it as
                       s_rep. The RRR/RR73/73 states all refuse to advance while s_rep is
                       empty, and in this contest no signal report is ever exchanged. */
                    else {
                      if (contest_.gridExchange && isGrid4 (parts[2])) param = parts[2];
                      status = QsoHistory::SCALL;
                    }
                  }
                else if (parts[2].size() == 3)
                  {
                    if (parts[2] == "RRR")
                      status = QsoHistory::SRR;
                    else if (parts[2].left(1) == "+" || parts[2].left(1) == "-") {
                      param = parts[2].left(3);
                      status = QsoHistory::SREPORT;
                      tyyp = skip_tx1;
                     }
                   }
                else if (parts[2] == "73" || parts[1] == "73" || parts[0] == "73"
                      || parts[2] == "TNX" || parts[1] == "TNX" || parts[0] == "TNX"
                      || parts[2] == "TKS" || parts[1] == "TKS" || parts[0] == "TKS"
                      || parts[2] == "TU" || parts[1] == "TU" || parts[0] == "TU"
                      || (parts.size() == 4 && (parts[3] == "73" || parts[3] == "TNX" || parts[3] == "TKS" || parts[3] == "TU")))
                   {
                    call = hisCall;
                     status = QsoHistory::S73;
                   }
              }  
            if (parts.size () == 2)
               {
                if (parts[0] == "CQ")
                  {
                    status = QsoHistory::SCQ;
                    call=parts[1];
                  }            
                else if (!myCall.isEmpty () && Radio::base_callsign (parts[1]) == myCall && call != "73" && call != "TNX" && call != "TKS" && call != "TU")  // 
                  {
                    status = QsoHistory::SCALL;
                  }
                else if (call == "73" || parts[1] == "73" || call == "TNX" || parts[1] == "TNX" || call == "TKS" || parts[1] == "TKS" || call == "TU" || parts[1] == "TU")
                  {
                    call = hisCall;
                    status = QsoHistory::S73;
                  }
               }
            mystatus_ = status;
            qsoHistory.message(call,status,0,param,tyyp,"","",ttime,"",txFreq,modeTx);
          }
        if (wastx_ && ttime - last_tx < 2 && hide_TX_messages_)
            appendText(t,bg,Radio::convert_dark("#000000",useDarkStyle_),0," ",Radio::convert_dark("#000000",useDarkStyle_)," ",false,false,false,false,true);
        else
            appendText(t,bg,Radio::convert_dark("#000000",useDarkStyle_),0," ",Radio::convert_dark("#000000",useDarkStyle_));
    }
    wastx_ = true;
    last_tx = ttime;
}

/* CE3TSK: a one-line notice in the decode window when the contest logic acts on the operator's
   behalf. Without it the software simply stops answering a station and the last thing on screen
   is his previous, perfectly ordinary message - which is what the operator reported: no way to
   tell whether the decoder had died, the station had gone, or a rule had fired. Amber, so it
   reads as "the software did something", distinct from a decode and from the pink QSY line. */
void DisplayText::displayContestNotice(QString text)
{
  /* CE3TSK 2026-11-07, the operator: "with less spaces so that more is visible without scrolling" - one space after the time, no
     longer the twelve of a decode's dB, DT and frequency columns */
  QString t = QDateTime::currentDateTimeUtc().toString("hhmmss") + " " + text;
  QString bg=Radio::convert_dark("#f0b23a",useDarkStyle_);
  appendText(t,bg,Radio::convert_dark("#000000",useDarkStyle_),0," ",Radio::convert_dark("#000000",useDarkStyle_));
}

/* CE3TSK: the Fox verifier's answer, SUPERFOX_PLAN.md milestone 2. `line` is a whole decode-style
   line ("hhmmss   0  0.0  750 ~ K1JT verified"), so it sits in the columns of the decodes around
   it; green for verified, red for invalid, as MSHV colours them. verdict 0 is the plain form, used
   when the operator asked to see the lines that carry the code (ini ShowOTP). */
void DisplayText::displayFoxVerification(QString const& line, int verdict)
{
  QString const bg = Radio::convert_dark(1 == verdict ? "#c8fac8" : 2 == verdict ? "#ff6e6e" : "#ffffff",useDarkStyle_);
  appendText(line,bg,Radio::convert_dark("#000000",useDarkStyle_),0," ",Radio::convert_dark("#000000",useDarkStyle_),"",1 == verdict || 2 == verdict);
}

void DisplayText::displayQSY(QString text)
{
  QString t = QDateTime::currentDateTimeUtc().toString("hhmmss") + "            " + text;
  QString bg=Radio::convert_dark("#ff69b4",useDarkStyle_);
  appendText(t,bg,Radio::convert_dark("#000000",useDarkStyle_),0," ",Radio::convert_dark("#000000",useDarkStyle_));
}

