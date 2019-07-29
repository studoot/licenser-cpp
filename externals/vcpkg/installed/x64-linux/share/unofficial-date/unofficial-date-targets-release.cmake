#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "unofficial::date::tz" for configuration "Release"
set_property(TARGET unofficial::date::tz APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(unofficial::date::tz PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libtz.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS unofficial::date::tz )
list(APPEND _IMPORT_CHECK_FILES_FOR_unofficial::date::tz "${_IMPORT_PREFIX}/lib/libtz.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
