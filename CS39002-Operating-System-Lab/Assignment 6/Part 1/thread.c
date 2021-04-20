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
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
//#ifdef USERPROG
#include "userprog/process.h"
//#endif

/* Random value for struct thread's `magic' member.
   Used to detect stack overflow.  See the big comment at the top
   of thread.h for details. */
#define THREAD_MAGIC 0xcd6abf4b

/* List of processes in THREAD_READY state, that is, processes
   that are ready to run but not actually running. */
static struct list ready_list;

/* List of all processes.  Processes are added to this list
   when they are first scheduled and removed when they exit. */
static struct list all_list;

/* Idle thread. */
static struct thread *idle_thread;

/* Initial thread, the thread running init.c:main(). */
static struct thread *initial_thread;

/* Wakeup thread, wakes up all the sleeping threads. */
static struct thread *wakeup_thread;

/* Lock used by allocate_tid(). */
static struct lock tid_lock;

/* Lock used for updating child threads */
//struct lock main_lock;

/* List of threads in THREAD_BLOCKED state, i.e. processes that
   sleeping and waiting to be ready once the ticks reach the wake_tick */
static struct list sleeping_threads;

/* Stack frame for kernel_thread(). */
struct kernel_thread_frame 
  {
    void *eip;                  /* Return address. */
    thread_func *function;      /* Function to call. */
    void *aux;                  /* Auxiliary data for function. */
  };

/* Statistics. */
static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */

/* Scheduling. */
#define TIME_SLICE 4            /* # of timer ticks to give each thread. */
static unsigned thread_ticks;   /* # of timer ticks since last yield. */
static unsigned total_ticks;    /* # of timer ticks since OS booted. */
static unsigned min_waketick;   /* wake_tick value of the 1st thread in sleeping_threads list
                                   if no thread in list, then maximum*/

static int load_avg;            /* system wide load average*/

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
bool thread_mlfqs;

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static void thread_wakeup (void* aux);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority, int nice);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void thread_schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);
static bool list_less_sleep (const struct list_elem *t1,
                             const struct list_elem *t2,
                             void *aux);

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
void
thread_init (void) 
{
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock);
  lock_init (&main_lock);
  lock_init (&filesys_lock);

  // initialize load_avg to 0
  if(thread_mlfqs)
  {
    load_avg_init();
  }

  // initialize ready_list and all_list
  list_init (&ready_list);
  list_init (&all_list);

  /* Set up a thread structure for the running thread. */
  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT, NICE_DEFAULT);
  initial_thread->status = THREAD_RUNNING;
  initial_thread->tid = allocate_tid ();

  // acquire main_lock to assign the initial values to the members of initial thread
  lock_acquire(&main_lock);
  
  initial_thread->parent = NULL;
  initial_thread->success = false;
  list_init(&(initial_thread->waiting_threads));
  initial_thread->child_shm = NULL;

  sema_init(&(initial_thread->load),0);
  // release main_lock
  lock_release(&main_lock);

  // initialize min_waketick for wakeup_thread
  // default set to wake at system INFINITE time (INT32_MAX)
  min_waketick = INT32_MAX;
  list_init(&sleeping_threads);
}

/* Starts preemptive thread scheduling by enabling interrupts.
   Also creates the idle thread. */
void
thread_start (void) 
{
  /* Create the idle thread. */
  struct semaphore idle_started;
  sema_init (&idle_started, 0);
  thread_create ("idle", PRI_MIN, idle, &idle_started);

  /* Create the wakeup thread. */
  tid_t tid = thread_create ("wakeup", PRI_MAX, thread_wakeup, NULL);
  if(tid==TID_ERROR)
    printf("Error in creating wakeup thread..\n");

  /* Start preemptive thread scheduling. */
  intr_enable ();

  /* Wait for the idle thread to initialize idle_thread. */
  sema_down (&idle_started);
}

/* Called by the timer interrupt handler at each timer tick.
   Thus, this function runs in an external interrupt context. */
void
thread_tick (void) 
{
  // obtain the current thread
  struct thread *t = thread_current ();

  /* Update statistics. */
  if (t == idle_thread)
    idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;
  total_ticks++;

  // obtain the least wake_tick value in the sleeping_threads
  // update the min_waketick if it is less than the least wake_tick value as obtained above
  struct list_elem *first_elem = list_begin(&sleeping_threads);
  if(first_elem!=NULL)
  {
    struct thread* first_sleeping_thread = list_entry(first_elem, struct thread, sleepelem);
    if (first_sleeping_thread!=NULL && first_sleeping_thread->wake_tick>0
        && first_sleeping_thread->wake_tick<min_waketick)
    {
      min_waketick = first_sleeping_thread->wake_tick;
    }
  }
  
  /* if mlfqs scheduling */
  if(thread_mlfqs)
  {
    /* increment recent_cpu of current thread */
    if(strcmp(thread_current()->name,"idle")!=0)
      increment_recent_cpu(thread_current());

    /* update recent_cpu-> occurs every second */
    if(total_ticks%100 == 0)
    {
      update_load_avg();
      thread_foreach(update_recent_cpu, NULL);
    }
    /* update priority for each thread after each TIME_SLICE */
    if(total_ticks%4 == 0)
    {
      thread_foreach(update_priority, NULL);
      sort_ready_list();
    }
  }

  /* wake the wakeup thread */
  if(total_ticks==min_waketick && wakeup_thread->status==THREAD_BLOCKED)
  {
    thread_unblock(wakeup_thread);
  }

  /* Enforce preemption only if current thread is not wakeup_thread */
  if (++thread_ticks >= TIME_SLICE && t!=wakeup_thread)
    intr_yield_on_return ();
}

/* Prints thread statistics. */
void
thread_print_stats (void) 
{
  printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
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
tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux) 
{
  //printf("Creating thread %s\n",name);
  struct thread *cur = thread_current();
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;
  enum intr_level old_level;

  ASSERT (function != NULL);

  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  /**
  * If parent thread not NULL
  * then inherit the nice value of parent
  * else set the nice value to be NICE_DEFAULT
  */
  int nice = cur!=NULL ? cur->nice : NICE_DEFAULT;
  init_thread(t, name, priority, nice);
  tid = t->tid = allocate_tid ();

  /* Prepare thread for first run by initializing its stack.
     Do this atomically so intermediate values for the 'stack' 
     member cannot be observed. */
  old_level = intr_disable ();

  /* Stack frame for kernel_thread(). */
  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  /* Stack frame for switch_entry(). */
  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;

  /* Stack frame for switch_threads(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;
  sf->ebp = 0;


  intr_set_level (old_level);



  /* Add to run queue. */
  thread_unblock (t);

  // acquire main_lock to update share memory and user program related memebers
  lock_acquire(&main_lock);
  struct thread* parent = thread_current();
  

  t->parent = parent;
  t->success = false;

  // initialize the semaphore to control the loading of child thread
  sema_init(&(t->load),0);
  // initialize the waiting thread list of the child
  list_init(&(t->waiting_threads));
  // create the shared child_shm
  t->child_shm = malloc(sizeof(struct waiting_thread));
  // assign the initial value to various members of child_shm
  if(parent!=NULL)
    t->child_shm->parent_tid = parent->tid;
  t->child_shm->child_tid = t->tid;
  t->child_shm->exitted = 0;
  t->child_shm->exit_status = -1;
  // initialize the wait_lock semaphore
  // this enables the parent to wait till the child exits
  sema_init(&(t->child_shm->wait_lock),0);
  if(t->parent!=NULL)
  {
    list_push_back(&(t->parent->waiting_threads),&(t->child_shm->waitelem));
  }

  // release main_lock
  lock_release(&main_lock);


  /* If new thread created has higher priority
  pre-empt the current thread */
  if(t->priority > thread_current()->priority)
    thread_yield();

  return tid;
}

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_unblock().
   This function must be called with interrupts turned off.  It
   is usually a better idea to use one of the synchronization
   primitives in synch.h. */
void
thread_block (void) 
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == INTR_OFF);

  thread_current ()->status = THREAD_BLOCKED;
  schedule ();
}

/* Transitions a blocked thread T to the ready-to-run state.
   This is an error if T is not blocked.  (Use thread_yield() to
   make the running thread ready.)
   This function does not preempt the running thread.  This can
   be important: if the caller had disabled interrupts itself,
   it may expect that it can atomically unblock a thread and
   update other data. */
void
thread_unblock (struct thread *t) 
{
  enum intr_level old_level;

  ASSERT (is_thread (t));

  old_level = intr_disable ();
  ASSERT (t->status == THREAD_BLOCKED);
  /* unblock the input thread
  insert the thread to ready_list in order of priority 
  set the thread status to THREAD_READY */
  list_insert_ordered(&ready_list, &(t->elem), priority_list_less_func, NULL);
  t->status = THREAD_READY;
  intr_set_level (old_level);
}

/* Returns the name of the running thread. */
const char *
thread_name (void) 
{
  return thread_current ()->name;
}

/* Returns the running thread.
   This is running_thread() plus a couple of sanity checks.
   See the big comment at the top of thread.h for details. */
struct thread *
thread_current (void) 
{
  struct thread *t = running_thread ();
  
  /* Make sure T is really a thread.
     If either of these assertions fire, then your thread may
     have overflowed its stack.  Each thread has less than 4 kB
     of stack, so a few big automatic arrays or moderate
     recursion can cause stack overflow. */
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_RUNNING);

  return t;
}

/* Returns the running thread's tid. */
tid_t
thread_tid (void) 
{
  return thread_current ()->tid;
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
void
thread_exit (void) 
{
  ASSERT (!intr_context ());

#ifdef USERPROG
  process_exit ();
#endif

  /* Remove thread from all threads list, set our status to dying,
     and schedule another process.  That process will destroy us
     when it calls thread_schedule_tail(). */
  intr_disable ();
  list_remove (&thread_current()->allelem);
  thread_current ()->status = THREAD_DYING;
  schedule ();
  NOT_REACHED ();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void
thread_yield (void) 
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  /* insert the current thread into ready_list respecting priority order */
  if (cur != idle_thread)
    list_insert_ordered(&ready_list, &cur->elem, priority_list_less_func, NULL);

  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

/* puts the current thread into sleeping_threads list. */
void
thread_sleep (int64_t ticks)
{
  if (ticks<=0)
    return;
  // ticks since OS boot time
  int64_t start = total_ticks;
  // get current thread
  struct thread* curr = thread_current();
  // set the wake_tick of current thread
  curr->wake_tick = start + ticks;
  // insert current thread into sleeping_threads list in increasing order of wake_tick
  list_insert_ordered(&sleeping_threads, &(curr->sleepelem), list_less_sleep, NULL);
  // block the current thread, i.e. make it waiting but not in ready queue
  thread_block();
}

/* Invoke function 'func' on all threads, passing along 'aux'.
   This function must be called with interrupts off. */
void
thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (&all_list); e != list_end (&all_list);
       e = list_next (e))
    {
      struct thread *t = list_entry (e, struct thread, allelem);
      func (t, aux);
    }
}

/* Sets the current thread's priority to NEW_PRIORITY. */
void
thread_set_priority (int new_priority) 
{
  enum intr_level old_level = intr_disable();
  /* In mlfqs scheduling a thread cannot set its own priority */
  if(!thread_mlfqs)
  {
    ASSERT (new_priority>=PRI_MIN && new_priority<=PRI_MAX);
    thread_current ()->priority = new_priority;
    if(!list_empty(&ready_list))
    {
      struct thread *front_thread = list_entry(list_front(&ready_list), struct thread, elem);
      if(front_thread->priority > thread_get_priority())
        thread_yield();
    }
  }
  intr_set_level(old_level);
}

/* Returns the current thread's priority. */
int
thread_get_priority (void) 
{
  return thread_current ()->priority;
}

/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice UNUSED) 
{
  enum intr_level old_level = intr_disable();
  struct thread *curr = thread_current();
  ASSERT (nice>=NICE_MIN && nice<=NICE_MAX);
  curr->nice = nice;

  /* update the priority of current thread 
  thread_yield if the updated priority is less than the highest priority in ready_list */
  update_priority(curr,NULL);
  if(!list_empty(&ready_list))
  {
    struct thread *front_thread = list_entry(list_front(&ready_list), struct thread, elem);
    if(front_thread->priority>=curr->priority && curr!=wakeup_thread && curr!=front_thread)
      thread_yield();
  }
  intr_set_level(old_level);
}

/* Returns the current thread's nice value. */
int
thread_get_nice (void) 
{
  return thread_current()->nice;
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void) 
{
  return to_int_round(load_avg*100);
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void) 
{
  return to_int_round(thread_current()->recent_cpu*100);
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
idle (void *idle_started_ UNUSED) 
{
  struct semaphore *idle_started = idle_started_;
  idle_thread = thread_current ();
  sema_up (idle_started);

  for (;;) 
    {
      /* Let someone else run. */
      intr_disable ();
      thread_block ();

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
      asm volatile ("sti; hlt" : : : "memory");
    }
}

/* thread_wakeup. Executes when the waketick value of any thread
   becomes equal to the current tick.
   The wakeup_thread executes this and wakes up all the threads
   in the sleeping_threads list whose wake_tick value is equal to 
   current tick. This thread has the highest priority 
   and hence is non-preemptive and hence an interrupt cannot cause
   its preemption and it executes atomically.
*/
static void thread_wakeup(void *wakeup_sema_ UNUSED)
{
  wakeup_thread = thread_current();
  while(1)
  {
    // wake up sleeping threads whose wake_tick value has been reached
    struct list_elem* curr;
    // traverse through sleeping_threads list upto the elem whose wake_tick = total_ticks
    for (curr = list_begin(&sleeping_threads); curr!=list_end(&sleeping_threads); curr=list_next(curr))
    {
      struct thread* curr_thread = list_entry(curr, struct thread, sleepelem);
      // break out of loop if curr thread still has ticks left to reach wake_tick because threads after this will have greater wake_tick
      if (curr_thread==NULL || curr_thread->wake_tick > total_ticks)
        break;
      // make wake_tick as 0 and unblock thread(move to ready queue)
      curr_thread->wake_tick = 0;
      thread_unblock(curr_thread);
      // remove current thread from list of sleeping_threads
      list_remove(curr);
    }
    curr = list_begin(&sleeping_threads);
    struct thread* min_wakethread = list_entry(curr, struct thread, sleepelem);
    // update min_waketick to know when to unblock the wakeup_thread next
    // else block it indefinitely
    if(min_wakethread!=NULL && min_wakethread->wake_tick>0 && min_wakethread->wake_tick<min_waketick)
      min_waketick = min_wakethread->wake_tick;
    else
      min_waketick = INT32_MAX;
    /* in order to block the wakeup_thread we have to disable interrupts */
    enum intr_level old_level = intr_disable();
    thread_block();
    intr_set_level(old_level);
  }
}

/* Function used as the basis for a kernel thread. */
static void
kernel_thread (thread_func *function, void *aux) 
{
  ASSERT (function != NULL);

  intr_enable ();       /* The scheduler runs with interrupts off. */
  function (aux);       /* Execute the thread function. */
  thread_exit ();       /* If function() returns, kill the thread. */
}

/* Returns the running thread. */
struct thread *
running_thread (void) 
{
  uint32_t *esp;

  /* Copy the CPU's stack pointer into `esp', and then round that
     down to the start of a page.  Because `struct thread' is
     always at the beginning of a page and the stack pointer is
     somewhere in the middle, this locates the curent thread. */
  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}

/* Returns true if T appears to point to a valid thread. */
static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/* Does basic initialization of T as a blocked thread named
   NAME. */
static void
init_thread (struct thread *t, const char *name, int priority, int nice)
{
  ASSERT (t != NULL);
  ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
  ASSERT (NICE_MIN <= nice && nice <= NICE_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  strlcpy (t->name, name, sizeof t->name);
  t->stack = (uint8_t *) t + PGSIZE;
  /* mlfqs scehduling */
  if(thread_mlfqs)
  {
    /* initialize the recent_cpu */
    if(strcmp(t->name,"main")==0)
      t->recent_cpu=0;
    else
      t->recent_cpu = thread_get_recent_cpu()/100;
    /* initialize the priority */
    t->priority = PRI_MAX - to_int_round((t->recent_cpu)/4) - ((t->nice)/2);
  }
  else
    t->priority = priority;

  /* initialize nice value s*/
  t->nice = nice;
  
  t->magic = THREAD_MAGIC;


  enum intr_level old_level = intr_disable();
  list_push_back (&all_list, &t->allelem);
  intr_set_level(old_level);
}

/* Allocates a SIZE-byte frame at the top of thread T's stack and
   returns a pointer to the frame's base. */
static void *
alloc_frame (struct thread *t, size_t size) 
{
  /* Stack data is always allocated in word-size units. */
  ASSERT (is_thread (t));
  ASSERT (size % sizeof (uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
static struct thread *
next_thread_to_run (void) 
{
  if (list_empty (&ready_list))
    return idle_thread;
  else
    return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/* Completes a thread switch by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.
   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.  This function is normally invoked by
   thread_schedule() as its final action before returning, but
   the first time a thread is scheduled it is called by
   switch_entry() (see switch.S).
   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function.
   After this function and its caller returns, the thread switch
   is complete. */
void
thread_schedule_tail (struct thread *prev)
{
  struct thread *cur = running_thread ();
  
  ASSERT (intr_get_level () == INTR_OFF);

  /* Mark us as running. */
  cur->status = THREAD_RUNNING;

  /* Start new time slice. */
  thread_ticks = 0;

#ifdef USERPROG
  /* Activate the new address space. */
  process_activate ();
#endif

  /* If the thread we switched from is dying, destroy its struct
     thread.  This must happen late so that thread_exit() doesn't
     pull out the rug under itself.  (We don't free
     initial_thread because its memory was not obtained via
     palloc().) */
  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread) 
    {
      ASSERT (prev != cur);
      palloc_free_page (prev);
    }
}

/* Schedules a new process.  At entry, interrupts must be off and
   the running process's state must have been changed from
   running to some other state.  This function finds another
   thread to run and switches to it.
   It's not safe to call printf() until thread_schedule_tail()
   has completed. */
static void
schedule (void) 
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);
}

/* Returns a tid to use for a new thread. */
static tid_t
allocate_tid (void) 
{
  static tid_t next_tid = 1;
  tid_t tid;

  lock_acquire (&tid_lock);
  tid = next_tid++;
  lock_release (&tid_lock);

  return tid;
}

/* convert integer to float in 17.14 format */
int to_fp(int x)
{
  return x*F1714;
}

/* multiply 2 integers in their 17.14 floating representation */
int mul_fp(int x,int y)
{
  return ((int64_t) x) * y / F1714;
}

/* divide 2 integers in their 17.14 floating representation */
int div_fp(int x,int y)
{
  return ((int64_t) x) * F1714 / y;
}

/* convert x to integer while rounding */
int to_int_round(int x)
{
  if(x>=0)
    return (x + (F1714)/2)/(F1714);
  else
    return (x - (F1714)/2)/(F1714);
}

/* initialize load average */
void
load_avg_init(void)
{
  load_avg = to_fp(0);
}

/* update recent cpu time for a thread */
void update_recent_cpu(struct thread* t,void *aux UNUSED)
{
  t->recent_cpu = mul_fp(div_fp(2* load_avg,2* load_avg +to_fp(1)),t->recent_cpu) + to_fp(t->nice);
}

/* increment recent cpu time for a thread */
void increment_recent_cpu(struct thread *t)
{
  t->recent_cpu = t->recent_cpu + to_fp(1);
}

/* update priority of thread, used in mlfqs */
void update_priority(struct thread *t,void *aux UNUSED)
{
  /* wakeup_thread cannot be preempted or have priority lowered */
  if(t==wakeup_thread)
    return;

  t->priority = PRI_MAX - to_int_round((t->recent_cpu)/4) - ((t->nice)*2);

  if (t->priority > PRI_MAX)
    t->priority = PRI_MAX;
  else if (t->priority < PRI_MIN)
    t->priority = PRI_MIN;
}

/* update load_avg */
void update_load_avg()
{
  load_avg = mul_fp(to_fp(59)/60,load_avg)+
             (to_fp(1)/60)*(thread_current()==idle_thread ? list_size(&ready_list):list_size(&ready_list)+1);
}

/* sort the ready queue */
void sort_ready_list()
{
  list_sort(&ready_list,priority_list_less_func,NULL);
}
/* list_less_func type of function. Compares wake_tick of list elements t1 & t2(embedded in struct thread).
 Returns true if wake_tick of t1 is less than wake_tick of t2 */
static bool list_less_sleep (const struct list_elem *t1,
                             const struct list_elem *t2,
                             void *aux UNUSED)
{
  struct thread* thread1 = list_entry(t1, struct thread, sleepelem);
  struct thread* thread2 = list_entry(t2, struct thread, sleepelem);
  if (thread1->wake_tick < thread2->wake_tick)
    return true;
  return false;
}

/* list_less_func type function. Compares priorities of ready queue elements t1&t2
  Returns true if priority of t1 is higher than priority of t2
  Used in priority scheduling */
bool priority_list_less_func (const struct list_elem *t1,
                             const struct list_elem *t2,
                             void *aux UNUSED)
{
  int priority1 = list_entry(t1, struct thread, elem)->priority;
  int priority2 = list_entry(t2, struct thread, elem)->priority;
  if (priority1 > priority2)
    return true;
  return false;
}


/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);

