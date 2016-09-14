# - Find libmemcached
# Find the native libmemcached includes and library.
# Once done this will define
#
#  LIBMEMCACHED_INCLUDE_DIRS - where to find memcached.h, etc.
#  LIBMEMCACHED_LIBRARIES    - List of libraries when using libmemcached.
#  LIBMEMCACHED_FOUND        - True if libmemcached found.

FIND_PATH(LIBMEMCACHED_INCLUDE_DIR NAMES libmemcached-1.0/memcached.h)

FIND_LIBRARY(LIBMEMCACHED_LIBRARY
    NAMES memcached
)

MARK_AS_ADVANCED(LIBMEMCACHED_LIBRARY LIBMEMCACHED_INCLUDE_DIR)

# handle the QUIETLY and REQUIRED arguments and set LIBMEMCACHED_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Libmemcached
    REQUIRED_VARS LIBMEMCACHED_LIBRARY LIBMEMCACHED_INCLUDE_DIR
)

IF(LIBMEMCACHED_FOUND)
    SET(LIBMEMCACHED_INCLUDE_DIRS ${LIBMEMCACHED_INCLUDE_DIR})
    SET(LIBMEMCACHED_LIBRARIES ${LIBMEMCACHED_LIBRARY})
ENDIF()
