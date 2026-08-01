vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL "https://github.com/FISCO-BCOS/paillier-lib.git"
    REF 8c9336a41e324f361bed60f1259e297db06b441a
)

# Patch: remove pkLen check in callpaillier.cpp
vcpkg_replace_string("${SOURCE_PATH}/paillierCpp/callpaillier.cpp"
    "if ((pkLen != 512 && pkLen != 1024) || cipher1.length() != cipherLen)"
    "if (cipher1.length() != cipherLen)"
)

# arpa/inet.h (POSIX) is unavailable on Windows. It is only used for ntohs()
# (a 16-bit big-endian -> host byte swap), so provide a portable implementation
# on Windows and drop the include to avoid a ws2_32 dependency.
set(PAILLIER_INET_FIX
"#ifdef _WIN32
#include <cstdint>
static inline std::uint16_t paillier_ntohs(std::uint16_t x) { return static_cast<std::uint16_t>((x << 8) | (x >> 8)); }
#else
#include <arpa/inet.h>
#endif")
vcpkg_replace_string("${SOURCE_PATH}/paillierCpp/callpaillier.cpp"
    "#include <arpa/inet.h>"
    "${PAILLIER_INET_FIX}"
)
vcpkg_replace_string("${SOURCE_PATH}/paillierCpp/callpaillier.cpp"
    "ntohs(len)"
    "paillier_ntohs(len)"
)

# paillier-config.cmake consumes the library as a STATIC IMPORTED target, and
# the upstream project's own test executable fails to link the DLL import lib
# ("cannot open input file paillierCpp\paillier.lib") when built shared on
# Windows. Force static libraries on Windows (matches FISCO-BCOS usage). This
# must be set before vcpkg_cmake_configure because that helper appends
# "-DBUILD_SHARED_LIBS=ON" for the dynamic triplet after our OPTIONS.
if(VCPKG_TARGET_IS_WINDOWS AND NOT VCPKG_TARGET_IS_MINGW)
    set(VCPKG_LIBRARY_LINKAGE "static")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_BUILD_TYPE=Release
)

vcpkg_cmake_install()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

# Install custom cmake config
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/paillier-config.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/paillier")

# Install copyright
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
