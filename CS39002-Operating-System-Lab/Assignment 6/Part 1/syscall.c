#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include <user/syscall.h>
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"


#define MAX_ARGS 3
#define STD_INPUT 0
#define STD_OUTPUT 1

static void syscall_handler (struct intr_frame *);
void get_args (struct intr_frame *f, int *arg, int num_of_args);
void exit (int status);
pid_t exec(const char* cmd_line);
int read(int fd, void *buffer, unsigned leng);
int write (int fd, const void * buf, unsigned byte_size);
void validate_ptr (const void* vaddr);
void validate_str (const void* str);
void validate_buf (const void* buf, unsigned byte_size);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	// array to hold the arguments
  int args[MAX_ARGS];
  int esp = getpage_ptr((const void*)f->esp);
  switch(*(int*)esp)
  {
  	case SYS_EXIT:	
  		// exit the function
  	  get_args(f, &args[0], 1);
  	  // syscall exit
      exit(args[0]);
  	  break;
  	case SYS_EXEC:	
  	  //get the main command
  	  get_args(f, &args[0], 1);
  	  //check if the command line is valid
  	  validate_str((const void*)args[0]);
  	  //syscall exec
  	  f->eax = exec((const char*)args[0]);
  	  break;
  	case SYS_WAIT:
  		// get the main command
  	  get_args(f,&args[0],1);
  	  // syscall wait
  	  f->eax = wait((tid_t)args[0]);
  	  break;
  	case SYS_WRITE:
  	  //file args with the no of arguments needed
  	  get_args(f, &args[0], 3);
  	  // Check if the buffer is part of our virtual memory
  	  validate_buf((const void*)args[1],(unsigned)args[2]);
  	  //syscall write
  	  f->eax=write(args[0],(const void*)args[1],(unsigned)args[2]);
  	  break;
  	default:
  	  break;
  }
}

/*
Get arguments from stack
*/
void
get_args (struct intr_frame *f, int *args, int num_of_args)
{
  int i;
  int *ptr;
  // to retrieve more than one arguments
  // loop over the essential portion of intr_frame
  // check the validity of each ptr
  // add to args array
  if(num_of_args>1){
  for (i = 0; i < num_of_args; i++)
  {
    ptr = (int *) (f->esp + 20+4*i);							
    validate_ptr((const void *) ptr);
    args[i] = *ptr;
  }
  }
  else
  	// if only single argument to be retrieved
  	// retrieve directly
  	args[0]=*((int*)(f->esp+4));
}

/*
System call exit
Terminates the current user program, returning status to kernel
If the process's parent waits for it, it returns the status to it
*/
void 
exit (int status)
{
	// obtain the current thread to exit
	struct thread* curr = thread_current();
	if(status<0)
		status=-1;
	printf("%s: exit(%d)\n",curr->name, status);
	// obtain the shared child_shm
	struct waiting_thread *child_shm = curr->child_shm;
	// update relevant parameters of child_shm to denote
	// the status of exit, and exitted or not
	child_shm->exit_status =status;
	child_shm->exitted = 1;
	// sema up the wait_lock 
	// parent process can stop waiting for the child
	sema_up(&(child_shm->wait_lock));
	thread_exit();
	
}

/*
System call exec
Runs the executable whose name is given in cmd_line,
passing any arguments, and returns new process's pid
*/

pid_t
exec (const char *cmd_line)
{
	tid_t tid = process_execute(cmd_line);
	if(tid==TID_ERROR)														
	{
		// error in execution
		return -1;
	}
	// wait for the loading of the child thread
	sema_down(&(thread_current()->load));					
	if(thread_current()->success==true)
		// if successfully executed
		return tid;
	else
		return -1;

}

/*
Waits for thread tid to die and return its exit status
*/
int
wait (tid_t tid)
{
	return(process_wait(tid));
}
/*
System call read
*/
int
read (int fd, void* buf, unsigned len)
{
	if (len <= 0)
		// read positive length 
		return len;

	if (fd == STD_INPUT)
	{
		uint8_t *local_buf = (uint8_t *) buf;
		unsigned i=0;
		for(; i <len; i++)
		{
			/* retireve key press from input buffer */
			local_buf[i] = input_getc();
		}
		return len;
	}
}
/*
System call write
*/
int
write (int fd, const void* buf, unsigned byte_size)
{
	if (byte_size <= 0)
		// nothing to write
		return byte_size;
	if (fd == STD_OUTPUT)
	{
		// check if buffer is in user program's virtual memory
		validate_buf(buf,byte_size);
		// write byte_size character from buf to console
		putbuf (buf, byte_size);
		return byte_size;
	}
	return byte_size;
}

/*
Check if a virtual address is valid
*/
void
validate_ptr (const void *vaddr)
{
	if (vaddr < USER_VADDR_BOTTOM || !is_user_vaddr(vaddr))
	{
		// virtual memory address out of bound 
		// or not a part of user virtual address space
		exit(ERROR);
	}
}
/*
Check if a string is valid
*/
void
validate_str (const void* str)
{
	// validate char by char
	for(; *(char*)getpage_ptr(str)!=0; str=(char*)str+1);
}
/*
Check if buffer is valid
*/
void
validate_buf(const void* buf, unsigned byte_size)
{
	unsigned i=0;
	char* local_buf = (char*)buf;
	// validate each pointer which is a part of buf
	for(;i<byte_size;i++)
	{
		validate_ptr((const void*)local_buf);
		local_buf+=4;
	}
}
/*
Get pointer to page
*/
int
getpage_ptr(const void *vaddr)
{
	void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
	if(!ptr)
		exit(ERROR);
	return (int)ptr;
}