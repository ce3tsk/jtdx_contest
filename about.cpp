#include "about.h"

#include <QCoreApplication>
#include <QString>

#include "revision_utils.hpp"

#include "ui_about.h"
#include "moc_about.cpp"

CAboutDlg::CAboutDlg(QWidget *parent, bool useDarkStyle) :
  QDialog(parent),
  ui(new Ui::CAboutDlg)
{
  ui->setupUi(this);

  /* CE3TSK: Qt draws <a href> in the palette Link color, a dark blue that is unreadable
     on the dark style background - the app stylesheet cannot set QPalette::Link, so the
     anchors carry the color themselves. */
  QString const link_color {useDarkStyle ? "#4EA3E0" : "#0000ff"};

  /* CE3TSK: the fork's About text - the fork first, the JTDX and WSJT-X lineage it is built
     on kept in full, as the GPL and plain honesty both require.

     The prose is translated; what is NOT wrapped in tr() is deliberate - people's names and
     callsigns, the contributor callsign list, version numbers and URLs read the same in every
     language, and a translator who edits them introduces a mistake rather than a translation.
     The <br> breaks live inside the translated strings so each language can break its own
     lines: the dialog is a fixed-width QLabel and a longer language needs different breaks. */
  ui->labelTxt->setText ("<html><h2>JTDX_contest v"
                         + QCoreApplication::applicationVersion () + "  &mdash; "
                         + tr ("JTDX Contest Edition") + "</h2>\n\n"
                         + tr ("Designed, built and measured by <b>Tihomir Sokcevic, CE3TSK</b>, Santiago de Chile, 2025-2026") + "<br>"
                         "<a style=\"color:" + link_color + "\" href=\"https://ce3tsk.com\">https://ce3tsk.com</a><br>"
                         /* the support button, shipped as a Qt resource (contrib/support_*.png, 252x44): a QLabel does not fetch remote images */
                         "<a href=\"https://ko-fi.com/ce3tsk\"><img src=\":/support_" + QString {useDarkStyle ? "dark" : "light"}
                         + ".png\" width=\"340\" height=\"59\" alt=\"Support this work on Ko-fi\"></a><br><br>"
                         + tr ("A rebuilt FT8 and FT4 decoder - alternate pass, ensemble, pipelined RX phase and TX<br>"
                               "background, four-period hint memory, fixed data races, measured presets - and built-in<br>"
                               "support for the WW Digi DX Contest: the grid exchange, points and multipliers, a separate<br>"
                               "contest log, contest-aware autoselect. Every number behind it comes from recorded audio<br>"
                               "and a script in the tree. The GUI has been repaired throughout and the dark style now works.") + "<br><br>"
                         + tr ("&copy; 2025-2026 Tihomir Sokcevic, CE3TSK (the decoder work, the contest support, the measurements and the documents).") + "<br><br>"
                         + tr ("<b>Thanks to the operators who beta tested the first release candidates</b> on the air:") + "<br>"
                         "Willy XQ3SK &middot; Eduardo CA3EAP &middot; Miguel CA7FKZ &middot; Cristian CA8CEU.<br><br>"
                         + tr ("<b>Derivative work of JTDX</b> by Igor Chernikov, UA3DJY, and Arvo J&auml;rve, ES1JA, &copy; 2016-2022,<br>"
                               "created with contributions from") + "<br>"
                         "5P1KZX, 9A5CW, BD3OOX, CE2EC, CT1AXS, DK7UY, DO1IP, EA1AHY, EA3W,<br>"
                         "EA7QL, ES2HV, ES2MC, ES4AW, ES5TF, F1DSZ, F5RUE, G4UJS, G7OED, HA3LI, IZ5ILJ, JA2BQX, JG1APX, JP1LRT,<br>"
                         "LU9DO, MM0HVU, N6ML, NL9222, OE1MWW, ON3CQ, PA7TWO, PP5FMM, R3BB, RK3AOL, RX3ASP, RA4UDC, RW4O, R0JF,<br>"
                         "SM0LTV, SP2L, SV1IYF, UA3ALE, US-E-12, VE3NEA, VK3AMA, VK6KXW, VK7YUM, VR2UPU, W9MDB,<br>"
                         + tr ("YL3GBC family and LY3BG family: Vytas and Rimas Kudelis.") + "<br><br>"
                         + tr ("<b>JTDX is derived from WSJT-X</b> (forked from WSJT-X v1.7 r6462; FT8 code from v1.8 and v2.0,<br>"
                               "FT4 from v2.1, WSPR from v2.1.2), &copy; 2001-2022 by Joe Taylor, K1JT, Bill Somerville, G4WJS,<br>"
                               "Steve Franke, K9AN, and Nico Palermo, IV3NWV.") + "<br><br>"
                         + tr ("Supports FT8, FT4, JT9, T10 and JT65A for HF amateur radio communication.") + "<br><br>"
                         + tr ("JTDX_contest, JTDX and WSJT-X are licensed under the terms of Version 3<br>"
                               "of the GNU General Public License (GPL)") + "<br>"
                         "<a style=\"color:" + link_color + "\" href=\"https://www.gnu.org/licenses/gpl-3.0.txt\">"
                         "https://www.gnu.org/licenses/gpl-3.0.txt</a>");
}

CAboutDlg::~CAboutDlg()
{
}
