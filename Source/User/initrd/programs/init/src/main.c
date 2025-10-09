#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>

int main(void)
{
    pid_t newPid;
    int retVal;
    char timeSec[32];
    struct timespec time;
    struct timespec sleepTime;

    write(6, "Init running\n", 13);
    newPid = fork();

    

    memset(timeSec, 0, 32);
    itoa(newPid, timeSec, 10);
    write(6, "Fork: ", 6);
    write(6, timeSec, strlen(timeSec));
    write(6, "\n", 1);

    while(newPid == 0) 
    {   
        retVal = 5;
    }

    sleepTime.tv_nsec = 500000000;
    sleepTime.tv_sec = 0;
    while(1)
    {
        nanosleep(&sleepTime, NULL);
        memset(timeSec, 0, 32);
        retVal = clock_gettime(CLOCK_MONOTONIC, &time);
        if(retVal == 0)
        {
            uitoa(time.tv_nsec + time.tv_sec * 1000000000, timeSec, 10);
            write(6, timeSec, strlen(timeSec));
            write(6, "\n", 1);
        }
        else
        {
            write(6, ".", 1);
        }
        sleep(1);
        memset(timeSec, 0, 32);
        retVal = clock_gettime(CLOCK_MONOTONIC, &time);
        if(retVal == 0)
        {
            uitoa(time.tv_nsec + time.tv_sec * 1000000000, timeSec, 10);
            write(6, timeSec, strlen(timeSec));
            write(6, "\n", 1);
        }
        else
        {
            write(6, ".", 1);
        }

        retVal = sched_yield();
        write(6, "\n", 1);
    }

    return 0;
}