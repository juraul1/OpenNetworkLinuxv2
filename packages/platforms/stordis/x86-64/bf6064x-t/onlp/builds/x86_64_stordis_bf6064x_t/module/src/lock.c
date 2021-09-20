#include "lock.h"

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

int lock_fd;

int bf6064x_lock_init()
{
    lock_fd = open( BF6064X_LOCK_FILE, O_RDWR | O_CREAT | O_CLOEXEC, 
            S_IRUSR | S_IWUSR);
    int err = errno;
    if (!lock_fd) {
        printf("Error opening lock file: %s\n", strerror(err));
    }
    return err;
}

int bf6064x_lock_acquire()
{
    off_t file_start = 0;
    int rv = lockf(lock_fd, F_LOCK, file_start);
    if (rv != 0) {
        printf("Error opening acquiring lock: %s\n", strerror(errno));
    }
    return rv;
}

int bf6064x_lock_release()
{
    off_t file_start = 0;
    int rv = lockf(lock_fd, F_ULOCK, file_start);
    if (rv != 0) {
        printf("Error opening acquiring lock: %s\n", strerror(errno));
    }
    return rv;
}

int bf6064x_lock_close()
{
    int rv = close(lock_fd);
    if (rv != 0) {
        printf("Error closing lock: %s\n", strerror(errno));
    }
    return rv;
}