aux_source_directory(. SRC_LIST)

set(BINARY_NAME fisco-bcos)
add_executable(${BINARY_NAME} ${SRC_LIST})
target_link_libraries(${BINARY_NAME} PUBLIC init pbft_init command gateway blst)
# System libraries required by the vendored Rust FFI static library
# (ffi_c_fisco_bcos from the wedprcrypto port): its bundled Rust std references
# ntdll (NtReadFile/NtWriteFile/...) and userenv (GetUserProfileDirectoryW).
target_link_libraries(${BINARY_NAME} PRIVATE ntdll userenv)
# clang-cl vs MSVC name-mangling fixes for symbols provided by MSVC-built vcpkg
# DLLs:
#  - boost_serialization: clang-cl emits a reference to the "vbase dtor"
#    (??_Darchive_exception) which the DLL does not export; alias it to the
#    exported virtual destructor (both the plain and __imp_ forms).
#  - protobuf: the header declares InternalSwap with a __restrict parameter,
#    which clang-cl mangles (PEIAV) but the MSVC-built DLL does not (PEAV);
#    alias the clang-cl spelling to the DLL export.
target_link_options(${BINARY_NAME} PRIVATE
    "/alternatename:??_Darchive_exception@archive@boost@@QEAAXXZ=??1archive_exception@archive@boost@@UEAA@XZ"
    "/alternatename:__imp_??_Darchive_exception@archive@boost@@QEAAXXZ=__imp_??1archive_exception@archive@boost@@UEAA@XZ"
    "/alternatename:__imp_?InternalSwap@RepeatedPtrFieldBase@internal@protobuf@google@@QEAAXPEIAV1234@@Z=__imp_?InternalSwap@RepeatedPtrFieldBase@internal@protobuf@google@@QEAAXPEAV1234@@Z"
)
set_target_properties(${BINARY_NAME} PROPERTIES UNITY_BUILD "ON")