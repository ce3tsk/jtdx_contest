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
      # Fortran comments, line by line. A "!" inside a character literal is not a comment, and
      # cutting there would drop the rest of a real declaration - two different declarations could
      # then hash alike, which is the one failure this function must not have. So a line carrying a
      # quote keeps its comment: over-sensitive (a reworded comment on such a line changes the hash,
      # costing one rebuild) is safe, under-sensitive is not (review 2026-09-25).
      string (REPLACE ";" "@JTDXSEMI@" part "${part}")     # ; would split the list below
      string (REPLACE "\n" ";" lines "${part}")
      set (part "")
      foreach (line IN LISTS lines)
        # is there a quote BEFORE the first "!"? If not, that "!" opens a comment and the rest of
        # the line goes. If there is, the "!" may be inside a literal, so the line is kept whole.
        # (A comment's own apostrophe - "the background's window" - sits AFTER the "!", so the
        # ordinary commented declaration is still stripped and the hash still ignores it.)
        set (before "${line}")
        string (REGEX REPLACE "!.*" "" before "${before}")
        if (NOT before MATCHES "['\"]")
          set (line "${before}")
        endif ()
        set (part "${part}${line}\n")
      endforeach ()
      string (REPLACE "@JTDXSEMI@" ";" part "${part}")
    else ()
      string (REGEX REPLACE "/\\*[^*]*\\*+([^/*][^*]*\\*+)*/" " " part "${part}") # C block comments
      string (REGEX REPLACE "//[^\n]*" " " part "${part}")                        # C line comments
    endif ()
    string (REGEX REPLACE "[ \t\r\n]+" " " part "${part}")
    string (STRIP "${part}" part)     # so a missing newline at end of file is not a difference
    string (TOLOWER "${part}" part)
    set (text "${text}${part}\n")   # one separator per file, so two files cannot glue into one text
  endforeach ()
  string (SHA256 hash "${text}")
  string (SUBSTRING "${hash}" 0 12 hash)
  set (${out} "${hash}" PARENT_SCOPE)
endfunction ()
