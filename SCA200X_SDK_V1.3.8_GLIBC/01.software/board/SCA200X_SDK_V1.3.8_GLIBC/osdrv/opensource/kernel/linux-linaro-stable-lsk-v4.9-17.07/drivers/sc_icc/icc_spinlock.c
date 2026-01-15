/*
 * icc spin lock
 */
#include <linux/preempt.h>
#include <linux/irqflags.h>

#include "icc_spinlock.h"

void icc_shm_lock_init(shm_lock *plock)
{
	*plock = ICC_SPINLOCK_UNLOCK;
}
#if 0
func spin_lock
mov     w2, #1
l2:
ldaxr   w1, [x0]
cbnz    w1, l2
stxr    w1, w2, [x0]
cbnz    w1, l2
ret
endfunc spin_lock

func spin_unlock
stlr    wzr, [x0]
ret
endfunc spin_unlock
#endif
void icc_shm_acquire_lock(shm_lock *plock)
{
	uint32_t tmp;
	local_irq_disable();
	asm volatile (
	    "1: ldaxr %w0, %1\n\t"
	    "cbnz %w0, 1b\n\t"
	    "stxr %w0, %w2, %1\n\t"
	    "cbnz %w0, 1b\n\t"
	    :"=&r" (tmp), "+Q" (*plock)
	    :"r"(ICC_SPINLOCK_LOCK)
	    :"memory"
	);
	asm volatile ("dsb sy" : : : "memory");
	asm volatile ("isb sy" : : : "memory");
}

void icc_shm_release_lock(shm_lock *plock)
{
	asm volatile (
	    "stlr %w1, %0\n\t"
	    :"+Q" (*plock)
	    :"r"(ICC_SPINLOCK_UNLOCK)
	    :"memory"
	);
	asm volatile ("dsb sy" : : : "memory");
	asm volatile ("isb sy" : : : "memory");
	local_irq_enable();
}
#if 0
void icc_shm_acquire_lock(shm_lock *plock)
{
	uint32_t tmp;
	local_irq_disable();
	asm volatile (
	    "1: ldrex %0, [%1]\n\t"
	    "teq %0, #0\n"
	    "strexeq %0, %2, [%1]\n\t"
	    "teqeq %0, #0\n\t"
	    "bne 1b\n\t"
	    "dsb sy\n\t"
	    :"=&r" (tmp)
	    :"r" (plock), "r"(ICC_SPINLOCK_LOCK)
	    :"cc"
	);
	asm volatile ( "DSB" );
	asm volatile ( "ISB" );
}

void icc_shm_release_lock(shm_lock *plock)
{
	asm volatile (
	    "str %1, [%0]\n\t"
	    "dsb sy\n\t"
	    :
	    :"r" (plock), "r"(ICC_SPINLOCK_UNLOCK)
	    :"cc"
	);
	asm volatile ( "DSB" );
	asm volatile ( "ISB" );
	local_irq_enable();

}
#endif
