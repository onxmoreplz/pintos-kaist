#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "intrinsic.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif

/* Random value for struct thread's `magic' member.
   Used to detect stack overflow.  See the big comment at the top
   of thread.h for details. */
#define THREAD_MAGIC 0xcd6abf4b

/* Random value for basic thread
   Do not modify this value. */
#define THREAD_BASIC 0xd42df210

#define MIN(a,b) (((a)<(b)) ? (a):(b)) // 최소를 구하는 삼항 연산자

/* List of processes in THREAD_READY state, that is, processes
   that are ready to run but not actually running. */
static struct list ready_list;

static struct list sleep_list; // sleep_list 자료 구조 추가

static int64_t next_tick_to_awake; // wakeup_tick 값 중 최소값 저장하기 위한 변수

/* Idle thread. */
static struct thread *idle_thread;

/* Initial thread, the thread running init.c:main(). */
static struct thread *initial_thread;

/* Lock used by allocate_tid(). */
static struct lock tid_lock;

/* Thread destruction requests */
static struct list destruction_req;

/* Statistics. */
static long long idle_ticks;   /* # of timer ticks spent idle. */
static long long kernel_ticks; /* # of timer ticks in kernel threads. */
static long long user_ticks;   /* # of timer ticks in user programs. */
static long long next_tick_to_awake;
// ㄴ> sleep_list에서 대기중인 스레드들의 wakeup_tick값 중 최소값들 저장

/* Scheduling. */
#define TIME_SLICE 4		  /* # of timer ticks to give each thread. */
static unsigned thread_ticks; /* # of timer ticks since last yield. */

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
bool thread_mlfqs;

static void kernel_thread(thread_func *, void *aux);

static void idle(void *aux UNUSED);
static struct thread *next_thread_to_run(void);
static void init_thread(struct thread *, const char *name, int priority);
static void do_schedule(int status);
static void schedule(void);
static tid_t allocate_tid(void);

/* Alarm Clock 구현을 위한 함수 추가 */
void thread_sleep(int64_t ticks);			   /* 실행 중인 스레드를 슬립으로 만듬 */
void thread_awake(int64_t ticks);			   /* 슬립큐에서 깨워야할 스레드를 깨움 */
void update_next_tick_to_awake(int64_t ticks); /* 최소 틱을 가진 스레드 저장 */
int64_t get_next_tick_to_awake(void);		   /* thread.c의 next_tick_to_awake 반환 */

/*pintos project7*/
void test_max_priority(void);
bool cmp_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);

/*pintos project9*/
bool thread_compare_donate_priority(const struct list_elem *l, const struct list_elem *s, void *aux UNUSED);
void remove_with_lock (struct lock *lock);

/* Returns true if T appears to point to a valid thread. */
#define is_thread(t) ((t) != NULL && (t)->magic == THREAD_MAGIC)

/* Returns the running thread.
 * Read the CPU's stack pointer `rsp', and then round that
 * down to the start of a page.  Since `struct thread' is
 * always at the beginning of a page and the stack pointer is
 * somewhere in the middle, this locates the curent thread. */
#define running_thread() ((struct thread *)(pg_round_down(rrsp())))

// Global descriptor table for the thread_start.
// Because the gdt will be setup after the thread_init, we should
// setup temporal gdt first.
static uint64_t gdt[3] = {0, 0x00af9a000000ffff, 0x00cf92000000ffff};

/* Initializes the threading system by transforming the code
   that's currently running into a thread.  This can't work in
   general and it is possible in this case only because loader.S
   was careful to put the bottom of the stack at a page boundary.

   Also initializes the run queue and the tid lock.

   After calling this function, be sure to initialize the page
   allocator before trying to create any threads with
   thread_create().

   It is not safe to call thread_current() until this function
   finishes. */
void thread_init(void)
{
	ASSERT(intr_get_level() == INTR_OFF);

	/* Reload the temporal gdt for the kernel
	 * This gdt does not include the user context.
	 * The kernel will rebuild the gdt with user context, in gdt_init (). */
	struct desc_ptr gdt_ds = {
		.size = sizeof(gdt) - 1,
		.address = (uint64_t)gdt};
	lgdt(&gdt_ds);

	/* Init the globla thread context */
	lock_init(&tid_lock);
	list_init(&ready_list);
	list_init(&destruction_req);
	list_init(&sleep_list); // sleep_list 자료구조 초기화
	next_tick_to_awake = INT64_MAX; //최소값을 찾기 위해 초기화를 정수 최대값으로

	/* Set up a thread structure for the running thread. */
	initial_thread = running_thread();
	init_thread(initial_thread, "main", PRI_DEFAULT);
	initial_thread->status = THREAD_RUNNING;
	initial_thread->tid = allocate_tid();
}

/* Starts preemptive thread scheduling by enabling interrupts.
   Also creates the idle thread. */
void thread_start(void)
{
	/* Create the idle thread. */
	struct semaphore idle_started;
	sema_init(&idle_started, 0);
	thread_create("idle", PRI_MIN, idle, &idle_started);

	/* Start preemptive thread scheduling. */
	intr_enable();

	/* Wait for the idle thread to initialize idle_thread. */
	sema_down(&idle_started);
}

/* Called by the timer interrupt handler at each timer tick.
   Thus, this function runs in an external interrupt context. */
void thread_tick(void)
{
	struct thread *t = thread_current();

	/* Update statistics. */
	if (t == idle_thread)
		idle_ticks++;
#ifdef USERPROG
	else if (t->pml4 != NULL)
		user_ticks++;
#endif
	else
		kernel_ticks++;

	/* Enforce preemption. */
	if (++thread_ticks >= TIME_SLICE)
		intr_yield_on_return();
}

/* Prints thread statistics. */
void thread_print_stats(void)
{
	printf("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
		   idle_ticks, kernel_ticks, user_ticks);
}

/* Creates a new kernel thread named NAME with the given initial
   PRIORITY, which executes FUNCTION passing AUX as the argument,
   and adds it to the ready queue.  Returns the thread identifier
   for the new thread, or TID_ERROR if creation fails.

   If thread_start() has been called, then the new thread may be
   scheduled before thread_create() returns.  It could even exit
   before thread_create() returns.  Contrariwise, the original
   thread may run for any amount of time before the new thread is
   scheduled.  Use a semaphore or some other form of
   synchronization if you need to ensure ordering.

   The code provided sets the new thread's `priority' member to
   PRIORITY, but no actual priority scheduling is implemented.
   Priority scheduling is the goal of Problem 1-3. */
tid_t thread_create(const char *name, int priority,
					thread_func *function, void *aux)
{
	struct thread *t;
	tid_t tid;

	ASSERT(function != NULL);

	/* Allocate thread. */
	t = palloc_get_page(PAL_ZERO);
	if (t == NULL)
		return TID_ERROR;

	/* Initialize thread. */
	init_thread(t, name, priority);
	tid = t->tid = allocate_tid();

	/* Call the kernel_thread if it scheduled.
	 * Note) rdi is 1st argument, and rsi is 2nd argument. */
	t->tf.rip = (uintptr_t)kernel_thread;
	t->tf.R.rdi = (uint64_t)function;
	t->tf.R.rsi = (uint64_t)aux;
	t->tf.ds = SEL_KDSEG;
	t->tf.es = SEL_KDSEG;
	t->tf.ss = SEL_KDSEG;
	t->tf.cs = SEL_KCSEG;
	t->tf.eflags = FLAG_IF;

	/* Add to run queue. */
	thread_unblock(t);

	/*pintos project7*/
	/*생성된 스레드의 우선순위가 현재 실행중인 스레드의 우선순위보다 높다면 CPU를 양보*/
	if (t->priority > thread_current()->priority)
		thread_yield(); //t의 우선순위가 더 크면 thread_yield함수를 실행해 cpu 양보

	return tid;
}

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_unblock().

   This function must be called with interrupts turned off.  It
   is usually a better idea to use one of the synchronization
   primitives in synch.h. */
void thread_block(void)
{
	ASSERT(!intr_context());
	ASSERT(intr_get_level() == INTR_OFF);
	thread_current()->status = THREAD_BLOCKED;
	schedule();
}

/* Transitions a blocked thread T to the ready-to-run state.
   This is an error if T is not blocked.  (Use thread_yield() to
   make the running thread ready.)

   This function does not preempt the running thread.  This can
   be important: if the caller had disabled interrupts itself,
   it may expect that it can atomically unblock a thread and
   update other data. */
void thread_unblock(struct thread *t)
{
	enum intr_level old_level;

	ASSERT(is_thread(t));

	old_level = intr_disable();
	ASSERT(t->status == THREAD_BLOCKED);
	// list_push_back(&ready_list, &t->elem); //pintos project1에서 했던 코드인데, 이건 그냥 맨 끝에 추가인 코드임
	list_insert_ordered(&ready_list, &t->elem, cmp_priority, NULL); /*pintos project7*/
	// → thread unblock될 때 우선순위 순으로 정렬되어 ready_list에 삽입되도록 수정
	t->status = THREAD_READY;
	intr_set_level(old_level);
}

/*pintos project7*/
bool cmp_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED){
	/*list_insert_ordered 함수에서 사용하기 위한 정렬 방법을 결정하는 함수 작성*/
	struct thread *thread_a = list_entry(a, struct thread, elem);
	struct thread *thread_b = list_entry(b, struct thread, elem);
	// 두 스레드를 각각 받아 우선순위를 비교하는 cmp_priority 함수를 추가

	if(thread_a != NULL && thread_b != NULL){
		if (thread_a->priority > thread_b->priority)
			return true; //a의 우선순위가 높으면 1을
		else
			return false; //b의 우선순위가 높은 0을 리턴
	}

	return false;
}

/* Returns the name of the running thread. */
const char *thread_name(void)
{
	return thread_current()->name;
}

/* Returns the running thread.
   This is running_thread() plus a couple of sanity checks.
   See the big comment at the top of thread.h for details. */
struct thread *thread_current(void)
{
	struct thread *t = running_thread();

	/* Make sure T is really a thread.
	   If either of these assertions fire, then your thread may
	   have overflowed its stack.  Each thread has less than 4 kB
	   of stack, so a few big automatic arrays or moderate
	   recursion can cause stack overflow. */
	ASSERT(is_thread(t));
	ASSERT(t->status == THREAD_RUNNING);

	return t;
}

/* Returns the running thread's tid. */
tid_t thread_tid(void)
{
	return thread_current()->tid;
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
void thread_exit(void)
{
	ASSERT(!intr_context());

#ifdef USERPROG
	process_exit();
#endif

	/* Just set our status to dying and schedule another process.
	   We will be destroyed during the call to schedule_tail(). */
	intr_disable();
	do_schedule(THREAD_DYING);
	NOT_REACHED();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void thread_yield(void)
{
	struct thread *curr = thread_current();
	enum intr_level old_level;

	ASSERT(!intr_context());

	old_level = intr_disable();
	if (curr != idle_thread)
		// list_push_back(&ready_list, &curr->elem); //pintos project1에서 사용하던 기존 코드 수정
		list_insert_ordered(&ready_list, &curr->elem, cmp_priority,NULL); /*pintos project7*/
		// → 현재 수행중인 스레드가 cpu를 양보할 때도 마찬가지로 수정해 우선순위에 따라 정렬되어 삽입
		// 이때 사용한 cmp_priority 함수는 ready_list의 우선순위가 높으면1, curr.elem의 우선순위가 높으면 0 반환
	do_schedule(THREAD_READY);
	intr_set_level(old_level);
}

/**
 * thread_sleep - alarm clock 구현을 위해
*/
void thread_sleep(int64_t ticks)
{
	struct thread *curr = thread_current();
	enum intr_level old_level;

	ASSERT(!intr_context());

	old_level = intr_disable(); // 현재 스레드가 idle 스레드가 아닐 경우

	curr->wakeup_tick = ticks;

	if (curr != idle_thread){
		list_push_back(&sleep_list, &curr->elem);
	}
	update_next_tick_to_awake(ticks);
	do_schedule(THREAD_BLOCKED);
	intr_set_level(old_level);
}

/**
 * thread_awake - sleep_list에서 현재 ticks 기준으로 깨워야 할 스레드 깨움
*/
void thread_awake(int64_t ticks_curr)
{
	next_tick_to_awake = INT64_MAX;
	struct list_elem *it = list_begin(&sleep_list);
	struct thread *t;
	for (it; it != list_end(&sleep_list);)
	{
		t = list_entry(it, struct thread, elem);

		if (t->wakeup_tick <= ticks_curr)
		{
			it = list_remove(&t->elem);
			thread_unblock(t);
		}
		else
		{
			update_next_tick_to_awake(t->wakeup_tick);
			it = list_next(it);
		}
	}
}

/**
 * update_next_tick_to_awake - next_tick_to_awake
*/
void update_next_tick_to_awake(int64_t ticks)
{
	/* next_tick_to_awake 가 깨워야 할 스레드중 가장 작은 tick을 갖도록 업데이트 한다 */
	next_tick_to_awake = MIN(next_tick_to_awake, ticks);
}

/**
 * get_next_tick_to_awake - 
*/
int64_t get_next_tick_to_awake(void)
{
	return next_tick_to_awake;
}

/* Sets the current thread's priority to NEW_PRIORITY. */
void thread_set_priority(int new_priority)
{
	// thread_current()->priority = new_priority;
	// → 기존 코드 = 단순히 새로운 우선순위 값을 스레드에 쓰는 방식을 아래와 같이 수정

	// /*project7 +
	// 스레드가 우선순위가 변경되었을 때 우선순위에 따라 선점이 발생하도록*/
	// if(!thread_mlfqs){ //우선순위가 조정되는 mlfqs가 발생하면
	// 	int previous_priority = thread_current()->priority; // 현재 우선순위 값을
	// 	thread_current()->priority = new_priority; // 새로 받은 new_priority로 변경

	// 	if(thread_current()->priority < previous_priority) //만약 현재 cpu에서 수행중인 스레드의 우선순위가 낮아졌다면
	// 		test_max_priority(); //ready_list의 스레드들과 우선순위를 비교할 수 있게 test_max_priority 함수를 호출
	// }

	/*pintos project9*/
	/* donation 을 고려하여 thread_set_priority() 함수를 수정한다 */
	/* refresh_priority() 함수를 사용하여 우선순위를 변경으로 인한 donation 관련 정보를 갱신한다.
	donation_priority(), test_max_pariority() 함수를 적절히 사용하여 priority donation 을 수행하고 스케줄링 한다. */
	thread_current()->init_priority = new_priority;

	refresh_priority();
	thread_test_preemption();
}

/*project7*/
void test_max_priority(void){
	/*ready_list에서 우선순위가 가장 높은 스레드와 현재 스레드의 우선순위를 비교해 스케줄링
	(ready_list가 비어있지 않은지 확인)*/
	struct thread *cp = thread_current(); //cp는 현재 cpu에서 수행중인 스레드
	struct thread *first_thread; //ready_list의 맨 앞에 있는 스레드

	if(list_empty(&ready_list))
		return;

	first_thread = list_entry(list_front(&ready_list), struct thread, elem);
	// → 앞선 코드에서 ready_list 삽입 방식을 우선순위 값에 따라 정렬하여 넣는 것으로 수정했기에,
	// first_thread가 곧 ready_list의 모든 thread 중 가장 높은 우선순위를 가진 thread

	if(cp->priority < first_thread->priority) //cp와 first를 비교해 first의 우선순위가 더 높으면
		thread_yield(); //cpu를 양보하는 yield함수를 호출
}

/*pintos project9*/
bool thread_compare_donate_priority(const struct list_elem *l, const struct list_elem *s, void *aux UNUSED){
	return list_entry(l, struct thread, donation_elem)->priority > list_entry(s, struct thread, donation_elem)->priority;
}

/* priority donation 을 수행하는 함수를 구현한다.
현재 스레드가 기다리고 있는 lock 과 연결 된 모든 스레드들을 순회하며
현재 스레드의 우선순위를 lock 을 보유하고 있는 스레드에게 기부 한다.
(Nested donation 그림 참고, nested depth 는 8로 제한한다. ) */
void donate_priority(void){
	int depth;
	struct thread *cur = thread_current();

	for (depth = 0; depth < 8; depth++){
		if(!cur->wait_on_lock)
			break;
		struct thread *holder = cur->wait_on_lock->holder;
		holder->priority = cur->priority;
		cur = holder;
	}
}

/* lock 을 해지 했을때 donations 리스트에서 해당 엔트리를 삭제 하기 위한 함수를 구현한다. */
/* 현재 스레드의 donations 리스트를 확인하여 해지 할 lock을 보유하고 있는 엔트리를 삭제 한다. */
void remove_with_lock (struct lock *lock){
	struct list_elem *e;
	struct thread *cur = thread_current ();

	for (e = list_begin (&cur->donations); e != list_end (&cur->donations); e = list_next (e)){
		struct thread *t = list_entry (e, struct thread, donation_elem);
		if (t->wait_on_lock == lock)
			list_remove (&t->donation_elem);
	}
}

/* 스레드의 우선순위가 변경 되었을때 donation 을 고려하여 우선순위를 다시 결정 하는 함수를 작성 한다. */
/* 현재 스레드의 우선순위를 기부받기 전의 우선순위로 변경 */
/* 가장 우선수위가 높은 donations 리스트의 스레드와 현재 스레드의 우선순위를 비교하여 높은 값을 현재 스레드의 우선순위로 설정한다. */
void refresh_priority (void){
	struct thread *cur = thread_current ();

	cur->priority = cur->init_priority;
  
	if (!list_empty (&cur->donations)) {
    	list_sort (&cur->donations, thread_compare_donate_priority, 0);
		struct thread *front = list_entry (list_front (&cur->donations), struct thread, donation_elem);
    	if (front->priority > cur->priority)
    		cur->priority = front->priority;
	}
}

/*pintos project8*/
void thread_test_preemption (void)
{
    if (!list_empty (&ready_list) && thread_current ()->priority < list_entry (list_front (&ready_list), struct thread, elem)->priority)
    	thread_yield ();
}

/* Returns the current thread's priority. */
int thread_get_priority(void)
{
	return thread_current()->priority;
}

/* Sets the current thread's nice value to NICE. */
void thread_set_nice(int nice UNUSED)
{
	/* TODO: Your implementation goes here */
}

/* Returns the current thread's nice value. */
int thread_get_nice(void)
{
	/* TODO: Your implementation goes here */
	return 0;
}

/* Returns 100 times the system load average. */
int thread_get_load_avg(void)
{
	/* TODO: Your implementation goes here */
	return 0;
}

/* Returns 100 times the current thread's recent_cpu value. */
int thread_get_recent_cpu(void)
{
	/* TODO: Your implementation goes here */
	return 0;
}

/* Idle thread.  Executes when no other thread is ready to run.

   The idle thread is initially put on the ready list by
   thread_start().  It will be scheduled once initially, at which
   point it initializes idle_thread, "up"s the semaphore passed
   to it to enable thread_start() to continue, and immediately
   blocks.  After that, the idle thread never appears in the
   ready list.  It is returned by next_thread_to_run() as a
   special case when the ready list is empty. */
static void
idle(void *idle_started_ UNUSED)
{
	struct semaphore *idle_started = idle_started_;

	idle_thread = thread_current();
	sema_up(idle_started);

	for (;;)
	{
		/* Let someone else run. */
		intr_disable();
		thread_block();

		/* Re-enable interrupts and wait for the next one.

		   The `sti' instruction disables interrupts until the
		   completion of the next instruction, so these two
		   instructions are executed atomically.  This atomicity is
		   important; otherwise, an interrupt could be handled
		   between re-enabling interrupts and waiting for the next
		   one to occur, wasting as much as one clock tick worth of
		   time.

		   See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
		   7.11.1 "HLT Instruction". */
		asm volatile("sti; hlt"
					 :
					 :
					 : "memory");
	}
}

/* Function used as the basis for a kernel thread. */
static void
kernel_thread(thread_func *function, void *aux)
{
	ASSERT(function != NULL);

	intr_enable(); /* The scheduler runs with interrupts off. */
	function(aux); /* Execute the thread function. */
	thread_exit(); /* If function() returns, kill the thread. */
}

/* Does basic initialization of T as a blocked thread named
   NAME. */
static void init_thread(struct thread *t, const char *name, int priority){
	ASSERT(t != NULL);
	ASSERT(PRI_MIN <= priority && priority <= PRI_MAX);
	ASSERT(name != NULL);

	memset(t, 0, sizeof *t);
	t->status = THREAD_BLOCKED;
	strlcpy(t->name, name, sizeof t->name);
	t->tf.rsp = (uint64_t)t + PGSIZE - sizeof(void *);
	t->priority = priority;
	t->magic = THREAD_MAGIC;

	/*pintos project9 + Priority donation관련 자료구조 초기화*/
	t->init_priority = priority;
	t->wait_on_lock = NULL;
	list_init(&t->donations);
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
static struct thread *
next_thread_to_run(void)
{
	if (list_empty(&ready_list))
		return idle_thread;
	else
		return list_entry(list_pop_front(&ready_list), struct thread, elem);
}

/* Use iretq to launch the thread */
void do_iret(struct intr_frame *tf)
{
	__asm __volatile(
		"movq %0, %%rsp\n"
		"movq 0(%%rsp),%%r15\n"
		"movq 8(%%rsp),%%r14\n"
		"movq 16(%%rsp),%%r13\n"
		"movq 24(%%rsp),%%r12\n"
		"movq 32(%%rsp),%%r11\n"
		"movq 40(%%rsp),%%r10\n"
		"movq 48(%%rsp),%%r9\n"
		"movq 56(%%rsp),%%r8\n"
		"movq 64(%%rsp),%%rsi\n"
		"movq 72(%%rsp),%%rdi\n"
		"movq 80(%%rsp),%%rbp\n"
		"movq 88(%%rsp),%%rdx\n"
		"movq 96(%%rsp),%%rcx\n"
		"movq 104(%%rsp),%%rbx\n"
		"movq 112(%%rsp),%%rax\n"
		"addq $120,%%rsp\n"
		"movw 8(%%rsp),%%ds\n"
		"movw (%%rsp),%%es\n"
		"addq $32, %%rsp\n"
		"iretq"
		:
		: "g"((uint64_t)tf)
		: "memory");
}

/* Switching the thread by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.

   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.

   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function. */
static void
thread_launch(struct thread *th)
{
	uint64_t tf_cur = (uint64_t)&running_thread()->tf;
	uint64_t tf = (uint64_t)&th->tf;
	ASSERT(intr_get_level() == INTR_OFF);

	/* The main switching logic.
	 * We first restore the whole execution context into the intr_frame
	 * and then switching to the next thread by calling do_iret.
	 * Note that, we SHOULD NOT use any stack from here
	 * until switching is done. */
	__asm __volatile(
		/* Store registers that will be used. */
		"push %%rax\n"
		"push %%rbx\n"
		"push %%rcx\n"
		/* Fetch input once */
		"movq %0, %%rax\n"
		"movq %1, %%rcx\n"
		"movq %%r15, 0(%%rax)\n"
		"movq %%r14, 8(%%rax)\n"
		"movq %%r13, 16(%%rax)\n"
		"movq %%r12, 24(%%rax)\n"
		"movq %%r11, 32(%%rax)\n"
		"movq %%r10, 40(%%rax)\n"
		"movq %%r9, 48(%%rax)\n"
		"movq %%r8, 56(%%rax)\n"
		"movq %%rsi, 64(%%rax)\n"
		"movq %%rdi, 72(%%rax)\n"
		"movq %%rbp, 80(%%rax)\n"
		"movq %%rdx, 88(%%rax)\n"
		"pop %%rbx\n" // Saved rcx
		"movq %%rbx, 96(%%rax)\n"
		"pop %%rbx\n" // Saved rbx
		"movq %%rbx, 104(%%rax)\n"
		"pop %%rbx\n" // Saved rax
		"movq %%rbx, 112(%%rax)\n"
		"addq $120, %%rax\n"
		"movw %%es, (%%rax)\n"
		"movw %%ds, 8(%%rax)\n"
		"addq $32, %%rax\n"
		"call __next\n" // read the current rip.
		"__next:\n"
		"pop %%rbx\n"
		"addq $(out_iret -  __next), %%rbx\n"
		"movq %%rbx, 0(%%rax)\n" // rip
		"movw %%cs, 8(%%rax)\n"	 // cs
		"pushfq\n"
		"popq %%rbx\n"
		"mov %%rbx, 16(%%rax)\n" // eflags
		"mov %%rsp, 24(%%rax)\n" // rsp
		"movw %%ss, 32(%%rax)\n"
		"mov %%rcx, %%rdi\n"
		"call do_iret\n"
		"out_iret:\n"
		:
		: "g"(tf_cur), "g"(tf)
		: "memory");
}

/* Schedules a new process. At entry, interrupts must be off.
 * This function modify current thread's status to status and then
 * finds another thread to run and switches to it.
 * It's not safe to call printf() in the schedule(). */
static void do_schedule(int status)
{
	ASSERT(intr_get_level() == INTR_OFF);
	ASSERT(thread_current()->status == THREAD_RUNNING);
	while (!list_empty(&destruction_req))
	{
		struct thread *victim = list_entry(list_pop_front(&destruction_req), struct thread, elem);
		palloc_free_page(victim);
	}
	thread_current()->status = status;
	schedule();
}

/** ready_queue의 맨 앞에 있던 스레드 running */
static void schedule(void)
{
	struct thread *curr = running_thread();
	struct thread *next = next_thread_to_run();

	ASSERT(intr_get_level() == INTR_OFF);
	ASSERT(curr->status != THREAD_RUNNING);
	ASSERT(is_thread(next));
	/* Mark us as running. */
	next->status = THREAD_RUNNING;

	/* Start new time slice. */
	thread_ticks = 0;

#ifdef USERPROG
	/* Activate the new address space. */
	process_activate(next);
#endif

	if (curr != next)
	{
		/* If the thread we switched from is dying, destroy its struct
		   thread. This must happen late so that thread_exit() doesn't
		   pull out the rug under itself.
		   We just queuing the page free reqeust here because the page is
		   currently used bye the stack.
		   The real destruction logic will be called at the beginning of the
		   schedule(). */
		if (curr && curr->status == THREAD_DYING && curr != initial_thread)
		{
			ASSERT(curr != next);
			list_push_back(&destruction_req, &curr->elem);
		}

		/* Before switching the thread, we first save the information
		 * of current running. */
		thread_launch(next);
	}
}

/* Returns a tid to use for a new thread. */
static tid_t
allocate_tid(void)
{
	static tid_t next_tid = 1;
	tid_t tid;

	lock_acquire(&tid_lock);
	tid = next_tid++;
	lock_release(&tid_lock);

	return tid;
}
