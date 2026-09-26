# CE3TSK 2026-09-25: fails the build when a Fortran MODULE-SCOPE declaration or DATA statement
# initialises something with an implied-DO. gfortran turns the index into a module variable, and
# every implicitly typed unit that uses the module without ONLY - and the module's own procedures,
# by host association - then share that one variable as their loop counter, across the decoder
# threads (MODULE_LOOP_VARIABLE_RACE.md; packjt77's nthrindex was the FT4 crash of rc08).
#
# This is the companion of check_module_globals.cmake, which reads the built library with nm. The
# two catch different things, measured with gfortran 2026-09-25:
#
#   construct at module scope                          nm check        this check
#   integer :: a(3)=(/ ((i-1)*2, i=1,3) /)             yes             yes
#   integer, parameter :: b(3)=(/ ((jx-1)*2, jx=1,3) /) yes            yes
#   the index named idx - three characters             NO              yes
#   data (e(ii),ii=1,3) /1,2,3/                        yes             yes
#   implicit none, with the index declared             only if 1-2 ch  yes
#
# IMPLICIT NONE does not make the construct safe: it only forces the index to be declared, and a
# declared module variable is shared just the same. An implied-DO INSIDE a procedure is fine and is
# not flagged - it is an ordinary local.
#
#   cmake -DSRCDIR=<source dir> -DSTAMP=<file written on success> -P check_module_implied_do.cmake

# empty list elements are kept: the line numbering and the fixed-form gluing below depend on
# them (CMake 2.6 and newer understand this policy, the tree's minimum is 3.7.2)
cmake_policy (SET CMP0007 NEW)

file (REMOVE "${STAMP}")
file (GLOB_RECURSE sources "${SRCDIR}/*.f90" "${SRCDIR}/*.f" "${SRCDIR}/*.F90")
set (bad)

foreach (path IN LISTS sources)
  file (READ "${path}" text)
  string (REPLACE ";" "@SEMI@" text "${text}")    # ; is CMake's list separator, put it back below
  string (REPLACE "\r" "" text "${text}")
  string (REPLACE "\n" ";" text "${text}")
  get_filename_component (ext "${path}" EXT)
  string (TOLOWER "${ext}" ext)
  if (ext MATCHES "^\\.f$|^\\.for$")
    # Fixed form continues a statement with a mark in column 6, so glue those onto the line they
    # continue BEFORE the scan, leaving an empty line behind to keep the line numbers right
    # (review 2026-09-25: a DATA implied-DO split that way was invisible).
    set (joined "")
    set (held "")
    foreach (raw IN LISTS text)
      string (REGEX REPLACE "\t" "        " raw "${raw}")
      set (mark "")
      string (LENGTH "${raw}" rawlen)
      if (rawlen GREATER 5 AND NOT raw MATCHES "^[c*!Cd]")
        string (SUBSTRING "${raw}" 5 1 mark)
      endif ()
      if (NOT mark STREQUAL "" AND NOT mark STREQUAL " " AND NOT mark STREQUAL "0" AND NOT held STREQUAL "")
        math (EXPR tail "${rawlen}-6")
        string (SUBSTRING "${raw}" 6 ${tail} rest)
        set (held "${held}${rest}")
        list (LENGTH joined n)
        math (EXPR n "${n}-1")
        list (REMOVE_AT joined ${n})
        list (APPEND joined "${held}" "")
      else ()
        set (held "${raw}")
        list (APPEND joined "${raw}")
      endif ()
    endforeach ()
    set (text "${joined}")
  endif ()
  set (in_module FALSE)
  set (past_contains FALSE)
  set (in_proc FALSE)
  set (type_depth 0)
  set (pending "")                                # a line continued with &
  set (lineno 0)
  set (startline 0)
  foreach (raw IN LISTS text)
    math (EXPR lineno "${lineno}+1")
    string (REPLACE "@SEMI@" ";" raw "${raw}")
    string (TOLOWER "${raw}" line)
    if (ext STREQUAL ".f")                        # fixed form: a comment marker in column 1
      if (line MATCHES "^[c*!]")
        continue ()
      endif ()
    endif ()
    string (REGEX REPLACE "!.*$" "" line "${line}")
    string (STRIP "${line}" line)
    if (line STREQUAL "")
      continue ()
    endif ()
    if (NOT pending STREQUAL "")                  # glue continuation lines together
      string (REGEX REPLACE "^&" "" line "${line}")
      set (line "${pending} ${line}")
    else ()
      set (startline ${lineno})
    endif ()
    if (line MATCHES "&$")
      string (REGEX REPLACE "&$" "" pending "${line}")
      continue ()
    endif ()
    set (pending "")

    # The end of a unit is tested FIRST: "end function nh" also matches the start pattern, and
    # testing that first latched in_proc for the rest of the module (review 2026-09-25).
    if (line MATCHES "^end[ \t]*module" OR line MATCHES "^end[ \t]*submodule")
      set (in_module FALSE)
      set (in_proc FALSE)
      set (type_depth 0)
    elseif (line MATCHES "^end[ \t]*(subroutine|function)")
      set (in_proc FALSE)
    elseif (line MATCHES "^end[ \t]*type")
      if (type_depth GREATER 0)
        math (EXPR type_depth "${type_depth}-1")
      endif ()
    elseif (line MATCHES "^module[ \t]+[a-z_]" AND NOT line MATCHES "^module[ \t]+procedure")
      set (in_module TRUE)
      set (past_contains FALSE)
      set (in_proc FALSE)
      set (type_depth 0)
    elseif (line MATCHES "^submodule[ \t]*\\(")
      set (in_module TRUE)
      set (past_contains FALSE)
      set (in_proc FALSE)
      set (type_depth 0)
    elseif (line MATCHES "^type[ \t]*(,|::|[ \t]+[a-z_])" AND NOT line MATCHES "^type[ \t]*\\(")
      math (EXPR type_depth "${type_depth}+1")     # a derived type has its own CONTAINS
    elseif (in_module AND type_depth EQUAL 0 AND line MATCHES "^contains$")
      set (past_contains TRUE)
    elseif (line MATCHES "(^|[ \t])(subroutine|function)[ \t]+[a-z_]")
      set (in_proc TRUE)
    endif ()
    if (NOT in_module OR past_contains OR in_proc)
      continue ()
    endif ()

    # An implied-DO is ", <name> = <start> , <end>" inside an array constructor (/ ... /) or
    # [ ... ], or inside the parentheses of a DATA statement. Two things keep ordinary code out
    # (review 2026-09-25, each one a real false positive before):
    #   - string literals are blanked first, so 'q, x=1' in a character array is not a hit;
    #   - each constructor is taken from its opener to its OWN closer, not to the last one on the
    #     line, so reshape((/1,2,3,4/), shape=(/2,2/)) and two constructors in one declaration are
    #     each looked at separately;
    #   - the name must be followed by "= something ," with no bracket in between, which is what
    #     an implied-DO's start,end looks like and a keyword argument such as kind=4) does not.
    string (REGEX REPLACE "'[^']*'" "''" line "${line}")
    string (REGEX REPLACE "\"[^\"]*\"" "\"\"" line "${line}")
    set (candidate "")
    if (line MATCHES "^data[ \t(]")
      set (candidate "${line}")
    elseif (line MATCHES "::")
      foreach (form "slash" "bracket")
        if (form STREQUAL "slash")
          set (open "(/")
          set (close "/)")
        else ()
          set (open "[")
          set (close "]")
        endif ()
        set (rest "${line}")
        set (scanning 1)
        while (scanning)
          string (FIND "${rest}" "${open}" a)
          if (a LESS 0)
            set (scanning 0)
            break ()
          endif ()
          string (LENGTH "${rest}" restlen)
          math (EXPR after "${a}+2")
          if (after GREATER restlen)
            set (scanning 0)
            break ()
          endif ()
          math (EXPR tail "${restlen}-${after}")
          string (SUBSTRING "${rest}" ${after} ${tail} rest)
          string (FIND "${rest}" "${close}" b)      # this constructor's own closer
          if (b LESS 0)
            set (candidate "${candidate} ${rest}")
            set (scanning 0)
            break ()
          endif ()
          string (SUBSTRING "${rest}" 0 ${b} part)
          set (candidate "${candidate} ,${part},")
          string (LENGTH "${rest}" restlen)
          math (EXPR after "${b}+1")
          math (EXPR tail "${restlen}-${after}")
          string (SUBSTRING "${rest}" ${after} ${tail} rest)
        endwhile ()
      endforeach ()
    endif ()
    if (NOT candidate STREQUAL "" AND candidate MATCHES ",[ \t]*[a-z_][a-z_0-9]*[ \t]*=[^,()\\[\\]]*,")
      file (RELATIVE_PATH rel "${SRCDIR}" "${path}")
      string (SUBSTRING "${line}" 0 90 shown)
      list (APPEND bad "${rel}:${startline}: ${shown}")
    endif ()
  endforeach ()
endforeach ()

if (bad)
  string (REPLACE ";" "\n  " bad "${bad}")
  message (FATAL_ERROR
    "implied-DO in a module-scope initialiser:\n  ${bad}\n"
    "gfortran turns the index into a module variable, which every implicitly typed unit that uses "
    "the module without ONLY - and the module's own procedures - then share as their loop counter, "
    "across the decoder threads (MODULE_LOOP_VARIABLE_RACE.md). Compute the values where they are "
    "used instead. Declaring the index does not help: a declared module variable is shared too.")
endif ()
file (WRITE "${STAMP}" "no module-scope implied-DO initialisers\n")
