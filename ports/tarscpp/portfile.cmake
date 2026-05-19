vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO FISCO-BCOS/TarsCpp
    REF f8df3a979490034c4cec0c3c1d208e5c6d0ef0c6
    SHA512 09e9bb7b0dc905a48ad9d2c27ff6ac371ff405be7ab4f95ca0a6749e870be4fe03c99ce4cc9bd82706cc4db31d9dc2cc1727a16cb6797da5a2ba950c27f7c248
    HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS -DTARS_MYSQL=OFF
    DISABLE_PARALLEL_CONFIGURE
)

vcpkg_cmake_build()
vcpkg_cmake_install()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
