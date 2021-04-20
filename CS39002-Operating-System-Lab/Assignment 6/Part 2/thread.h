#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "synch.h"

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

#define F1714 16384     /* f=2^14 for converting between int and float
                           in 17.14 format */

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* Thread nice values */
#define NICE_DEFAULT 0                  /* Default nice value. */
#define NICE_MIN -20                    /* Lowest nice value. */
#define NICE_MAX 20                     /* Highest nice value. */

/* shared memory for parent and child thread */
struct waiting_thread {
  tid_t parent_tid;
  tid_t child_tid;
  struct semaphore wait_lock;
  struct list_elem waitelem;
  int exit_status;
  int exitted;
};

/* information about file  */
struct file_info {
  struct file* file;
  int fd;
  struct list_elem fileelem;
};

/* A kernel thread or user process.
   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:
        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+
   The upshot of this is twofold:
      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.
      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.
   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */

struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    int nice;                           /* Nice value. */
    int recent_cpu;                     /* recent cpu time. */
    struct list_elem allelem;           /* List element for all threads list. */
    
    int64_t wake_tick;                  /* tick when a sleeping thread will wake up */
    struct list_elem sleepelem;         /* list element for all sleeping threads list */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
    struct thread* parent;              /* parent thread */
    bool success;                       /* for exec */
    struct semaphore load;              /* load semaphore */

    struct waiting_thread* child_shm;   /* waiting_thread shared memory of current thread as a child */
    struct list waiting_threads;        /* list of waiting_threads of current thread as a parent */

    struct list file_list;              /* list of files */
    int fd;                             /* file descriptor to assign to new open files */

    struct file* exec_file;             /* executable file of this thread */
    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

/* If false (default), use default priority scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;
struct lock main_lock;
struct lock filesys_lock;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

void thread_sleep(int64_t ticks);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);


int to_fp(int x);
int mul_fp(int x,int y);
int div_fp(int x,int y);
int to_int_round(int x);

void update_load_avg(void);

void load_avg_init(void);

void update_recent_cpu(struct thread* t,void *aux);

void increment_recent_cpu(struct thread* t);


void update_priority(struct thread *t,void *aux);

bool priority_list_less_func(const struct list_elem *t1,
                                const struct list_elem *t2,
                                void *aux);

void sort_ready_list(void);

#endif /* threads/thread.h */
