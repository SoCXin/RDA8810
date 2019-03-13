/*
 * Copyright (c) 2014 Rdamicro Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _linuxver_h_
#define _linuxver_h_

#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
#include <linux/config.h>
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33))
#include <generated/autoconf.h>
#else
#include <linux/autoconf.h>
#endif
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)) */
#include <linux/module.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 0))
/* __NO_VERSION__ must be defined for all linkables except one in 2.2 */
#ifdef __UNDEF_NO_VERSION__
#undef __NO_VERSION__
#else
#define __NO_VERSION__
#endif
#endif	/* LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
#define module_param(_name_, _type_, _perm_)	                MODULE_PARM(_name_, "i")
#define module_param_string(_name_, _string_, _size_, _perm_) 	MODULE_PARM(_string_, "c" __MODULE_STRING(_size_))
#endif

/* linux/malloc.h is deprecated, use linux/slab.h instead. */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 9))
#include <linux/malloc.h>
#else
#include <linux/slab.h>
#endif

#include <linux/types.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)) */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28))
#undef IP_TOS
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)) */
#include <asm/io.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 5, 41))
#include <linux/workqueue.h>
#else
#include <linux/tqueue.h>
#ifndef work_struct
#define work_struct                 tq_struct
#endif
#ifndef INIT_WORK
#define INIT_WORK(_work, _func, _data)  INIT_TQUEUE((_work), (_func), (_data))
#endif
#ifndef schedule_work
#define schedule_work(_work)        schedule_task((_work))
#endif
#ifndef flush_scheduled_work
#define flush_scheduled_work()      flush_scheduled_tasks()
#endif
#endif	/* LINUX_VERSION_CODE > KERNEL_VERSION(2, 5, 41) */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#define DAEMONIZE(a)                daemonize(a); \
                    	            allow_signal(SIGKILL); \
                    	            allow_signal(SIGTERM);
#else /* Linux 2.4 (w/o preemption patch) */
#define RAISE_RX_SOFTIRQ() 	        cpu_raise_softirq(smp_processor_id(), NET_RX_SOFTIRQ)
#define DAEMONIZE(a)                daemonize(); \
                                	do { if (a) \
                                		strncpy(current->comm, a, MIN(sizeof(current->comm), (strlen(a)))); \
                                	} while (0);
#endif /* LINUX_VERSION_CODE  */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 17)
#ifdef	CONFIG_NET_RADIO
#define	CONFIG_WIRELESS_EXT
#endif
#endif	/* < 2.6.17 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 32)
#include <linux/sched.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
#include <net/lib80211.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
#include <linux/ieee80211.h>
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
#include <net/ieee80211.h>
#endif
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30) */


#ifndef __exit
#define __exit
#endif
#ifndef __devexit
#define __devexit
#endif
#ifndef __devinit
#define __devinit	__init
#endif
#ifndef __devinitdata
#define __devinitdata
#endif
#ifndef __devexit_p
#define __devexit_p(x)	x
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 2, 18))
#ifdef MODULE
#define module_init(x) 	int  init_module(void) { return x(); }
#define module_exit(x) 	void cleanup_module(void) { x(); }
#else
#define module_init(x)	__initcall(x);
#define module_exit(x)	__exitcall(x);
#endif
#endif	/* LINUX_VERSION_CODE < KERNEL_VERSION(2, 2, 18) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 48))
#define list_for_each(pos, head) 	    for (pos = (head)->next; pos != (head); pos = pos->next)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 14))
#define net_device                      device
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 43))
#define dev_kfree_skb_any(a)		    dev_kfree_skb(a)
#define netif_down(dev)			        do { (dev)->start = 0; } while (0)

/* pcmcia-cs provides its own netdevice compatibility layer */
#ifndef _COMPAT_NETDEVICE_H
/*
 * SoftNet
 *
 * For pre-softnet kernels we need to tell the upper layer not to
 * re-enter start_xmit() while we are in there. However softnet
 * guarantees not to enter while we are in there so there is no need
 * to do the netif_stop_queue() dance unless the transmit queue really
 * gets stuck. This should also improve performance according to tests
 * done by Aman Singla.
 */

#define dev_kfree_skb_irq(a)	        dev_kfree_skb(a)
#define netif_wake_queue(dev) 	        do { clear_bit(0, &(dev)->tbusy); mark_bh(NET_BH); } while (0)
#define netif_stop_queue(dev)	        set_bit(0, &(dev)->tbusy)

static inline void netif_start_queue(struct net_device *dev)
{
	dev->tbusy      = 0;
	dev->interrupt  = 0;
	dev->start      = 1;
}

#define netif_queue_stopped(dev)	    (dev)->tbusy
#define netif_running(dev)		        (dev)->start

#endif /* _COMPAT_NETDEVICE_H */

/* 2.4.x renamed bottom halves to tasklets */
#define tasklet_struct				    tq_struct

static inline void tasklet_schedule(struct tasklet_struct *tasklet)
{
	queue_task(tasklet, &tq_immediate);
	mark_bh(IMMEDIATE_BH);
}

static inline void tasklet_init(struct tasklet_struct *tasklet, void (*func)(unsigned long), unsigned long data)
{
	tasklet->next    = NULL;
	tasklet->sync    = 0;
	tasklet->routine = (void (*)(void *))func;
	tasklet->data    = (void *)data;
}
#define tasklet_kill(tasklet)	        { do {} while (0); }

/* 2.4.x introduced del_timer_sync() */
#define del_timer_sync(timer)           del_timer(timer)
#else
#define netif_down(dev)
#endif /* SoftNet */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 3))
/* Emit code to initialise a tq_struct's routine and data pointers */
#define PREPARE_TQUEUE(_tq, _routine, _data)	\
	do {							            \
		(_tq)->routine = _routine;			    \
		(_tq)->data    = _data;				    \
	} while (0)

/* Emit code to initialise all of a tq_struct */
#define INIT_TQUEUE(_tq, _routine, _data)		\
	do {							            \
		INIT_LIST_HEAD(&(_tq)->list);			\
		(_tq)->sync = 0;				        \
		PREPARE_TQUEUE((_tq), (_routine), (_data));	\
	} while (0)
#endif	/* LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 3) */


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
#define CHECKSUM_HW	                CHECKSUM_PARTIAL
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19) */

struct tsk_ctl{
	void 	            *parent;  /* some external entity that the thread supposed to work for */
	struct	task_struct *p_task;
	long 	             thr_pid;
	int 	             prio; /* priority */
	struct	semaphore    sema;
	int	                 terminated;
	struct	completion   completed;
};


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#define SMP_RD_BARRIER_DEPENDS(x)   smp_read_barrier_depends(x)
#else /*LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)*/
#define SMP_RD_BARRIER_DEPENDS(x)   smp_rmb(x)
#endif/*LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)*/

#define PROC_START(thread_func, owner, tsk_ctl, flags) \
{ \
	sema_init(&((tsk_ctl)->sema), 0); \
	init_completion(&((tsk_ctl)->completed)); \
	(tsk_ctl)->parent     = owner; \
	(tsk_ctl)->terminated = false; \
	(tsk_ctl)->thr_pid = kernel_thread(thread_func, tsk_ctl, flags); \
	WLAND_DBG(DEFAULT, TRACE,"thr:%lx created\n", (tsk_ctl)->thr_pid); \
	if ((tsk_ctl)->thr_pid > 0) \
		wait_for_completion(&((tsk_ctl)->completed)); \
	WLAND_DBG(DEFAULT, TRACE,"thr:%lx started\n", (tsk_ctl)->thr_pid); \
}

#define PROC_STOP(tsk_ctl) \
{ \
	(tsk_ctl)->terminated = true; \
	smp_wmb(); \
	up(&((tsk_ctl)->sema));	\
	wait_for_completion(&((tsk_ctl)->completed)); \
	WLAND_DBG(DEFAULT, TRACE,"thr:%lx terminated OK\n", (tsk_ctl)->thr_pid); \
	(tsk_ctl)->thr_pid = -1; \
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31))
#define KILL_PROC(nr, sig) \
{ \
    struct task_struct *tsk; \
    struct pid *pid;                 \
    pid = find_get_pid((pid_t)nr);   \
    tsk = pid_task(pid, PIDTYPE_PID);\
    if (tsk) send_sig(sig, tsk, 1);  \
}
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31) */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)) && (LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 30))
#define KILL_PROC(pid, sig) \
{ \
	struct task_struct *tsk; \
	tsk = find_task_by_vpid(pid); \
	if (tsk) send_sig(sig, tsk, 1); \
}
#else
#define KILL_PROC(pid, sig) \
{ \
	kill_proc(pid, sig, 1); \
}
#endif
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31) */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#include <linux/time.h>
#include <linux/wait.h>
#else
#include <linux/sched.h>

#define __wait_event_interruptible_timeout(wq, condition, ret)		\
do {									\
	wait_queue_t __wait;						\
	init_waitqueue_entry(&__wait, current);				\
									\
	add_wait_queue(&wq, &__wait);					\
	for (;;) {							\
		set_current_state(TASK_INTERRUPTIBLE);			\
		if (condition)						\
			break;						\
		if (!signal_pending(current)) {				\
			ret = schedule_timeout(ret);			\
			if (!ret)					\
				break;					\
			continue;					\
		}							\
		ret = -ERESTARTSYS;					\
		break;							\
	}								\
	current->state = TASK_RUNNING;					\
	remove_wait_queue(&wq, &__wait);				\
} while (0)

#define wait_event_interruptible_timeout(wq, condition, timeout)	\
({									\
	long __ret = timeout;						\
	if (!(condition))						\
		__wait_event_interruptible_timeout(wq, condition, __ret); \
	__ret;								\
})
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)) */


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
#define netdev_priv(dev)            dev->priv
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)) */

/* Detect compiler type. */
#if defined(__GNUC__) || defined(__lint)
	#define GNU_COMPILER
#elif defined(__CC_ARM) && __CC_ARM
	#define ARMCC_COMPILER
#else
	#error "Unknown compiler!"
#endif 

/* Declare compiler-specific directives for structure packing. */
#if defined(__GNUC__) || defined(__lint)
	#define	PRE_PACKED
	#define	POST_PACKED	__attribute__ ((packed))
#elif defined(__CC_ARM)
	#define	PRE_PACKED	__packed
	#define	POST_PACKED
#else
	#error "Unknown Compiler!"
#endif

#ifndef INLINE
	#if defined(MICROSOFT_COMPILER)
		#define INLINE __inline
	#elif defined(GNU_COMPILER)
		#define INLINE __inline__
	#elif defined(ARMCC_COMPILER)
		#define INLINE	__inline
	#else
		#define INLINE
	#endif 
#endif /* INLINE */

#endif /* _linuxver_h_ */
