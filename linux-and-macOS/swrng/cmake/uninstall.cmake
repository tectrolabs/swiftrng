# Removes the files listed in install_manifest.txt, which 'cmake --install' writes.
# The manifest holds the paths without DESTDIR, so set the same DESTDIR here if one was used for the install.
if(NOT EXISTS "${MANIFEST}")
  message(FATAL_ERROR "Cannot find install manifest: ${MANIFEST}. Run 'cmake --install' first.")
endif()

file(STRINGS "${MANIFEST}" installed_files)
foreach(f ${installed_files})
  set(target "$ENV{DESTDIR}${f}")
  if(EXISTS "${target}" OR IS_SYMLINK "${target}")
    message(STATUS "Removing ${target}")
    file(REMOVE "${target}")
  else()
    message(STATUS "Not found, skipping: ${target}")
  endif()
endforeach()
