# FindIPP.cmake
#
# This module defines
#  IPP_FOUND - Set to true if IPP is found
#  IPP_INCLUDE_DIRS - Where to find IPP headers
#  IPP_LIBRARIES - The IPP libraries to link against
#  IPP_VERSION - The version of IPP found
#
# Variables that can be set before calling find_package():
#  IPP_ROOT_DIR - The root directory of the IPP installation

find_path(IPP_INCLUDE_DIRS
  NAMES ipp.h
  PATHS
    ${IPP_ROOT_DIR}/include
    $ENV{IPP_ROOT_DIR}/include
)

find_library(IPPSL9_LIBRARY
  NAMES ippsl9
  PATHS
    ${IPP_ROOT_DIR}/lib
    ${IPP_ROOT_DIR}/lib/intel64
    $ENV{IPP_ROOT_DIR}/lib
    $ENV{IPP_ROOT_DIR}/lib/intel64
)

find_library(IPPIL9_LIBRARY
  NAMES ippil9
  PATHS
    ${IPP_ROOT_DIR}/lib
    ${IPP_ROOT_DIR}/lib/intel64
    $ENV{IPP_ROOT_DIR}/lib
    $ENV{IPP_ROOT_DIR}/lib/intel64
)


find_library(IPPI_LIBRARY
  NAMES ippi
  PATHS
    ${IPP_ROOT_DIR}/lib
    ${IPP_ROOT_DIR}/lib/intel64
    $ENV{IPP_ROOT_DIR}/lib
    $ENV{IPP_ROOT_DIR}/lib/intel64
)

find_library(IPPS_LIBRARY
  NAMES ipps
  PATHS
    ${IPP_ROOT_DIR}/lib
    ${IPP_ROOT_DIR}/lib/intel64
    $ENV{IPP_ROOT_DIR}/lib
    $ENV{IPP_ROOT_DIR}/lib/intel64
)

find_library(IPPCORE_LIBRARY
  NAMES ippcore
  PATHS
    ${IPP_ROOT_DIR}/lib
    ${IPP_ROOT_DIR}/lib/intel64
    $ENV{IPP_ROOT_DIR}/lib
    $ENV{IPP_ROOT_DIR}/lib/intel64
)

if(IPP_INCLUDE_DIRS AND IPPIL9_LIBRARY AND IPPSL9_LIBRARY AND IPPI_LIBRARY AND IPPS_LIBRARY AND IPPCORE_LIBRARY)
  set(IPP_FOUND TRUE)
  set(IPP_LIBRARIES ${IPPIL9_LIBRARY} ${IPPSL9_LIBRARY} ${IPPI_LIBRARY} ${IPPS_LIBRARY} ${IPPCORE_LIBRARY})
  
  # Try to find the version of IPP
  file(READ "${IPP_INCLUDE_DIRS}/ipp/ippversion.h" IPP_VERSION_HEADER)
  string(REGEX MATCH "#define IPP_VERSION_MAJOR[ \t]+([0-9]+)" _ ${IPP_VERSION_HEADER})
  set(IPP_VERSION_MAJOR ${CMAKE_MATCH_1})
  string(REGEX MATCH "#define IPP_VERSION_MINOR[ \t]+([0-9]+)" _ ${IPP_VERSION_HEADER})
  set(IPP_VERSION_MINOR ${CMAKE_MATCH_1})
  string(REGEX MATCH "#define IPP_VERSION_UPDATE[ \t]+([0-9]+)" _ ${IPP_VERSION_HEADER})
  set(IPP_VERSION_UPDATE ${CMAKE_MATCH_1})
  set(IPP_VERSION "${IPP_VERSION_MAJOR}.${IPP_VERSION_MINOR}.${IPP_VERSION_UPDATE}")

  message(STATUS "Found IPP: ${IPP_VERSION} (found suitable version ${IPP_VERSION})")
else()
  set(IPP_FOUND FALSE)
  message(STATUS "IPP not found")
endif()

mark_as_advanced(IPP_INCLUDE_DIRS IPP_LIBRARIES)

