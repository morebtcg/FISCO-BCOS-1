cmake_minimum_required(VERSION 3.28)

if(NOT DEFINED URL_BASE)
    set(URL_BASE "github.com")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(VERSION "3.18.0")
set(VERSION_SUFFIX "")
include(Options)
configure_project()

if (WIN32)
# boost.log's namespace encodes the target Windows API version: with
# BOOST_USE_WINAPI_VERSION >= WIN8 it is "v2_mt_nt62", while WIN7 gives
# "v2_mt_nt6". The vcpkg boost_log DLL (boost_log-vc145-mt-x64-1_88) is built
# with WIN8 (its exports live in boost::log::v2_mt_nt62), so the project must
# use the SAME value or the header-only symbols reference a different namespace
# and the final link fails with undefined boost::log::v2_mt_nt6 symbols.
    add_definitions(-DBOOST_USE_WINAPI_VERSION=BOOST_WINAPI_VERSION_WIN8)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS "ON")
endif ()

# vcpkg init
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    find_package(Git REQUIRED)
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- vcpkg WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(FISCO-BCOS VERSION ${VERSION})

# jsoncpp: FISCO-BCOS links the bare name "jsoncpp_static", which the vcpkg
# static triplet exposes as an imported target. With the dynamic (shared)
# triplet the jsoncpp config only provides jsoncpp_lib; alias jsoncpp_static to
# it so all bare-name references resolve to a full path (otherwise lld-link
# gets a bare "jsoncpp_static.lib" it cannot locate).
if(WIN32 AND NOT TARGET jsoncpp_static)
    find_package(jsoncpp CONFIG QUIET)
    if(TARGET jsoncpp_lib)
        add_library(jsoncpp_static INTERFACE IMPORTED)
        set_target_properties(jsoncpp_static PROPERTIES
            INTERFACE_LINK_LIBRARIES jsoncpp_lib)
    endif()
endif()

include(CompilerSettings)
include(BuildInfoGenerator)
include(FixFmtAppleConsteval)
fisco_patch_fmt_apple_consteval()

include(IncludeDirectories)

add_subdirectory(bcos-boostssl)
add_subdirectory(concepts)
add_subdirectory(bcos-framework)
add_subdirectory(bcos-crypto)
add_subdirectory(bcos-utilities)
add_subdirectory(libtask)
add_subdirectory(bcos-tars-protocol)
add_subdirectory(bcos-protocol)
add_subdirectory(bcos-codec)
if(NOT ONLY_CPP_SDK)
    find_package(ittapi CONFIG REQUIRED)
endif ()

add_subdirectory(legacy)

if(WITH_LIGHTNODE)
    add_subdirectory(legacy/lightnode)
endif()

if(FULLNODE)
    find_package(group-sig CONFIG REQUIRED)
    find_package(paillier CONFIG REQUIRED)
    find_package(blst CONFIG REQUIRED)

    add_subdirectory(bcos-evm)
    add_subdirectory(bcos-sealer)
    add_subdirectory(bcos-security)
    add_subdirectory(bcos-scheduler)
    add_subdirectory(bcos-executor)
    add_subdirectory(bcos-storage)
    add_subdirectory(bcos-ledger)
    add_subdirectory(bcos-rpc)
    add_subdirectory(bcos-gateway)
    add_subdirectory(bcos-pbft)
    add_subdirectory(bcos-rpbft)
    add_subdirectory(bcos-tool)
    add_subdirectory(bcos-table)
    add_subdirectory(engine)
    add_subdirectory(mempool)
    add_subdirectory(bcos-txpool)
    add_subdirectory(bcos-sync)
    add_subdirectory(bcos-front)
    add_subdirectory(libinitializer)
    add_subdirectory(fisco-bcos-air)

    add_subdirectory(transaction-executor)
    add_subdirectory(transaction-scheduler)

    if(WITH_TARS_SERVICES)
        add_subdirectory(fisco-bcos-tars-service)
        add_subdirectory(bcos-leader-election)
    endif()

    if(TESTS)
        include(CTest)
        enable_testing()
        add_subdirectory(tests)
        add_subdirectory(benchmark)
    endif()

    # bcos-leader-election can also be built standalone (without the full Tars
    # stack) when only WITH_LEDGER_ELECTION is set. Place this AFTER
    # enable_testing() so that the module's UTs are visible to ctest.
    if(NOT WITH_TARS_SERVICES AND WITH_LEDGER_ELECTION)
        add_subdirectory(bcos-leader-election)
    endif()
endif()

if(WITH_CPPSDK)
    add_subdirectory(bcos-sdk)
endif()

if(TOOLS)
  add_subdirectory(tools)
endif()

if(WITH_L2_CONTRACTS)
    add_subdirectory(bcos-l2-contracts)
endif()

# for code coverage
if(COVERAGE)
    include(Coverage)
    set(TESTS ON)
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        set(CMAKE_CXX_FLAGS "-g --coverage ${CMAKE_CXX_FLAGS} -fprofile-update=atomic")
        set(CMAKE_C_FLAGS "-g --coverage ${CMAKE_C_FLAGS} -fprofile-update=atomic")
        set(CMAKE_SHARED_LINKER_FLAGS "--coverage ${CMAKE_SHARED_LINKER_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "--coverage ${CMAKE_EXE_LINKER_FLAGS}")
    elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        add_compile_options(-Wno-unused-command-line-argument)
        set(CMAKE_CXX_FLAGS "-g -fprofile-arcs -ftest-coverage ${CMAKE_CXX_FLAGS}")
        set(CMAKE_C_FLAGS "-g -fprofile-arcs -ftest-coverage ${CMAKE_C_FLAGS}")
    endif()
    config_coverage("coverage" "")
endif()

print_config("FISCO BCOS")

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_SOURCE_DIR}/cmake/fiscobcos-config.cmake.in"
  "${CMAKE_BINARY_DIR}/fiscobcos-config.cmake"
  INSTALL_DESTINATION "share/fiscobcos"
)

install(
  FILES "${CMAKE_BINARY_DIR}/fiscobcos-config.cmake"
  DESTINATION "share/fiscobcos"
)

install(
    EXPORT fiscobcosTargets
    DESTINATION share/fiscobcos
    NAMESPACE fiscobcos::
)
# copy pre-commit to git hook

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/.git/hooks/pre-commit AND EXISTS ${CMAKE_SOURCE_DIR}/.git/hooks)
    file(COPY ${CMAKE_SOURCE_DIR}/tools/.ci/pre-commit DESTINATION ${CMAKE_SOURCE_DIR}/.git/hooks
            FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()
