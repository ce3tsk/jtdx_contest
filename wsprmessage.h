#ifndef WSPRMESSAGE_H
#define WSPRMESSAGE_H

#include <QString>

/* CE3TSK 2026-10-14: the WSPR message of one transmission (TODO 3.-64).

   A standard call sends type 1, "CALL GRID4 dBm". A compound call (one with a '/') cannot: it alternates type 2,
   "CALL/P dBm" - the call and the power, no locator - with type 3, "<CALL/P> GRID6 dBm" - a hash of the call, a
   locator of SIX characters and the power. The encoder (lib/wqencode.f90) takes the six characters after the
   blank and reads the power from the eighth character on. The program handed it the whole grid:
     - 8 characters ("FF46AB12"): the power was read from the grid's last digit, 2, and rounded to WSPR's next
       level - 37 dBm went out as 3;
     - 4 characters ("FF46"): nothing stands where the power is read, and the Fortran runtime ENDED THE PROGRAM
       ("End of file", wqencode.f90 line 53) when the type-3 transmission began - stock JTDX's code, and WSJT-X
       3.3.0-beta1 builds the same message;
     - 6 characters: right.
   Now type 3 carries the grid's first six characters, and a compound call whose grid has fewer than six sends
   type 2 every time: the locator cannot travel, and padding it would claim a precision the operator never gave.
   The same when the type-3 message is longer than 22 characters - the encoder (genwspr, character*22) takes 22,
   and a call of 11 characters and more lost the power's last digit ("<VP2V/WA1ABC> FF46AB 37" went out as 3 dBm)
   or the whole power, which ended the program again (review of 2026-10-14). A call that long is no WSPR call
   either: type 2 carries a prefix of up to three characters or a suffix; what goes out then is the encoder's
   affair, but nothing ends and no power is read from the grid.
   (The grid comes in upper case, Configuration::my_grid (); the encoder's call packer upper-cases on its own too.)

   type3: a standard call sends type 3 as well (MainWindow's m_grid6, which nothing sets today).
   second: which of the pair this transmission is (MainWindow's m_tx after its toggle: 0 the type-3 message). */
namespace wspr_message
{
  inline QString of (QString const& call, QString const& grid, int dBm, bool second, bool type3)
  {
    QString const sdBm {QString::asprintf (" %d", dBm)};
    bool const compound {call.indexOf ('/') > 0};
    QString const type1 {call + " " + grid.left (4) + sdBm};
    if (!compound && !type3) return type1;                    // the normal WSPR message
    QString const pair {compound ? call + sdBm : type1};      // type 2, or type 1
    QString const grid6 {grid.left (6)};
    QString const message3 {"<" + call + "> " + grid6 + sdBm};
    if (6 != grid6.size ()) return pair;                      // no six-character locator: no type 3
    if (message3.size () > 22) return pair;                     // longer than the encoder takes (review 3.-67)
    return second ? pair : message3;
  }
}

#endif
