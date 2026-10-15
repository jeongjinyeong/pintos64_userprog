#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

static thread_func t_low_func;
static thread_func t_mid_func;
static thread_func vip_func;

static struct lock lock_a;
static struct lock lock_b;

void
test_priority_donate_unsorted (void) 
{
    lock_init (&lock_a);
    lock_init (&lock_b);

    thread_set_priority (5);
    lock_acquire (&lock_a);

    thread_create ("T_low", 10, t_low_func, NULL);
    thread_create ("T_mid", 20, t_mid_func, NULL);
    thread_create ("VIP",   30, vip_func, NULL);

    msg ("Main thread should have priority 30.  Actual priority: %d.", thread_get_priority ());
    
    thread_set_priority (6);


    msg ("Main thread should have priority 30.  Actual priority: %d.", thread_get_priority ());

    lock_release (&lock_a);
}

static void
t_low_func (void *aux UNUSED) 
{
    lock_acquire (&lock_b);
    lock_acquire (&lock_a);
    msg ("T_low acquired lock_a.");
    lock_release (&lock_a);
    lock_release (&lock_b);
}

static void
t_mid_func (void *aux UNUSED) 
{
    lock_acquire (&lock_a);
    msg ("T_mid acquired lock_a.");
    lock_release (&lock_a);
}

static void
vip_func (void *aux UNUSED) 
{
    lock_acquire (&lock_b);
    msg ("VIP acquired lock_b.");
    lock_release (&lock_b);
}