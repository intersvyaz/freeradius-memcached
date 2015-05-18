cmake_minimum_required(VERSION 2.8)
project(rlm_memcache_ops C)

if(CMAKE_BINARY_DIR STREQUAL ${CMAKE_SOURCE_DIR})
        message( FATAL_ERROR "Building in source dir prohibited. Remove CMakeCache.txt and try 'mkdir build && cd build && cmake ..'." )
endif()

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE EQUAL "")
    message(STATUS "No build type selected, default to RelWithDebInfo")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(Libmemcached REQUIRED)
find_package(Libfreeradius REQUIRED)

set(INCLUDES ${LIBMEMCACHED_INCLUDE_DIRS} ${LIBFREERADIUS_INCLUDE_DIRS})

set(LIBS ${LIBMEMCACHED_LIBRARIES} ${LIBFREERADIUS_LIBRARIES})

set(SOURCES rlm_memcache_ops.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

add_definitions(-DSTANDALONE_BUILD=1)

include_directories(${INCLUDES})

add_library(rlm_memcache_ops SHARED ${SOURCES})
set_target_properties(rlm_memcache_ops PROPERTIES PREFIX "")
target_link_libraries(rlm_memcache_ops ${LIBS})
