#ifndef THREAD_SHUTDOWN_HPP_
#define THREAD_SHUTDOWN_HPP_

// CE3TSK 2026-09-26: bounded shutdown of the worker threads that sit in third-party library calls.
// A thread sees QThread::quit () only once it is back in its event loop, and a library call can
// keep it away for minutes: Hamlib's rig_open against an FLRig that accepted the connection but
// never answered (FLRig itself waiting on an IC-7300 that had dropped off USB), or a CoreAudio call
// in SoundInput::start waiting on the audio server (a microphone permission prompt). An unbounded
// QThread::wait () at shutdown then left the program unable to quit.
//
// Past the limit the thread is left running. Deleting a running QThread is fatal ("Destroyed while
// thread is still running"), so it is taken out of the QObject tree and must not be deleted, and
// abandoned () is set: main () then ends the process without the remaining teardown, which that
// thread could otherwise wake into.
//
// How it was tested (macOS; the same idea works elsewhere). A test-mode instance keeps its own
// settings under ~/.qttest, so copy the operator's .ini to "~/.qttest/Library/Preferences/JTDX -
// test.ini" and point it at a CAT server that accepts connections and never answers:
//     python3 -c 'import socket;s=socket.socket();s.bind(("127.0.0.1",12399));s.listen(8);c=[s.accept() for _ in iter(int,1)]'
// with Rig=FLRig FLRig and CATNetworkPort=127.0.0.1:12399 (PSKReporter=false, UDPServer= empty to
// keep it off the air). Start it with
//     open -n --stderr /tmp/jtdx_test.err -a JTDX_contest.app --args --test-mode
// (without --stderr the message below goes nowhere), check with `sample <pid>` that the rig thread
// sits in rig_open, then quit it as Cmd-Q does:
//     osascript -l JavaScript -e "ObjC.import('AppKit'); \$.NSRunningApplication.runningApplicationWithProcessIdentifier(<pid>).terminate"
// It must exit within rig_msecs, with "Rig control did not stop within 15 s, leaving it" in that
// file, the .ini rewritten and no lock file left. The audio case needs an audio start that waits on
// the microphone prompt. macOS asks only while it has no decision stored for the app, and a rebuild
// does not reliably clear one, so the operator first runs
//     tccutil reset Microphone tech.jtdx.contest
// then leaves the prompt unanswered and quits (audio_msecs). Rig=None and an answered prompt give
// the normal path: a quit well under a second, no message.
//
// The queued quit (queue_quit below): Rig=Hamlib NET rigctl against rigctld -m 1 (the dummy rig)
// behind a proxy that logs each command and delays each reply by 2 s, so that a poll takes about
// 10 s and a quit lands in one. With the quit set at once the log ended ... m, t, q: the stop
// closeEvent had queued was dropped and PTT off ("T 0") never sent. Queued, T 0 follows the poll.

#include <atomic>
#include <iostream>

#include <QAbstractEventDispatcher>
#include <QMetaObject>
#include <QThread>

namespace thread_shutdown
{
  // The limits. The rig's is the longer one: closeEvent queues the rig's shutdown
  // (TransceiverBase::shutdown - PTT off, then post-PTT and split restore), and on a slow but
  // answering rig (a remote rigctld, commands that run into Hamlib timeouts) that sequence must
  // not be cut short - with CAT PTT through a network server nothing else would unkey the radio.
  // The audio thread has no such sequence.
  constexpr unsigned long rig_msecs {15000};
  constexpr unsigned long audio_msecs {5000};

  // true once any worker thread has been left running at shutdown
  inline std::atomic<bool>& abandoned ()
  {
    static std::atomic<bool> flag {false};
    return flag;
  }

  // Quit the thread's event loop only after the events already posted to it. QThread::quit () ends
  // the loop at its next check, and whatever is still posted then is never delivered - a finishing
  // thread sends only its DeferredDelete events. The rig's stop that closeEvent queued (PTT off,
  // split restore) was lost that way whenever a poll was still running. Posted events are delivered
  // in order, so a quit posted through the thread's own event dispatcher is delivered after them.
  // Not necessarily after they have finished, though: a nested event loop inside one of them
  // delivers it too, and QThread::exit () ends every loop in the thread. TCI waits for its replies
  // in nested loops; its stop still runs on to do_stop, whose close () sends what is buffered, and
  // its commands are also flushed where they are sent (TCITransceiver.cpp, sendTextMessage) for
  // when that close is never reached. Hamlib, DXLab, HRD and OmniRig's stop paths have no nested
  // loop. A stuck thread never reaches the quit, and the wait times out as before.
  inline void queue_quit (QThread * thread)
  {
    if (auto * dispatcher = QAbstractEventDispatcher::instance (thread))
      {
        QMetaObject::invokeMethod (dispatcher, [thread] {thread->quit ();}, Qt::QueuedConnection);
      }
    else
      {
        thread->quit ();          // no event loop to queue behind (not started, or finished)
      }
  }

  // Ask the thread to quit, after what is already posted to it, and wait up to msecs for it. True
  // if it stopped. False if it was left running - then the caller must not delete it. std::cerr,
  // not qWarning (): release builds compile Qt warnings out (QT_NO_WARNING_OUTPUT).
  inline bool quit_and_wait (QThread * thread, unsigned long msecs, char const * what)
  {
    queue_quit (thread);
    if (thread->wait (msecs)) return true;
    thread->setParent (nullptr);
    abandoned () = true;
    std::cerr << what << " did not stop within " << msecs / 1000. << " s, leaving it\n";
    return false;
  }

  // Deleter for an owning std::unique_ptr<QThread>: stops the thread as quit_and_wait () does and
  // deletes it, or leaves it running. Also covers an owner whose constructor throws after start ().
  struct stop_and_delete
  {
    unsigned long msecs;
    char const * what;

    void operator () (QThread * thread) const
    {
      if (quit_and_wait (thread, msecs, what)) delete thread;
    }
  };
}

#endif
