# Version number components
# CE3TSK: JTDX_CONTEST 3.0.0 release candidate 5 (was 2.2.159-32A). WSJTX_RC is appended
# to the patch level as "-rc05" by CMake/VersionCompute.cmake while IS_RELEASE is 0; the
# 32A marker of the 32-bit audio experiment is retired.
set (WSJTX_VERSION_MAJOR 3)
set (WSJTX_VERSION_MINOR 0)
set (WSJTX_VERSION_32A 0)
set (WSJTX_VERSION_SUB 0)
set (WSJTX_RC 05)		 # release candidate number, comment out or zero for development versions
set (WSJTX_VERSION_IS_RELEASE 0) # set to 1 for final release build
