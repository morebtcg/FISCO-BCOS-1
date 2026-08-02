/*
 * pthread.h - minimal Windows shim implementing just the mutex API used by
 * group-signature-lib (CommonStruct.h). Implemented as an interlocked
 * spinlock over a plain struct (copyable, like the real pthread_mutex_t the
 * upstream code assumes), and it avoids pulling in <windows.h> so no macro
 * pollution leaks into the rest of the translation units.
 */
#ifndef _WIN32_PTHREAD_STUB_H
#define _WIN32_PTHREAD_STUB_H

#ifdef __cplusplus
#include <intrin.h>

typedef struct
{
    volatile long flag;
} pthread_mutex_t;

inline int pthread_mutex_init(pthread_mutex_t* m, void*)
{
    m->flag = 0;
    return 0;
}

inline int pthread_mutex_destroy(pthread_mutex_t*)
{
    return 0;
}

inline int pthread_mutex_lock(pthread_mutex_t* m)
{
    while (_InterlockedExchange(&m->flag, 1) != 0)
    {
        /* spin while another thread holds the lock */
        while (m->flag)
        {
        }
    }
    return 0;
}

inline int pthread_mutex_unlock(pthread_mutex_t* m)
{
    _InterlockedExchange(&m->flag, 0);
    return 0;
}
#else
#error "The win32 pthread shim requires a C++ compiler"
#endif

#endif /* _WIN32_PTHREAD_STUB_H */
