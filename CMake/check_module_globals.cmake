# CE3TSK 2026-09-25: fails the build when a Fortran module exports a variable that looks like a
# leaked loop counter. A module-scope initialiser with an implied-DO,
#     integer, dimension(1:49) :: nthrindex=(/ ((i-1)*100, i=1,49) /)
# makes gfortran create a module variable i. packjt77 had one from rc02 to rc08: every implicitly
# typed unit that used the module without ONLY - and the module's own procedures - shared that one
# i as its loop counter across the decoder threads. FT4 died at ft4b.f90 "Index '3296' of
# dimension 1 of array 'cd'", and without -fbounds-check the loops silently skipped or repeated
# elements. ft8_mod1's maskincallthr had the same shape.
#
# An implied-DO index is an integer, so an undeclared one starts with i-n. The check flags module
# data named with one or two characters starting with i-n, unless it is on the list below.
#
#   cmake -DNM=<nm> -DLIB=<static library> -DSTAMP=<file written on success> -P check_module_globals.cmake

# Declared on purpose: the upstream timer's PRIVATE counters, and the SuperFox code dimensions.
set (allowed
  timer_impl_MOD_i timer_impl_MOD_j timer_impl_MOD_l timer_impl_MOD_m timer_impl_MOD_lu
  sfox_mod_MOD_kk sfox_mod_MOD_mm sfox_mod_MOD_nn sfox_mod_MOD_nq sfox_mod_MOD_ns sfox_mod_MOD_nz
  )

file (REMOVE "${STAMP}")
execute_process (COMMAND "${NM}" "${LIB}"
  OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
if (NOT rc EQUAL 0)
  message (FATAL_ERROR "check_module_globals: \"${NM}\" \"${LIB}\" failed (${rc}): ${err}")
endif ()

# nm lines are "<address> <type> <symbol>". Data the object defines has an upper case type (B, C,
# D, G or S, depending on the platform); U is a reference, lower case is local. The symbol carries
# the platform's leading underscores and, for a THREADPRIVATE variable, an emutls prefix
# ("___emutls_v.__mod_MOD_x"), so the module and name are taken from the end.
set (leaked)
string (REGEX MATCHALL "[^\n]* [BCDGS] [^\n]*_MOD_[i-n][a-z0-9]?\n" hits "${out}\n")
foreach (hit IN LISTS hits)
  string (REGEX MATCH "[a-z0-9][a-z0-9_]*_MOD_[i-n][a-z0-9]?\n" sym "${hit}")
  string (STRIP "${sym}" sym)
  list (FIND allowed "${sym}" idx)
  if (idx LESS 0)
    list (APPEND leaked "${sym}")
  endif ()
endforeach ()

if (leaked)
  list (REMOVE_DUPLICATES leaked)
  string (REPLACE "_MOD_" "::" leaked "${leaked}")
  string (REPLACE ";" ", " leaked "${leaked}")
  message (FATAL_ERROR
    "${LIB} has module variables that look like leaked loop counters: ${leaked}\n"
    "gfortran turns the index of an implied-DO in a module-scope initialiser into one, and every "
    "implicitly typed unit that uses the module without ONLY then shares it - across the decoder "
    "threads. Compute the values where they are used instead. If the variable is meant to be "
    "there, declare it and add it to the list in CMake/check_module_globals.cmake.")
endif ()
file (WRITE "${STAMP}" "no leaked module loop variables\n")
