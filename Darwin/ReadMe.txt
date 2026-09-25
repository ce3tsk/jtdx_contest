                 Notes on JTDX_contest Installation for macOS
                 --------------------------------------------

                             Updated 25 September 2026
                             -------------------------

JTDX_contest installs as JTDX_contest.app, so it does not replace a stock JTDX
(jtdx.app) - both can stay in Applications.  If you have a previous version of
JTDX_contest, you can rename it to JTDX_contest_previous before proceeding.

This installer is for {{DMG_FOR}}.
JTDX_contest is built separately for Apple Silicon and for Intel Macs; the
installer's file name ends in -arm64.dmg (Apple Silicon) or -x86_64.dmg (Intel).

The app is not signed with an Apple Developer ID, so the first time you open it
macOS will refuse.  To allow it once:

  macOS 15 Sequoia and later:  double-click JTDX_contest in Applications and close
      the warning.  Then open System Settings > Privacy & Security, scroll down
      to the message about JTDX_contest, click "Open Anyway" and confirm.
  macOS 14 Sonoma and earlier:  right-click (or Control-click) JTDX_contest in
      Applications, choose Open, and confirm.

After that it opens normally.

BEGIN:

JTDX_contest needs more shared memory than macOS allows by default.  Whenever it starts
and finds the limit too small, JTDX_contest offers to raise it for you: click Yes and
enter your password when macOS asks.  This installs a small system setting that stays in
effect, also after restarts and JTDX_contest updates.  You can skip the rest of this
section then.

To do it by hand instead, open a Terminal window by going to Applications->Utilities
and clicking on Terminal.

Along with this ReadMe file there is a file:  com.jtdx.sysctl.plist  which must be copied to a
system area by typing this line in the Terminal window and then pressing the Return key.

      sudo  cp  /Volumes/JTDX_contest/com.jtdx.sysctl.plist  /Library/LaunchDaemons

you will be asked for your normal password because authorisation is needed to copy this file.
(Your password will not be echoed but press the Return key when completed.)
Now re-boot your Mac. This is necessary to install the changes.  After the
reboot you should re-open the Terminal window as before and you can check that the
change has been made by typing:

      sysctl -a | grep sysv.shm

If shmmax is less than 13692348 - or if shmall, counted in pages of the page size that
"sysctl -n hw.pagesize" prints, comes to less than that - write to jtdx_contest@ce3tsk.com, since
JTDX_contest will fail to load with an error message: "Unable to create shared
memory segment".

You are now finished with system changes.  You should make certain that NO error messages
have been produced during these steps.   You can now close the Terminal window.  It will
not be necessary to repeat this procedure again, even when you download an updated
version of JTDX_contest.  It might be necessary if you upgrade macOS.


NEXT:

Drag the JTDX_contest app to your preferred location, such as Applications.

You need to configure your sound card.   Visit Applications > Utilities > Audio MIDI 
Setup and select your sound card and then set Format to be "48000Hz 2ch-16bit" for 
input and output.

Now double-click on the JTDX_contest app and two windows will appear.
It is mandatory to allow JTDX_contest to use the microphone (audio input) when the first launch asks.
Select Preferences under the JTDX_contest Menu and fill in various station details on the General panel.
I recommend checking the 4 boxes under the Display heading and the first 4 boxes under 
the Behaviour heading.

Next visit the Audio panel and select the Audio Codec you use to communicate between 
JTDX_contest and your rig.   There are so many audio interfaces available that it is not 
possible to give detailed advice on selection.  If you have difficulties, write to
jtdx_contest@ce3tsk.com.
Note the location of the Save Directory.  Decoded wave forms are located here.

Look at the Reporting panel.  If you check the "Prompt me" box, a logging panel will appear 
at the end of the QSO.  Two log files are provided in Library/Application Support/JTDX.
These are a simple wsjtx.log file and wsjtx_log.adi which is formatted for use with 
logging databases.    The "File" menu bar items include a button "Open log directory" 
to open the log directory in Finder for you, ready for processing by any logging 
application you use.

Finally, visit the Radio panel.  JTDX_contest is most effective when operated with CAT 
control.  You will need to install the relevant Mac driver for your rig.   This must 
be located in the device driver directory  /dev. You should install your driver 
and then re-launch JTDX_contest. Return to the the Radio panel in Preferences and in 
the "Serial port" panel select your driver from the list that is presented.   If 
for some reason your driver is not shown, then insert the full name 
of your driver in the Serial Port panel.   Such as:  /dev/tty.PL2303-00002226 or 
whatever driver you have.  The /dev/ prefix is mandatory.  Set the relevant 
communication parameters as required by your transceiver and click "Test CAT" to
check.

JTDX_contest needs the Mac clock to be accurate.  Visit System Preferences > Date & Time 
and make sure that date and time are set automatically.  The drop-down menu will 
normally offer you several time servers to choose from.

On the Help menu, JTDX_contest Web site (F1) opens https://ce3tsk.com/, where the
decoder documents, the SuperFox explainer and the measurements are published.

Please write to jtdx_contest@ce3tsk.com if you have problems.

--- Tihomir Sokcevic CE3TSK     (jtdx_contest@ce3tsk.com)

These installation notes started as the JTDX notes for macOS by Arvo ES1JA and are
kept here adapted for JTDX_contest.  Questions about JTDX_contest go to the address
above, not to him.

Addendum:  Information about com.jtdx.sysctl.plist and multiple instances of JTDX_contest.

JTDX_contest makes use of a block of memory which is shared between different parts of
the code.  The normal allocation of shared memory on a Mac is insufficient and this 
has to be increased.  The com.jtdx.sysctl.plist file is used for this purpose.  You can
use a Mac editor to examine the file.  (Do not use another editor - the file 
would probably be corrupted.)

It is possible to run multiple instances of JTDX_contest simultaneously.  The setting
allows 512 MB of shared memory in total, enough for about 39 instances at 13692348 bytes
each (13 MB, which JTDX_contest's own message rounds up to 14).  If you ever need more,
change the 536870912 (512 MB) in the com.jtdx.sysctl.plist file to (n * 14680064), where
'n' is the number of instances you want to run at once.  That is 13692348 rounded up to
14 MB - a whole number of pages at either page size, with room to spare, because the limit
counts pages and other programs draw on the same pool.
Remember to reboot your Mac afterwards.  The setting only ever raises the limits, so it
does not reduce values that another program (WSJT-X, for example) has set higher.

The shmmax parameter is raised to 104857600 (100 MB) by the same setting.  It is the largest
single block any one instance may ask for, a ceiling that reserves nothing, and it does not
need changing.

If two instances of JTDX_contest are running, it is likely that you might need additional
audio devices, from two rigs for example.  Visit Audio MIDI Setup and create an Aggregate Device
which will allow you to specify more than one interface.  I recommend you consult Apple's guide
on combining multiple audio interfaces which is at https://support.apple.com/en-us/HT202000.  
