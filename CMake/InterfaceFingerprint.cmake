# CE3TSK 2026-09-25: one fingerprint for the GUI/decoder shared-memory interface, compiled into
# BOTH halves so a mismatched pair can say so instead of decoding rubbish.
#
# jtdx writes commons.h's `struct dec_data` into the shared segment and jtdxjt9 reads jt9com.f90's
# `type dec_data` out of it. The two are separate files that must agree, and this fork has changed
# the params block several times (340, 348, 356, 360, 364, 368 bytes); deploying one half alone
# across such a change decodes wrongly or not at all, and nothing said so.
#
# jtdx_interface_fingerprint (<out var> <file> ...) hashes the DECLARATIONS of the files it is
# given: comments, blank lines and indentation are removed first, so a reworded comment does not
# invalidate a pair while any change to the struct does.
function (jtdx_interface_fingerprint out)
  set (text "")
  foreach (f IN LISTS ARGN)
    file (READ "${f}" part)
    # each language's comment syntax on its own files only: "!" is a Fortran comment but a C
    # negation (#if !defined(X)), and "//" is a C comment but Fortran concatenation
    get_filename_component (ext "${f}" EXT)
    string (TOLOWER "${ext}" ext)
    if (ext MATCHES "^\\.f")
      string (REGEX REPLACE "![^\n]*" " " part "${part}")                         # Fortran comments
    else ()
      string (REGEX REPLACE "/\\*[^*]*\\*+([^/*][^*]*\\*+)*/" " " part "${part}") # C block comments
      string (REGEX REPLACE "//[^\n]*" " " part "${part}")                        # C line comments
    endif ()
    string (REGEX REPLACE "[ \t\r\n]+" " " part "${part}")
    string (TOLOWER "${part}" part)
    set (text "${text}${part}")
  endforeach ()
  string (SHA256 hash "${text}")
  string (SUBSTRING "${hash}" 0 12 hash)
  set (${out} "${hash}" PARENT_SCOPE)
endfunction ()
