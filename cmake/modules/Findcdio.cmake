# Find cdio
#
# Defines the following variables:
#
# * CDIO_FOUND
# * CDIO_INCLUDE_DIRS
# * CDIO_LIBRARIES
# * CDIO_VERSION

if(CDIO_INCLUDE_DIR AND CDIO_LIBRARY)
  set(CDIO_FIND_QUIETLY TRUE)
endif()

find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)

  pkg_check_modules(PC_CDIO QUIET cdio)

  set(CDIO_VERSION ${PC_CDIO_VERSION})
endif(PkgConfig_FOUND)

find_path(
  CDIO_INCLUDE_DIR
  NAMES cdio/cdio.h cdio.h
  HINTS ${PC_CDIO_INCLUDEDIR} ${PC_CDIO_INCLUDE_DIRS})

find_library(
  CDIO_LIBRARY
  NAMES cdio libcdio
  HINTS ${PC_CDIO_LIBDIR} ${PC_CDIO_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  cdio
  REQUIRED_VARS CDIO_INCLUDE_DIR CDIO_LIBRARY
  VERSION_VAR CDIO_VERSION
  FAIL_MESSAGE DEFAULT_MSG)

set(CDIO_LIBRARIES ${CDIO_LIBRARY})
set(CDIO_INCLUDE_DIRS ${CDIO_INCLUDE_DIR})

mark_as_advanced(CDIO_INCLUDE_DIR CDIO_LIBRARY)
