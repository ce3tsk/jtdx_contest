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

/* CE3TSK: the name this build shows in a window title. The application name itself stays JTDX -
   it names the settings and data directories, and changing it would strand every existing
   profile - so this is display only. One copy, used by the main window and by the wide graph,
   so a build cannot end up half branded: JTDX_contest on one window and JTDX on the other.
   A named instance keeps its suffix, "JTDX - test" becoming "JTDX_contest - test". */
QString fork_name ()
{
  QString name {QCoreApplication::applicationName ()};
  if (name.startsWith ("JTDX")) name.replace (0, 4, "JTDX_contest");
  return name;
}

QString program_title (QString const& revision, QString const& contest)
{
  /* CE3TSK: this fork announces itself as JTDX_contest (the application name itself stays
     JTDX - it names the settings and data directories), and a running contest is named up
     front, before everything the title already carried, so the window and the task bar say
     which contest this instance is in. */
  QString const name {fork_name ()};
  QString const activity {contest.isEmpty () ? QString {} : " - " + contest + " -"};
  QString id {name + activity + "  by CE3TSK                                v" + QCoreApplication::applicationVersion ()};
  return id + " " + revision + ", derivative work of JTDX by UA3DJY/ES1JA and WSJT-X by K1JT";
}
