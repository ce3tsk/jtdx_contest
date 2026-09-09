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

/* CE3TSK: the leading token MUST be QCoreApplication::applicationName() exactly - "JTDX", or
   "JTDX - <rig>" when -r names an instance. JTAlert, and anything else that reconstructs JTDX's
   settings path from the window title, reads the rig name from precisely that position and uses
   it to find "JTDX - <rig>.ini", which is where the UDP port and server live.

   This build used to put "JTDX_contest" there. JTAlert's own diagnostic (2026-09-09) then read
   the rig name as "_contest  by CE3TSK", looked for an ini of that name, found none, never
   learned the UDP settings and reported "no messages received" - the Heartbeat was never the
   problem, it never opened the socket. With -r it was worse: "_contest - test  by CE3TSK".

   So which build this is now sits AFTER the version, where nothing parses it, and a running
   contest is no longer named in the title at all. */
QString program_title (QString const& revision)
{
  QString id {QCoreApplication::applicationName () + "  by CE3TSK                                v"
              + QCoreApplication::applicationVersion () + " contest"};
  if (!revision.isEmpty ()) id += ' ' + revision;
  return id + ", derivative work of JTDX by UA3DJY/ES1JA and WSJT-X by K1JT";
}
