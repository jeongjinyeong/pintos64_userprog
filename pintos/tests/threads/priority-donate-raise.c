//donate받은 후 set_priority로 받은 값보다 더 크게 올렸을 시 체크


#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

static thread_func a_thread_func;

void
test_priority_donate_raise (void) 
{
    struct lock lock;
    lock_init (&lock);

    thread_set_priority (31);
    lock_acquire (&lock);

    thread_create ("thread A", 41, a_thread_func, &lock);
    msg ("Main thread should have priority 41.  Actual priority: %d.", thread_get_priority ());
    
    thread_set_priority (50);
    msg ("Main thread should have priority 50.  Actual priority: %d.", thread_get_priority ());

    lock_release (&lock);
    
    msg ("Main thread should have priority 50.  Actual priority: %d.", thread_get_priority ());

    thread_set_priority (31);
}

static void
a_thread_func (void *lock_) 
{
    struct lock *lock = lock_;
    lock_acquire (lock);
    msg ("Thread A acquired lock.");
    lock_release (lock);
    msg ("Thread A finished.");
}