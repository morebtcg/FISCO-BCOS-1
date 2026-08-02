/*
 * get_time_win32.c - Win32 replacement for pbc-0.5.14/misc/get_time.c.
 * Cleaned up version of the upstream get_time.win32.c (which re-declared
 * FILETIME/GetSystemTimeAsFileTime and included the non-existent
 * <sys/time.h>). Uses the Win32 API directly; `struct timeval` comes from
 * <winsock2.h>.
 */
#include <winsock2.h>
#include <windows.h>
#include <stdint.h>
#include "pbc_utils.h"

int gettimeofday(struct timeval* p, void* tz)
{
    (void)tz;
    FILETIME ft;
    ULARGE_INTEGER li;
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    /* 100ns ticks since 1601-01-01 -> microseconds since 1970-01-01 */
    li.QuadPart -= 116444736000000000ULL;
    li.QuadPart /= 10;
    p->tv_sec = (long)(li.QuadPart / 1000000);
    p->tv_usec = (long)(li.QuadPart % 1000000);
    return 0;
}

double pbc_get_time(void)
{
    static struct timeval last_tv, tv;
    static int first = 1;
    static double res = 0;

    if (first)
    {
        gettimeofday(&last_tv, NULL);
        first = 0;
        return 0;
    }
    else
    {
        gettimeofday(&tv, NULL);
        res += tv.tv_sec - last_tv.tv_sec;
        res += (tv.tv_usec - last_tv.tv_usec) / 1000000.0;
        last_tv = tv;

        return res;
    }
}
