/*
 * sys/time.h - minimal Windows shim providing gettimeofday()/struct timeval
 * and strtok_r (via MSVC strtok_s). No <windows.h> macro pollution.
 */
#ifndef _WIN32_SYS_TIME_STUB_H
#define _WIN32_SYS_TIME_STUB_H

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _TIMEVAL_DEFINED
struct timeval
{
    long tv_sec;
    long tv_usec;
};
#define _TIMEVAL_DEFINED
#endif

/* gettimeofday shim backed by the MSVC CRT (no Win32 headers needed). */
#include <sys/timeb.h>

static __inline int gettimeofday(struct timeval* tv, void* tz)
{
    (void)tz;
    struct __timeb64 tb;
    _ftime64_s(&tb);
    tv->tv_sec = (long)tb.time;
    tv->tv_usec = (int)tb.millitm * 1000;
    return 0;
}

/* strtok_r has the identical signature/semantics as MSVC strtok_s. */
#ifndef strtok_r
#define strtok_r strtok_s
#endif

#ifdef __cplusplus
}
#endif

#endif /* _WIN32_SYS_TIME_STUB_H */
