# CE3TSK: finishes the installed Mac bundle, once everything is in it - the top-level
# CMakeLists.txt adds this directory last, so it runs after fixup_bundle and after everything the
# other install rules put into the bundle (manpages, doc, ...). In this order:
#  1. min_macos.cmake: the oldest macOS the bundle can run on - the highest minimum of any binary
#     in it, usually one of the libraries fixup_bundle copied in - into Info.plist
#     (LSMinimumSystemVersion) and into the DMG's ReadMe.txt, together with the kind of Mac;
#  2. sign_bundle.cmake: fixup_bundle rewrote the binaries' install names, which breaks their
#     signatures, and Apple Silicon refuses to run unsigned code - so re-sign ad hoc. After 1,
#     because the signature seals Info.plist.
# Both use bundle_code.cmake. They run for Debug builds too (no fixup_bundle there): the ReadMe
# is installed either way and must not keep its placeholder - it then says that the build only
# runs on the Mac that built it - and re-signing is harmless.
if (is_debug_build)
  set (FINISH_DEBUG_BUILD 1)
else ()
  set (FINISH_DEBUG_BUILD 0)
endif ()
find_program (OTOOL_EXECUTABLE otool)
find_program (LIPO_EXECUTABLE lipo)
find_program (PLUTIL_EXECUTABLE plutil)
find_program (CODESIGN_EXECUTABLE codesign)
mark_as_advanced (OTOOL_EXECUTABLE LIPO_EXECUTABLE PLUTIL_EXECUTABLE CODESIGN_EXECUTABLE)
# CE3TSK 2026-09-25: what min_macos.cmake assumes when lipo is not there to ask. The TARGET's
# architecture: CMAKE_OSX_ARCHITECTURES when the build sets it (a cross-build), else this machine's.
if (CMAKE_OSX_ARCHITECTURES)
  string (REPLACE ";" " " finish_fallback_archs "${CMAKE_OSX_ARCHITECTURES}")
else ()
  set (finish_fallback_archs "${PROJECT_ARCHITECTURE}")
endif ()

configure_file (bundle_code.cmake "${CMAKE_CURRENT_BINARY_DIR}/bundle_code.cmake" COPYONLY)
configure_file (min_macos.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/min_macos.cmake" @ONLY)
configure_file (sign_bundle.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/sign_bundle.cmake" @ONLY)
install (SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/min_macos.cmake")
install (SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/sign_bundle.cmake")
