/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
bool compare_condition_vrb_priority(struct list_elem *a, struct list_elem *b, void *aux);
/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void sema_init(struct semaphore *sema, unsigned value)
{
  ASSERT(sema != NULL);

  sema->value = value;
  list_init(&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
/*
sort descending to make high priority in the front of our list
*/

void sema_down(struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT(sema != NULL);
  ASSERT(!intr_context());

  old_level = intr_disable();

  while (sema->value == 0)
  {
    list_insert_ordered(&sema->waiters, &thread_current()->elem, &compare_priority, NULL);
    thread_block();
  }

  /* If semaphore is not acquired, we decrease the value by 1. */
  sema->value--;
  intr_set_level(old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool sema_try_down(struct semaphore *sema)
{
  enum intr_level old_level;
  bool success;

  ASSERT(sema != NULL);

  old_level = intr_disable();

  /* Semaphone is acquired. */
  if (sema->value > 0)
  {
    sema->value--;
    success = true;
  }
  /* Semaphore is not acquired. */
  else
    success = false;

  intr_set_level(old_level);
  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */

void sema_up(struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT(sema != NULL);

  old_level = intr_disable();

  if (!list_empty(&sema->waiters))
  {
    struct list_elem *max_priority = list_min(&sema->waiters,&compare_priority,NULL);
    list_remove(max_priority);
    thread_unblock(list_entry(max_priority, struct thread, elem));
  }
  sema->value++;

  intr_set_level(old_level);
  thread_yield();
}

static void sema_test_helper(void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void sema_self_test(void)
{
  struct semaphore sema[2];
  int i;

  printf("Testing semaphores...");
  sema_init(&sema[0], 0);
  sema_init(&sema[1], 0);
  thread_create("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++)
  {
    sema_up(&sema[0]);
    sema_down(&sema[1]);
  }
  printf("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper(void *sema_)
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++)
  {
    sema_down(&sema[0]);
    sema_up(&sema[1]);
  }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void lock_init(struct lock *lock)
{
  ASSERT(lock != NULL);

  lock->holder = NULL;
  sema_init(&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void lock_acquire(struct lock *lock)
{
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(!lock_held_by_current_thread(lock));

  enum intr_level old_level;
  old_level = intr_disable();

  /* If current thread cannot acquire lock, it means that the lock is acquired by another. */
  if (!lock_try_acquire(lock))
  {
    thread_current()->wait_on_lock = lock; /* Current thread is waiting on this lock. */
    /* Check if the scheduling is advanced or not.
    If not advanced then we handle the priority donation scheme. */
    if (!thread_mlfqs)
      donate_priority();
    /* Current thread now is acquiring the lock */
    sema_down (&lock -> semaphore);
    lock -> holder = thread_current ();
    thread_current () -> wait_on_lock = NULL;
    /* Add the lock to currently held locks in
    current thread after acquiring the lock. */
    list_push_back (&thread_current() -> donations, &lock -> lock_elem);
  }

  /* If current thread can acquire lock, simply set interrupt level back to old level. */
  intr_set_level(old_level);
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool lock_try_acquire(struct lock *lock)
{
  bool success;
  ASSERT(lock != NULL);
  ASSERT(!lock_held_by_current_thread(lock));

  success = sema_try_down(&lock->semaphore);

  /* If current thread can acquire the lock we add it to donations list. */
  if (success)
  {
    lock->holder = thread_current();
    list_push_back (&thread_current () -> donations, &lock -> lock_elem);
  }
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void lock_release(struct lock *lock)
{
  ASSERT(lock != NULL);
  ASSERT(lock_held_by_current_thread(lock));

  enum intr_level old_level;
  old_level = intr_disable();

  /* If scheduling is not advanced we handle the priority reset. */
  if (!thread_mlfqs)
  {
    /* Remove the instances of the lock element in donatons list.
       Reset the priority of threads. */
    list_remove(&lock->lock_elem);
    reset_priority();
  }

  lock->holder = NULL;
  sema_up(&lock->semaphore);

  intr_set_level(old_level);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool lock_held_by_current_thread(const struct lock *lock)
{
  ASSERT(lock != NULL);

  return lock->holder == thread_current();
}

/* One semaphore in a list. */
struct semaphore_elem
{
  struct list_elem elem;      /* List element. */
  struct semaphore semaphore; /* This semaphore. */
};

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void cond_init(struct condition *cond)
{
  ASSERT(cond != NULL);

  list_init(&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void cond_wait(struct condition *cond, struct lock *lock)
{
  struct semaphore_elem waiter;

  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  sema_init(&waiter.semaphore, 0);
  list_push_back(&cond->waiters, &waiter.elem);
  lock_release(lock);
  sema_down(&waiter.semaphore);
  lock_acquire(lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_signal(struct condition *cond, struct lock *lock UNUSED)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  if (!list_empty(&cond->waiters))
  {
    //     struct list_elem *max= list_max(&cond->waiters,compare_priority_less,NULL);
    // struct thread *t=list_entry(max,struct thread,elem);
    // thread_unblock(t);
    // list_remove(max);
    list_sort(&cond->waiters, &compare_condition_vrb_priority, NULL);
    sema_up(&list_entry(list_pop_front(&cond->waiters),
                        struct semaphore_elem, elem)
                 ->semaphore);
  }
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_broadcast(struct condition *cond, struct lock *lock)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);

  while (!list_empty(&cond->waiters))
    cond_signal(cond, lock);
}

bool compare_condition_vrb_priority(struct list_elem *a, struct list_elem *b, void *aux UNUSED)
{
  struct semaphore_elem *a_semaphore = list_entry(a, struct semaphore_elem, elem);
  struct semaphore_elem *b_semaphore = list_entry(b, struct semaphore_elem, elem);

  list_sort(&a_semaphore->semaphore.waiters, &compare_priority, NULL);
  list_sort(&b_semaphore->semaphore.waiters, &compare_priority, NULL);

  struct thread *a_thread = list_entry(list_front(&a_semaphore->semaphore.waiters), struct thread, elem);

  struct thread *b_thread = list_entry(list_front(&b_semaphore->semaphore.waiters), struct thread, elem);
  return a_thread->priority > b_thread->priority;
}
bool compare_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
  int priority_a = (list_entry(a, struct thread, elem))->priority;
  int priority_b = (list_entry(b, struct thread, elem))->priority;
  return priority_a > priority_b;
}