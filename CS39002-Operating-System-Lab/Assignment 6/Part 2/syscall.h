#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"
#include "threads/thread.h"
void syscall_init (void);

#define ERROR -1
#define USER_VADDR_BOTTOM ((void *) 0x08048000)

int getpage_ptr (const void *vaddr);

#endif /* userprog/syscall.h */

