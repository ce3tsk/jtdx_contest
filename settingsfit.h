// -*- Mode: C++ -*-
#ifndef SETTINGSFIT_H
#define SETTINGSFIT_H

/* CE3TSK 2026-10-19 (TODO.md 3.-143, 3.-144, 3.-147): the Settings dialog on a small screen.
 *
 * The dialog was as tall as its tallest tab: 915 px inside in the light style and 744 in the dark one at an 11 pt font -
 * on a screen 768 px high its bottom, OK and Cancel with it, was below the screen, and it could not be made smaller.
 * Every page now sits in a scroll area (scroll_pages), so the dialog can be as small as the screen needs; before it
 * shows (fit_dialog) each page is held at the height it needs (pin_pages), so a page that does not fit scrolls and is
 * never squeezed, and the dialog - its natural size, or the one it was left at - is cut to the screen's work area and
 * put inside it. The Notifications tab's colour table is in two columns (Configuration.ui), so General is the tallest
 * page now. test/harness/test_settingsfit pins all of it; tools/ui-check/settingsfit.sh looks at it on a 1366x768 screen.
 */

#include <algorithm>
#include <QBoxLayout>
#include <QEvent>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLayout>
#include <QRect>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>
#include <QTabWidget>
#include <QWidget>

namespace settings_fit
{
  /* ---- the height a page needs ----
   * What it asks for, less what the spacers that stretch it ask for (Filters asks 218 px more than it holds, for an empty
   * spacer at its foot), and never less than its layout's minimum. Not the minimum alone: there a group box can get less
   * than it needs and draw its lines over each other (General's "Logbook filtering", 60 px of its 83). Spacers count
   * where they stack: in a vertical box, in a grid row of their own or under a grid row's only item, and inside a widget
   * (a group box) stacked so; side by side they add no height.
   * A page is held at that height with those spacers collapsed (collapse true: each asks no height, and still takes
   * the room left over): a layout given less than it asks takes the shortfall from every item alike, the spacers no
   * more than the rest - General's "Logbook filtering" lost 2 px, the Contest tab's frequency table 20. */
  inline int spacer_slack (QLayout * l, bool collapse = false);

  inline int item_slack (QLayoutItem * it, bool collapse = false)
  {
    if (QSpacerItem * sp = it->spacerItem ())
      {
        if (!(sp->expandingDirections () & Qt::Vertical)) return 0;
        int const h = sp->sizeHint ().height ();
        if (collapse && h) sp->changeSize (sp->sizeHint ().width (), 0, sp->sizePolicy ().horizontalPolicy (), sp->sizePolicy ().verticalPolicy ());
        return h;
      }
    if (it->layout ()) return spacer_slack (it->layout (), collapse);
    if (QWidget * w = it->widget ()) return w->isHidden () ? 0 : spacer_slack (w->layout (), collapse);
    return 0;
  }

  inline int spacer_slack (QLayout * l, bool collapse)
  {
    if (!l) return 0;
    if (auto * g = qobject_cast<QGridLayout *> (l))
      {
        int s = 0;
        for (int r = 0; r < g->rowCount (); ++r)
          {
            QLayoutItem * only = nullptr;
            int items = 0, spacers = 0, spacerSlack = 0;
            for (int i = 0; i < g->count (); ++i)
              {
                int row, col, rows, cols;
                g->getItemPosition (i, &row, &col, &rows, &cols);
                if (r < row || r >= row + rows) continue;
                ++items;
                if (rows > 1) { items = 2; spacers = -1; continue; }   // a row another row's item spans: not its own
                only = g->itemAt (i);
                if (only->spacerItem ()) { ++spacers; spacerSlack = std::max (spacerSlack, item_slack (only)); }
              }
            if (items > 0 && spacers == items)   // a row of spacers alone
              {
                s += spacerSlack;
                if (collapse)
                  for (int i = 0; i < g->count (); ++i)
                    {
                      int row, col, rows, cols;
                      g->getItemPosition (i, &row, &col, &rows, &cols);
                      if (row == r) item_slack (g->itemAt (i), true);
                    }
              }
            else if (items == 1) s += item_slack (only, collapse);   // a row of one item: its own slack
          }
        return s;
      }
    auto * b = qobject_cast<QBoxLayout *> (l);
    if (!b || (b->direction () != QBoxLayout::TopToBottom && b->direction () != QBoxLayout::BottomToTop)) return 0;
    int s = 0;
    for (int i = 0; i < b->count (); ++i) s += item_slack (b->itemAt (i), collapse);
    return s;
  }

  /* Asked of hints as they are now: the dialog is built at the font the program started with and gets the configured
     one after (Configuration::impl::set_application_font), and its layouts and their items keep sizes from the first -
     the Notifications page asked 110 px more than it holds at an 11 pt font started from a 12 pt one, and General
     asked less. Every widget's size cache and every layout are cleared, so they are asked again at the font in use. */
  inline void refresh_hints (QWidget * root)
  {
    root->updateGeometry ();
    for (auto * w : root->findChildren<QWidget *> ()) w->updateGeometry ();
    if (root->layout ()) root->layout ()->invalidate ();
    for (auto * l : root->findChildren<QLayout *> ()) l->invalidate ();
  }

  inline int needed_height (QWidget * page)
  {
    return std::max (page->minimumSizeHint ().height (), page->sizeHint ().height () - spacer_slack (page->layout ()));
  }

  /* ---- a page in a scroll area ----
   * In height it asks for what its page needs (QScrollArea's own hint stops at 24 lines of text, and the dialog would
   * open short of its pages) and can be made as short as the dialog needs - the page scrolls. In width it is never
   * narrower than its page beside a vertical scroll bar: a page is never scrolled sideways. (The Radio tab's rig list
   * is held to 16 characters for that - Configuration.cpp - or its longest name would set the dialog's width.) */
  inline void pin_page (QWidget * page);

  class PageScroll : public QScrollArea
  {
  public:
    explicit PageScroll (QWidget * parent = nullptr) : QScrollArea {parent} {}

    void pinned () {pinned_ = true;}

    /* a pinned page whose layout changes while the dialog is open (a row shown or hidden) is held at its new height */
    bool eventFilter (QObject * o, QEvent * e) override
    {
      if (o == widget () && e->type () == QEvent::LayoutRequest && pinned_) pin_page (widget ());
      return QScrollArea::eventFilter (o, e);
    }

    QSize sizeHint () const override
    {
      if (!widget ()) return QScrollArea::sizeHint ();
      return {minimumSizeHint ().width (), needed_height (widget ()) + 2 * frameWidth ()};
    }

    QSize minimumSizeHint () const override
    {
      QSize s {QScrollArea::minimumSizeHint ()};
      if (widget ()) s.setWidth (std::max (s.width (), widget ()->minimumSizeHint ().width () + 2 * frameWidth ()
                                                       + verticalScrollBar ()->sizeHint ().width ()));
      return s;
    }

  private:
    bool pinned_ {false};
  };

  /* every page of the tab widget into a PageScroll - its tab's text, icon, tooltip, What's This and state kept, the
     current tab too */
  inline void scroll_pages (QTabWidget * tabs)
  {
    int const current = tabs->currentIndex ();
    for (int i = 0; i < tabs->count (); ++i)
      {
        QWidget * page = tabs->widget (i);
        if (dynamic_cast<PageScroll *> (page)) continue;   // already in one
        QString const text {tabs->tabText (i)}, tip {tabs->tabToolTip (i)}, what {tabs->tabWhatsThis (i)};
        QIcon const icon {tabs->tabIcon (i)};
        bool const enabled {tabs->isTabEnabled (i)};
        auto * area = new PageScroll;
        area->setObjectName (page->objectName () + "_scroll");
        area->setProperty ("settingsPage", true);   // darkstyle.qss: no frame of its own, the tab pane is the page's
        area->setWidgetResizable (true);
        area->setFrameShape (QFrame::NoFrame);
        area->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
        tabs->removeTab (i);
        area->setWidget (page);
        /* the page drawn as before it scrolled (3.-147): setWidget () fills it with the window's colour and the viewport
           with its parent's, where the tab pane showed through - in a lighter colour (Fusion: 251, the window 239) */
        page->setAutoFillBackground (false);
        area->viewport ()->setAutoFillBackground (false);
        tabs->insertTab (i, area, icon, text);
        tabs->setTabToolTip (i, tip);
        tabs->setTabWhatsThis (i, what);
        tabs->setTabEnabled (i, enabled);
      }
    tabs->setCurrentIndex (current);
  }

  /* the tab that holds a page - QTabWidget::indexOf () knows only the scroll areas */
  inline int page_index (QTabWidget const * tabs, QWidget const * page)
  {
    for (int i = 0; i < tabs->count (); ++i)
      {
        QWidget const * w = tabs->widget (i);
        if (w == page) return i;
        if (auto const * area = qobject_cast<QScrollArea const *> (w))
          if (area->widget () == page) return i;
      }
    return -1;
  }

  /* a page held at the height it needs, its stacked spacers collapsed - an unchanged height sets nothing, and asks no
     new layout */
  inline void pin_page (QWidget * page)
  {
    if (spacer_slack (page->layout (), true)) refresh_hints (page);   // a spacer changed: the layouts keep its old height
    page->setMinimumHeight (needed_height (page));
  }

  /* every page pinned - asked again, of fresh hints, before every showing: the font may have changed */
  inline void pin_pages (QTabWidget * tabs)
  {
    for (int i = 0; i < tabs->count (); ++i)
      if (QWidget * w = tabs->widget (i))
        if (auto * area = dynamic_cast<PageScroll *> (w))
          if (QWidget * page = area->widget ())
            {
              refresh_hints (page);
              pin_page (page);
              area->pinned ();
              area->updateGeometry ();
            }
  }

  /* a control brought into view: its tab current, its page scrolled to it - setFocus () scrolls nothing (a scroll area
     follows the focus on Tab alone), and "Correct it" (Configuration.cpp) put the cursor in a call box scrolled out of
     sight (3.-147) */
  inline void show_on_page (QTabWidget * tabs, QWidget * w)
  {
    for (int i = 0; i < tabs->count (); ++i)
      if (tabs->widget (i)->isAncestorOf (w))
        {
          tabs->setCurrentIndex (i);
          if (auto * area = qobject_cast<QScrollArea *> (tabs->widget (i))) area->ensureWidgetVisible (w);
        }
  }

  /* ---- the dialog on the screen ----
   * A window's geometry is its inside; its title bar (title) sits above it and its border (border) around it. The
   * dialog keeps the size it asks for but no more than the screen's work area leaves room for, and goes inside it. */
  inline QRect fitted (QRect wanted, QRect const& available, int title, int border)
  {
    wanted.setWidth (std::min (wanted.width (), available.width () - 2 * border));
    wanted.setHeight (std::min (wanted.height (), available.height () - title - border));
    if (wanted.right () > available.right () - border) wanted.moveRight (available.right () - border);
    if (wanted.left () < available.left () + border) wanted.moveLeft (available.left () + border);
    if (wanted.bottom () > available.bottom () - border) wanted.moveBottom (available.bottom () - border);
    if (wanted.top () < available.top () + title) wanted.moveTop (available.top () + title);
    return wanted;
  }

  /* the room for a title bar: what the style says, and never under 32 px - window managers draw taller ones (37 px
     under metacity) than Qt's styles report */
  inline int title_room (QWidget const * w)
  {
    return std::max (32, w->style ()->pixelMetric (QStyle::PM_TitleBarHeight, nullptr, w) + 8);
  }

  /* the point whose screen the dialog is fitted to: its own middle once it has been placed, or - never shown, it sits at
     the screen's origin - the middle of its parent's window, on whose screen Qt centres it. Two screens, the main window
     on a 768 px one right of a 1080 px one at the origin: the dialog was cut to 1080 and shown on 768 (3.-147) */
  inline QPoint fit_point (QWidget const * dialog, bool placed)
  {
    QWidget const * parent = dialog->parentWidget () ? dialog->parentWidget ()->window () : nullptr;
    return (placed || !parent ? dialog : parent)->geometry ().center ();
  }

  /* before the dialog shows: its hints refreshed - not only the pages': the hidden dialog keeps its tab widget's old size,
     and on the screen it opened 66 px wider and 38 px taller in the dark style without it - and the pages pinned, then
     its size - the one it was left at (placed), or its natural one (Qt then centres it on its parent, inside the screen)
     - cut to the work area of its screen */
  inline void fit_dialog (QWidget * dialog, QTabWidget * tabs, bool placed)
  {
    refresh_hints (dialog);
    pin_pages (tabs);
    QScreen * screen = QGuiApplication::screenAt (fit_point (dialog, placed));
    if (!screen) screen = QGuiApplication::primaryScreen ();
    if (!screen) return;
    QRect const available {screen->availableGeometry ()};
    int const border = 8;
    if (placed)
      dialog->setGeometry (fitted (dialog->geometry (), available, title_room (dialog), border));
    else
      dialog->resize (fitted (QRect {available.topLeft (), dialog->sizeHint ().expandedTo (dialog->minimumSizeHint ())},
                              available, title_room (dialog), border).size ());
  }
}

#endif
