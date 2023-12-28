/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-03-28     Bernard      first version
 * 2006-04-29     Bernard      implement thread timer
 * 2006-04-30     Bernard      added THREAD_DEBUG
 * 2006-05-27     Bernard      fixed the rt_thread_yield bug
 * 2006-06-03     Bernard      fixed the thread timer init bug
 * 2006-08-10     Bernard      fixed the timer bug in thread_sleep
 * 2006-09-03     Bernard      changed rt_timer_delete to rt_timer_detach
 * 2006-09-03     Bernard      implement rt_thread_detach
 * 2008-02-16     Bernard      fixed the rt_thread_timeout bug
 * 2010-03-21     Bernard      change the errno of rt_thread_delay/sleep to
 *                             RT_EOK.
 * 2010-11-10     Bernard      add cleanup callback function in thread exit.
 * 2011-09-01     Bernard      fixed rt_thread_exit issue when the current
 *                             thread preempted, which reported by Jiaxing Lee.
 * 2011-09-08     Bernard      fixed the scheduling issue in rt_thread_startup.
 * 2012-12-29     Bernard      fixed compiling warning.
 * 2016-08-09     ArdaFu       add thread suspend and resume hook.
 * 2017-04-10     armink       fixed the rt_thread_delete and rt_thread_detach
 *                             bug when thread has not startup.
 * 2018-11-22     Jesven       yield is same to rt_schedule
 *                             add support for tasks bound to cpu
 */

#include <rthw.h>
#include <rtthread.h>

#ifdef RT_USING_HOOK
static void (*rt_thread_suspend_hook)(rt_thread_t thread);
static void (*rt_thread_resume_hook) (rt_thread_t thread);
static void (*rt_thread_inited_hook) (rt_thread_t thread);

/**
 * @ingroup Hook
 * This function sets a hook function when the system suspend a thread.
 *
 * @param hook the specified hook function
 *
 * @note the hook function must be simple and never be blocked or suspend.
 */
void rt_thread_suspend_sethook(void (*hook)(rt_thread_t thread))
{
    rt_thread_suspend_hook = hook;
}

/**
 * @ingroup Hook
 * This function sets a hook function when the system resume a thread.
 *
 * @param hook the specified hook function
 *
 * @note the hook function must be simple and never be blocked or suspend.
 */
void rt_thread_resume_sethook(void (*hook)(rt_thread_t thread))
{
    rt_thread_resume_hook = hook;
}

/**
 * @ingroup Hook
 * This function sets a hook function when a thread is initialized.
 *
 * @param hook the specified hook function
 */
void rt_thread_inited_sethook(void (*hook)(rt_thread_t thread))
{
    rt_thread_inited_hook = hook;
}

#endif

void rt_thread_exit(void)
{
    struct rt_thread *thread;
    register rt_base_t level;

    /* get current thread */
    thread = rt_thread_self();

    /* disable interrupt */
    level = rt_hw_interrupt_disable();

    /* remove from schedule */
    rt_schedule_remove_thread(thread);
    /* change stat */
    thread->stat = RT_THREAD_CLOSE;

    /* remove it from timer list */
    rt_timer_detach(&thread->thread_timer);

    if ((rt_object_is_systemobject((rt_object_t)thread) == RT_TRUE) &&
        thread->cleanup == RT_NULL)
    {
        rt_object_detach((rt_object_t)thread);
    }
    else
    {
        /* insert to defunct thread list */
        rt_thread_defunct_enqueue(thread);
    }

    /* switch to next task */
    rt_schedule();

    /* enable interrupt */
    rt_hw_interrupt_enable(level);
}

static rt_err_t _rt_thread_init(struct rt_thread *thread,
                                const char       *name,
                                void (*entry)(void *parameter),
                                void             *parameter,
                                void             *stack_start,
                                rt_uint32_t       stack_size,
                                rt_uint8_t        priority,
                                rt_uint32_t       tick)
{
    /* init thread list */
    rt_list_init(&(thread->tlist));

#ifdef RT_USING_LWP
    thread->wakeup.func = RT_NULL;
#endif

    thread->entry = (void *)entry;
    thread->parameter = parameter;

    /* stack init */
    thread->stack_addr = stack_start;
    thread->stack_size = stack_size;

    /* init thread stack */
    rt_memset(thread->stack_addr, '#', thread->stack_size);
#ifdef ARCH_CPU_STACK_GROWS_UPWARD
    thread->sp = (void *)rt_hw_stack_init(thread->entry, thread->parameter,
                                          (void *)((char *)thread->stack_addr),
                                          (void *)rt_thread_exit);
#else
    thread->sp = (void *)rt_hw_stack_init(thread->entry, thread->parameter,
                                          (rt_uint8_t *)((char *)thread->stack_addr + thread->stack_size - sizeof(rt_ubase_t)),
                                          (void *)rt_thread_exit);
#endif

    /* priority init */
    RT_ASSERT(priority < RT_THREAD_PRIORITY_MAX);
    thread->init_priority    = priority;
    thread->current_priority = priority;

    thread->number_mask = 0;

#ifdef RT_USING_MUTEX
    rt_list_init(&thread->taken_object_list);
    thread->pending_object = RT_NULL;
#endif

#if RT_THREAD_PRIORITY_MAX > 32
    thread->number = 0;
    thread->high_mask = 0;
#endif

    /* tick init */
    thread->init_tick      = tick;
    thread->remaining_tick = tick;

    /* error and flags */
    thread->error = RT_EOK;
    thread->stat  = RT_THREAD_INIT;

#ifdef RT_USING_SMP
    /* not bind on any cpu */
    thread->bind_cpu = RT_CPUS_NR;
    thread->oncpu = RT_CPU_DETACHED;

    /* lock init */
    thread->scheduler_lock_nest = 0;
    thread->cpus_lock_nest = 0;
    thread->critical_lock_nest = 0;
#endif /*RT_USING_SMP*/

    /* initialize cleanup function and user data */
    thread->cleanup   = 0;
    thread->user_data = 0;

    /* initialize thread timer */
    rt_timer_init(&(thread->thread_timer),
                  thread->name,
                  rt_thread_timeout,
                  thread,
                  0,
                  RT_TIMER_FLAG_ONE_SHOT);

    /* initialize signal */
#ifdef RT_USING_SIGNALS
    thread->sig_mask    = 0x00;
    thread->sig_pending = 0x00;

#ifndef RT_USING_SMP
    thread->sig_ret     = RT_NULL;
#endif
    thread->sig_vectors = RT_NULL;
    thread->si_list     = RT_NULL;
#endif

#ifdef RT_USING_LWP
    thread->lwp = RT_NULL;
    rt_list_init(&(thread->sibling));
    rt_memset(&thread->signal, 0, sizeof(lwp_sigset_t));
    rt_memset(&thread->signal_mask, 0, sizeof(lwp_sigset_t));
    thread->signal_mask_bak = 0;
    thread->signal_in_process = 0;
    rt_memset(&thread->user_ctx, 0, sizeof thread->user_ctx);
#endif

    RT_OBJECT_HOOK_CALL(rt_thread_inited_hook, (thread));

    return RT_EOK;
}

/**
 * @addtogroup Thread
 */

/**@{*/

/**
 * This function will initialize a thread, normally it's used to initialize a
 * static thread object.
 *
 * @param thread the static thread object
 * @param name the name of thread, which shall be unique
 * @param entry the entry function of thread
 * @param parameter the parameter of thread enter function
 * @param stack_start the start address of thread stack
 * @param stack_size the size of thread stack
 * @param priority the priority of thread
 * @param tick the time slice if there are same priority thread
 *
 * @return the operation status, RT_EOK on OK, -RT_ERROR on error
 */
rt_err_t rt_thread_init(struct rt_thread *thread,
                        const char       *name,
                        void (*entry)(void *parameter),
                        void             *parameter,
                        void             *stack_start,
                        rt_uint32_t       stack_size,
                        rt_uint8_t        priority,
                        rt_uint32_t       tick)
{
    /* thread check */
    RT_ASSERT(thread != RT_NULL);
    RT_ASSERT(stack_start != RT_NULL);

    /* initialize thread object */
    rt_object_init((rt_object_t)thread, RT_Object_Class_Thread, name);

    return _rt_thread_init(thread,
                           name,
                           entry,
                           parameter,
                           stack_start,
                           stack_size,
                           priority,
                           tick);
}
RTM_EXPORT(rt_thread_init);

/**
 * This function will return self thread object
 *
 * @return the self thread object
 */
rt_thread_t rt_thread_self(void)
{
#ifdef RT_USING_SMP
    rt_base_t lock;
    rt_thread_t self;

    lock = rt_hw_local_irq_disable();
    self = rt_cpu_self()->current_thread;
    rt_hw_local_irq_enable(lock);
    return self;
#else
    extern rt_thread_t rt_current_thread;

    return rt_current_thread;
#endif
}
RTM_EXPORT(rt_thread_self);

/**
 * This function will start a thread and put it to system ready queue
 *
 * @param thread the thread to be started
 *
 * @return the operation status, RT_EOK on OK, -RT_ERROR on error
 */
rt_err_t rt_thread_startup(rt_thread_t thread)
{
    /* thread check */
    RT_ASSERT(thread != RT_NULL);
    RT_ASSERT((thread->stat & RT_THREAD_STAT_MASK) == RT_THREAD_INIT);
    RT_ASSERT(rt_object_get_type((rt_object_t)thread) == RT_Object_Class_Thread);

    /* set current priority to initialize priority */
    thread->current_priority = thread->init_priority;

    /* calculate priority attribute */
#if RT_THREAD_PRIORITY_MAX > 32
    thread->number      = thread->current_priority >> 3;            /* 5bit */
    thread->number_mask = 1L << thread->number;
    thread->high_mask   = 1L << (thread->current_priority & 0x07);  /* 3bit */
#else
    thread->number_mask = 1L << thread->current_priority;
#endif

    RT_DEBUG_LOG(RT_DEBUG_THREAD, ("startup a thread:%s with priority:%d\n",
                                   thread->name, thread->init_priority));
    /* change thread stat */
    thread->stat = RT_THREAD_SUSPEND;
    /* then resume it */
    rt_thread_resume(thread);
    if (rt_thread_self() != RT_NULL)
    {
        /* do a scheduling */
        rt_schedule();
    }

    return RT_EOK;
}
RTM_EXPORT(rt_thread_startup);

/**
 * This function will detach a thread. The thread object will be removed from
 * thread queue and detached/deleted from system object management.
 *
 * @param thread the thread to be deleted
 *
 * @return the operation status, RT_EOK on OK, -RT_ERROR on error
 */
rt_err_t rt_thread_detach(rt_thread_t thread)
{
    rt_base_t lock;

    /* thread check */
    RT_ASSERT(thread != RT_NULL);
    RT_ASSERT(rt_object_get_type((rt_object_t)thread) == RT_Object_Class_Thread);
    RT_ASSERT(rt_object_is_systemobject((rt_object_t)thread));

    if ((thread->stat & RT_THREAD_STAT_MASK) == RT_THREAD_CLOSE)
        return RT_EOK;

    if ((thread->stat & RT_THREAD_STAT_MASK) != RT_THREAD_INIT)
    {
        /* remove from schedule */
        rt_schedule_remove_thread(thread);
    }

    /* release thread timer */
    rt_timer_detach(&(thread->thread_timer));

    /* change stat */
    thread->stat = RT_THREAD_CLOSE;

#ifdef RT_USING_MUTEX
    if ((thread->pending_object) &&
        (rt_object_get_type(thread->pending_object) == RT_Object_Class_Mutex))
    {
        struct rt_mutex *mutex = (struct rt_mutex*)thread->pending_object;
        rt_mutex_drop_thread(mutex, thread);
        thread->pending_object = RT_NULL;
    }
#endif

    if ((rt_object_is_systemobject((rt_object_t)thread) == RT_TRUE) &&
        thread->cleanup == RT_NULL)
    {
        rt_object_detach((rt_object_t)thread);
    }
    else
    {
        /* disable interrupt */
        lock = rt_hw_interrupt_disable();
        /* insert to defunct thread list */
        rt_thread_defunct_enqueue(thread);
        /* enable interrupt */
        rt_hw_interrupt_enable(lock);
    }

    return RT_EOK;
}
RTM_EXPORT(rt_thread_detach);

#ifdef RT_USING_HEAP
/**
 * This function will create a thread object and allocate thread object memory
 * and stack.
 *
 * @param name the name of thread, which shall be unique
 * @param entry the entry function of thread
 * @param parameter the parameter of thread enter function
 * @param stack_size the size of thread stack
 * @param priority the priority of thread
 * @param tick the time slice if there are same priority thread
 *
 * @return the created thread object
 */
rt_thread_t rt_thread_create(const char *name,
                             void (*entry)(void *parameter),
                             void       *parameter,
                             rt_uint32_t stack_size,
                             rt_uint8_t  priority,
                             rt_uint32_t tick)
{
    struct rt_thread *thread;
    void *stack_start;

    thread = (struct rt_thread *)rt_object_allocate(RT_Object_Class_Thread,
                                                    name);
    if (thread == RT_NULL)
        return RT_NULL;

    stack_start = (void *)RT_KERNEL_MALLOC(stack_size);
    if (stack_start == RT_NULL)
    {
        /* allocate stack failure */
        rt_object_delete((rt_object_t)thread);

        return RT_NULL;
    }

    _rt_thread_init(thread,
                    name,
                    entry,
                    parameter,
                    stack_start,
                    stack_size,
                    priority,
                    tick);

    return thread;
}
RTM_EXPORT(rt_thread_create);

/**
 * This function will delete a thread. The thread object will be removed from
 * thread queue and deleted from system object management in the idle thread.
 *
 * @param thread the thread to be deleted
 *
 * @return the operation status, RT_EOK on OK, -RT_ERROR on error
 */
rt_err_t rt_thread_delete(rt_thread_t thread)
{
    rt_base_t lock;

    /* thread check */
    RT_ASSERT(thread != RT_NULL);
    RT_ASSERT(rt_object_get_type((rt_object_t)thread) == RT_Object_Class_Thread);
    RT_ASSERT(rt_object_is_systemobject((rt_object_t)thread) == RT_FALSE);

    if ((thread->stat & RT_THREAD_STAT_MASK) == RT_THREAD_CLOSE)
        return RT_EOK;

    if ((thread->stat & RT_THREAD_STAT_MASK) != RT_THREAD_INIT)
    {
        /* remove from schedule */
        rt_schedule_remove_thread(thread);
    }

    /* disable interrupt */
    lock = rt_hw_interrupt_disable();

    /* release thread timer */
    rt_timer_detach(&(thread->thread_timer));

    /* change stat */
    thread->stat = RT_THREAD_CLOSE;

#ifdef RT_USING_MUTEX
    if ((thread->pending_object) &&
        (rt_object_get_type(thread->pending_object) == RT_Object_Class_Mutex))
    {
        struct rt_mutex *mutex = (struct rt_mutex*)thread->pending_object;
        rt_mutex_drop_thread(mutex, thread);
        thread->pending_object = RT_NULL;
    }
#endif

    /* insert to defunct thread list */
    rt_thread_defunct_enqueue(thread);

    /* enable interrupt */
    rt_hw_interrupt_enable(lock);

    return RT_EOK;
}
RTM_EXPORT(rt_thread_delete);
#endif

/**
 * This function will let current thread yield processor, and scheduler will
 * choose a highest thread to run. After yield processor, the current thread
 * is still in READY state.
 *
 * @return RT_EOK
 */
rt_err_t rt_thread_yield(void)
{
    rt_base_t level;
    struct rt_thread *thread;

    thread = rt_thread_self();

    level = rt_hw_interrupt_disable();

    thread->remaining_tick = thread->init_tick;
    thread->stat |= RT_THREAD_STAT_YIELD;

    rt_schedule();
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}
RTM_EXPORT(rt_thread_yield);

/**
 * This function will let current thread sleep for some ticks.
 *
 * @param tick the sleep ticks
 *
 * @return RT_EOK on OK, -RT_ERROR on error
 */
rt_err_t rt_thread_sleep(rt_tick_t tick)
{
    register rt_base_t temp;
    struct rt_thread *thread;
    int err;

    /* set to current thread */
    thread = rt_thread_self();
    RT_ASSERT(thread != RT_NULL);
    RT_ASSERT(rt_object_get_type((rt_object_t)thread) == RT_Object_Class_Thread);

    /* disable interrupt */
    temp = rt_hw_interrupt_disable();

    /* suspend thread */
    err = rt_thread_suspend_with_flag(thread, RT_INTERRUPTIBLE);

    /* reset the timeout of thread timer and start it */
    if (err == RT_EOK)
    {
        rt_timer_control(&(thread->thread_timer), RT_TIMER_CTRL_SET_TIME, &tick);
        rt_timer_start(&(thread->thread_timer));

        /* enable interrupt */
        rt_hw_interrupt_enable(temp);
        
        thread->error = -RT_EINTR;
        rt_schedule();

        /* clear error number of this thread to RT_EOK */
        if (thread->error == -RT_ETIMEOUT)
            thread->error = RT_EOK;
    }
    else
    {
        rt_hw_interrupt_enable(temp);
    }

    return err;
}

/**
 * This function will let current thread delay for some ticks.
 *
 * @param tick the delay ticks
 *
 * @return RT_EOK
 */
rt_err_t rt_thread_delay(rt_tick_t tick)
{
    return rt_thread_sleep(tick);
}
RTM_EXPORT(rt_thread_delay);

/**
 * This function will let current thread delay until (*tick + inc_tick).
 *
 * @param tick the tick of last wakeup.
 * @param inc_tick the increment tick
 *
 * @return RT_EOK
 */
rt_err_t rt_thread_delay_until(rt_tick_t *tick, rt_tick_t inc_tick)
{
    register rt_base_t level;
    struct rt_thread *thread;

    RT_ASSERT(tick != RT_NULL);

    /* set to current thread */
    thread = rt_thread_self();
    RT_ASSERT(thread != RT_NULL);
    RT_ASSERT(rt_object_get_type((rt_object_t)thread) == RT_Object_Class_Thread);

    /* disable interrupt */
    level = rt_hw_interrupt_disable();

    if (rt_tick_get() - *tick < inc_tick)
    {
        *tick = *tick + inc_tick - rt_tick_get();

        /* suspend thread */
        rt_thread_suspend_with_flag(thread, RT_UNINTERRUPTIBLE);

        /* reset the timeout of thread timer and start it */
        rt_timer_control(&(thread->thread_timer), RT_TIMER_CTRL_SET_TIME, tick);
        rt_timer_start(&(thread->thread_timer));

        /* enable interrupt */
        rt_hw_interrupt_enable(level);

        rt_schedule();

        /* clear error number of this thread to RT_EOK */
        if (thread->error == -RT_ETIMEOUT)
        {
            thread->error = RT_EOK;
        }
    }
    else
    {
        rt_hw_interrupt_enable(level);
    }

    /* get the wakeup tick */
    *tick = rt_tick_get();

    return RT_EOK;
}
RTM_EXPORT(rt_thread_delay_until);

/**
 * This function will let current thread delay for some milliseconds.
 *
 * @param ms the delay ms time
 *
 * @return RT_EOK
 */
rt_err_t rt_thread_mdelay(rt_int32_t ms)
{
    rt_tick_t tick;

    tick = rt_tick_from_millisecond(ms);

    return rt_thread_sleep(tick);
}
RTM_EXPORT(rt_thread_mdelay);

#ifdef RT_USING_SMP
static void rt_thread_cpu_bind(rt_thread_t thread, int cpu)
{
    rt_base_t level;

    if (cpu >= RT_CPUS_NR)
    {
        cpu = RT_CPUS_NR;
    }

    level = rt_hw_interrupt_disable();
    if ((thread->stat & RT_THREAD_STAT_MASK) == RT_THREAD_READY)
    {
        /* unbind */
        /* remove from old ready queue */
        rt_schedule_remove_thread(thread);
        /* change thread bind cpu */
        thread->bind_cpu = cpu;
        /* add to new ready queue */
        rt_schedule_insert_thread(thread);
        if (rt_thread_self() != RT_NULL)
        {
            rt_schedule();
        }
    }
    else
    {
        thread->bind_cpu = cpu;
        if ((thread->stat & RT_THREAD_STAT_MASK) == RT_THREAD_RUNNING)
        {
            /* thread is running on a cpu */
            int current_cpu = rt_hw_cpu_id();

            if (cpu != RT_CPUS_NR)
            {
                if (thread->oncpu == current_cpu)
                {
                    /* current thread on current cpu */
                    if (cpu != current_cpu)
                    {
                        /* bind to other cpu */
                        rt_hw_ipi_send(RT_SCHEDULE_IPI, 1U << cpu);
                        /* self cpu need reschedule */
                        rt_schedule();
                    }
                    /* else do nothing */
                }
                else
                {
                    /* no running on self cpu, but dest cpu can be itself */
                    rt_hw_ipi_send(RT_SCHEDULE_IPI, 1U << thread->oncpu);
                }
            }
            /* else do nothing */
        }
    }
    rt_hw_interrupt_enable(level);
}
#endif

/**
 * This function will control thread behaviors according to control command.
 *
 * @param thread the specified thread to be controlled
 * @param cmd the control command, which includes
 *  RT_THREAD_CTRL_CHANGE_PRIORITY for changing priority level of thread;
 *  RT_THREAD_CTRL_STARTUP for starting a thread;
 *  RT_THREAD_CTRL_CLOSE for delete a thread;
 *  RT_THREAD_CTRL_BIND_CPU for bind the thread to a CPU.
 * @param arg the argument of control command
 *
 * @return RT_EOK
 */
rt_err_t rt_thread_control(rt_thread_t thread, int cmd, void *arg)
{
    register rt_base_t temp;

    /* thread check */
    RT_ASSERT(thread != RT_NULL);
    RT_ASSERT(rt_object_get_type((rt_object_t)thread) == RT_Object_Class_Thread);

    switch (cmd)
    {
    case RT_THREAD_CTRL_UPDATE_PRIORITY:
    case RT_THREAD_CTRL_CHANGE_PRIORITY:
        /* disable interrupt */
        temp = rt_hw_interrupt_disable();

        /* for ready thread, change queue */
        if ((thread->stat & RT_THREAD_STAT_MASK) == RT_THREAD_READY)
        {
            /* remove thread from schedule queue first */
            rt_schedule_remove_thread(thread);
            /* change thread priority */
            thread->current_priority = *(rt_uint8_t *)arg;
            if (cmd == RT_THREAD_CTRL_CHANGE_PRIORITY)
                thread->init_priority = thread->current_priority;

            /* recalculate priority attribute */
#if RT_THREAD_PRIORITY_MAX > 32
            thread->number      = thread->current_priority >> 3;            /* 5bit */
            thread->number_mask = 1 << thread->number;
            thread->high_mask   = 1 << (thread->current_priority & 0x07);   /* 3bit */
#else
            thread->number_mask = 1 << thread->current_priority;
#endif

            /* insert thread to schedule queue again */
            rt_schedule_insert_thread(thread);
        }
        else
        {
            thread->current_priority = *(rt_uint8_t *)arg;
            if (cmd == RT_THREAD_CTRL_CHANGE_PRIORITY)
                thread->init_priority = thread->current_priority;

            /* recalculate priority attribute */
#if RT_THREAD_PRIORITY_MAX > 32
            thread->number      = thread->current_priority >> 3;            /* 5bit */
            thread->number_mask = 1 << thread->number;
            thread->high_mask   = 1 << (thread->current_priority & 0x07);   /* 3bit */
#else
            thread->number_mask = 1 << thread->current_priority;
#endif
        }

        /* enable interrupt */
        rt_hw_interrupt_enable(temp);
        break;

    case RT_THREAD_CTRL_STARTUP:
        return rt_thread_startup(thread);

    case RT_THREAD_CTRL_CLOSE:

        if (rt_object_is_systemobject((rt_object_t)thread) == RT_TRUE)
        {
            return rt_thread_detach(thread);
        }
#ifdef RT_USING_HEAP
        else
        {
            return rt_thread_delete(thread);
        }
#endif

#ifdef RT_USING_SMP
    case RT_THREAD_CTRL_BIND_CPU:
    {
        rt_uint8_t cpu;

        cpu = (rt_uint8_t)(size_t)arg;
        rt_thread_cpu_bind(thread, cpu);
        break;
    }
#endif /*RT_USING_SMP*/
    default:
        break;
    }

    return RT_EOK;
}
RTM_EXPORT(rt_thread_control);

#ifdef RT_USING_LWP
int lwp_suspend_sigcheck(rt_thread_t thread, int suspend_flag);
#endif

static void rt_thread_set_suspend_state(struct rt_thread *thread, int suspend_flag)
{
    rt_uint8_t stat = RT_THREAD_SUSPEND_UNINTERRUPTIBLE;

    RT_ASSERT(thread != RT_NULL);
    switch (suspend_flag)
    {
    case RT_INTERRUPTIBLE:
        stat = RT_THREAD_SUSPEND_INTERRUPTIBLE;
        break;
    case RT_KILLABLE:
        stat = RT_THREAD_SUSPEND_KILLABLE;
        break;
    case RT_UNINTERRUPTIBLE:
        stat = RT_THREAD_SUSPEND_UNINTERRUPTIBLE;
        break;
    default:
        RT_ASSERT(0);
        break;
    }
    thread->stat = stat | (thread->stat & ~RT_THREAD_STAT_MASK);
}

/**
 * This function will suspend the specified thread.
 *
 * @param thread the thread to be suspended
 * @param suspend_flag status flag of the thread to be suspended
 *
 * @return the operation status, RT_EOK on OK, -RT_ERROR on error
 *
 * @note if suspend self thread, after this function call, the
 * rt_schedule() must be invoked.
 */
rt_err_t rt_thread_suspend_with_flag(rt_thread_t thread, int suspend_flag)
{
    register rt_base_t stat;
    register rt_base_t temp;

    /* thread check */
    RT_ASSERT(thread != RT_NULL);
    RT_ASSERT(rt_object_get_type((rt_object_t)thread) == RT_Object_Class_Thread);

    RT_DEBUG_LOG(RT_DEBUG_THREAD, ("thread suspend:  %s\n", thread->name));

    stat = thread->stat & RT_THREAD_STAT_MASK;
    if ((stat != RT_THREAD_READY) && (stat != RT_THREAD_RUNNING))
    {
        RT_DEBUG_LOG(RT_DEBUG_THREAD, ("thread suspend: thread disorder, 0x%2x\n",
                                       thread->stat));
        return -RT_ERROR;
    }

    /* disable interrupt */
    temp = rt_hw_interrupt_disable();
    if (stat == RT_THREAD_RUNNING)
    {
        /* not suspend running status thread on other core */
        RT_ASSERT(thread == rt_thread_self());
    }
#ifdef RT_USING_LWP
    if (lwp_suspend_sigcheck(thread, suspend_flag) == 0)
    {
        /* not to suspend */
        rt_hw_interrupt_enable(temp);
        return -RT_EINTR;
    }
#endif

    /* change thread stat */
    rt_schedule_remove_thread(thread);
    rt_thread_set_suspend_state(thread, suspend_flag);

    /* stop thread timer anyway */
    rt_timer_stop(&(thread->thread_timer));

    /* enable interrupt */
    rt_hw_interrupt_enable(temp);

    RT_OBJECT_HOOK_CALL(rt_thread_suspend_hook, (thread));
    return RT_EOK;
}
RTM_EXPORT(rt_thread_suspend_with_flag);

rt_err_t rt_thread_suspend(rt_thread_t thread)
{
    return rt_thread_suspend_with_flag(thread, RT_UNINTERRUPTIBLE);
}
RTM_EXPORT(rt_thread_suspend);


/**
 * This function will resume a thread and put it to system ready queue.
 *
 * @param thread the thread to be resumed
 *
 * @return the operation status, RT_EOK on OK, -RT_ERROR on error
 */
rt_err_t rt_thread_resume(rt_thread_t thread)
{
    register rt_base_t temp;

    /* thread check */
    RT_ASSERT(thread != RT_NULL);
    RT_ASSERT(rt_object_get_type((rt_object_t)thread) == RT_Object_Class_Thread);

    RT_DEBUG_LOG(RT_DEBUG_THREAD, ("thread resume:  %s\n", thread->name));

    if ((thread->stat & RT_THREAD_SUSPEND_MASK) != RT_THREAD_SUSPEND_MASK)
    {
        RT_DEBUG_LOG(RT_DEBUG_THREAD, ("thread resume: thread disorder, %d\n",
                                       thread->stat));

        return -RT_ERROR;
    }

    /* disable interrupt */
    temp = rt_hw_interrupt_disable();

    /* remove from suspend list */
    rt_list_remove(&(thread->tlist));

    rt_timer_stop(&thread->thread_timer);

#ifdef RT_USING_LWP
    thread->wakeup.func = RT_NULL;
#endif

    /* enable interrupt */
    rt_hw_interrupt_enable(temp);

    /* insert to schedule ready list */
    rt_schedule_insert_thread(thread);

    RT_OBJECT_HOOK_CALL(rt_thread_resume_hook, (thread));
    return RT_EOK;
}
RTM_EXPORT(rt_thread_resume);

#ifdef RT_USING_LWP
/**
 * This function will wakeup a thread with customized operation.
 *
 * @param thread the thread to be resumed
 *
 * @return the operation status, RT_EOK on OK, -RT_ERROR on error
 */
rt_err_t rt_thread_wakeup(rt_thread_t thread)
{
    register rt_base_t temp;
    rt_err_t ret;

    RT_ASSERT(thread != RT_NULL);
    RT_ASSERT(rt_object_get_type((rt_object_t)thread) == RT_Object_Class_Thread);
    /* disable interrupt */
    temp = rt_hw_interrupt_disable();
    if (thread->wakeup.func)
    {
        ret = thread->wakeup.func(thread->wakeup.user_data, thread);
        thread->wakeup.func = RT_NULL;
    }
    else
    {
        ret = rt_thread_resume(thread);
    }

    rt_hw_interrupt_enable(temp);
    return ret;
}
RTM_EXPORT(rt_thread_wakeup);

void rt_thread_wakeup_set(struct rt_thread *thread, rt_wakeup_func_t func, void* user_data)
{
    register rt_base_t temp;

    RT_ASSERT(thread != RT_NULL);
    RT_ASSERT(rt_object_get_type((rt_object_t)thread) == RT_Object_Class_Thread);

    temp = rt_hw_interrupt_disable();
    thread->wakeup.func = func;
    thread->wakeup.user_data = user_data;
    rt_hw_interrupt_enable(temp);
}
RTM_EXPORT(rt_thread_wakeup_set);
#endif

/**
 * This function is the timeout function for thread, normally which is invoked
 * when thread is timeout to wait some resource.
 *
 * @param parameter the parameter of thread timeout function
 */
void rt_thread_timeout(void *parameter)
{
    struct rt_thread *thread;

    thread = (struct rt_thread *)parameter;

    /* thread check */
    RT_ASSERT(thread != RT_NULL);
    RT_ASSERT((thread->stat & RT_THREAD_SUSPEND_MASK) == RT_THREAD_SUSPEND_MASK);
    RT_ASSERT(rt_object_get_type((rt_object_t)thread) == RT_Object_Class_Thread);

    /* set error number */
    thread->error = -RT_ETIMEOUT;

    /* remove from suspend list */
    rt_list_remove(&(thread->tlist));

    /* insert to schedule ready list */
    rt_schedule_insert_thread(thread);

    /* do schedule */
    rt_schedule();
}
RTM_EXPORT(rt_thread_timeout);

/**
 * This function will find the specified thread.
 *
 * @param name the name of thread finding
 *
 * @return the found thread
 *
 * @note please don't invoke this function in interrupt status.
 */
rt_thread_t rt_thread_find(char *name)
{
    struct rt_object_information *information;
    struct rt_object *object;
    struct rt_list_node *node;

    /* enter critical */
    if (rt_thread_self() != RT_NULL)
        rt_enter_critical();

    /* try to find device object */
    information = rt_object_get_information(RT_Object_Class_Thread);
    RT_ASSERT(information != RT_NULL);
    for (node  = information->object_list.next;
         node != &(information->object_list);
         node  = node->next)
    {
        object = rt_list_entry(node, struct rt_object, list);
        if (rt_strncmp(object->name, name, RT_NAME_MAX) == 0)
        {
            /* leave critical */
            if (rt_thread_self() != RT_NULL)
                rt_exit_critical();

            return (rt_thread_t)object;
        }
    }

    /* leave critical */
    if (rt_thread_self() != RT_NULL)
        rt_exit_critical();

    /* not found */
    return RT_NULL;
}
RTM_EXPORT(rt_thread_find);

/**@}*/
