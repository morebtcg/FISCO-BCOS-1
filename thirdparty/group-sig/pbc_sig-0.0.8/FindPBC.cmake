# CMake module for finding the PBC includes and libraries

# PATHS seems to be optional in the following two commands,
# despite what the man page says
find_path(PBC_INCLUDE_DIR pbc.h PATH_SUFFIXES pbc)
find_library(PBC_LIBRARY NAMES pbc)
if (PBC_INCLUDE_DIR AND PBC_LIBRARY)
   set(PBC_FOUND TRUE)
endif (PBC_INCLUDE_DIR AND PBC_LIBRARY)

if (PBC_FOUND)
   if (NOT PBC_FIND_QUIETLY)
      message(STATUS "Found PBC: ${PBC_LIBRARY}")
   endif (NOT PBC_FIND_QUIETLY)
else (PBC_FOUND)
   if (PBC_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find PBC library")
   endif (PBC_FIND_REQUIRED)
endif (PBC_FOUND)
