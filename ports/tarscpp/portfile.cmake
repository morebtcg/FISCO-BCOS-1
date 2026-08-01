vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO FISCO-BCOS/TarsCpp
    REF 324eca1e66eedc14fe088b83b829386faa637714
    SHA512 d451a5595445f4406ea13797be3aa44e52eb6279dd1e05bd9dfb40ad5838db3cb28d395ea9cf7d2b2073dde09f60ed894a8ef314f6e3a76b399f378844163cff
    HEAD_REF master
)

# TarsCpp headers declare no dllexport/dllimport macros, so on Windows a shared
# tarsutil/tarsservant/tarsparse cannot export symbols and the tools fail to
# link with LNK2019. Force static libraries on Windows (matches FISCO-BCOS usage
# and the x64-windows-static CI triplet). Setting VCPKG_LIBRARY_LINKAGE here is
# required because vcpkg_cmake_configure otherwise appends
# "-DBUILD_SHARED_LIBS=ON" (for the dynamic triplet) after our OPTIONS.
if(VCPKG_TARGET_IS_WINDOWS AND NOT VCPKG_TARGET_IS_MINGW)
    set(VCPKG_LIBRARY_LINKAGE "static")
endif()

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
