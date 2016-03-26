#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* These header files were added in */
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "lib/kernel/list.h"
#include "threads/synch.h"

#include <string.h>
#include "threads/malloc.h"
#include "devices/input.h"

#include "threads/init.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include "threads/pte.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"

/* End of header files added in */

/* These Global variables were added in */
#define MAX_FILE_SIZE 204800
#define MAX_FILE_NAME_LENGTH 64
#define MAX_ARGS 10

struct file* file_check(int fd);
static struct lock LOCK;
typedef int pid_t;

struct process_file
{
	struct file *file;
	int fd;
	struct list_elem elem;
};

struct file* file_check (int fd)
{
  struct thread *cur = thread_current();
  struct list_elem *e =  list_head (&cur->file_list);

  while ((e = list_next (e)) != list_end (&cur->file_list)){
    struct process_file *pf = list_entry (e, struct process_file, elem);
    if (fd == pf->fd) {
	  return pf->file;
	}
    e = list_next(e);
  }

  return NULL;
}
/* End of Global variables added in */

static void syscall_handler (struct intr_frame *);

/* The following were added in */
int process_file_add (struct file *f);
struct file* process_file_get( int fd);
void arg_get (struct intr_frame *f, int *arg, int n);
static bool validaddr (const void * validaddr, unsigned size);

/* End of elements aded in     */

/* Functions */
static void halt (void);
static void exit (int status);
static pid_t exec (const char *file_name);
static int wait (pid_t pid);
static int read(int fd, void *buffer, unsigned size);
static int write(int fd, const void *buffer, unsigned size);
static bool create (const char *file, unsigned initial_size);
static bool remove (const char *file);
static int open (const char *file); 
static int filesize (int fd);
static void seek(int fd, unsigned position);
static unsigned tell(int fd);
//static int close(struct intr_frame *f)

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	int arg[MAX_ARGS];
	switch (* (int *) f-> esp)
	{
		case SYS_HALT:
		{
			halt();
			break;
		}

		case SYS_EXIT:
		{
			arg_get(f, &arg[0], 1);
			exit(arg[0]);
			break;
		}

		case SYS_EXEC:
		{
			arg_get(f, &arg[0], 1);
			f -> eax = exec((const char *) arg[0]);
			break;
		}

		case SYS_WAIT:
		{
			arg_get(f, &arg[0], 1);
			f -> eax = wait(arg[0]);
			break;
		}

		case SYS_CREATE:
		{
			arg_get(f, &arg[0], 1);
			f -> eax = create((const char *) arg[0], (unsigned) arg[1]);
			break;
		}

		case SYS_REMOVE:
		{
			arg_get(f, &arg[0], 1);
			f -> eax = remove((const char *) arg[0]);
			break;
		}

		case SYS_OPEN:
		{
			arg_get(f, &arg[0], 1);
			f -> eax = open((const char *) arg[0]);
			break;
		}

		case SYS_FILESIZE:
		{
			arg_get(f, &arg[0], 1);
			f -> eax = filesize(arg[0]);
			break;
		}

		case SYS_READ:
		{
			arg_get(f, &arg[0], 1);
			f -> eax = read(arg[0], (void *) arg[1], (unsigned) arg[2]);
			break;
		}

		case SYS_WRITE:
		{
			arg_get(f, &arg[0], 1);
			write(arg[0], (const void *) arg[1], (unsigned) arg[2]);
			break;
		}

		case SYS_SEEK:
		{
			arg_get(f, &arg[0], 1);
			seek(arg[0], (unsigned) arg[1]);
			break;
		}

		case SYS_TELL:
		{
			arg_get(f, &arg[0], 1);
			f -> eax = tell(arg[0]);
			break;
		}

		/*case SYS_CLOSE:
		{
			arg_get(f, &arg[0], 1);
			close(arg[0]);
			break;
		}*/

	}

  //This was originally here
  /*
  printf ("system call!\n");
  thread_exit ();
  */
}

static void
halt(void){
  shutdown(); //Terminates Pintos by calling shutdown() function
}

static void
exit(int status){
  //thread_kill(status); //Terminates the current user program and returns the current status to the kernel
  //status = 0;
  /*
  struct thread *cur = thread_current();
  status = THREAD_DYING;
  cur -> status = status;
  */
}

static pid_t
exec(const char *file_name){
  //Check address
  if(!validaddr (file_name,0) || !validaddr(file_name, strlen(file_name))) 
  {
  	thread_exit();
  }

  return process_execute (file_name); //Runs the executable whose name is given, passing any arguments and returns the new process's pid
}

static int 
wait(pid_t pid){
   return process_wait(pid); //Waits for a child process retrievs the child's exit status
}

//NOTE: Global Variables "MAX_FILE_SIZE" and "MAX_FILE_NAME_LENGTH" must be defined
static bool
create (const char *file, unsigned initial_size)
{	
  //Check address
  if(!validaddr (file, 0) || !validaddr(file, strlen(file))) 
  {
  	thread_exit();
  }

	lock_acquire(&LOCK);

	//Case 1: "initial_size" is lower than the limit
	if (initial_size <= MAX_FILE_SIZE){
		//Must check if file name length is lower than limit too
		if (strlen(file) > MAX_FILE_NAME_LENGTH){
			printf("Cannot create file because file name is too long");
			}
		else if (strlen(file) == 0){
			printf("Cannot create file because there is no file name");
			}
		//file name length and initial size are lower than the limits...
		//...then it's ok to create the file then
		else{
			bool file_created = filesys_create(file, initial_size);
			lock_release(&LOCK);
			return file_created;
		}
	}
	//Case 2: "initial_size" is larger than the limit
	else{
		printf("File size is too large!");
	}

lock_release(&LOCK);
return false;
}

static bool
remove (const char *file)
{	
  //Check address
  if(!validaddr (file,0) || !validaddr(file, strlen(file))) 
  {
  	thread_exit();
  }

	lock_acquire(&LOCK);
	bool file_removed = filesys_remove(file);
	lock_release(&LOCK);
	return file_removed;
}

static int 
open (const char *file) 
{
  //Check address
  if(!validaddr (file,0) || !validaddr(file, strlen(file))) 
  {
  	thread_exit();
  }


	lock_acquire(&LOCK);
	if (strlen(file) == 0) {
		printf("File open failed because file name can not be empty!\n");
	return -1;
	} 
	else 
	{
		struct file *f = filesys_open(file);
		if (!f)
		{
			lock_release(&LOCK);
		}
		int fd = process_file_add(f);
		lock_release(&LOCK);
		return fd;
	}
}

static int
filesize (int fd)
{	
	lock_acquire(&LOCK);
	
	struct file *f = process_file_get(fd);
	if (f != NULL){
		lock_release(&LOCK);
		return file_length(f);
	}
	else
	{
		lock_release(&LOCK);
		return -1;
	}
}

/* The following function was added in */

/* Reads size bytes from buffer to open file fd. Returns the 
number of bytes actually read (0 at end of file), or -1 if the file
couldn't be read. Fd 0 reads from the keyboard using input_getc().*/
static int 
read(int fd, void *buffer, unsigned size)
{	

  //Check address
  if(!validaddr (buffer,size)) 
  {
  	thread_exit();
  }
	//return value
	int returnVal;
	//declared 'offset' outside of for loop because of error in compliation
	unsigned offset;

	lock_acquire(&LOCK);
	
	//read syscall
	if (fd == STDIN_FILENO){
		//read each input 
		for(offset = 0; offset < size; offset++){
			//use uint8 because we are reading size bytes
			*(uint8_t *)(buffer + offset) = input_getc();		
		}
		//make the return value equal to the size
		returnVal = size;
	} else {
		returnVal = -1;
	}
	
	lock_release(&LOCK);
	return returnVal;
}


static int 
write(int fd, const void *buffer, unsigned size)
{
  //Check address
  if(!validaddr (buffer,size)) 
  {
  	thread_exit();
  }
	int returnVal;
	lock_acquire(&LOCK);

	//write sycall
	if (fd == STDOUT_FILENO) {
		putbuf(buffer, size);
		returnVal = size;
		goto done;
	}
	
	struct file *f = file_check(fd);
	if (!f){
		lock_release(&LOCK);
		returnVal = -1;
	} else {
		returnVal = file_write(f, buffer, size);
	}
	lock_release(&LOCK);
	
	done:
	  return returnVal;
}

static void
seek(int fd, unsigned position)
{
	lock_acquire(&LOCK);

    struct file *f = file_check(fd);

    if (!f) {
     goto done;
    }
    file_seek(f, position);

	done:
	  lock_release(&LOCK);
	  return;
}

static unsigned
tell(int fd)
{
	lock_acquire(&LOCK);
	struct file *f = file_check(fd);
  	if (!f){
      	lock_release(&LOCK);
      	return -1;
    }
  	lock_release(&LOCK);
  	return file_tell(f);
}

void arg_get (struct intr_frame *f, int *arg, int n)
{
	int *ptr;
	int i;
	for (i = 0; i < n; i++)
	{
		ptr = (int *) f->esp + i + 1;
		arg[i] = *ptr;
	}
}

//NOTE: These two functions will NOT work if threads.c isnt edited
int process_file_add (struct file *f)
{
	struct process_file *pf = malloc(sizeof(struct process_file));
	pf -> file = f;
	pf -> fd = thread_current() -> fd;
	thread_current() -> fd++;
	list_push_back(&thread_current() -> file_list, &pf -> elem);
	return pf -> fd;
}

struct file* process_file_get (int fd)
{
	struct thread *t = thread_current();
	struct list_elem *e;

	for(e = list_begin( &t -> file_list); e != list_end(&t -> file_list); e = list_next(e))
	{
		struct process_file *pf = list_entry(e, struct process_file, elem);
		if( fd == pf -> fd)
		{
			return pf -> file;
		}
	}
return NULL;

}
/*
static int
findUser (const uint8_t *uaddr)
{
  if(!is_user_vaddr(uaddr))
    return -1;
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}

static bool pointerValid(void * esp, uint8_t argc){
  uint8_t i = 0;
  for (; i < argc; ++i)
  {
    if (findUser(((uint8_t *)esp)+i) == -1){
      return false;
    }
  }
  return true;
}

static int
close(struct intr_frame *f)
{
  if (!pointerValid(f->esp +4, 4)){
    return -1;
  }
  int fd = *(int *)(f->esp + 4);
  process_kill(fd);
  return 0;
}*/

/* The following function is used to check valid virtual address for pointers */

static bool
validaddr (const void * validaddr, unsigned size)
{
	//is_user_vaddr is a built in function in /threads/vaddr.h
	if(!is_user_vaddr (validaddr + size))
	{
		return false;
	}

return true;

}

/* End of added in function */


/* End of function added in */
