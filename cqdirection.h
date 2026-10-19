#ifndef CQDIRECTION_H
#define CQDIRECTION_H

// CE3TSK 2026-10-12: the word after CQ.
//
// "CQ DX K1ABC FN42" wants an answer from another continent. "CQ TEST K1ABC FN42" wants one from anybody,
// and TEST says what K1ABC is doing: a contest. The program told the two apart by length alone. A word of
// one or two letters was a region - a continent, DX, or the first letters of a callsign - and a message with
// a longer word was not a CQ at all, POTA, SOTA and IOTA excepted: it was not coloured as one, its sender
// got no new-country mark, and the autoselect never offered it. The contest words of two letters - FD, RU,
// WW - were read as regions nobody lives in (WW too, outside the WW Digi mode), so their senders were never
// offered either, and a CQ of our own with such a word made the autoselect pass over everybody who
// answered it.
// Measured on sixteen months of this station's decodes: 7 CQs in 100 carry a word, and one in eight of
// those was a word of three or four letters the program did not know - TEST, WWA, CERT, QRP, WWFF and
// more than a hundred and sixty others.
//
// The rule is here, once, for the line received (DecodedText::CQersCall) and for the record of the CQ we
// send (DisplayText::displayTransmittedText), which the autoselect holds the callers against:
//   no word, CQ, LP          anybody may answer, as before
//   FD, RU, WW               the contest words of two letters: anybody
//   one or two letters       a region, as before
//   three or four letters    anybody - except ASIA and USA, which were regions before and still are; on a
//                            line received only as "CQ TEST K1ABC", in FT8, FT4 and FT2 (receivedWordStands)
//   three digits             as before: 908 is Japan, any other is not taken as a CQ
// A word that is not a region is an ACTIVITY: the CQ is anybody's to answer. What an operator means by a
// word this table has never seen cannot be known; that it is not a continent or a prefix can.
//
// The second half is the box the word is typed into (two of them, one on each tab of the main window): it
// took two letters, so "CQ TEST" could not be sent. It takes four now. FT8, FT4 and FT2 carry a word of up
// to four letters in a standard message; JT65, JT9 and T10 carry two, and a longer one would go out as
// thirteen characters of free text with the call cut short - so there the box keeps the word and shows it
// as not in effect, and the CQ goes out plain.

#include <QString>
#include <QRegularExpression>

namespace cq_direction
{
  struct Meaning
  {
    bool cq;          // false: the message is not taken as a CQ (three digits, 908 apart)
    QString region;   // what the autoselect holds a station against: empty - anybody -, a continent, DX, or
                      // the first letters of a callsign
  };

  // word: what stands between CQ and the callsign, upper case - one to four letters or three digits; empty
  // when nothing does
  inline Meaning meaning (QString const& word)
  {
    // ASIA and USA: the two words of three and four letters the program took for regions before, kept as
    // they were. No other word is given a region. What an operator means by JPN, EUR or DXDX is that
    // operator's own, a table of such words is never complete, and a wrong guess keeps the autoselect from
    // a station that would have been glad of the call - so every other word is an activity, and anybody may
    // answer (the operator, 2026-10-12: no lists of such words).
    static struct {char const * word; char const * region;} const regions[] = {
      {"ASIA", "AS"}, {"USA", "K"},
    };
    int const n = word.size ();
    if (!n || "CQ" == word || "LP" == word) return {true, QString {}};
    bool allLetters {true};
    bool allDigits {true};
    for (QChar const c : word)
      {
        allLetters = allLetters && c >= 'A' && c <= 'Z';
        allDigits = allDigits && c >= '0' && c <= '9';
      }
    if (allDigits) return "908" == word ? Meaning {true, "JA"} : Meaning {false, word};
    if (!allLetters || n > 4) return {false, word};   // nothing the decoders print after CQ
    if (n < 3)
      {
        if ("FD" == word || "RU" == word || "WW" == word) return {true, QString {}};
        return {true, word};
      }
    for (auto const& r : regions) if (word == r.word) return {true, r.region};
    return {true, QString {}};
  }

  // A word is taken when the typing stops, not at each letter. With four letters to type, TE and TES are
  // words too: taken as they came, each put a CQ of its own into Tx6, and each restarted a CQ that was on the
  // air - "CQ TE K1ABC FN42" was sent on the way to "CQ TEST". So a word typed by hand waits this long after
  // the last key - the box shows it as not in effect meanwhile - or for Enter, or for the box being left. A
  // word the program sets itself (the settings, a contest, a request over the network) is taken at once.
  constexpr int typingPauseMs {1000};

  // what may be typed into the box: up to four letters, either case
  inline QRegularExpression typed ()
  {
    return QRegularExpression {"[a-z]{0,4}", QRegularExpression::CaseInsensitiveOption};
  }

  inline bool letters (QString const& word)
  {
    for (QChar const c : word) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) return false;
    return true;
  }

  // a word kept in the settings is put back when it is two to four letters, and dropped otherwise
  inline bool kept (QString const& stored)
  {
    return stored.size () >= 2 && stored.size () <= 4 && letters (stored);
  }

  // what the box holds. mode77: the mode sends 77-bit messages - FT8, FT4, FT2
  enum class Box {Empty, Incomplete, InEffect};
  inline Box box (QString const& word, bool mode77)
  {
    int const n = word.size ();
    if (!n) return Box::Empty;
    if (2 == n) return Box::InEffect;                                  // as before, in every mode
    if (mode77 && (3 == n || 4 == n) && letters (word)) return Box::InEffect;
    return Box::Incomplete;                                            // one letter; or a word the mode cannot carry
  }

  // CE3TSK 2026-10-14 (review 2026-10-12, A7): whether the word of a line RECEIVED stands where a CQ carries
  // it. A word of one or two letters, or three digits, is taken as it always was. One of three or four letters
  // only right after "CQ " and before a space, and only in FT8, FT4 and FT2 (mode77) - since three and four
  // letters became an activity (2026-10-12) these lines had become CQs with a caller:
  //   - "CQTEST K1ABC FN42", "DELTA K1ABC FN42" (DE, then LTA), "DE TEST K1ABC", "QRZ TEST K1ABC": the pattern
  //     reading the line lets the space after CQ, DE or QRZ go, as stock JTDX's did - for DX ("CQDX") and a
  //     region of two letters it still may;
  //   - in JT65, JT9 and T10 a CQ with such a word is thirteen characters of free text, and a call of six
  //     characters is cut short: "CQ TEST DL1AB" is DL1ABC's CQ, and DL1AB would be called.
  // Stock JTDX took POTA, SOTA, IOTA, ASIA and USA in all these forms and modes too; not one line of sixteen
  // months of this station's ALL.TXT had one of them in a form refused here (seven lines in all were: "CQWWA",
  // "CQDXSA", "CQLAST" - free text).
  inline bool receivedWordStands (QString const& message, QString const& word, bool mode77)
  {
    if (word.size () < 3 || !letters (word)) return true;
    return mode77 && message.startsWith ("CQ " + word + " ");
  }

  // the word that goes into the CQ generated: none for a word of three letters or more that the mode cannot
  // carry (or that is no word). One and two characters are passed as they always were.
  inline QString inMessage (QString const& word, bool mode77)
  {
    if (word.size () < 3) return word;
    return Box::InEffect == box (word, mode77) ? word : QString {};
  }

  // the word a CQ generated now carries (MainWindow::genCQMsg): inMessage's - and none with a nonstandard own call
  // (standardCall false: MainWindow::stdCall, which takes K1ABC/P and K1ABC/R as standard - their CQ carries the
  // word -, and not EA8/DL1ABC or K1ABCD), whose CQ carries no word at all
  inline QString wordInCq (QString const& word, bool mode77, bool standardCall)
  {
    return standardCall ? inMessage (word, mode77) : QString {};
  }

  // CE3TSK 2026-10-14 (review of 2026-10-14): does a word typed change the CQ that is sent? Selecting the CQ again
  // restarts a transmission, clears a DX call that was typed, the QSO's messages and turns AutoSeq back on - which
  // is what choosing ANOTHER CQ means. A word that leaves the CQ as it was - TEST typed in JT65, where the CQ goes
  // out plain, or WWFF for TEST there - chose none. sent: the word in the CQ generated last (MainWindow keeps it).
  // Asked of that, not of the word taken before (3.-61 did, review 3.-67): a one-letter word on the way generates
  // nothing, so "DX" -> "D" -> "DX" restarted a CQ that had stayed "CQ DX" all along.
  // generated: a CQ was generated at all (review 3.-72). Without one - Tx6 empty, no own grid yet - every word
  // typed selects the CQ again, as it always did: "no CQ" and "a CQ without a word" are both an empty sent, and a
  // word cleared or one that is not carried (TEST in JT65) left Tx6 empty after a grid was given in the Settings.
  inline bool changesCq (bool generated, QString const& sent, QString const& word, bool mode77, bool standardCall)
  {
    return !generated || sent != wordInCq (word, mode77, standardCall);
  }

  // CE3TSK 2026-10-14 (review of 2026-10-14): the word the settings keep - the one in effect, or the one typed
  // while it still waits for the pause: File > Exit and the window's close button end no editing, and the word
  // typed a moment before was lost, the old one coming back at the next start.
  inline QString toKeep (QString const& inEffect, QString const& typed, bool waiting)
  {
    return waiting ? typed : inEffect;
  }
}

#endif
