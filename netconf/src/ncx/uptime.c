#include <time.h>
#include <assert.h>

time_t uptime(time_t *t)
{
    int ret;
    struct timespec tp;
    ret = clock_gettime(CLOCK_MONOTONIC, &tp);
    assert(ret==0);
    *t = tp.tv_sec;
    return *t;
}


time_t reset_time(time_t *t)
{
    int ret;
    struct timespec tp;
    tp.tv_sec=0;
    *t = tp.tv_sec;
    return *t;
}

