#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

static thread_func t1_func;
static thread_func t2_func;
static thread_func t3_func;
static thread_func vip_func;

static struct lock lock_a;
static struct lock lock_b;
static struct lock lock_c;

void
test_priority_donate_release_unsorted (void) 
{
    lock_init (&lock_a);
    lock_init (&lock_b);
    lock_init (&lock_c);

    thread_set_priority (5);
    lock_acquire (&lock_a);
    lock_acquire (&lock_b);

    thread_create ("T1", 10, t1_func, NULL);
    thread_create ("T2", 20, t2_func, NULL);
    thread_create ("T3", 15, t3_func, NULL);

    thread_create ("VIP", 30, vip_func, NULL);

    msg ("Main thread should have priority 30.  Actual priority: %d.", thread_get_priority ());
    
    lock_release (&lock_a);
    
    msg ("Main thread should have priority 30.  Actual priority: %d.", thread_get_priority ());

    lock_release (&lock_b);
}

static void t1_func (void *aux UNUSED) {
    lock_acquire (&lock_c);
    lock_acquire (&lock_b);
    msg ("T1 acquired lock_b.");
    lock_release (&lock_b);
    lock_release (&lock_c);
}

static void t2_func (void *aux UNUSED) {
    lock_acquire (&lock_b);
    msg ("T2 acquired lock_b.");
    lock_release (&lock_b);
}

static void t3_func (void *aux UNUSED) {
    lock_acquire (&lock_a);
    msg ("T3 acquired lock_a.");
    lock_release (&lock_a);
}

static void vip_func (void *aux UNUSED) {
    lock_acquire (&lock_c);
    msg ("VIP acquired lock_c.");
    lock_release (&lock_c);
}