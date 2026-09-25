// This source code file was last time modified by Arvo ES1JA on 20191202 (JTDX)
// All changes are shown in the patch file coming together with the full JTDX source code.
// Changed since for JTDX_contest by Tihomir Sokcevic CE3TSK; those changes are marked CE3TSK.

#include "Radio.hpp"

#include <cmath>

#include <QString>
#include <QChar>
#include <QRegularExpression>

namespace Radio
{
  namespace
  {
    double constexpr MHz_factor {1.e6};
    int constexpr frequency_precsion {6};

    // very loose validation - callsign must contain a letter next to
    // a number
//    QRegularExpression valid_callsign_regexp {R"(\d[[:alpha:]]|[[:alpha:]]\d)"};
    QRegularExpression valid_callsign_regexp {R"([2,4-9]{1,1}[A-Z]{1,1}[0-9]{1,4}|[3]{1,1}[A-Z]{1,2}[0-9]{1,4}|[A-Z]{1,2}[0-9]{1,4})"};
    
    // Extracting prefix from callsign
    QRegularExpression prefix_re {R"(^(?:(?<prefix>[2-9]{0,1}[A-Z]{1,2}[0-9]{1,4})?)?)"};
  }


  Frequency frequency (QVariant const& v, int scale, QLocale const& locale)
  {
    double value {0};
    if (QVariant::String == v.type ())
      {
        value = locale.toDouble (v.value<QString> ());
      }
    else
      {
        value = v.toDouble ();
      }
    return std::llround (value * std::pow (10., scale));
  }

  FrequencyDelta frequency_delta (QVariant const& v, int scale, QLocale const& locale)
  {
    double value {0};
    if (QVariant::String == v.type ())
      {
        value = locale.toDouble (v.value<QString> ());
      }
    else
      {
        value = v.toDouble ();
      }
    return std::llround (value * std::pow (10., scale));
  }


  QString frequency_MHz_string (Frequency f, QLocale const& locale)
  {
    return locale.toString (f / MHz_factor, 'f', frequency_precsion);
  }

  QString frequency_MHz_string (FrequencyDelta d, QLocale const& locale)
  {
    return locale.toString (d / MHz_factor, 'f', frequency_precsion);
  }

  QString pretty_frequency_MHz_string (Frequency f, QLocale const& locale)
  {
    auto f_string = locale.toString (f / MHz_factor, 'f', frequency_precsion);
    return f_string.insert (f_string.size () - 3, QChar::Nbsp);
  }

  QString pretty_frequency_MHz_string (double f, int scale, QLocale const& locale)
  {
    auto f_string = locale.toString (f / std::pow (10., scale - 6), 'f', frequency_precsion);
    return f_string.insert (f_string.size () - 3, QChar::Nbsp);
  }

  QString pretty_frequency_MHz_string (FrequencyDelta d, QLocale const& locale)
  {
    auto d_string = locale.toString (d / MHz_factor, 'f', frequency_precsion);
    return d_string.insert (d_string.size () - 3, QChar::Nbsp);
  }

  bool is_callsign (QString const& callsign)
  {
    if ((!callsign.at(1).isDigit() && callsign.size () == 2) || callsign == "F" || callsign == "G" || callsign == "I" || callsign == "K" || callsign == "W") {
        auto call = callsign + "0";
        return call.contains (valid_callsign_regexp);
    }
    else
        return callsign.contains (valid_callsign_regexp);
  }

  bool is_compound_callsign (QString const& callsign)
  {
    return callsign.contains ('/');
  }

  // split on first '/' and return the larger portion or the whole if
  // there is no '/'
  QString base_callsign (QString callsign)
  {
    auto slash_pos = callsign.indexOf ('/');
    if (slash_pos >= 0)
      {
        auto right_size = callsign.size () - slash_pos - 1;
        if (right_size>= slash_pos)
          {
            callsign = callsign.mid (slash_pos + 1);
          }
        else
          {
            callsign = callsign.left (slash_pos);
          }
      }
    return callsign.toUpper ();
  }

  // analyze the callsign and determine the effective prefix, returns
  // the full call if no valid prefix (or prefix as a suffix) is specified
  QString effective_prefix (QString callsign)
  {
    QStringList parts = callsign.split('/');
    if (parts.size() == 2 && (parts.at(1) == "P" || parts.at(1) == "A")) return callsign;
    auto prefix = parts.at(0);
    int size = prefix.size();
    int region = -1;
    for (int i = 1; i < parts.size(); ++i) {
      if (parts.at(i) == "MM" || parts.at(i) == "AM") {
          prefix="1B1ABCD";
          size = prefix.size();
      } else if (is_callsign(parts.at(i)) && size > parts.at(i).size() && !((parts.at(i) == "LH" || parts.at(i) == "ND" || parts.at(i) == "AG" || parts.at(i) == "AE" || parts.at(i) == "KT") && i == (parts.size() -1))) {
          prefix = parts.at(i);
          size = prefix.size();
      } else {
        bool ok;
        auto reg = parts.at(i).toInt(&ok);
        if (ok) region = reg;
      }
    }
    auto const& match = prefix_re.match (prefix);  
    auto shorted = match.captured ("prefix");
    if (shorted.isEmpty() || region > -1) {
        if (shorted.isEmpty()) {
          if (region > -1)
            shorted = prefix+"0";
          else
            shorted = prefix;
        }
        if (region > -1) shorted = shorted.left(shorted.size()-1) + QString::number(region);
        return shorted.toUpper ();
    } else return prefix;
  }

  //  strip to prefix only
  QString striped_prefix (QString callsign)
  {
    auto const& match = prefix_re.match (callsign);  
    return match.captured ("prefix");
  }
/* CE3TSK: the fixed UI colours, one row per literal the code asks for: the value to use in the
   light style, then in the dark one. They are not adjustable - unlike the notification colours
   in Settings, these are part of the layout.

   This used to be an algorithm (subtract 0x60 per channel, XOR the greys) applied to whatever
   colour it was handed. That cannot work, because it does not know whether the colour is a
   background or text: darkening both members of a pair collapses the contrast between them. A
   luminance inversion was tried too and is no better - WCAG contrast is not symmetric under
   inversion, so mid-luminance pairs still collide. Measured over the 25 background/text pairs
   the main window actually produces, the old rule left 16 of them below 4.5:1 and 14 below 3:1.

   The table instead fixes each literal once, with its role already decided: backgrounds land near
   0.055 relative luminance, text colours stay light. All 25 pairs now clear 4.5:1. Regenerate the
   numbers rather than editing by eye - the derivation is in UI_DARK_STYLE.md.

   A literal that is not listed falls back to the old rule, so nothing breaks if one is added. */
namespace
{
  struct FixedColor { char const * ask; char const * light; char const * dark; };
  FixedColor const fixed_colors[] = {
    {"#000000", "#000000", "#e6dcd2"},
    {"#0000ff", "#0000ff", "#93b8ff"},
    {"#00ff00", "#00ff00", "#004f00"},
    {"#00ffff", "#00ffff", "#004b4b"},
    {"#1400b1", "#1400b1", "#a9beff"},
    {"#222222", "#222222", "#2b2b2b"},
    {"#6699ff", "#6699ff", "#0038a7"},
    {"#66ff66", "#66ff66", "#004f00"},
    {"#7bff7b", "#7bff7b", "#004f00"},
    {"#7fff7f", "#7fff7f", "#004f00"},
    {"#808080", "#555555", "#b9b9b9"},
    {"#82ff8c", "#82ff8c", "#004f06"},
    {"#88ff88", "#88ff88", "#004f00"},
    {"#96ffff", "#96ffff", "#004b4b"},
    {"#9999ff", "#9999ff", "#0000e3"},
    {"#99ff99", "#99ff99", "#004f00"},
    {"#99ffff", "#99ffff", "#004b4b"},
    {"#a99ee2", "#a99ee2", "#402f9c"},
    {"#aabec8", "#aabec8", "#33454e"},
    {"#aaffff", "#aaffff", "#004b4b"},
    {"#adadad", "#adadad", "#d4d4d4"},
    {"#bebebe", "#bebebe", "#434343"},
    {"#c4c4ff", "#c4c4ff", "#0000e3"},
    {"#c4ffc4", "#c4ffc4", "#004f00"},
    {"#d2d2d2", "#d2d2d2", "#434343"},
    {"#dcdcdc", "#dcdcdc", "#434343"},
    {"#e0e0e0", "#e0e0e0", "#434343"},
    {"#e1e1e1", "#e1e1e1", "#434343"},
    {"#fdedc5", "#fdedc5", "#563f03"},
    {"#ff0000", "#ff0000", "#8c0000"},
    {"#ff3c3c", "#ff3c3c", "#8c0000"},
    {"#ff66ff", "#ff66ff", "#790079"},
    {"#ff8000", "#ff8000", "#693500"},
    {"#ff8080", "#ff8080", "#8c0000"},
    {"#ff99cc", "#ff99cc", "#870043"},
    {"#ffa500", "#ffa500", "#5c3b00"},
    {"#ffbbbb", "#ffbbbb", "#8c0000"},
    {"#fffa82", "#fffa82", "#474500"},
    {"#ffff00", "#ffff00", "#454500"},
    {"#ffff33", "#ffff33", "#454500"},
    {"#ffff66", "#ffff66", "#454500"},
    {"#ffff76", "#ffff76", "#454500"},
    {"#ffff88", "#ffff88", "#454500"},
    {"#ffff96", "#ffff96", "#454500"},
    {"#ffffff", "#ffffff", "#19232d"},
  };
}

QString convert_dark(QString const& color, bool useDarkStyle)
{
    for (auto const& c : fixed_colors)
      {
        if (color == c.ask) return useDarkStyle ? c.dark : c.light;
      }

    // not in the table - the original rule, so an unlisted colour still gets something sane
    QString res;
    bool ok;
    auto hexcolor = color.mid(1).toUInt(&ok, 16);
    if (ok && useDarkStyle) {
        auto r = (hexcolor & 0xff0000) >> 16;
        auto g = (hexcolor & 0xff00) >> 8;
        auto b = hexcolor & 0xff;
        if (r==g && r==b) {
             hexcolor = hexcolor xor 0xe6dcd2;
         } else {
          if (r >= 0x60) r -= 0x60;
          else r = 0;
          if (g >= 0x60) g -= 0x60;
          else g = 0;
          if (!(b == 0xff && r == 0 && g == 0)) {
            if (b >= 0x60) b -= 0x60;
            else b = 0;
          } else {
            r += 0x60;
            g += 0x60;
          }
          hexcolor = r << 16 | g << 8 | b;
        }
        res.setNum(hexcolor,16);
        res = res.rightJustified(6, '0');
        res = "#" + res.right(6);
    } else res = color;
    return res;
}


QString convert_Smeter(int level, bool Sunits)
  {
  if (Sunits) {
    if (level < -54) return QString {"S0-%1dB"}.arg (-level-54);
    else if (level == -54) return "S0";
    else if (level < -48) return QString {"S0+%1dB"}.arg (level+54);
    else if (level == -48) return "S1";
    else if (level < -42) return QString {"S1+%1dB"}.arg (level+48);
    else if (level == -42) return "S2";
    else if (level < -36) return QString {"S2+%1dB"}.arg (level+42);
    else if (level == -36) return "S3";
    else if (level < -30) return QString {"S3+%1dB"}.arg (level+36);
    else if (level == -30) return "S4";
    else if (level < -24) return QString {"S4+%1dB"}.arg (level+30);
    else if (level == -24) return "S5";
    else if (level < -18) return QString {"S5+%1dB"}.arg (level+24);
    else if (level <= -18) return "S6";
    else if (level < -12) return QString {"S6+%1dB"}.arg (level+18);
    else if (level == -12) return "S7";
    else if (level < -6) return QString {"S7+%1dB"}.arg (level+12);
    else if (level == -6) return "S8";
    else if (level < 0) return QString {"S8+%1dB"}.arg (level+6);
    else if (level == 0) return "S9";
    else return QString {"S9+%1dB"}.arg (level);
    }
    else
      return QString {"%1 dBm"}.arg (level-73);
  }
}
