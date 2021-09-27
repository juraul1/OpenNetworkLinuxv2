#pragma once

#define BF6064X_LOCK_FILE "/run/lock/i2c0.lock"

int bf6064x_lock_init();
int bf6064x_lock_acquire();
int bf6064x_lock_release();
int bf6064x_lock_close();