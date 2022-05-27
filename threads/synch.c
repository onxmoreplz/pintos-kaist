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
<<<<<<< HEAD
bool sema_compare_priority(const struct list_elem *a, const struct list_elem *b,
                           void *aux UNUSED);

void sema_init(struct semaphore *sema, unsigned value) {
  ASSERT(sema != NULL);

  sema->value = value;
  list_init(&sema->waiters);
=======
bool sema_compare_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);

void sema_init(struct semaphore *sema, unsigned value)
{
	ASSERT(sema != NULL);

	sema->value = value;
	list_init(&sema->waiters);
>>>>>>> 0984309b8ae8f80b6e36b72e57738cd237b05f88
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. This is
   sema_down function. */
<<<<<<< HEAD
void sema_down(struct semaphore *sema) {
  enum intr_level old_level;

  ASSERT(sema != NULL);
  ASSERT(!intr_context());

  old_level = intr_disable();
  while (sema->value == 0) {
    // list_push_back (&sema->waiters, &thread_current ()->elem);
    list_insert_ordered(&sema->waiters, &thread_current()->elem,
                        thread_cmp_priority, 0);
    thread_block();
  }
  sema->value--;
  intr_set_level(old_level);
=======
void sema_down(struct semaphore *sema)
{
	enum intr_level old_level;

	ASSERT(sema != NULL);
	ASSERT(!intr_context());

	old_level = intr_disable();
	while (sema->value == 0)
	{
		list_push_back (&sema->waiters, &thread_current ()->elem); 
		// list_insert_ordered(&sema->waiters, &thread_current()->elem, thread_cmp_priority, 0);
		// 정렬하면서 넣어주지 않아도 되는 이유 : sema_up() 함수에서 꺼내기 전에 sema_waiters 리스트 정렬함
		thread_block();
	}
	sema->value--;
	intr_set_level(old_level);
>>>>>>> 0984309b8ae8f80b6e36b72e57738cd237b05f88
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
<<<<<<< HEAD
bool sema_try_down(struct semaphore *sema) {
  enum intr_level old_level;
  bool success;

  ASSERT(sema != NULL);

  old_level = intr_disable();
  if (sema->value > 0) {
    sema->value--;
    success = true;
  } else
    success = false;
  intr_set_level(old_level);

  return success;
=======
bool sema_try_down(struct semaphore *sema)
{
	enum intr_level old_level;
	bool success;

	ASSERT(sema != NULL);

	old_level = intr_disable();
	if (sema->value > 0)
	{
		sema->value--;
		success = true;
	}
	else
		success = false;
	intr_set_level(old_level);

	return success;
>>>>>>> 0984309b8ae8f80b6e36b72e57738cd237b05f88
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
<<<<<<< HEAD
void sema_up(struct semaphore *sema) {
  enum intr_level old_level;

  ASSERT(sema != NULL);

  old_level = intr_disable();
  if (!list_empty(&sema->waiters)) {
    list_sort(&sema->waiters, thread_cmp_priority, 0);
    thread_unblock(
        list_entry(list_pop_front(&sema->waiters), struct thread, elem));
  }

  sema->value++;
  thread_test_preemption();
  intr_set_level(old_level);
=======
void sema_up(struct semaphore *sema)
{
	enum intr_level old_level;

	ASSERT(sema != NULL);

	old_level = intr_disable();
	if (!list_empty(&sema->waiters))
	{
		list_sort(&sema->waiters, thread_cmp_priority, 0);
		thread_unblock(list_entry(list_pop_front(&sema->waiters), struct thread, elem));
	}

	sema->value++;
	thread_test_preemption();
	intr_set_level(old_level);
>>>>>>> 0984309b8ae8f80b6e36b72e57738cd237b05f88
}

static void sema_test_helper(void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
<<<<<<< HEAD
void sema_self_test(void) {
  struct semaphore sema[2];
  int i;

  printf("Testing semaphores...");
  sema_init(&sema[0], 0);
  sema_init(&sema[1], 0);
  thread_create("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) {
    sema_up(&sema[0]);
    sema_down(&sema[1]);
  }
  printf("done.\n");
}

/* Thread function used by sema_self_test(). */
static void sema_test_helper(void *sema_) {
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) {
    sema_down(&sema[0]);
    sema_up(&sema[1]);
  }
}

bool sema_compare_priority(const struct list_elem *a, const struct list_elem *b,
                           void *aux UNUSED) {
  struct semaphore_elem *l_sema = list_entry(a, struct semaphore_elem, elem);
  struct semaphore_elem *s_sema = list_entry(b, struct semaphore_elem, elem);

  struct list *waiter_l_sema = &(l_sema->semaphore.waiters);
  struct list *waiter_s_sema = &(s_sema->semaphore.waiters);

  return list_entry(list_begin(waiter_l_sema), struct thread, elem)->priority >
         list_entry(list_begin(waiter_s_sema), struct thread, elem)->priority;
=======
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

bool sema_compare_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
	struct semaphore_elem *l_sema = list_entry(a, struct semaphore_elem, elem);
	struct semaphore_elem *s_sema = list_entry(b, struct semaphore_elem, elem);

	struct list *waiter_l_sema = &(l_sema->semaphore.waiters);
	struct list *waiter_s_sema = &(s_sema->semaphore.waiters);

	return list_entry(list_begin(waiter_l_sema), struct thread, elem)->priority > list_entry(list_begin(waiter_s_sema), struct thread, elem)->priority;
>>>>>>> 0984309b8ae8f80b6e36b72e57738cd237b05f88
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
<<<<<<< HEAD
void lock_init(struct lock *lock) {
  ASSERT(lock != NULL);

  lock->holder = NULL;
  sema_init(&lock->semaphore, 1);
=======
void lock_init(struct lock *lock)
{
	ASSERT(lock != NULL);

	lock->holder = NULL;
	sema_init(&lock->semaphore, 1);
>>>>>>> 0984309b8ae8f80b6e36b72e57738cd237b05f88
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
<<<<<<< HEAD
void lock_acquire(struct lock *lock) {
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(!lock_held_by_current_thread(lock));

  struct thread *cur = thread_current();
  if (lock->holder) {
    cur->wait_on_lock = lock;
    list_insert_ordered(&lock->holder->donations, &cur->donation_elem,
                        thread_compare_donate_priority, 0);
    donate_priority();
  }

  sema_down(&lock->semaphore);

  cur->wait_on_lock = NULL;
  lock->holder = cur;
=======
/**
 * lock_acquire - 스레드가 lock을 요청할 때 실행
 * 				   누군가가 해당 lock을 점유하고 있다면 자신의 priority 양도
 *				=> acquire()한 스레드만이 해당 lock을 release() 할 수 있다.
*/
void lock_acquire(struct lock *lock)
{
	ASSERT(lock != NULL);
	ASSERT(!intr_context());
	ASSERT(!lock_held_by_current_thread(lock));

	struct thread *curr = thread_current();
	if (lock->holder) // 애초에 lock_acquire() 를 요청한 스레드가 실행되고 있다는 것이 lock을 갖고있는 thread보다 우선순위가 높다는 뜻
	{
		curr->wait_on_lock = lock;
		list_insert_ordered(
			&lock->holder->donations, 
			&curr->donation_elem,
			thread_cmp_donate_priority, 0);
		donate_priority(); //lock->holder의 우선순위 donate 받음
	}

	sema_down(&lock->semaphore); // sema_down을 기점으로 lock의 소유 여부가 다름

	curr->wait_on_lock = NULL;
	lock->holder = thread_current();
>>>>>>> 0984309b8ae8f80b6e36b72e57738cd237b05f88
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
<<<<<<< HEAD
bool lock_try_acquire(struct lock *lock) {
  bool success;

  ASSERT(lock != NULL);
  ASSERT(!lock_held_by_current_thread(lock));

  success = sema_try_down(&lock->semaphore);
  if (success) lock->holder = thread_current();
  return success;
=======
bool lock_try_acquire(struct lock *lock)
{
	bool success;

	ASSERT(lock != NULL);
	ASSERT(!lock_held_by_current_thread(lock));

	success = sema_try_down(&lock->semaphore);
	if (success)
		lock->holder = thread_current();
	return success;
>>>>>>> 0984309b8ae8f80b6e36b72e57738cd237b05f88
}

/* Releases LOCK, which must be owned by the current thread.
   This is lock_release function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
<<<<<<< HEAD
void lock_release(struct lock *lock) {
  ASSERT(lock != NULL);
  ASSERT(lock_held_by_current_thread(lock));

  remove_with_lock(lock);
  refresh_priority();

  lock->holder = NULL;
  sema_up(&lock->semaphore);
=======
/**
 * lock_release - priority를 양도받아 critical section을 마치고 lock을 반환할 때의 경우를 만들어 줘야함
*/
void lock_release(struct lock *lock)
{
	ASSERT(lock != NULL);
	ASSERT(lock_held_by_current_thread(lock));

	remove_with_lock(lock); //lock을 반납했을 때, 해당 스레드(현재 스레드) 내의 donations에서 lock 요청한 스레드 삭제
	refresh_priority();

	lock->holder = NULL;
	sema_up(&lock->semaphore);
>>>>>>> 0984309b8ae8f80b6e36b72e57738cd237b05f88
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
<<<<<<< HEAD
bool lock_held_by_current_thread(const struct lock *lock) {
  ASSERT(lock != NULL);

  return lock->holder == thread_current();
=======
bool lock_held_by_current_thread(const struct lock *lock)
{
	ASSERT(lock != NULL);

	return lock->holder == thread_current();
>>>>>>> 0984309b8ae8f80b6e36b72e57738cd237b05f88
}

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
<<<<<<< HEAD
void cond_init(struct condition *cond) {
  ASSERT(cond != NULL);

  list_init(&cond->waiters);
=======
void cond_init(struct condition *cond)
{
	ASSERT(cond != NULL);

	list_init(&cond->waiters);
>>>>>>> 0984309b8ae8f80b6e36b72e57738cd237b05f88
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
<<<<<<< HEAD
void cond_wait(struct condition *cond, struct lock *lock) {
  struct semaphore_elem waiter;

  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  sema_init(&waiter.semaphore, 0);
  // list_push_back(&cond->waiters, &waiter.elem);
  list_insert_ordered(&cond->waiters, &waiter.elem, sema_compare_priority, 0);
  lock_release(lock);
  sema_down(&waiter.semaphore);
  lock_acquire(lock);
=======
void cond_wait(struct condition *cond, struct lock *lock)
{
	struct semaphore_elem waiter;

	ASSERT(cond != NULL);
	ASSERT(lock != NULL);
	ASSERT(!intr_context());
	ASSERT(lock_held_by_current_thread(lock)); //현재 스레드가 lock을 갖고 있으면 True

	sema_init(&waiter.semaphore, 0);
	// list_push_back(&cond->waiters, &waiter.elem);
	list_insert_ordered(&cond->waiters, &waiter.elem, sema_compare_priority, 0); // condition 변수(cond) 안에 semaphore들을 넣음
	lock_release(lock);
	sema_down(&waiter.semaphore);
	lock_acquire(lock);
>>>>>>> 0984309b8ae8f80b6e36b72e57738cd237b05f88
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
<<<<<<< HEAD
void cond_signal(struct condition *cond, struct lock *lock UNUSED) {
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  if (!list_empty(&cond->waiters)) {
    list_sort(&cond->waiters, sema_compare_priority, 0);
    sema_up(
        &list_entry(list_pop_front(&cond->waiters), struct semaphore_elem, elem)
             ->semaphore);
  }
=======
void cond_signal(struct condition *cond, struct lock *lock UNUSED)
{
	ASSERT(cond != NULL);
	ASSERT(lock != NULL);
	ASSERT(!intr_context());
	ASSERT(lock_held_by_current_thread(lock));

	if (!list_empty(&cond->waiters))
	{
		list_sort(&cond->waiters, sema_compare_priority, 0);
		sema_up(&list_entry(list_pop_front(&cond->waiters),
							struct semaphore_elem, elem)
					 ->semaphore);
	}
>>>>>>> 0984309b8ae8f80b6e36b72e57738cd237b05f88
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.


   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
<<<<<<< HEAD
void cond_broadcast(struct condition *cond, struct lock *lock) {
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);

  while (!list_empty(&cond->waiters)) cond_signal(cond, lock);
}
=======
void cond_broadcast(struct condition *cond, struct lock *lock)
{
	ASSERT(cond != NULL);
	ASSERT(lock != NULL);

	while (!list_empty(&cond->waiters))
		cond_signal(cond, lock);
}
>>>>>>> 0984309b8ae8f80b6e36b72e57738cd237b05f88
