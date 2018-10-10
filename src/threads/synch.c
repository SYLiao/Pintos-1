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

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) 
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
		list_insert_ordered (&sema->waiters,&thread_current ()->elem, (list_less_func *) &priority_compare, NULL);
      //list_push_back (&sema->waiters, &thread_current ()->elem);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0) 
    {
      sema->value--;
      success = true; 
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
   sema->value++;									// pushing it above otherwise when thread will be in ready queue and gets CPU will start executing sema down while loop code which will tell sema -->value still zero, so will make it positive first then unblock thread, and if we dont do it last thread will never woke up.
  if (!list_empty (&sema->waiters)) 
  {
    list_sort(&sema->waiters,(list_less_func *) &priority_compare, NULL);    // sort before pulling thread as priority donation changes priority after thread being added to queue
    thread_unblock (list_entry (list_pop_front (&sema->waiters),
                                struct thread, elem));
  }
 
  intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) 
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
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
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);

  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));
  struct thread *cur_thread = thread_current();
  struct thread *cur_lock_holder = lock->holder;   // thread holding lock
  struct lock *cur_lock = lock;

  cur_thread->current_lock_requested = lock;        // Current requested lock set to null once lock is aquired

  if(list_empty(&cur_thread->locks_holds))
    cur_thread->initial_priority=cur_thread->priority;    // set initial priority here too  before it holds any locks

  //First thread to aquire lock should attach its priority to lock
  if(cur_lock_holder == NULL)
    lock->lock_priority = cur_thread->priority;

  while(cur_lock_holder != NULL)
  {
	  if(cur_thread->priority > cur_lock_holder->priority)
    {
		  cur_lock_holder->priority = cur_thread->priority;
      cur_lock->lock_priority = cur_thread->priority;           // Needed when current lock release and threads picks next lock from its queue
    }

    cur_lock = cur_lock_holder->current_lock_requested;
    if(cur_lock == NULL)
      break;        // if there are no lock is requested further no nested donation
    
    cur_lock_holder = cur_lock->holder;         // if it has requested for any lock get the holder of lock for priority donation under nested donation
												// Change the priority of current lock too else causing issue with priority-donate-chain as priority while lock release depend on locks priority
  }

  sema_down (&lock->semaphore);
  lock->holder = thread_current ();
  cur_thread->current_lock_requested = NULL;               // Remove lock request as now we have aquired the lock
  //list_push_back(&cur_thread->locks_holds,&lock->elem);   // no point pushing in the list based lock priority, as donation will change the priority later, so will sort list at the time of releasing lock
  list_insert_ordered (&cur_thread->locks_holds, &lock->elem, (list_less_func *) &lock_priority_compare, NULL);
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = thread_current ();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  lock->holder = NULL;
  list_remove(&lock->elem);       // remove current lock from threads hold list as its been relesed
  sema_up (&lock->semaphore);

  // Priority inversion take priority away as it release lock
  // struct thread *cur_thread = thread_current();
  // cur_thread->priority = cur_thread->initial_priority;
  /* Steps in lock release should contains these
  1. inversion of the priority of the thread releasing it to the either initial state or 
  if thread has received priority from any other thread for some other lock then set that locks priority to thread
  2. Remove this lock from threads locks_holds list as its released now
  */
 
  struct thread *cur_thread = thread_current();
  // list_remove(&lock->elem);       // remove current lock from threads hold list as its been relesed

  if(!list_empty(&cur_thread->locks_holds))
    {
      // if it holds more locks then find the highest priorty lock and assign its priority to thread
      list_sort(&(cur_thread->locks_holds),(list_less_func *) &lock_priority_compare, NULL);
      //int size = list_size(&(cur_thread->locks_holds));
      struct lock *highest_priority_lock = list_entry( list_front(&(cur_thread->locks_holds)), struct lock, elem );
      cur_thread->priority = highest_priority_lock->lock_priority;
      //thread_set_priority(highest_priority_lock->lock_priority);
    }
    else
    {
      // if there are no more locks it hold change its priority to initial priority
      cur_thread->priority = cur_thread->initial_priority;
      //thread_set_priority(cur_thread->initial_priority);
    }
    thread_yield();
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem 
  {
    struct list_elem elem;              /* List element. */
    struct semaphore semaphore;         /* This semaphore. */
  };

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
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
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  
  sema_init (&waiter.semaphore, 0);
  //list_insert_ordered (&cond->waiters,  &waiter.elem, (list_less_func *) &priority_compare_for_condition, NULL);		
  list_push_back (&cond->waiters, &waiter.elem);
  cond->is_sorted = false;
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) 
  {
	if(!cond->is_sorted)
	{
		list_sort(&cond->waiters,(list_less_func *) &priority_compare_for_condition, NULL);
		cond->is_sorted = true;
	}

    sema_up (&list_entry (list_pop_front (&cond->waiters),struct semaphore_elem, elem)->semaphore);
  }
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}

bool priority_compare_for_condition(struct list_elem *e1, struct list_elem *e2)
{
	struct semaphore sema1=list_entry (e1,struct semaphore_elem, elem)->semaphore;
	struct semaphore sema2=list_entry (e2,struct semaphore_elem, elem)->semaphore;
	int pr1= list_entry (list_front (&sema1.waiters),struct thread, elem)->priority;
	int pr2= list_entry (list_front (&sema2.waiters),struct thread, elem)->priority;
	return  pr1 > pr2;
}

bool lock_priority_compare(struct list_elem *e1, struct list_elem *e2)
{
	return (list_entry(e1, struct lock, elem)->lock_priority > list_entry(e2, struct lock, elem)->lock_priority);
}