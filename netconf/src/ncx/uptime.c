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

