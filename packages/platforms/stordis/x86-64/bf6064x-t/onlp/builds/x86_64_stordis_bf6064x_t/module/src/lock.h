#pragma once

#include <stdint.h>
#include <time.h>

#include <pthread.h> 
#include <fcntl.h>     /* open, fcntl      */
#include <stdatomic.h> /* atomic_int       */


#define SLOCK_FLAGS_NONE      0x00
#define SLOCK_FLAGS_REENTRANT 0x01


typedef struct flock flock_t;


/*  Counter needed even if mutex is re-entrant, we need to know when
    the final lock has been released */

struct shared_lock_s
{
    int             fd;
    flock_t         flock;
    pthread_mutex_t mutex;
    atomic_int      counter;
    int             recursive;
};
typedef struct shared_lock_s shared_lock_t;



enum slock_error_e
{
    SLOCK_ERROR_OK = 0,
    SLOCK_ERROR_FILE,           /* Check errno */
    SLOCK_ERROR_MUTEXATTR_INIT,
    SLOCK_ERROR_MUTEXATTR_SETROBUST,
    SLOCK_ERROR_MUTEXATTR_SETTYPE,
    SLOCK_ERROR_MUTEX_INIT,
    SLOCK_ERROR_MUTEXATTR_DESTROY,
    SLOCK_ERROR_MUTEX_LOCK_TIMEOUT,
    SLOCK_ERROR_MUTEX_LOCK_EINVAL,
    SLOCK_ERROR_MUTEX_LOCK_EACCES,
    SLOCK_ERROR_MUTEX_LOCK_EAGAIN,
    SLOCK_ERROR_MUTEX_LOCK_EBUSY,
    SLOCK_ERROR_MUTEX_LOCK_EPERM,
    SLOCK_ERROR_MUTEX_LOCK_EDEADLK,
    SLOCK_ERROR_MUTEX_LOCK_EOWNERDEAD,
    SLOCK_ERROR_MUTEX_LOCK_NOT_ROBUST_OR_INCONSISTENT,
    SLOCK_ERROR_MUTEX_LOCK_UNKNOWN,
    SLOCK_ERROR_FLOCK_TIMEOUT,
    SLOCK_ERROR_UNLOCK_FILE_ACCESS,
    SLOCK_ERROR_UNLOCK_FILE_TRY_AGAIN,
    SLOCK_ERROR_UNLOCK_MUTEX_PERMISSION,
    SLOCK_ERROR_UNLOCK_MUTEX_INVALID,
    SLOCK_ERROR_UNLOCK_MUTEX_TRY_AGAIN,
    SLOCK_ERROR_UNLOCK_SLEEP_INTERRUPTED,
    SLOCK_ERROR_DEFAULT = 255
};


int shared_lock_init(char const * file, shared_lock_t * restrict lock,
        uint8_t flags);

int shared_lock_timedlock(shared_lock_t * restrict lock,
        struct timespec const * restrict spec);

int shared_lock_unlock(shared_lock_t * restrict lock);

/* Not implemented */
int shared_lock_lock(shared_lock_t * restrict lock);

/* Not implemented */
int shared_lock_trylock(shared_lock_t * restrict lock);


char const * shared_lock_strerror(int err);



#define BF6064X_LOCK_FILE "/run/lock/i2c0.lock"

int bf6064x_lock_init();
int bf6064x_lock_acquire();
int bf6064x_lock_release();
int bf6064x_lock_close();
