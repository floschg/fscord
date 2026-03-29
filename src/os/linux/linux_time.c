#define _POSIX_C_SOURCE 200809L

#include <os/os.h>

#include <string.h>
#include <time.h>

OSTime
os_time_get_now(void)
{
    struct timespec ts;
    memset(&ts, 0, sizeof(ts));
    i32 err = clock_gettime(CLOCK_REALTIME, &ts);
    if (err != 0) {
        printf("failed to get time\n");
    }

    OSTime result;
    result.seconds = ts.tv_sec;
    result.nanoseconds = ts.tv_nsec;
    return result;
}

