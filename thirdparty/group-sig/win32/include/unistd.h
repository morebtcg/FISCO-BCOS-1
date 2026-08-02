/*
 * unistd.h - minimal Windows shim for the POSIX API surface used by
 * group-signature-lib (CommonFunc.h). Uses MSVC CRT so it adds no
 * <windows.h> macro pollution.
 */
#ifndef _WIN32_UNISTD_STUB_H
#define _WIN32_UNISTD_STUB_H

#include <direct.h> /* _getcwd, _mkdir */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef getcwd
#define getcwd _getcwd
#endif

/* POSIX mkdir(path, mode) -> MSVC _mkdir(path); mode is ignored. */
#ifndef mkdir
#define mkdir(path, mode) _mkdir(path)
#endif

#ifndef S_IRWXU
#define S_IRWXU 0700
#endif

#ifdef __cplusplus
}
#endif

#endif /* _WIN32_UNISTD_STUB_H */
