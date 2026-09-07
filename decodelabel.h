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

#endif
