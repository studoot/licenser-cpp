#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "unofficial::date::tz" for configuration "Debug"
set_property(TARGET unofficial::date::tz APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(unofficial::date::tz PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/libtz.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS unofficial::date::tz )
list(APPEND _IMPORT_CHECK_FILES_FOR_unofficial::date::tz "${_IMPORT_PREFIX}/debug/lib/libtz.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
