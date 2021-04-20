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
bool create(const char* file_name, unsigned initial_size);
bool remove(const char* file_name);
int open(const char* file_name);
int read(int fd, void *buffer, unsigned size);
int write (int fd, const void * buf, unsigned size);
void close(int fd);
struct file* get_file(int fd);
void close_file(int fd);
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
  	  // get the arguments (status)
  	  get_args(f, &args[0], 1);
  	  // syscall exit
      exit(args[0]);
  	  break;
  	case SYS_EXEC:	
  	  //get the arguments (command line)
  	  get_args(f, &args[0], 1);
  	  //check if the command line is valid
  	  validate_str((const void*)args[0]);
  	  //syscall exec
  	  f->eax = exec((const char*)args[0]);
  	  break;
  	case SYS_WAIT:
  	  // get the arguments (thread id)
  	  get_args(f,&args[0],1);
  	  // syscall wait
  	  f->eax = wait((tid_t)args[0]);
  	  break;
  	case SYS_CREATE:
  	  //get the arguments (filename and size)
  	  get_args(f, &args[0], 2);
  	  //check if filename is valid
  	  validate_str((const void*)args[0]);
  	  //syscall create
  	  f->eax = create((const char*)args[0], (unsigned)args[1]);
  	  break;
  	case SYS_REMOVE:
  	  //get the arguments (filename) 
  	  get_args(f, &args[0], 1);
  	  //check if filename is valid
  	  validate_str((const void*)args[0]);
  	  //syscall remove
  	  f->eax = remove((const char*)args[0]);
  	  break;
  	case SYS_OPEN:
  	  //get the arguments (filename)
  	  get_args(f, &args[0], 1);
  	  //check if filename is valid
  	  validate_str((const void*)args[0]);
  	  //syscall open
  	  f->eax = open((const char*)args[0]);
  	  break;
  	case SYS_FILESIZE:
  	  //get the arguments (filename)
  	  get_args(f, &args[0], 1);
  	  //syscall filesize
  	  f->eax = filesize(args[0]);
  	  break;
  	case SYS_READ:
  	  //get the arguments (filename, buffer pointer, no of bytes)
  	  get_args(f, &args[0], 3);
  	  //check if the buffer is in user's pagedir
  	  getpage_ptr((const void*)args[1]);
  	  //syscall read
  	  f->eax=read(args[0],(const void*)args[1],(unsigned)args[2]);
  	  break;
  	case SYS_WRITE:
  	  //get the arguments (filename, buffer pointer, no of bytes)
  	  get_args(f, &args[0], 3);
  	  //check if the buffer is in user's pagedir
  	  getpage_ptr((const void*)args[1]);
  	  //syscall write
  	  f->eax=write(args[0],(const void*)args[1],(unsigned)args[2]);
  	  break;
  	case SYS_CLOSE:
  	  //get the arguments (file descriptor)
  	  get_args(f, &args[0], 1);
  	  //syscall close
  	  close(args[0]);
  	  break;
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

  //when no of arguments is 3
  if(num_of_args==MAX_ARGS){
  for (i = 0; i < num_of_args; i++)
  {
    ptr = (int *) (f->esp + 20+4*i);							
    validate_ptr((const void *) ptr);
    args[i] = *ptr;
  }
  }//when no of arguments is 2
  else if(num_of_args==2)
  {
  for (i = 0; i < num_of_args; i++)
  {
    ptr = (int *) (f->esp + 16+4*i);							
    validate_ptr((const void *) ptr);
    args[i] = *ptr;
  }
  }//when no of arguments is 1
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
system call create
Creates a new file with name as file_name 
and size as initial_size
*/
bool
create(const char* file_name, unsigned initial_size)
{
	lock_acquire(&filesys_lock);
	bool success = filesys_create(file_name, initial_size);
	lock_release(&filesys_lock);
	return success;
}

/*
system call remove
Removes file with name as file_name
*/
bool
remove(const char* file_name)
{
	lock_acquire(&filesys_lock);
	bool success = filesys_remove(file_name);
	lock_release(&filesys_lock);
	return success;
}

/*
System call open
Opens a file with name as file_name
if open is successful
adds the file to process's list of open files
and returns the file descriptor
*/
int 
open(const char* file_name)
{
	lock_acquire(&filesys_lock);
	struct file* file_ptr = filesys_open(file_name);
	if(!file_ptr)
	{
		lock_release(&filesys_lock);
		return ERROR;
	}
	//add file pointer to this process's list of open files
	//create a new file_info pointer
	struct file_info* file_info_ptr = malloc(sizeof(struct file_info));
	if(!file_info_ptr)
	{
		return ERROR;
	}
	struct thread* curr = thread_current();
	//assign the file_ptr to file
	file_info_ptr->file = file_ptr;
	//assign current fd of the thread
	file_info_ptr->fd = curr->fd;
	//increment the fd by 1 to allocate to next files
	curr->fd++;
	//push file_info_ptr to file_list
	list_push_back(&(curr->file_list), &file_info_ptr->fileelem);
	//return file_info_ptr->fd;
	int fd = file_info_ptr->fd;
	lock_release(&filesys_lock);
	return fd;
}

/*
System call filesize
Retrieve file from list of files held by this process using fd
and returns its size
*/
int
filesize(int fd)
{
	lock_acquire(&filesys_lock);
	//retrieve file from list of files held by this process
	struct file* file_ptr = get_file(fd);
	if(!file_ptr)
	{
		lock_release(&filesys_lock);
		return ERROR;
	}
	int size = file_length(file_ptr);
	lock_release(&filesys_lock);
	return size;
}

/*
System call read
Reads from file with fd as fd into buf
size number of bytes
*/
int
read (int fd, void* buf, unsigned size)
{
	if (size <= 0)
		// read positive length 
		return size;

	if (fd == STD_INPUT)
	{
		uint8_t *local_buf = (uint8_t *) buf;
		unsigned i=0;
		for(; i <size; i++)
		{
			/* retireve key press from input buffer */
			local_buf[i] = input_getc();
		}
		return size;
	}
	//read from file
	lock_acquire(&filesys_lock);
	//retrieve file from list of files held by this process
	struct file* file_ptr = get_file(fd);
	if(!file_ptr)
	{
		lock_release(&filesys_lock);
		return ERROR;
	}
	int bytes_read = file_read(file_ptr, buf, size);
	lock_release(&filesys_lock);
	return bytes_read;
}
/*
System call write
Writes into file with fd as fd from buf
size number of bytes
*/
int
write (int fd, const void* buf, unsigned size)
{
	if (size <= 0)
		// nothing to write
		return size;
	if (fd == STD_OUTPUT)
	{
		// check if buffer is in user program's virtual memory
		validate_buf(buf,size);
		// write size characters from buf to console
		putbuf (buf, size);
		return size;
	}
	//write to file
	lock_acquire(&filesys_lock);
	struct file* file_ptr = get_file(fd);
	if(!file_ptr)
	{
		lock_release(&filesys_lock);
		return ERROR;
	}
	int bytes_writ = file_write(file_ptr, buf, size);
	lock_release(&filesys_lock);
	return bytes_writ;
}

/*
Syscall close
Close  file with fiven fd
*/
void 
close(int fd)
{
	lock_acquire(&filesys_lock);
	close_file(fd);
	lock_release(&filesys_lock);
}

/*
get file from file descriptor
*/
struct file*
get_file(int fd)
{
	struct thread* curr = thread_current();
	struct list_elem* e;
	//traverse through file_list and retrieve the file
	for(e = list_begin(&curr->file_list);e!=list_end(&curr->file_list);e=list_next(e))
	{
		struct file_info* file_info_ptr = list_entry(e, struct file_info, fileelem);
		if(file_info_ptr && fd==file_info_ptr->fd)
		{
			return file_info_ptr->file;
		}
	}
	return NULL;
}

/*
close a file descriptor
*/
void
close_file(int fd)
{
	struct thread* curr = thread_current();
	struct list_elem* e;
	for(e=list_begin(&curr->file_list);e!=list_end(&curr->file_list);e=list_next(e))
	{
		struct file_info* file_info_ptr = list_entry(e, struct file_info, fileelem);
		//if file_info_ptr is not NULL and its fd matches input fd close the file
		if(file_info_ptr && fd==file_info_ptr->fd)
		{
			//close the file
			file_close(file_info_ptr->file);
			//remove from file_list
			list_remove(&file_info_ptr->fileelem);
			//free file_info_ptr
			free(file_info_ptr);
			return;
		}
	}
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
	//char* local_buf = (char*)buf;
	char* local_buf = (char*)buf;
	// validate each pointer which is a part of buf
	for(;i<byte_size;i++)
	{
		validate_ptr((const void*)local_buf);
		local_buf+=4;
	}
}
/*
Get pointer to page from the virtual address
*/
int
getpage_ptr(const void *vaddr)
{
	//check if it is a valid user virtual address
	validate_ptr(vaddr);
	//get page using the virtual address
	void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
	if(!ptr)
		exit(ERROR);
	return (int)ptr;
}