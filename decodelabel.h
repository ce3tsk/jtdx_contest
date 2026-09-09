#ifndef DECODELABEL_H
#define DECODELABEL_H
#include <QString>

/* CE3TSK: the decoder label's count (PIPELINED_DECODE_PLAN.md section 10): "/D-" while the
   period's decode runs (D grows as lines arrive), "/D" when it is done, "/D+N=S|" while
   the TX background runs (D from the decode, N from the background, S their sum), "/D+N=S"
   when it is done: "-" means the RX phase is still running, "|" the TX background (the
   pipe, the background decodes' own mark). Item 81: "X" after a background that did NOT
   finish - the next period's decode cut it short - the visual clue that the machine load or
   the effort level wants a look; it stays until a background completes again.

   No spaces and no brackets anywhere in it: the label is clipped at the right on Windows
   long before it is here, and every character dropped is one more that keeps the trailing
   marker - the point of the whole line - on screen. The tints do the separating instead. */
inline QString decode_count_label (bool rx_running, bool bg_running, bool bg_ran, int decodes, int decodes_rx, bool bg_cut = false)
{
  int const D = decodes_rx, N = decodes - decodes_rx;
  if (rx_running) return QString ("/%1-").arg (decodes);
  if (bg_running) return QString ("/%1+%2=%3|").arg (D).arg (N).arg (D + N);
  if (bg_ran) return QString ("/%1+%2=%3%4").arg (D).arg (N).arg (D + N).arg (bg_cut ? "X" : "");
  return QString ("/%1").arg (D);
}

/* the same, as rich text for the QLabel: D on blue, N on red, S on green (lighter tints in
   the light style, deeper ones in the dark style), the spacing of a prefix kept with
   non-breaking spaces by decode_label_prefix_html */
inline QString decode_count_label_html (bool rx_running, bool bg_running, bool bg_ran, int decodes, int decodes_rx, bool dark, bool bg_cut = false)
{
  auto tint = [dark] (int value, char const* light, char const* deep) {
    return QString ("<span style=\"background-color:%1;\">%2</span>").arg (dark ? deep : light).arg (value);
  };
  int const D = decodes_rx, N = decodes - decodes_rx;
  QString const d = tint (rx_running ? decodes : D, "#c4d8ff", "#1e3c8a");
  QString const n = tint (N, "#ffc4bc", "#8a2a1e");
  QString const s = tint (D + N, "#c4f0c4", "#1e6a1e");
  if (rx_running) return "/" + d + "-";
  if (bg_running) return "/" + d + "+" + n + "=" + s + "|";
  if (bg_ran) return "/" + d + "+" + n + "=" + s + (bg_cut ? QString ("<span style=\"background-color:%1;\">X</span>").arg (dark ? "#8a2a1e" : "#ffc4bc") : QString ());   // item 81: cut short, on the background's red
  return "/" + d;
}

inline QString decode_label_prefix_html (QString const& prefix)
{
  return prefix.toHtmlEscaped ().replace (" ", "&nbsp;");
}

/* the average DT ("0.10") on lavender, after the "Avg=" of the prefix */
inline QString decode_avg_html (QString const& avg, bool dark)
{
  if (avg.isEmpty ()) return QString ();
  return QString ("<span style=\"background-color:%1;\">%2</span>").arg (dark ? "#4a2a7a" : "#e6d4ff").arg (avg.toHtmlEscaped ());
}

/* the lag figure ("+1.70") on amber, between the "Lag=" and the count */
inline QString decode_lag_html (QString const& lag, bool dark)
{
  if (lag.isEmpty ()) return QString ();
  return QString ("<span style=\"background-color:%1;\">%2</span>").arg (dark ? "#7a6a1e" : "#fff0b0").arg (lag.toHtmlEscaped ());
}


/* CE3TSK: the two timing lamps that sit between the Contest and Preset lamps.

   They answer one question at a glance - is this preset asking for more effort than this
   machine has time for? RX follows the same figure the label above the decodes prints as
   "Lag=": how far past the start of the next period the receive decode ran, when the reply
   has to be decided about a second into it. TX says whether the background decoding that
   keeps working through your own transmission finished, or was cut short by the next period
   (the "X" the count carries).

   The colours are the ones the decode label already uses - pale tints under black text in the
   light style, deep ones under white in the dark - so the lamps and the line they summarise
   read as one thing. Measured against WCAG AA (4.5:1): the light tints run 13.9 to 18.3:1 on
   black, the deep ones 5.4 to 8.6:1 on white. */
enum class TimingLevel { Good, Tight, Late, Unknown };

/* FT4 has 7.5 s to FT8's 15 and must answer inside 1.36 s, so its thresholds are halved. */
inline TimingLevel rx_timing_level (double lag_seconds, bool ft4)
{
  if (lag_seconds <= (ft4 ? 1.0 : 2.0)) return TimingLevel::Good;
  if (lag_seconds <= (ft4 ? 1.4 : 2.8)) return TimingLevel::Tight;
  return TimingLevel::Late;
}

inline QString timing_lamp_style (TimingLevel level, bool dark)
{
  char const* bg = nullptr;
  switch (level)
    {
    case TimingLevel::Good:  bg = dark ? "#1e6a1e" : "#c4f0c4"; break;   // the count's green
    case TimingLevel::Tight: bg = dark ? "#7a6a1e" : "#fff0b0"; break;   // the lag's amber
    case TimingLevel::Late:  bg = dark ? "#8a2a1e" : "#ffc4bc"; break;   // the background's red
    case TimingLevel::Unknown: break;
    }
  if (!bg) return QString ("QLabel{border: 1px solid #808080; border-radius: 3px; padding: 1px 4px}");
  return QString ("QLabel{color: %1; background: %2; border: 1px solid %3; border-radius: 3px;"
                  " padding: 1px 4px}")
    .arg (dark ? "#ffffff" : "#000000", bg, dark ? "#000000" : "#808080");
}

#endif
