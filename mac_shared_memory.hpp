// CE3TSK: the macOS System V shared memory limits and JTDX's launch daemon that raises them
// (Darwin/com.jtdx.sysctl.plist) - used by main.cpp, which offers to install the daemon when the
// limits keep JTDX's segment from being created. Header-only and free of Qt, so it can be tested
// on its own.
#ifndef MAC_SHARED_MEMORY_HPP
#define MAC_SHARED_MEMORY_HPP

#if defined (__APPLE__)

#include <cstdint>
#include <string>
#include <sys/types.h>
#include <sys/sysctl.h>

#include "jtdx_sysctl_plist.h"

namespace mac_shared_memory
{
  // a sysctl number, 0 if it can't be read (64 bits in the kernel; a smaller answer lands in the
  // low bytes of the zeroed value, as macOS is little-endian on Intel and Apple Silicon)
  inline std::uint64_t sysctl_value (char const * name)
  {
    std::uint64_t value = 0;
    size_t size = sizeof value;
    return sysctlbyname (name, &value, &size, nullptr, 0) == 0 ? value : 0;
  }

  // whether the limits keep a segment of `bytes` from being created; shmall counts pages of the
  // kernel's page size, 4 KB on Intel and 16 KB on Apple Silicon
  inline bool limits_too_small (std::uint64_t bytes)
  {
    auto page = sysctl_value ("hw.pagesize");
    if (!page) page = 4096;
    return sysctl_value ("kern.sysv.shmmax") < bytes
      || sysctl_value ("kern.sysv.shmall") * page < bytes;
  }

  // The shell script main.cpp runs as root: writes the daemon to `target`, makes it root's, and
  // runs its command at once, so the limits are raised now as well as at every boot. Everything
  // that runs as root is in this program: the daemon's text is compiled in (jtdx_sysctl_plist.h),
  // never read from the app bundle, which anything running as the user could change. The one file
  // read back, the written daemon, is root's by then (/Library/LaunchDaemons is root's).
  inline std::string install_script (std::string const& target = "/Library/LaunchDaemons/com.jtdx.sysctl.plist")
  {
    std::string plist {jtdx_sysctl_plist};
    if (plist.empty () || plist.back () != '\n') plist += '\n';
    return "set -e\n"
      "target='" + target + "'\n"
      "/bin/cat > \"$target\" <<'JTDX_PLIST'\n" + plist + "JTDX_PLIST\n"
      "/usr/sbin/chown root:wheel \"$target\"\n"
      "/bin/chmod 644 \"$target\"\n"
      // The daemon's own command, read back from the file just written and run at once. It is
      // ASSIGNED first on purpose: `set -e` does not look at the exit status of a command
      // substitution used as an argument, so `sh -c "$(...)"` with a failing or missing
      // PlistBuddy would run the empty string and report success - and the caller would then
      // say the limits were raised when they were not (review 2026-09-25).
      "cmd=$(/usr/libexec/PlistBuddy -c 'Print :ProgramArguments:2' \"$target\")\n"
      "[ -n \"$cmd\" ]\n"
      "/bin/sh -c \"$cmd\"\n";
  }
}

#endif

#endif
