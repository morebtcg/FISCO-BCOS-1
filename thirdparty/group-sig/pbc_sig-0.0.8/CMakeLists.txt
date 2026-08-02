cmake_minimum_required(VERSION 2.4)
project(pbc_sig)
set(CMAKE_MODULE_PATH .)

# cross compiling for Windows using Debian's mingw package
#   cmake -D crossmingw:BOOL=1
if(crossmingw)
    set(GMP_INCLUDE_DIR $ENV{HOME}/cross/gmp/include)
    set(GMP_LIBRARY $ENV{HOME}/cross/gmp/lib/libgmp.a)
    set(CMAKE_EXECUTABLE_SUFFIX ".exe")
    set(CMAKE_COMPILER_IS_GNUCXX 1)
    set(CMAKE_CXX_COMPILER "i586-mingw32msvc-c++")
    set(CMAKE_C_COMPILER "i586-mingw32msvc-gcc")
    set(CMAKE_AR "i586-mingw32msvc-ar")
    set(CMAKE_RANLIB "i586-mingw32msvc-ranlib")
    set(CMAKE_SKIP_RPATH ON)
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
else(crossmingw)
    find_package(GMP REQUIRED)
endif(crossmingw)

#For my system...I could set environment variables in my dotfile instead
#set(CMAKE_INCLUDE_PATH $ENV{HOME}/.local/include)
#set(CMAKE_LIBRARY_PATH $ENV{HOME}/.local/lib)
set(CMAKE_INCLUDE_PATH ../include)
set(CMAKE_LIBRARY_PATH ..)

find_package(PBC REQUIRED)

include_directories(${PBC_INCLUDE_DIR} ${GMP_INCLUDE_DIR})
include_directories("./include")
add_library(pbc_sig
sig/sig.c sig/hash.c sig/bbs.c
)

foreach(bin
    test/bb test/bls test/ibs test/bbs
)
    add_executable(${bin} ${bin}.c)
    target_link_libraries(${bin} ${PBC_LIBRARY} pbc_sig ${GMP_LIBRARY})
endforeach(bin)

if(debug)
set(optflags "-O0 -g")
else(debug)
set(optflags "-O3 -ffast-math -fomit-frame-pointer")
endif(debug)

set(warnflags "-Wall -W -Wfloat-equal -Wendif-labels -Wshadow  \\
    -Wpointer-arith -Wcast-align -Wstrict-prototypes \\
    -Wredundant-decls"
)

set(CMAKE_C_FLAGS "-pipe ${warnflags} ${optflags}")
