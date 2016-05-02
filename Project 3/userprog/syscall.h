#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "userprog/process.h"
#include <stdbool.h>
#include <stdint.h>

#include "threads/synch.h"

/* 
 * The following were added in...
 */

#define CLOSE_ALL -1
#define ERROR -1

#define NOT_LOADED 0
#define LOAD_SUCCESS 1
#define LOAD_FAIL 2

/*
 * End of elements added in...
 */

// Not exactly sure about this but it is used in exception.c
#define USER_VADDR_BOTTOM ((void*) 0x08048000)
#define STACK_H 32

//took file lock from syscall.c and added it here
struct lock file_lock;

struct child_process
{
	int pid;
	int load;
	bool wait;
	bool exit;
	int status;
	struct semaphore load_sema;
	struct sempahore exit_sema;
	struct list_elem elem;
};

struct child_process* add_child_process (int pid);
struct child_process* get_child_process (int pid);
void remove_child_process (struct child_process *cp);
void remove_child_processes (void);

void process_close_file (int fd);


void syscall_init (void);

/* 
 * Edit 5/2/16
 */

struct file *get_file(int fd);
bool not_valid(const void *pointer);
void halt (void);
void exit (int status);
pid_t exec (const char *cmd_line);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
int mmap (int fd, void *addr);
void munmap (int mapping);


#endif /* userprog/syscall.h */