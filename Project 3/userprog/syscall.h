#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "userprog/process.h"
#include <stdbool.h>
#include <stdint.h>

// Not exactly sure about this but it is used in exception.c
#define USER_VADDR_BOTTOM ((void*) 0x08048000)
#define STACK_H 32

void syscall_init (void);

#endif /* userprog/syscall.h */