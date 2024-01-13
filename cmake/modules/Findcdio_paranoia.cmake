# Find cdio
#
# Defines the following variables:
#
# * CDIO_PARANOIA_FOUND
# * CDIO_PARANOIA_INCLUDE_DIRS
# * CDIO_PARANOIA_LIBRARIES
# * CDIO_PARANOIA_VERSION

if(CDIO_PARANOIA_INCLUDE_DIR AND CDIO_PARANOIA_LIBRARY)
  set(CDIO_PARANOIA_FIND_QUIETLY TRUE)
endif()

find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)

  pkg_check_modules(PC_CDIO_PARANOIA QUIET cdio_paranoia)

  set(CDIO_PARANOIA_VERSION ${PC_CDIO_PARANOIA_VERSION})
endif(PkgConfig_FOUND)

find_path(
  CDIO_PARANOIA_INCLUDE_DIR
  NAMES cdio/paranoia/paranoia.h
  HINTS ${PC_CDIO_PARANOIA_INCLUDEDIR} ${PC_CDIO_PARANOIA_INCLUDE_DIRS})

find_library(
  CDIO_PARANOIA_LIBRARY
  NAMES cdio_paranoia
  HINTS ${PC_CDIO_PARANOIA_LIBDIR} ${PC_CDIO_PARANOIA_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  cdio_paranoia
  REQUIRED_VARS CDIO_PARANOIA_INCLUDE_DIR CDIO_PARANOIA_LIBRARY
  VERSION_VAR CDIO_PARANOIA_VERSION
  FAIL_MESSAGE DEFAULT_MSG)

set(CDIO_PARANOIA_LIBRARIES ${CDIO_PARANOIA_LIBRARY})
set(CDIO_PARANOIA_INCLUDE_DIRS ${CDIO_PARANOIA_INCLUDE_DIR})

mark_as_advanced(CDIO_PARANOIA_INCLUDE_DIR CDIO_PARANOIA_LIBRARY)