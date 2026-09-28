# CE3TSK 2026-09-28: gfortran 8 or later, the one Fortran compiler whose string-length convention
# this program knows.
#
# A Fortran routine with a character dummy takes each string's length as an extra argument after
# the declared ones, and C++ has to pass it with the type the Fortran compiler reads (fortran_charlen_t
# in wsjtx_config.h). gfortran changed that type from int to size_t in GCC 8. wsjtx_config.h.in used
# to choose it with `#if __GNUC__ > 7`, which tests the C/C++ compiler: Apple clang reports
# __GNUC__ 4, so every macOS build passed 32-bit int lengths to a gfortran that reads 64-bit ones.
# azdist's two lengths are its 10th and 11th arguments and travel on the stack, where the Apple
# arm64 ABI packs two ints into one 8-byte slot: the first length arrived as 8 + 8 * 2^32 and the
# second came from memory the caller never wrote. Pressing OK in Settings with AutoSeq7 then ended
# JTDX in a Fortran bounds error ("upper bound (5) of 'hisgrid' exceeds string length (3)").
# Found, and fixed in his own build, by Luigi IW5DNZ. Linux and Windows build the C++ with g++,
# whose __GNUC__ matches its gfortran, and were never affected.
#
# The first fix let CMake choose int or size_t; review showed the choice itself was the hazard,
# because an unset choice (#cmakedefine01) silently meant int. Only gfortran before 8 (2018) ever
# passed int, so the header now says size_t, full stop, and this refuses everything else.
#
# jtdx_require_fortran_charlen (<compiler id> <compiler version>) stops the configure for anything
# but gfortran 8 or later. Intel and LLVM flang are believed to pass size_t as well, but nobody has
# measured them against this program, and a wrong guess builds, runs and reads garbage lengths.
# test/fortran_charlen.sh measures what a compiler really passes; add one here only with that result.
function (jtdx_require_fortran_charlen id version)
  if (NOT id STREQUAL "GNU")
    message (FATAL_ERROR
      "Fortran compiler '${id}' (version '${version}'): the C type of its hidden string lengths has "
      "not been measured, and every C++ -> Fortran call with a character argument depends on it. "
      "Use gfortran 8 or later, or measure this compiler with test/fortran_charlen.sh and add it "
      "to CMake/FortranCharlen.cmake.")
  endif ()
  if (NOT version MATCHES "^[0-9]+(\\.[0-9]+)*$")
    message (FATAL_ERROR
      "Cannot read the gfortran version ('${version}'), so cannot tell whether it passes string "
      "lengths as size_t (gfortran 8 and later) or int (earlier).")
  endif ()
  if (version VERSION_LESS 8)
    message (FATAL_ERROR
      "gfortran ${version} passes hidden string lengths as int; this program passes size_t "
      "(wsjtx_config.h). Use gfortran 8 or later.")
  endif ()
endfunction ()
