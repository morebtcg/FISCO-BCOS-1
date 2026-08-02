# group-sig portfile - builds the vendored group-signature-lib stack
# (group-signature-lib + pbc-0.5.14 + pbc_sig-0.0.8) directly with the host
# compiler (MSVC on Windows).
#
# The upstream project builds pbc/pbc_sig through autotools
# (./configure + make + a Unix shell), which cannot run on Windows/MSVC. The
# sources are therefore vendored under ${VENDOR_DIR} and built by a dedicated
# MSVC-friendly CMakeLists.txt that compiles the (portable) C sources directly.
set(VENDOR_DIR "${CMAKE_CURRENT_LIST_DIR}/../../thirdparty/group-sig")

set(SOURCE_PATH "${CURRENT_BUILDTREES_DIR}/src/group-sig-vendored")
file(REMOVE_RECURSE "${SOURCE_PATH}")
file(COPY "${VENDOR_DIR}/" DESTINATION "${SOURCE_PATH}")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
)

vcpkg_cmake_install()

# Install the hand-written config that creates the GroupSig/Pbc/PbcSig/Gmp
# imported targets that FISCO-BCOS consumers (bcos-executor) expect.
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/group-sig-config.cmake"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/group-sig")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

# Install copyright
file(INSTALL "${VENDOR_DIR}/group-signature-lib/LICENSE"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
