#ifndef __ICC_SPINLOCK_H__
#define __ICC_SPINLOCK_H__

#include <linux/types.h>

#define ICC_SPINLOCK_LOCK       1
#define ICC_SPINLOCK_UNLOCK     0

typedef uint32_t shm_lock;

void icc_shm_lock_init(shm_lock *plock);
void icc_shm_acquire_lock(shm_lock *plock);
void icc_shm_release_lock(shm_lock *plock);

#endif
