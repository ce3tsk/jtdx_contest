#ifndef REVISION_UTILS_HPP__
#define REVISION_UTILS_HPP__

#include <QString>

QString revision (QString const& scs_rev_string = QString {});
QString version (bool include_patch = true);
/* CE3TSK: contest, when not empty, is inserted after the application name */
QString fork_name ();   /* CE3TSK: the build's own name for a window title */
QString program_title (QString const& revision = QString {}, QString const& contest = QString {});

#endif
