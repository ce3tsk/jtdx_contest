#include "revision_utils.hpp"

#include <cstring>

#include <QCoreApplication>
#include <QRegularExpression>

#include "scs_version.h"

namespace
{
  QString revision_extract_number (QString const& s)
  {
    QString revision;

    // try and match a number (hexadecimal allowed)
    QRegularExpression re {R"(^[$:]\w+: (r?[\da-f]+[^$]*)\$$)"};
    auto match = re.match (s);
    if (match.hasMatch ())
      {
        revision = match.captured (1);
      }
    return revision;
  }
}

QString revision (QString const& scs_rev_string)
{
  QString result;
  auto revision_from_scs = revision_extract_number (scs_rev_string);

#if defined (CMAKE_BUILD)
  QString scs_info {":Rev: " WSJTX_STRINGIZE (SCS_VERSION) " $"};

  auto revision_from_scs_info = revision_extract_number (scs_info);
  if (!revision_from_scs_info.isEmpty ())
    {
      // we managed to get the revision number from svn info etc.
      result = revision_from_scs_info;
    }
  else if (!revision_from_scs.isEmpty ())
    {
      // fall back to revision passed in if any
      result = revision_from_scs;
    }
  else
    {
      // match anything
      QRegularExpression re {R"(^[$:]\w+: ([^$]*)\$$)"};
      auto match = re.match (scs_info);
      if (match.hasMatch ())
        {
          result = match.captured (1);
        }
    }
#else
  if (!revision_from_scs.isEmpty ())
    {
      // not CMake build so all we have is revision passed
      result = revision_from_scs;
    }
#endif
  return result.trimmed ();
}

QString version (bool include_patch)
{
#if defined (CMAKE_BUILD)
  QString v {WSJTX_STRINGIZE (WSJTX_VERSION_MAJOR) "." WSJTX_STRINGIZE (WSJTX_VERSION_MINOR)};
  if (include_patch)
    {
      v += "." WSJTX_STRINGIZE (WSJTX_VERSION_PATCH)
# if defined (WSJTX_RC)
        + "-rc" WSJTX_STRINGIZE (WSJTX_RC)
# endif
        ;
    }
#else
  QString v {"Not for Release"};
#endif
  return v;
}

/* CE3TSK: everything between the application name and the version is the RIG-NAME REGION, and
   it must contain nothing but spaces.

   JTAlert reconstructs JTDX's settings path from the window title: it takes the text between
   "JTDX" and the version, strips the literal "by HF community" that stock puts there, and calls
   what is left the rig name - then looks for "JTDX - <rig>.ini", which is where the UDP port and
   server live. Stock's "by HF community" is an anchor, not decoration.

   Two rounds of evidence from JTAlert's own diagnostic, 2026-09-09:
     "JTDX_contest  by CE3TSK   v..."  ->  rig name "_contest  by CE3TSK"
     "JTDX  by CE3TSK   v..."          ->  rig name "by CE3TSK"
     "JTDX - TEST  by CE3TSK   v..."   ->  rig name "TEST  by CE3TSK"
   No ini of any of those names exists, so JTAlert never learned the UDP settings and reported
   "no messages received" - the Heartbeat was never the problem, it never opened a socket. It
   still FOUND the window (matched by the jtdx.exe process name), so dropping the anchor costs
   nothing.

   So: the name, then spaces, then the version - and who built it, which build it is and the
   attribution all live after the version, where nothing parses them. The gap keeps the version
   in stock's own column. */
QString program_title (QString const& revision)
{
  QString id {QCoreApplication::applicationName () + "                                                          v" + QCoreApplication::applicationVersion ()};
  if (!revision.isEmpty ()) id += ' ' + revision;
  return id + " contest by CE3TSK, derivative work of JTDX by UA3DJY/ES1JA and WSJT-X by K1JT";
}
