//lock_holder가 ready_list에 있을 때 donate받았을 시 ready_list체크

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

static thread_func a_thread_func;
static thread_func b_thread_func;
static thread_func c_thread_func;

void
test_priority_donate_ready (void) 
{
    struct lock lock;
    lock_init (&lock);

    thread_set_priority (5);

    thread_create ("thread B", 10, b_thread_func, &lock);
}

static void
b_thread_func (void *lock_) 
{
    struct lock *lock = lock_;
    lock_acquire (lock);
    msg ("Thread B acquired lock.");

    thread_create ("thread C", 20, c_thread_func, lock);
    
    msg ("Thread B finished.");
    lock_release (lock);
}

static void
c_thread_func (void *lock_) 
{
    msg ("Thread C is running.");

    thread_create ("thread A", 30, a_thread_func, lock_);

    msg ("Thread C finished.");
}

static void
a_thread_func (void *lock_) 
{
    struct lock *lock = lock_;
    msg ("Thread A will acquire lock.");
    
    lock_acquire (lock);
    msg ("Thread A acquired lock.");
    lock_release (lock);
    msg ("Thread A finished.");
}