# CE3TSK: fixup_bundle rewrites the binaries' install names, which breaks their signatures, and
# Apple Silicon refuses to run unsigned code - so the installed bundle is re-signed ad hoc. The
# top-level CMakeLists.txt adds this directory last, so the signature also seals everything the
# other install rules put into the bundle (manpages, doc, ...).
find_program (CODESIGN_EXECUTABLE codesign)
mark_as_advanced (CODESIGN_EXECUTABLE)
configure_file (sign_bundle.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/sign_bundle.cmake" @ONLY)
install (SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/sign_bundle.cmake")
