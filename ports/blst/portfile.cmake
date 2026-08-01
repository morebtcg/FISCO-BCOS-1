vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL "https://github.com/supranational/blst.git"
    REF 52cc60d78591a56abb2f3d0bd1cdafc6ba242997
)

if(VCPKG_TARGET_IS_WINDOWS AND NOT VCPKG_TARGET_IS_MINGW)
    # blst's build.sh only works with a GCC-family compiler; on MSVC use blst's
    # native build.bat (ml64 + cl + lib) which produces blst.lib in the source dir.
    vcpkg_cmake_get_vars(cmake_vars_file)
    include("${cmake_vars_file}")
    get_filename_component(BLST_MSVC_BIN_DIR "${VCPKG_DETECTED_CMAKE_C_COMPILER}" DIRECTORY)
    vcpkg_execute_build_process(
        COMMAND "${CMAKE_COMMAND}" -E env "PATH=${BLST_MSVC_BIN_DIR};$ENV{PATH}" cmd /c build.bat
        WORKING_DIRECTORY "${SOURCE_PATH}"
        LOGNAME "build-${TARGET_TRIPLET}"
    )
else()
    # Build using build.sh (blst's native build system)
    set(BLST_CC "${CMAKE_C_COMPILER}")
    if(CMAKE_OSX_SYSROOT)
        set(BLST_CC "${BLST_CC} ${CMAKE_C_SYSROOT_FLAG} ${CMAKE_OSX_SYSROOT}")
    endif()
    if(CMAKE_C_OSX_DEPLOYMENT_TARGET_FLAG AND CMAKE_OSX_DEPLOYMENT_TARGET)
        set(BLST_CC "${BLST_CC} ${CMAKE_C_OSX_DEPLOYMENT_TARGET_FLAG}${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()

    find_program(BASH_COMMAND NAMES bash REQUIRED)

    vcpkg_execute_build_process(
        COMMAND "${BASH_COMMAND}" "-lc" "CC='${BLST_CC}' AR='${CMAKE_AR}' RANLIB='${CMAKE_RANLIB}' ./build.sh"
        WORKING_DIRECTORY "${SOURCE_PATH}"
        LOGNAME "build-${TARGET_TRIPLET}"
    )
endif()

# Install headers
file(INSTALL "${SOURCE_PATH}/bindings/blst.h" DESTINATION "${CURRENT_PACKAGES_DIR}/include")
file(INSTALL "${SOURCE_PATH}/bindings/blst_aux.h" DESTINATION "${CURRENT_PACKAGES_DIR}/include")

# Install static library
set(BLST_LIB_PATH "${SOURCE_PATH}/${CMAKE_STATIC_LIBRARY_PREFIX}blst${CMAKE_STATIC_LIBRARY_SUFFIX}")
file(INSTALL "${BLST_LIB_PATH}" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")

# Install copyright
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

# Install custom cmake config that creates plain 'blst' target
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/blst-config.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/blst")

# Create usage file
configure_file("${CMAKE_CURRENT_LIST_DIR}/usage" "${CURRENT_PACKAGES_DIR}/share/${PORT}/usage" @ONLY)
