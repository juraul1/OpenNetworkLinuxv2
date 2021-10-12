#include "lock.h"

#include <fcntl.h>     /* open, fcntl      */
#include <sys/stat.h>  /* S_IRUSR, S_IWUSR */
#include <string.h>    /* strerror         */
#include <errno.h>     /* errno            */
#include <pthread.h> 
#include <stdio.h>     /* SEEK_SET         */
#include <stdint.h>    /* uint8_t          */
#include <time.h>      /* timespec         */
#include <stdatomic.h> /* atomic_int       */

#include <unistd.h>    /* getpid */
#include <sys/types.h> /* getpid */

#include <stdio.h>  /* itoa */
#include <stdlib.h> /* itoa */


#define PTHREAD_MUTEX_DEFAULTS NULL
#define NANOSLEEP_NO_REMAINDER NULL





int shared_lock_init(char const * file, shared_lock_t * restrict lock, uint8_t flags)
{
    int fd = open(file, O_RDWR | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if (!fd) {
        /* Check errno immediately */
        return SLOCK_ERROR_FILE;
    }

    lock->fd = fd;
    lock->recursive = 0;

    lock->flock.l_type   = F_WRLCK;  /* Exclusive lock - read/write  */
    lock->flock.l_whence = SEEK_SET; /* Region relative to beginning */
    lock->flock.l_start  = 0;        /* Beginning of region          */
    lock->flock.l_len    = 0;        /* To EOF                       */


    int rv = 0;

    pthread_mutexattr_t mattr;


    if ((rv = pthread_mutexattr_init(&mattr)) != 0) {
        /* Can return EINVAL or ENOMEM */
        return SLOCK_ERROR_MUTEXATTR_INIT;
    }

    /* Let's allow the mutex to be cleaned up. We're not expecting the owner
       to terminate, but since we control what this is going to be, might as
       well anyway. Default is PTHREAD_MUTEX_STALLED. */
    if ((rv = pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST)) != 0) {
        /* Can return EINVAL */
        return SLOCK_ERROR_MUTEXATTR_SETROBUST;
    }


    if (flags & SLOCK_FLAGS_REENTRANT) {
        if ((rv = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE)) != 0) {
            /* Can return EINVAL */
            return SLOCK_ERROR_MUTEXATTR_SETTYPE;
        }
        lock->recursive = 1;
    }
    else {
        /* Noop for now. We could however set PTHREAD_MUTEX_ERRORCHECK instead
           of having the default of PTHREAD_MUTEX_DEFAULT as it's less likely to
           invoke undefined behaviour. */
    }

    if ((rv = pthread_mutex_init(&lock->mutex, &mattr)) != 0) {
        /* Can return EAGAIN, ENOMEM or EPERM */
        return SLOCK_ERROR_MUTEX_INIT;
    }

    if ((rv = pthread_mutexattr_destroy(&mattr)) != 0) {
        /* Can return EBUSY, EINVAL */
        return SLOCK_ERROR_MUTEXATTR_DESTROY;
    }

    return SLOCK_ERROR_OK;
};





int shared_lock_timedlock(shared_lock_t * restrict lock,
        struct timespec const * restrict spec)
{

    int rv = 0, error = SLOCK_ERROR_DEFAULT;

    if ((rv = pthread_mutex_timedlock(&lock->mutex, spec)) != 0) {
       switch (rv) {
           case ETIMEDOUT: return SLOCK_ERROR_MUTEX_LOCK_TIMEOUT; //printf("timedout!\n"); break;
           case EINVAL:    return SLOCK_ERROR_MUTEX_LOCK_EINVAL; //printf("invalid!\n");  break;
           case EAGAIN:    return SLOCK_ERROR_MUTEX_LOCK_EAGAIN; //printf("again!\n");    break;
           case EDEADLK:   return SLOCK_ERROR_MUTEX_LOCK_EDEADLK; //printf("deadlock!\n"); break;
       }
    }

    struct timespec current_time;

    struct timespec wait_for;
    wait_for.tv_sec = 0;
    wait_for.tv_nsec = 1000000; /* 10 microseconds */ 

    lock->flock.l_type = F_WRLCK;


    for (;;)
    {
        clock_gettime(CLOCK_REALTIME, &current_time);
        if (current_time.tv_sec > spec->tv_sec && 
                current_time.tv_nsec > spec->tv_nsec)
        {
            /* some error */
            error = SLOCK_ERROR_FLOCK_TIMEOUT;
            break;
        }

        if (fcntl(lock->fd, F_SETLK, &lock->flock) == 0) {
            error = SLOCK_ERROR_OK;
            break;
        }

        /* Sets errno to EACCES or EAGAIN */

        if (nanosleep(&wait_for, NANOSLEEP_NO_REMAINDER) == -1) {
            /* Interrupted. errno is set to EINTR */
            error = SLOCK_ERROR_UNLOCK_SLEEP_INTERRUPTED;
            break;
        }
    }


    if (!error) {
        if (lock->recursive) {
            lock->counter++;
        }
    }
    else {
        pthread_mutex_unlock(&lock->mutex);
    }
    
    return error;
}





int shared_lock_unlock(shared_lock_t * restrict lock)
{
    if (lock->counter == 0) {

        lock->flock.l_type = F_UNLCK;
        if (fcntl(lock->fd, F_SETLK, &lock->flock) == -1) {
            /* Sets errno to EACCES or EAGAIN */
            switch (errno) {
                case EACCES: return SLOCK_ERROR_UNLOCK_FILE_ACCESS;
                case EAGAIN: return SLOCK_ERROR_UNLOCK_FILE_TRY_AGAIN;
            }
        }
    }

    if (lock->recursive) {
        lock->counter--;
    }
    
    if (pthread_mutex_unlock(&lock->mutex) == -1) {
        /* Sets errno to EPERM, EINVAL or EAGAIN. But this should never 
           happen. */
        switch (errno) {
            case EPERM:  return SLOCK_ERROR_UNLOCK_MUTEX_PERMISSION;
            case EINVAL: return SLOCK_ERROR_UNLOCK_MUTEX_INVALID;
            case EAGAIN: return SLOCK_ERROR_UNLOCK_MUTEX_TRY_AGAIN;
        }
    }

    return SLOCK_ERROR_OK;
}


int shared_lock_lock(shared_lock_t * restrict lock)
{
    int rv = 0, err = SLOCK_ERROR_OK;
    if ((rv = pthread_mutex_unlock(&lock->mutex)) != 0) {
        switch (rv) {
            case EAGAIN:  return SLOCK_ERROR_MUTEX_LOCK_EAGAIN;
            case EINVAL:  return SLOCK_ERROR_MUTEX_LOCK_EINVAL;
            case EBUSY:   return SLOCK_ERROR_MUTEX_LOCK_EBUSY;
            case EDEADLK: return SLOCK_ERROR_MUTEX_LOCK_EDEADLK;
            case EPERM:   return SLOCK_ERROR_MUTEX_LOCK_EPERM;
        }
    }

    lock->flock.l_type = F_WRLCK;

    if ((rv = fcntl(lock->fd, F_SETLK, &lock->flock)) == -1) {
        switch (rv) {
            case EAGAIN: err = SLOCK_ERROR_MUTEX_LOCK_EAGAIN; break;
            case EACCES: err = SLOCK_ERROR_MUTEX_LOCK_EACCES; break;
        }
    }

    if (err) {
        if (pthread_mutex_unlock(&lock->mutex) == -1) {
            /* Sets errno to EPERM, EINVAL or EAGAIN. But this should never 
            happen. */
            switch (errno) {
                case EPERM:  err = SLOCK_ERROR_UNLOCK_MUTEX_PERMISSION;
                case EINVAL: err = SLOCK_ERROR_UNLOCK_MUTEX_INVALID;
                case EAGAIN: err = SLOCK_ERROR_UNLOCK_MUTEX_TRY_AGAIN;
            }
        }
    }
    return err;
}


char const * shared_lock_strerror(int err)
{
    switch (err) {
        case SLOCK_ERROR_OK:                       return "SLOCK_ERROR_OK";
        case SLOCK_ERROR_FILE:                     return "SLOCK_ERROR_FILE";
        case SLOCK_ERROR_MUTEXATTR_INIT:           return "SLOCK_ERROR_MUTEXATTR_INIT";
        case SLOCK_ERROR_MUTEXATTR_SETROBUST:      return "SLOCK_ERROR_MUTEXATTR_SETROBUST";
        case SLOCK_ERROR_MUTEXATTR_SETTYPE:        return "SLOCK_ERROR_MUTEXATTR_SETTYPE";
        case SLOCK_ERROR_MUTEX_INIT:               return "SLOCK_ERROR_MUTEX_INIT";
        case SLOCK_ERROR_MUTEXATTR_DESTROY:        return "SLOCK_ERROR_MUTEXATTR_DESTROY";
        case SLOCK_ERROR_MUTEX_LOCK_TIMEOUT:       return "SLOCK_ERROR_MUTEX_LOCK_TIMEOUT";
        case SLOCK_ERROR_MUTEX_LOCK_EACCES:        return "SLOCK_ERROR_MUTEX_LOCK_EACCES";
        case SLOCK_ERROR_MUTEX_LOCK_EPERM:         return "SLOCK_ERROR_MUTEX_LOCK_EPERM";
        case SLOCK_ERROR_MUTEX_LOCK_EINVAL:        return "SLOCK_ERROR_MUTEX_LOCK_EINVAL";
        case SLOCK_ERROR_MUTEX_LOCK_EBUSY:         return "SLOCK_ERROR_MUTEX_LOCK_EBUSY";
        case SLOCK_ERROR_MUTEX_LOCK_EAGAIN:        return "SLOCK_ERROR_MUTEX_LOCK_EAGAIN";
        case SLOCK_ERROR_MUTEX_LOCK_EDEADLK:       return "SLOCK_ERROR_MUTEX_LOCK_EDEADLK";
        case SLOCK_ERROR_FLOCK_TIMEOUT:            return "SLOCK_ERROR_FLOCK_TIMEOUT";
        case SLOCK_ERROR_UNLOCK_FILE_ACCESS:       return "SLOCK_ERROR_UNLOCK_FILE_ACCESS";
        case SLOCK_ERROR_UNLOCK_FILE_TRY_AGAIN:    return "SLOCK_ERROR_UNLOCK_FILE_TRY_AGAIN";
        case SLOCK_ERROR_UNLOCK_MUTEX_PERMISSION:  return "SLOCK_ERROR_UNLOCK_MUTEX_PERMISSION";
        case SLOCK_ERROR_UNLOCK_MUTEX_INVALID:     return "SLOCK_ERROR_UNLOCK_MUTEX_INVALID";
        case SLOCK_ERROR_UNLOCK_MUTEX_TRY_AGAIN:   return "SLOCK_ERROR_UNLOCK_MUTEX_TRY_AGAIN";
        case SLOCK_ERROR_UNLOCK_SLEEP_INTERRUPTED: return "SLOCK_ERROR_UNLOCK_SLEEP_INTERRUPTED";
        case SLOCK_ERROR_DEFAULT:                  return "SLOCK_ERROR_DEFAULT";
    }
    return "<UNKNOWN>";
}



shared_lock_t * bf6064_get_lock__()
{
    static shared_lock_t lock;
    return &lock;
}


int bf6064x_lock_init()
{
    shared_lock_t * plock = bf6064_get_lock__();
    memset(plock, 0, sizeof(shared_lock_t));
    int rv = 0;
    if ((rv = shared_lock_init("/tmp/lock", plock, SLOCK_FLAGS_NONE)) 
            != SLOCK_ERROR_OK)
    {
        printf("Error initialising shared lock: %s\n", shared_lock_strerror(rv));
    }
    return rv;
}

int bf6064x_lock_acquire()
{
    shared_lock_t * plock = bf6064_get_lock__();
    int rv = 0;

    if ((rv = shared_lock_lock(plock)) 
            != SLOCK_ERROR_OK)
    {
        printf("Error acquiring shared lock: %s\n", shared_lock_strerror(rv));
    }
    return rv;
}

int bf6064x_lock_release()
{
    shared_lock_t * plock = bf6064_get_lock__();
    int rv = 0;

    if ((rv = shared_lock_unlock(plock)) 
            != SLOCK_ERROR_OK)
    {
        printf("Error releasing shared lock: %s\n", shared_lock_strerror(rv));
    }
    return rv;
}

int bf6064x_lock_close()
{
    return 0;
}