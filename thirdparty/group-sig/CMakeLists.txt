# Driver build for the vendored group-signature-lib stack:
#   pbc-0.5.14  (C)  -> static  pbc
#   pbc_sig-0.0.8 (C) -> static  pbc_sig  (links pbc)
#   group-signature-lib (C++) -> static group_sig (links pbc_sig)
#
# On Windows/MSVC the upstream autotools-based pbc/pbc_sig builds (which use
# ./configure + make and a Unix shell) are replaced by direct MSVC builds of
# the (portable) C sources, plus a small Win32 compat layer in ./win32.
cmake_minimum_required(VERSION 3.16)
project(group-sig VERSION 1.0.0 LANGUAGES C CXX)

# This port always produces static libraries.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(PBC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/pbc-0.5.14")
set(PBC_SIG_SRC "${CMAKE_CURRENT_SOURCE_DIR}/pbc_sig-0.0.8")
set(GROUP_SIG_SRC "${CMAKE_CURRENT_SOURCE_DIR}/group-signature-lib")

# pbc installs its public headers under include/pbc/ (library_includedir), and
# pbc_sig's headers are consumed as <pbc/pbc_sig.h>, so stage all of them into
# a single <prefix>/include/pbc/ directory in the build tree.
set(PBC_INCLUDE_STAGE "${CMAKE_CURRENT_BINARY_DIR}/pbc-include")
file(MAKE_DIRECTORY "${PBC_INCLUDE_STAGE}/pbc")
file(COPY "${PBC_SRC}/include/" DESTINATION "${PBC_INCLUDE_STAGE}/pbc")
file(COPY "${PBC_SIG_SRC}/include/" DESTINATION "${PBC_INCLUDE_STAGE}/pbc")

# --------------------------------------------------------------------------
# GMP (vcpkg ships no CMake config for gmp)
# --------------------------------------------------------------------------
find_path(GMP_INCLUDE_DIR NAMES gmp.h REQUIRED)
find_library(GMP_LIBRARY NAMES gmp libgmp REQUIRED)
add_library(gmp_imp UNKNOWN IMPORTED)
set_target_properties(gmp_imp PROPERTIES
    IMPORTED_LOCATION "${GMP_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}")

# --------------------------------------------------------------------------
# pbc (C library)
# --------------------------------------------------------------------------
file(GLOB PBC_SOURCES
    "${PBC_SRC}/arith/*.c"
    "${PBC_SRC}/ecc/*.c"
    "${PBC_SRC}/misc/*.c")
list(REMOVE_ITEM PBC_SOURCES
    "${PBC_SRC}/arith/init_random.c"
    "${PBC_SRC}/misc/get_time.c"
    "${PBC_SRC}/arith/init_random.win32.c"
    "${PBC_SRC}/misc/get_time.win32.c")

if(WIN32)
    list(APPEND PBC_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/win32/init_random_win32.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/win32/get_time_win32.c")
    set(WIN32_COMPAT_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/win32/include")
else()
    list(APPEND PBC_SOURCES
        "${PBC_SRC}/arith/init_random.c"
        "${PBC_SRC}/misc/get_time.c")
    set(WIN32_COMPAT_INCLUDE "")
endif()

add_library(pbc STATIC ${PBC_SOURCES})
target_include_directories(pbc PUBLIC
    "${PBC_INCLUDE_STAGE}"   # <pbc/pbc.h> and friends
    "${PBC_SRC}/include")    # <pbc_utils.h> etc. (flat)
target_include_directories(pbc PRIVATE
    "${PBC_SRC}"
    ${WIN32_COMPAT_INCLUDE})
target_link_libraries(pbc PUBLIC gmp_imp)
set_target_properties(pbc PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --------------------------------------------------------------------------
# pbc_sig (C library)
# --------------------------------------------------------------------------
add_library(pbc_sig STATIC
    "${PBC_SIG_SRC}/sig/bbs.c"
    "${PBC_SIG_SRC}/sig/hash.c"
    "${PBC_SIG_SRC}/sig/sig.c")
target_include_directories(pbc_sig PUBLIC
    "${PBC_INCLUDE_STAGE}"
    "${PBC_SRC}/include")
target_include_directories(pbc_sig PRIVATE
    "${PBC_SIG_SRC}/include"
    ${WIN32_COMPAT_INCLUDE})
target_link_libraries(pbc_sig PUBLIC pbc)
set_target_properties(pbc_sig PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --------------------------------------------------------------------------
# group_sig (C++ library)
# --------------------------------------------------------------------------
find_package(jsoncpp CONFIG REQUIRED)
find_package(cryptopp CONFIG REQUIRED)

file(GLOB_RECURSE GROUP_SIG_SOURCES "${GROUP_SIG_SRC}/group_sig/*.cpp")
add_library(group_sig STATIC ${GROUP_SIG_SOURCES})
target_include_directories(group_sig PUBLIC
    "${GROUP_SIG_SRC}"      # <group_sig/...>
    "${PBC_INCLUDE_STAGE}"  # <pbc/...>
    "${PBC_SRC}/include")
target_include_directories(group_sig PRIVATE ${WIN32_COMPAT_INCLUDE})
target_compile_definitions(group_sig PRIVATE STATICLIB)
if(WIN32)
    target_compile_definitions(group_sig PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        _WIN32_WINNT=0x0601)
endif()
target_link_libraries(group_sig PUBLIC pbc_sig gmp_imp JsonCpp::JsonCpp cryptopp::cryptopp)
set_target_properties(group_sig PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --------------------------------------------------------------------------
# install
# --------------------------------------------------------------------------
install(TARGETS pbc pbc_sig group_sig ARCHIVE DESTINATION lib)
install(DIRECTORY "${PBC_INCLUDE_STAGE}/pbc" DESTINATION include)
install(DIRECTORY "${GROUP_SIG_SRC}/group_sig" DESTINATION include
        FILES_MATCHING PATTERN "*.h")
