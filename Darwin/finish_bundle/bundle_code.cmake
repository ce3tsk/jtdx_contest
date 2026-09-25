# CE3TSK: shared by min_macos.cmake and sign_bundle.cmake (see CMakeLists.txt next to this file),
# so both look at the same binaries and find their tools the same way.

# The code in an installed bundle:
#   helpers:    every file in Contents/MacOS except the main executable (helper programs, dylibs)
#   plugins:    Contents/PlugIns/**/*.dylib
#   frameworks: the Contents/Frameworks/*.framework folders
function (bundle_code bundle main_exe helpers_var plugins_var frameworks_var)
  file (GLOB helpers LIST_DIRECTORIES false "${bundle}/Contents/MacOS/*")
  list (REMOVE_ITEM helpers "${bundle}/Contents/MacOS/${main_exe}")
  file (GLOB_RECURSE plugins "${bundle}/Contents/PlugIns/*.dylib")
  file (GLOB frameworks "${bundle}/Contents/Frameworks/*.framework")
  set (${helpers_var} "${helpers}" PARENT_SCOPE)
  set (${plugins_var} "${plugins}" PARENT_SCOPE)
  set (${frameworks_var} "${frameworks}" PARENT_SCOPE)
endfunction ()

# The binary of a framework folder: Foo.framework/Foo, a link to Versions/Current/Foo. Only the
# ".framework" goes - a name may contain other dots (Foo.Bar.framework/Foo.Bar).
function (framework_binary framework binary_var)
  get_filename_component (name "${framework}" NAME)
  string (REGEX REPLACE "\\.framework$" "" name "${name}")
  set (${binary_var} "${framework}/${name}" PARENT_SCOPE)
endfunction ()

# A tool found when the build was configured, or looked up again if it has gone since (MacPorts'
# cctools removed, a build tree moved to another Mac, ...). Empty if there is none.
function (bundle_tool configured name tool_var)
  if (configured AND EXISTS "${configured}")
    set (${tool_var} "${configured}" PARENT_SCOPE)
  else ()
    find_program (bundle_tool_${name} ${name})
    if (bundle_tool_${name})
      set (${tool_var} "${bundle_tool_${name}}" PARENT_SCOPE)
    else ()
      set (${tool_var} "" PARENT_SCOPE)
    endif ()
  endif ()
endfunction ()
